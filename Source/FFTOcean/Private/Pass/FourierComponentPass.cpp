// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Pass/FourierComponentPass.h"
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

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FFourierComponentComputeShaderParameters, )
	SHADER_PARAMETER(float,     Time)
	SHADER_PARAMETER(FVector2D, WindSpeed)
	SHADER_PARAMETER(FVector2D, WaveDirection)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FFourierComponentComputeShaderParameters, "FourierComponentUniform");

class FFourierComponentComputeShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FFourierComponentComputeShader, Global)
	using FParameters = FFourierComponentComputeShaderParameters;

public:
	FFourierComponentComputeShader() {}
	FFourierComponentComputeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		OutputSurfaceTextureX.Bind(Initializer.ParameterMap, TEXT("OutputSurfaceTextureX"));
		OutputSurfaceTextureY.Bind(Initializer.ParameterMap, TEXT("OutputSurfaceTextureY"));
		OutputSurfaceTextureZ.Bind(Initializer.ParameterMap, TEXT("OutputSurfaceTextureZ"));
		InputPhillipsFourierTexture.Bind(Initializer.ParameterMap, TEXT("InputPhillipsFourierTexture"));
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
		Ar << OutputSurfaceTextureX << OutputSurfaceTextureY << OutputSurfaceTextureZ << InputPhillipsFourierTexture;
		return bShaderHasOutdatedParameters;
	}

	void BindShaderTextures(
		FRHICommandList& RHICmdList,
		FUnorderedAccessViewRHIRef OutputTextureXUAV,
		FUnorderedAccessViewRHIRef OutputTextureYUAV,
		FUnorderedAccessViewRHIRef OutputTextureZUAV,
		FShaderResourceViewRHIRef InputTextureSRV)
	{
		FRHIComputeShader* ComputeShaderRHI = GetComputeShader();

		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputSurfaceTextureX, OutputTextureXUAV);
		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputSurfaceTextureY, OutputTextureYUAV);
		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputSurfaceTextureZ, OutputTextureZUAV);
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputPhillipsFourierTexture, InputTextureSRV);
	}

	void UnbindShaderTextures(FRHICommandList& RHICmdList)
	{
		FRHIComputeShader* ComputeShaderRHI = GetComputeShader();

		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputSurfaceTextureX, FUnorderedAccessViewRHIRef());
		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputSurfaceTextureY, FUnorderedAccessViewRHIRef());
		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputSurfaceTextureZ, FUnorderedAccessViewRHIRef());
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputPhillipsFourierTexture, FShaderResourceViewRHIRef());
	}

	void SetShaderParameters(FRHICommandList& RHICmdList, const FParameters& Parameters)
	{
		FRHIComputeShader* ComputeShaderRHI = GetComputeShader();
		SetUniformBufferParameterImmediate(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FParameters>(), Parameters);
	}

private:

	FShaderResourceParameter OutputSurfaceTextureX;
	FShaderResourceParameter OutputSurfaceTextureY;
	FShaderResourceParameter OutputSurfaceTextureZ;
	FShaderResourceParameter InputPhillipsFourierTexture;
};

IMPLEMENT_SHADER_TYPE(, FFourierComponentComputeShader, TEXT("/Plugin/FFTOcean/FourierComponentComputeShader.usf"), TEXT("ComputeFourierComponent"), SF_Compute);

inline bool operator==(const FFourierComponentPassConfig& A, const FFourierComponentPassConfig& B)
{
	return FMemory::Memcmp(&A, &B, sizeof(FFourierComponentPassConfig)) == 0;
}

inline bool operator!=(const FFourierComponentPassConfig& A, const FFourierComponentPassConfig& B)
{
	return !(A == B);
}

FFourierComponentPass::FFourierComponentPass()
{
}

FFourierComponentPass::~FFourierComponentPass()
{
	ReleaseRenderResource();
}

