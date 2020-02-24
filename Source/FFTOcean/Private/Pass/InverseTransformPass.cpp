// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Pass/InverseTransformPass.h"
#include "RenderCore/Public/GlobalShader.h"
#include "RenderCore/Public/ClearQuad.h"
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

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FInverseTransformComputeShaderParameters, )
	SHADER_PARAMETER(int, Stage)
	SHADER_PARAMETER(int, StageCount)
	SHADER_PARAMETER(int, Direction)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FInverseTransformComputeShaderParameters, "InverseTransformUniform");

class FInverseTransformComputeShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FInverseTransformComputeShader, Global)
	using FParameters = FInverseTransformComputeShaderParameters;

public:
	FInverseTransformComputeShader() {}
	FInverseTransformComputeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		OutputInverseTransformTexture.Bind(Initializer.ParameterMap, TEXT("OutputInverseTransformTexture"));
		InputTwiddleFactorsTexture.Bind(Initializer.ParameterMap, TEXT("InputTwiddleFactorsTexture"));
		InputFourierComponentTexture.Bind(Initializer.ParameterMap, TEXT("InputFourierComponentTexture"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << OutputInverseTransformTexture << InputTwiddleFactorsTexture << InputFourierComponentTexture;
		return bShaderHasOutdatedParameters;
	}

	void BindShaderTextures(FRHICommandList& RHICmdList, FUnorderedAccessViewRHIRef OutputTextureUAV, FShaderResourceViewRHIRef InputTwiddleFactorsTextureSRV, FShaderResourceViewRHIRef InputFourierComponentTextureSRV)
	{
		FRHIComputeShader* ComputeShaderRHI = GetComputeShader();
		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputInverseTransformTexture, OutputTextureUAV);
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputTwiddleFactorsTexture, InputTwiddleFactorsTextureSRV);
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputFourierComponentTexture, InputFourierComponentTextureSRV);		
	}

	void UnbindShaderTextures(FRHICommandList& RHICmdList)
	{
		FRHIComputeShader* ComputeShaderRHI = GetComputeShader();
		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputInverseTransformTexture, FUnorderedAccessViewRHIRef());
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputTwiddleFactorsTexture, FShaderResourceViewRHIRef());
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputFourierComponentTexture, FShaderResourceViewRHIRef());
	}

	void SetShaderParameters(FRHICommandList& RHICmdList, const FParameters& Parameters)
	{
		FRHIComputeShader* ComputeShaderRHI = GetComputeShader();
		SetUniformBufferParameterImmediate(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FParameters>(), Parameters);
	}

private:

	FShaderResourceParameter OutputInverseTransformTexture;
	FShaderResourceParameter InputTwiddleFactorsTexture;
	FShaderResourceParameter InputFourierComponentTexture;
};

IMPLEMENT_SHADER_TYPE(, FInverseTransformComputeShader, TEXT("/Plugin/FFTOcean/InverseTransformComputeShader.usf"), TEXT("ComputeInverseTransform"), SF_Compute);

inline bool operator==(const FInverseTransformPassConfig& A, const FInverseTransformPassConfig& B)
{
	return FMemory::Memcmp(&A, &B, sizeof(FInverseTransformPassConfig)) == 0;
}

inline bool operator!=(const FInverseTransformPassConfig& A, const FInverseTransformPassConfig& B)
{
	return !(A == B);
}

FInverseTransformPass::FInverseTransformPass()
{
}

FInverseTransformPass::~FInverseTransformPass()
{
	ReleaseRenderResource();
}

bool FInverseTransformPass::IsValidPass() const
{
	bool bValid = !!OutputInverseTransformTextures;
	bValid &= !!OutputInverseTransformTextureUAVs;
	bValid &= !!OutputInverseTransformTextureSRVs;

	return bValid;
}

void FInverseTransformPass::ReleaseRenderResource()
{
	for (FTexture2DRHIRef Texture : OutputInverseTransformTextures)
	{
		SafeReleaseTextureResource(Texture);
	}

	for (FShaderResourceViewRHIRef TextureSRV : OutputInverseTransformTextureSRVs)
	{
		SafeReleaseTextureResource(TextureSRV);
	}

	for (FUnorderedAccessViewRHIRef TextureUAV : OutputInverseTransformTextureUAVs)
	{
		SafeReleaseTextureResource(TextureUAV);
	}
}

