// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Pass/PhillipsFourierPass.h"
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

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FPhillipsFourierComputeShaderParameters, )
	SHADER_PARAMETER(float,     WaveAmplitude)
	SHADER_PARAMETER(FVector2D, WindSpeed)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FPhillipsFourierComputeShaderParameters, "PhillipsFourierUniform");

class FPhillipsFourierComputeShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPhillipsFourierComputeShader, Global)
	using FParameters = FPhillipsFourierComputeShaderParameters;

public:
	FPhillipsFourierComputeShader() {}
	FPhillipsFourierComputeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		OutputPhillipsFourierTexture.Bind(Initializer.ParameterMap, TEXT("OutputPhillipsFourierTexture"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	void BindShaderTextures(FRHICommandList& RHICmdList, FUnorderedAccessViewRHIRef OutputTextureUAV)
	{
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();
		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputPhillipsFourierTexture, OutputTextureUAV);
	}

	void UnbindShaderTextures(FRHICommandList& RHICmdList)
	{
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();
		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputPhillipsFourierTexture, FUnorderedAccessViewRHIRef());
	}

	void SetShaderParameters(FRHICommandList& RHICmdList, const FParameters& Parameters)
	{
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();
		SetUniformBufferParameterImmediate(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FParameters>(), Parameters);
	}

private:

	LAYOUT_FIELD(FShaderResourceParameter, OutputPhillipsFourierTexture);
};

IMPLEMENT_GLOBAL_SHADER(FPhillipsFourierComputeShader, "/Plugin/FFTOcean/PhillipsFourierComputeShader.usf", "ComputePhillipsFourier", SF_Compute);

inline bool operator==(const FPhillipsFourierPassConfig& A, const FPhillipsFourierPassConfig& B)
{
	return FMemory::Memcmp(&A, &B, sizeof(FPhillipsFourierPassConfig)) == 0;
}

inline bool operator!=(const FPhillipsFourierPassConfig& A, const FPhillipsFourierPassConfig& B)
{
	return !(A == B);
}

FPhillipsFourierPass::FPhillipsFourierPass()
{
}

FPhillipsFourierPass::~FPhillipsFourierPass()
{
	ReleaseRenderResource();
}

bool FPhillipsFourierPass::IsValidPass() const
{
	bool bValid = !!OutputPhillipsFourierTexture
		&& !!OutputPhillipsFourierTextureUAV
		&& !!OutputPhillipsFourierTextureSRV;

	return bValid;
}

void FPhillipsFourierPass::ReleaseRenderResource()
{
	SafeReleaseTextureResource(OutputPhillipsFourierTexture);
	SafeReleaseTextureResource(OutputPhillipsFourierTextureUAV);
	SafeReleaseTextureResource(OutputPhillipsFourierTextureSRV);
}

void FPhillipsFourierPass::ConfigurePass(const FPhillipsFourierPassConfig& InConfig)
{
	// Always release current resource before creating new render resources
	ReleaseRenderResource();
	
	Config = InConfig;
	
	FRHIResourceCreateInfo CreateInfo;
	uint32 TextureWidth = InConfig.TextureWidth;
	uint32 TextureHeight = InConfig.TextureHeight;
	
	OutputPhillipsFourierTexture = RHICreateTexture2D(TextureWidth, TextureHeight, PF_FloatRGBA, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
	OutputPhillipsFourierTextureUAV = RHICreateUnorderedAccessView(OutputPhillipsFourierTexture);
	OutputPhillipsFourierTextureSRV = RHICreateShaderResourceView(OutputPhillipsFourierTexture, 0);
}

void FPhillipsFourierPass::Render(const FPhillipsFourierPassConfig& InConfig, const FPhillipsFourierPassParam& Param, FRHITexture* DebugTextureRef)
{
	if (Config != InConfig)
	{
		ConfigurePass(InConfig);
	}

	if (IsValidPass())
	{
		ENQUEUE_RENDER_COMMAND(PhillipsFourierPassCommand)
		(
			[Param, DebugTextureRef, this](FRHICommandListImmediate& RHICmdList)
			{
				check(IsInRenderingThread());

				TShaderMapRef<FPhillipsFourierComputeShader> PhillipsFourierComputeShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
				RHICmdList.SetComputeShader(PhillipsFourierComputeShader.GetComputeShader());

				// Bind shader textures
				PhillipsFourierComputeShader->BindShaderTextures(RHICmdList, OutputPhillipsFourierTextureUAV);

				// Bind shader uniform
				FPhillipsFourierComputeShader::FParameters UniformParam;
				UniformParam.WaveAmplitude = Param.WaveAmplitude;
				UniformParam.WindSpeed = Param.WindSpeed;
				PhillipsFourierComputeShader->SetShaderParameters(RHICmdList, UniformParam);

				// Dispatch shader
				const int ThreadGroupCountX = StaticCast<int>(Config.TextureWidth / 32);
				const int ThreadGroupCountY = StaticCast<int>(Config.TextureHeight / 32);
				DispatchComputeShader(RHICmdList, PhillipsFourierComputeShader, ThreadGroupCountX, ThreadGroupCountY, 1);

				// Unbind shader textures
				PhillipsFourierComputeShader->UnbindShaderTextures(RHICmdList);

				// Debug drawing
				if (DebugTextureRef)
				{
					RHICmdList.CopyToResolveTarget(OutputPhillipsFourierTexture, DebugTextureRef, FResolveParams());
				}
			}
		);
	}
}
