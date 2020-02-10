// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Pass/TildeZeroPass.h"
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
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"

#include "Math/UnrealMathUtility.h"

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FTildeZeroComputeShaderParameters, )
	SHADER_PARAMETER(float,     Time)
	SHADER_PARAMETER(float,     WaveAmplitude)
	SHADER_PARAMETER(FVector2D, WindSpeed)
	SHADER_PARAMETER(FVector2D, WaveDirection)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FTildeZeroComputeShaderParameters, "TildeZeroUniform");

class FTildeZeroComputeShader : public FGlobalShader
{

	DECLARE_SHADER_TYPE(FTildeZeroComputeShader, Global)

public:
	FTildeZeroComputeShader() {}
	FTildeZeroComputeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		OutputTildeZeroTexture.Bind(Initializer.ParameterMap, TEXT("OutputTildeZeroTexture"));
		InputGaussianNoiseTexture.Bind(Initializer.ParameterMap, TEXT("InputGaussianNoiseTexture"));
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
		return bShaderHasOutdatedParameters;
	}

	void BindShaderTextures(FRHICommandList& RHICmdList, FUnorderedAccessViewRHIRef OutputTextureUAV, FShaderResourceViewRHIRef InputTextureSRV)
	{
		FRHIComputeShader* ComputeShaderRHI = GetComputeShader();

		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputTildeZeroTexture, OutputTextureUAV);
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputGaussianNoiseTexture, InputTextureSRV);
	}

	void UnbindShaderTextures(FRHICommandList& RHICmdList)
	{
		FRHIComputeShader* ComputeShaderRHI = GetComputeShader();

		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputTildeZeroTexture, FUnorderedAccessViewRHIRef());
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputGaussianNoiseTexture, FShaderResourceViewRHIRef());
	}

	void SetShaderParameters(FRHICommandList& RHICmdList, const FTildeZeroComputeShaderParameters& Parameters)
	{
		FRHIComputeShader* ComputeShaderRHI = GetComputeShader();
		SetUniformBufferParameterImmediate(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FTildeZeroComputeShaderParameters>(), Parameters);
	}

private:

	FShaderResourceParameter OutputTildeZeroTexture;
	FShaderResourceParameter InputGaussianNoiseTexture;
};

IMPLEMENT_SHADER_TYPE(, FTildeZeroComputeShader, TEXT("/Plugin/FFTOcean/TildeZeroComputeShader.usf"), TEXT("ComputeTildeZero"), SF_Compute);

FTildeZeroPass::FTildeZeroPass()
{
}

FTildeZeroPass::~FTildeZeroPass()
{
	ReleaseRenderResource();
}

bool FTildeZeroPass::IsValidPass() const
{
	bool bValid = !!OutputTildeZeroTexture;
	bValid &= !!OutputTildeZeroTextureUAV;
	bValid &= !!OutputTildeZeroTextureSRV;
	bValid &= !!InputGaussianNoiseTexture;
	bValid &= !!InputGaussianNoiseTextureSRV;

	return bValid;
}

void FTildeZeroPass::ReleaseRenderResource()
{
	SafeReleaseTextureResource(OutputTildeZeroTexture);
	SafeReleaseTextureResource(OutputTildeZeroTextureUAV);
	SafeReleaseTextureResource(OutputTildeZeroTextureSRV);
	SafeReleaseTextureResource(InputGaussianNoiseTextureSRV);

	// Don't release InputGaussianNoiseTexture since it's not created by the pass
}

void FTildeZeroPass::InitPass(const FTildeZeroPassConfig& InConfig)
{
	if (Config != InConfig)
	{
		// Always release current resource before creating new render resources
		ReleaseRenderResource();

		Config = InConfig;

		FRHIResourceCreateInfo CreateInfo;
		uint32 TextureWidth = InConfig.TextureWidth;
		uint32 TextureHeight = InConfig.TextureHeight;

		OutputTildeZeroTexture = RHICreateTexture2D(TextureWidth, TextureHeight, PF_FloatRGBA, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
		OutputTildeZeroTextureUAV = RHICreateUnorderedAccessView(OutputTildeZeroTexture);
		OutputTildeZeroTextureSRV = RHICreateShaderResourceView(OutputTildeZeroTexture, 0);

		if (InConfig.GaussianNoiseTextureRef)
		{
			InputGaussianNoiseTexture = StaticCast<FRHITexture2D*>(InConfig.GaussianNoiseTextureRef);
			InputGaussianNoiseTextureSRV = RHICreateShaderResourceView(InputGaussianNoiseTexture, 0);
		}
	}
}

void FTildeZeroPass::Render(const FTildeZeroPassParam& Param, FRHITexture* DebugTextureRef)
{
	if (IsValidPass())
	{
		ENQUEUE_RENDER_COMMAND(TildeZeroPassCommand)
		(
			[&Param, DebugTextureRef, this](FRHICommandListImmediate& RHICmdList)
			{
				check(IsInRenderingThread());

				// Bind shader textures
				TShaderMapRef<FTildeZeroComputeShader> TildeZeroComputeShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
				RHICmdList.SetComputeShader(TildeZeroComputeShader->GetComputeShader());
				TildeZeroComputeShader->BindShaderTextures(RHICmdList, OutputTildeZeroTextureUAV, InputGaussianNoiseTextureSRV);

				// Bind shader uniform
				FTildeZeroComputeShaderParameters UniformParam;
				UniformParam.Time = Param.Time;
				UniformParam.WindSpeed = Param.WindSpeed;
				UniformParam.WaveDirection = Param.WaveDirection;
				TildeZeroComputeShader->SetShaderParameters(RHICmdList, UniformParam);

				// Dispatch shader
				const int ThreadGroupCountX = StaticCast<int>(Config.TextureWidth / 32);
				const int ThreadGroupCountY = StaticCast<int>(Config.TextureHeight / 32);
				DispatchComputeShader(RHICmdList, *TildeZeroComputeShader, ThreadGroupCountX, ThreadGroupCountY, 1);

				// Unbind shader textures
				TildeZeroComputeShader->UnbindShaderTextures(RHICmdList);

				// Debug drawing
				if (DebugTextureRef)
				{
					RHICmdList.CopyToResolveTarget(OutputTildeZeroTexture, DebugTextureRef, FResolveParams());
				}
			}
		);
	}
}