void FInverseTransformPass::ConfigurePass(const FInverseTransformPassConfig& InConfig)
{
	// Always release current resource before creating new render resources
	ReleaseRenderResource();
	
	Config = InConfig;
	
	FRHIResourceCreateInfo CreateInfo;
	uint32 TextureWidth = InConfig.TextureWidth;
	uint32 TextureHeight = InConfig.TextureHeight;
	
	for (int32 Index = 0; Index < 3; ++Index)
	{
		OutputInverseTransformTextures[Index] = RHICreateTexture2D(TextureWidth, TextureHeight, PF_G32R32F, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
		OutputInverseTransformTextureUAVs[Index] = RHICreateUnorderedAccessView(OutputInverseTransformTextures[Index]);
		OutputInverseTransformTextureSRVs[Index] = RHICreateShaderResourceView(OutputInverseTransformTextures[Index], 0);
	}
}

void FInverseTransformPass::Render(
	const FInverseTransformPassConfig& InConfig,
	const FInverseTransformPassParam& Param,
	FRHITexture* XDebugTextureRef,
	FRHITexture* YDebugTextureRef,
	FRHITexture* ZDebugTextureRef)
{
	if (Config != InConfig)
	{
		ConfigurePass(InConfig);
	}

	if (IsValidPass())
	{
		ENQUEUE_RENDER_COMMAND(InverseTransformPassCommand)
		(
			[Param, XDebugTextureRef, YDebugTextureRef, ZDebugTextureRef, this](FRHICommandListImmediate& RHICmdList)
			{
				check(IsInRenderingThread());

				RenderInverseTransform(RHICmdList, Param, 0, XDebugTextureRef);
				RenderInverseTransform(RHICmdList, Param, 1, YDebugTextureRef);
				RenderInverseTransform(RHICmdList, Param, 2, ZDebugTextureRef);
			}
		);
	}
}

void FInverseTransformPass::RenderInverseTransform(
	FRHICommandListImmediate& RHICmdList,
	const FInverseTransformPassParam& Param,
	int32 TextureIndex,
	FRHITexture* DebugTextureRef)
{
	// Set up compute shader
	TShaderMapRef<FInverseTransformComputeShader> InverseTransformComputeShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
	RHICmdList.SetComputeShader(InverseTransformComputeShader->GetComputeShader());

	// Set up ping pong texture
	FTexture2DRHIRef           PingPongTextures[2];
	FShaderResourceViewRHIRef  PingPongTextureSRVs[2];
	FUnorderedAccessViewRHIRef PingPongTextureUAVs[2];

	PingPongTextures[0] = Param.FourierComponentTextures[TextureIndex];
	PingPongTextureSRVs[0] = Param.FourierComponentTextureSRVs[TextureIndex];
	PingPongTextureUAVs[0] = Param.FourierComponentTextureUAVs[TextureIndex];

	PingPongTextures[1] = OutputInverseTransformTextures[TextureIndex];
	PingPongTextureSRVs[1] = OutputInverseTransformTextureSRVs[TextureIndex];
	PingPongTextureUAVs[1] = OutputInverseTransformTextureUAVs[TextureIndex];

	const uint32 StageCount = StaticCast<uint32>(FMath::Log2(Config.TextureHeight));

	uint32 FrameIndex = 0;

	// Start ping pong
	for (uint32 Direction = 0; Direction < 2; ++Direction)
	{
		for (uint32 Stage = 0; Stage < StageCount; ++Stage, ++FrameIndex)
		{
			// Bind shader uniform
			FInverseTransformComputeShader::FParameters UniformParam;
			UniformParam.Stage = Stage;
			UniformParam.StageCount = StageCount;
			UniformParam.Direction = Direction;
			InverseTransformComputeShader->SetShaderParameters(RHICmdList, UniformParam);

			// Bind shader textures
			const uint32 InputIndex = FrameIndex % 2;
			const uint32 OutputIndex = (FrameIndex + 1) % 2;
			FUnorderedAccessViewRHIRef OutputUAV = PingPongTextureUAVs[OutputIndex];
			FShaderResourceViewRHIRef InputSRV = PingPongTextureSRVs[InputIndex];
			FShaderResourceViewRHIRef TwiddleFactorsSRV = Param.TwiddleFactorsTextureSRV;
			InverseTransformComputeShader->BindShaderTextures(RHICmdList, OutputUAV, TwiddleFactorsSRV, InputSRV);

			// Dispatch shader
			const int ThreadGroupCountX = StaticCast<int>(Config.TextureWidth / 32);
			const int ThreadGroupCountY = StaticCast<int>(Config.TextureHeight / 32);
			DispatchComputeShader(RHICmdList, *InverseTransformComputeShader, ThreadGroupCountX, ThreadGroupCountY, 1);

			// Unbind shader textures
			InverseTransformComputeShader->UnbindShaderTextures(RHICmdList);
		}
	}

	// The final output ping pong texture is always the original input texture. So copy to output texture here.
	// Alternatively, outside the pass, the renderer could reuse the input texture for next render pass. However,
	// that is tightly coupled with the knowledge that the texture will be used right away.
	RHICmdList.CopyToResolveTarget(Param.FourierComponentTextures[TextureIndex], OutputInverseTransformTextures[TextureIndex], FResolveParams());

	if (DebugTextureRef)
	{
		RHICmdList.CopyToResolveTarget(OutputInverseTransformTextures[TextureIndex], DebugTextureRef, FResolveParams());
	}
}
