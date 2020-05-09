// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Pass/TwiddleFactorsPass.h"
#include "RenderCore/Public/GlobalShader.h"
#include "RenderCore/Public/ShaderParameterUtils.h"
#include "RenderCore/Public/ShaderParameterMacros.h"

#include "Public/GlobalShader.h"
#include "Public/PipelineStateCache.h"
#include "Public/RHIStaticStates.h"
#include "Public/SceneUtils.h"
#include "Public/SceneInterface.h"
#include "Public/ShaderParameterUtils.h"
#include "Public/Logging/MessageLog.h"
#include "Public/Internationalization/Internationalization.h"
#include "Public/StaticBoundShaderState.h"
#include "RHI/Public/RHICommandList.h"

#include "Classes/Engine/World.h"
#include "Engine/Classes/Kismet/KismetRenderingLibrary.h"

#include "Math/UnrealMathUtility.h"

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FTwiddleFactorsComputeShaderParameters, )
END_GLOBAL_SHADER_PARAMETER_STRUCT()
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FTwiddleFactorsComputeShaderParameters, "TwiddleFactorsUniform");

class FTwiddleFactorsComputeShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FTwiddleFactorsComputeShader, Global)
	using FParameters = FTwiddleFactorsComputeShaderParameters;

public:
	FTwiddleFactorsComputeShader() {}
	FTwiddleFactorsComputeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		OutputTwiddleFactorsTexture.Bind(Initializer.ParameterMap, TEXT("OutputTwiddleFactorsTexture"));
		InputTwiddleIndicesBuffer.Bind(Initializer.ParameterMap, TEXT("InputTwiddleIndicesBuffer"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	void BindShaderTextures(FRHICommandList& RHICmdList, FUnorderedAccessViewRHIRef OutputTextureUAV, FUnorderedAccessViewRHIRef InputIndicesBufferUAV)
	{
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();
		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputTwiddleFactorsTexture, OutputTextureUAV);
		SetUAVParameter(RHICmdList, ComputeShaderRHI, InputTwiddleIndicesBuffer, InputIndicesBufferUAV);
	}

	void UnbindShaderTextures(FRHICommandList& RHICmdList)
	{
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();
		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputTwiddleFactorsTexture, FUnorderedAccessViewRHIRef());
		SetUAVParameter(RHICmdList, ComputeShaderRHI, InputTwiddleIndicesBuffer, FUnorderedAccessViewRHIRef());
	}

	void SetShaderParameters(FRHICommandList& RHICmdList, const FParameters& Parameters)
	{
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();
		SetUniformBufferParameterImmediate(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FParameters>(), Parameters);
	}

private:

	LAYOUT_FIELD(FShaderResourceParameter, OutputTwiddleFactorsTexture);
	LAYOUT_FIELD(FShaderResourceParameter, InputTwiddleIndicesBuffer);
};

IMPLEMENT_GLOBAL_SHADER(FTwiddleFactorsComputeShader, "/Plugin/FFTOcean/TwiddleFactorsComputeShader.usf", "ComputeTwiddleFactors", SF_Compute);

inline bool operator==(const FTwiddleFactorsPassConfig& A, const FTwiddleFactorsPassConfig& B)
{
	return FMemory::Memcmp(&A, &B, sizeof(FTwiddleFactorsPassConfig)) == 0;
}

inline bool operator!=(const FTwiddleFactorsPassConfig& A, const FTwiddleFactorsPassConfig& B)
{
	return !(A == B);
}

namespace
{
	FORCEINLINE uint32 ReverseBits(uint32 Value)
	{
		uint32 Count = 31;
		uint32 Reversed = Value;

		while (Value >>= 1)
		{
			Reversed <<= 1;
			Reversed |= Value & 1;
			Count--;
		}

		Reversed <<= Count;
		return Reversed;
	}

	FORCEINLINE uint32 RotateLeft(uint32 Value, uint32 Bits)
	{
		return (Value << Bits) | (Value >> (32 - Bits));
	}

	// Reverse bits on uint32 value on the lower N bits
	FORCEINLINE uint32 ReverseBits(uint32 Value, uint32 Bits)
	{
		return RotateLeft(ReverseBits(Value), Bits);
	}
}

FTwiddleFactorsPass::FTwiddleFactorsPass()
{
}

FTwiddleFactorsPass::~FTwiddleFactorsPass()
{
	ReleaseRenderResource();
}

