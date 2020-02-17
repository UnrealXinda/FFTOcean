// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

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
	SHADER_PARAMETER(float,     Time)
	SHADER_PARAMETER(float,     WaveAmplitude)
	SHADER_PARAMETER(FVector2D, WindSpeed)
	SHADER_PARAMETER(FVector2D, WaveDirection)
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
		Ar << OutputPhillipsFourierTexture << InputGaussianNoiseTexture;
		return bShaderHasOutdatedParameters;
	}

	void BindShaderTextures(FRHICommandList& RHICmdList, FUnorderedAccessViewRHIRef OutputTextureUAV, FShaderResourceViewRHIRef InputTextureSRV)
	{
		FRHIComputeShader* ComputeShaderRHI = GetComputeShader();

		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputPhillipsFourierTexture, OutputTextureUAV);
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputGaussianNoiseTexture, InputTextureSRV);
	}

	void UnbindShaderTextures(FRHICommandList& RHICmdList)
	{
		FRHIComputeShader* ComputeShaderRHI = GetComputeShader();

		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputPhillipsFourierTexture, FUnorderedAccessViewRHIRef());
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputGaussianNoiseTexture, FShaderResourceViewRHIRef());
	}

	void SetShaderParameters(FRHICommandList& RHICmdList, const FPhillipsFourierComputeShaderParameters& Parameters)
	{
		FRHIComputeShader* ComputeShaderRHI = GetComputeShader();
		SetUniformBufferParameterImmediate(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FPhillipsFourierComputeShaderParameters>(), Parameters);
	}

private:

	FShaderResourceParameter OutputPhillipsFourierTexture;
	FShaderResourceParameter InputGaussianNoiseTexture;
};

IMPLEMENT_SHADER_TYPE(, FPhillipsFourierComputeShader, TEXT("/Plugin/FFTOcean/PhillipsFourierComputeShader.usf"), TEXT("ComputePhillipsFourier"), SF_Compute);

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
	bool bValid = !!OutputPhillipsFourierTexture;
	bValid &= !!OutputPhillipsFourierTextureUAV;
	bValid &= !!OutputPhillipsFourierTextureSRV;
	bValid &= !!InputGaussianNoiseTexture;
	bValid &= !!InputGaussianNoiseTextureSRV;

	return bValid;
}

void FPhillipsFourierPass::ReleaseRenderResource()
{
	SafeReleaseTextureResource(OutputPhillipsFourierTexture);
	SafeReleaseTextureResource(OutputPhillipsFourierTextureUAV);
	SafeReleaseTextureResource(OutputPhillipsFourierTextureSRV);
	SafeReleaseTextureResource(InputGaussianNoiseTextureSRV);

	// Don't release InputGaussianNoiseTexture since it's not created by the pass
}

void FPhillipsFourierPass::ConfigurePass(const FPhillipsFourierPassConfig& InConfig)
{
	if (Config != InConfig)
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

		if (InConfig.GaussianNoiseTextureRef)
		{
			InputGaussianNoiseTexture = StaticCast<FRHITexture2D*>(InConfig.GaussianNoiseTextureRef);
			InputGaussianNoiseTextureSRV = RHICreateShaderResourceView(InputGaussianNoiseTexture, 0);
		}
	}
}

void FPhillipsFourierPass::Render(const FPhillipsFourierPassParam& Param, FRHITexture* DebugTextureRef)
{
	if (IsValidPass())
	{
		ENQUEUE_RENDER_COMMAND(OutputPhillipsFourierPassCommand)
		(
			[Param, DebugTextureRef, this](FRHICommandListImmediate& RHICmdList)
			{
				check(IsInRenderingThread());

				// Bind shader textures
				TShaderMapRef<FPhillipsFourierComputeShader> PhillipsFourierComputeShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
				RHICmdList.SetComputeShader(PhillipsFourierComputeShader->GetComputeShader());
				PhillipsFourierComputeShader->BindShaderTextures(RHICmdList, OutputPhillipsFourierTextureUAV, InputGaussianNoiseTextureSRV);

				// Bind shader uniform
				FPhillipsFourierComputeShaderParameters UniformParam;
				UniformParam.Time = Param.Time;
				UniformParam.WaveAmplitude = Param.WaveAmplitude;
				UniformParam.WindSpeed = Param.WindSpeed;
				UniformParam.WaveDirection = Param.WaveDirection;
				PhillipsFourierComputeShader->SetShaderParameters(RHICmdList, UniformParam);

				// Dispatch shader
				const int ThreadGroupCountX = StaticCast<int>(Config.TextureWidth / 32);
				const int ThreadGroupCountY = StaticCast<int>(Config.TextureHeight / 32);
				DispatchComputeShader(RHICmdList, *PhillipsFourierComputeShader, ThreadGroupCountX, ThreadGroupCountY, 1);

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