bool FFourierComponentPass::IsValidPass() const
{
	bool bValid = true;

	for (FTexture2DRHIRef Texture : OutputSurfaceTextures)
	{
		bValid &= !!Texture;
	}

	for (FShaderResourceViewRHIRef TextureSRV : OutputSurfaceTexturesSRV)
	{
		bValid &= !!TextureSRV;
	}

	for (FUnorderedAccessViewRHIRef TextureUAV : OutputSurfaceTexturesUAV)
	{
		bValid &= !!TextureUAV;
	}

	return bValid;
}

void FFourierComponentPass::ReleaseRenderResource()
{
	for (FTexture2DRHIRef Texture : OutputSurfaceTextures)
	{
		SafeReleaseTextureResource(Texture);
	}
	
	for (FShaderResourceViewRHIRef TextureSRV : OutputSurfaceTexturesSRV)
	{
		SafeReleaseTextureResource(TextureSRV);
	}

	for (FUnorderedAccessViewRHIRef TextureUAV : OutputSurfaceTexturesUAV)
	{
		SafeReleaseTextureResource(TextureUAV);
	}
}

void FFourierComponentPass::ConfigurePass(const FFourierComponentPassConfig& InConfig)
{
	if (Config != InConfig)
	{
		// Always release current resource before creating new render resources
		ReleaseRenderResource();

		Config = InConfig;

		FRHIResourceCreateInfo CreateInfo;
		uint32 TextureWidth = InConfig.TextureWidth;
		uint32 TextureHeight = InConfig.TextureHeight;

		for (int32 Index = 0; Index < 3; ++Index)
		{
			OutputSurfaceTextures[Index] = RHICreateTexture2D(TextureWidth, TextureHeight, PF_G32R32F, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
			OutputSurfaceTexturesSRV[Index] = RHICreateShaderResourceView(OutputSurfaceTextures[Index], 0);
			OutputSurfaceTexturesUAV[Index] = RHICreateUnorderedAccessView(OutputSurfaceTextures[Index]);
		}
	}
}

void FFourierComponentPass::Render(const FFourierComponentPassParam& Param, FRHITexture* XDebugTextureRef, FRHITexture* YDebugTextureRef, FRHITexture* ZDebugTextureRef)
{
	if (IsValidPass())
	{
		ENQUEUE_RENDER_COMMAND(FourierComponentPassCommand)
		(
			[Param, XDebugTextureRef, YDebugTextureRef, ZDebugTextureRef, this](FRHICommandListImmediate& RHICmdList)
			{
				check(IsInRenderingThread());

				// Bind shader textures
				TShaderMapRef<FFourierComponentComputeShader> FourierComponentComputeShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
				RHICmdList.SetComputeShader(FourierComponentComputeShader->GetComputeShader());
				FourierComponentComputeShader->BindShaderTextures(
					RHICmdList,
					OutputSurfaceTexturesUAV[0],
					OutputSurfaceTexturesUAV[1],
					OutputSurfaceTexturesUAV[2],
					Param.PhillipsFourierTextureSRV);

				// Bind shader uniform
				FFourierComponentComputeShader::FParameters UniformParam;
				UniformParam.Time = Param.Time;
				FourierComponentComputeShader->SetShaderParameters(RHICmdList, UniformParam);

				// Dispatch shader
				const int ThreadGroupCountX = StaticCast<int>(Config.TextureWidth / 32);
				const int ThreadGroupCountY = StaticCast<int>(Config.TextureHeight / 32);
				DispatchComputeShader(RHICmdList, *FourierComponentComputeShader, ThreadGroupCountX, ThreadGroupCountY, 1);

				// Unbind shader textures
				FourierComponentComputeShader->UnbindShaderTextures(RHICmdList);

				// Debug drawing
				if (XDebugTextureRef)
				{
					RHICmdList.CopyToResolveTarget(OutputSurfaceTextures[0], XDebugTextureRef, FResolveParams());
				}

				if (YDebugTextureRef)
				{
					RHICmdList.CopyToResolveTarget(OutputSurfaceTextures[1], YDebugTextureRef, FResolveParams());
				}

				if (ZDebugTextureRef)
				{
					RHICmdList.CopyToResolveTarget(OutputSurfaceTextures[2], ZDebugTextureRef, FResolveParams());
				}
			}
		);
	}
}