bool FTwiddleFactorsPass::IsValidPass() const
{
	bool bValid = !!OutputTwiddleFactorsTexture
		&& !!OutputTwiddleFactorsTextureUAV
		&& !!OutputTwiddleFactorsTextureSRV;

	return bValid;
}

void FTwiddleFactorsPass::ReleaseRenderResource()
{
	SafeReleaseTextureResource(OutputTwiddleFactorsTexture);
	SafeReleaseTextureResource(OutputTwiddleFactorsTextureUAV);
	SafeReleaseTextureResource(OutputTwiddleFactorsTextureSRV);
}

void FTwiddleFactorsPass::ConfigurePass(const FTwiddleFactorsPassConfig& InConfig)
{
	// Always release current resource before creating new render resources
	ReleaseRenderResource();
	
	Config = InConfig;
	
	FRHIResourceCreateInfo CreateInfo;
	uint32 TextureWidth = StaticCast<uint32>(FMath::Log2(InConfig.TextureWidth));
	uint32 TextureHeight = InConfig.TextureHeight;
	
	OutputTwiddleFactorsTexture = RHICreateTexture2D(TextureWidth, TextureHeight, PF_FloatRGBA, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
	OutputTwiddleFactorsTextureUAV = RHICreateUnorderedAccessView(OutputTwiddleFactorsTexture);
	OutputTwiddleFactorsTextureSRV = RHICreateShaderResourceView(OutputTwiddleFactorsTexture, 0);
}

void FTwiddleFactorsPass::Render(const FTwiddleFactorsPassConfig& InConfig, const FTwiddleFactorsPassParam& Param, FRHITexture* DebugTextureRef)
{
	// We only need to render twiddle factors pass once as long as the texture dimensions remain unchanged
	if (Config != InConfig)
	{
		ConfigurePass(InConfig);
	
		if (IsValidPass())
		{
			ENQUEUE_RENDER_COMMAND(TwiddleFactorsPassCommand)
			(
				[Param, DebugTextureRef, this](FRHICommandListImmediate& RHICmdList)
				{
					check(IsInRenderingThread());

					// Init indices buffer
					const int N = Config.TextureHeight;
					TResourceArray<uint32> IndicesArray;
					IndicesArray.AddUninitialized(N);

					const uint32 Bits = StaticCast<uint32>(FMath::Log2(N));

					for (int32 Index = 0; Index < N; ++Index)
					{
						IndicesArray[Index] = ReverseBits(Index, Bits);
					}

					FRHIResourceCreateInfo CreateInfo(&IndicesArray);
					FStructuredBufferRHIRef IndicesBufferRef = RHICreateStructuredBuffer(
						sizeof(uint32),                           // Stride
						sizeof(uint32) * N,                       // Size
						BUF_UnorderedAccess | BUF_ShaderResource, // Usage
						CreateInfo                                // Create info
					);
					FUnorderedAccessViewRHIRef IndicesBufferUAVRef = RHICreateUnorderedAccessView(IndicesBufferRef, true, false);

					// Set up compute shader
					TShaderMapRef<FTwiddleFactorsComputeShader> TwiddleFactorsComputeShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
					RHICmdList.SetComputeShader(TwiddleFactorsComputeShader.GetComputeShader());

					// Bind shader textures
					TwiddleFactorsComputeShader->BindShaderTextures(RHICmdList, OutputTwiddleFactorsTextureUAV, IndicesBufferUAVRef);

					// Bind shader uniform
					FTwiddleFactorsComputeShader::FParameters UniformParam;
					TwiddleFactorsComputeShader->SetShaderParameters(RHICmdList, UniformParam);

					// Dispatch shader
					const int ThreadGroupCountX = StaticCast<int>(FMath::Log2(Config.TextureHeight));
					const int ThreadGroupCountY = StaticCast<int>(Config.TextureHeight / 16);
					DispatchComputeShader(RHICmdList, TwiddleFactorsComputeShader, ThreadGroupCountX, ThreadGroupCountY, 1);

					// Unbind shader textures
					TwiddleFactorsComputeShader->UnbindShaderTextures(RHICmdList);

					// Debug drawing
					if (DebugTextureRef)
					{
						RHICmdList.CopyToResolveTarget(OutputTwiddleFactorsTexture, DebugTextureRef, FResolveParams());
					}

					// Release structured buffer
					SafeReleaseTextureResource(IndicesBufferUAVRef);
					SafeReleaseTextureResource(IndicesBufferRef);
				}
			);
		}
	}
}
