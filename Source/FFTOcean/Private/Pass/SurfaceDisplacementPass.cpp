// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Pass/SurfaceDisplacementPass.h"
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

class FSurfaceDisplacementComputeShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSurfaceDisplacementComputeShader, Global);

public:

	FSurfaceDisplacementComputeShader() {}
	FSurfaceDisplacementComputeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		OutputDisplacementTexture.Bind(Initializer.ParameterMap, TEXT("OutputDisplacementTexture"));
		InputDisplacementTextureX.Bind(Initializer.ParameterMap, TEXT("InputDisplacementTextureX"));
		InputDisplacementTextureY.Bind(Initializer.ParameterMap, TEXT("InputDisplacementTextureY"));
		InputDisplacementTextureZ.Bind(Initializer.ParameterMap, TEXT("InputDisplacementTextureZ"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

	void BindShaderTextures(
		FRHICommandList& RHICmdList,
		FUnorderedAccessViewRHIRef OutputTextureUAV,
		FShaderResourceViewRHIRef XInputTextureSRV,
		FShaderResourceViewRHIRef YInputTextureSRV,
		FShaderResourceViewRHIRef ZInputTextureSRV)
	{
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();
		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputDisplacementTexture, OutputTextureUAV);
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputDisplacementTextureX, XInputTextureSRV);
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputDisplacementTextureY, YInputTextureSRV);
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputDisplacementTextureZ, ZInputTextureSRV);
	}

	void UnbindShaderTextures(FRHICommandList& RHICmdList)
	{
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();
		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputDisplacementTexture, FUnorderedAccessViewRHIRef());
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputDisplacementTextureX, FShaderResourceViewRHIRef());
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputDisplacementTextureY, FShaderResourceViewRHIRef());
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputDisplacementTextureZ, FShaderResourceViewRHIRef());
	}

private:

	LAYOUT_FIELD(FShaderResourceParameter, OutputDisplacementTexture);
	LAYOUT_FIELD(FShaderResourceParameter, InputDisplacementTextureX);
	LAYOUT_FIELD(FShaderResourceParameter, InputDisplacementTextureY);
	LAYOUT_FIELD(FShaderResourceParameter, InputDisplacementTextureZ);
};

IMPLEMENT_GLOBAL_SHADER(FSurfaceDisplacementComputeShader, "/Plugin/FFTOcean/SurfaceDisplacementComputeShader.usf", "ComputeSurfaceDisplacement", SF_Compute)

inline bool operator==(const FSurfaceDisplacementPassConfig& A, const FSurfaceDisplacementPassConfig& B)
{
	return FMemory::Memcmp(&A, &B, sizeof(FSurfaceDisplacementPassConfig)) == 0;
}

inline bool operator!=(const FSurfaceDisplacementPassConfig& A, const FSurfaceDisplacementPassConfig& B)
{
	return !(A == B);
}

FSurfaceDisplacementPass::FSurfaceDisplacementPass()
{

}

FSurfaceDisplacementPass::~FSurfaceDisplacementPass()
{
	ReleaseRenderResource();
}

bool FSurfaceDisplacementPass::IsValidPass() const
{
	bool bValid = !!OutputSurfaceDisplacementTexture
		&& !!OutputSurfaceDisplacementTextureSRV
		&& !!OutputSurfaceDisplacementTextureUAV;

	return bValid;
}

void FSurfaceDisplacementPass::ReleaseRenderResource()
{
	SafeReleaseTextureResource(OutputSurfaceDisplacementTexture);
	SafeReleaseTextureResource(OutputSurfaceDisplacementTextureSRV);
	SafeReleaseTextureResource(OutputSurfaceDisplacementTextureUAV);
}

void FSurfaceDisplacementPass::Render(const FSurfaceDisplacementPassConfig& InConfig, const FSurfaceDisplacementPassParam& Param, FRHITexture* DebugTextureRef)
{
	if (Config != InConfig)
	{
		ConfigurePass(InConfig);
	}

	if (IsValidPass())
	{
		ENQUEUE_RENDER_COMMAND(SurfaceDisplacementPassCommand)
		(
			[InConfig, Param, DebugTextureRef, this](FRHICommandListImmediate& RHICmdList)
			{
				check(IsInRenderingThread());

				TShaderMapRef<FSurfaceDisplacementComputeShader> SurfaceDisplacementComputeShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
				RHICmdList.SetComputeShader(SurfaceDisplacementComputeShader.GetComputeShader());

				// Bind shader textures
				SurfaceDisplacementComputeShader->BindShaderTextures(
					RHICmdList,
					OutputSurfaceDisplacementTextureUAV,
					Param.InverseTransformTextureSRVs[0],
					Param.InverseTransformTextureSRVs[1],
					Param.InverseTransformTextureSRVs[2]);

				// Dispatch shader
				const int ThreadGroupCountX = StaticCast<int>(Config.TextureWidth / 32);
				const int ThreadGroupCountY = StaticCast<int>(Config.TextureHeight / 32);
				DispatchComputeShader(RHICmdList, SurfaceDisplacementComputeShader, ThreadGroupCountX, ThreadGroupCountY, 1);

				// Unbind shader textures
				SurfaceDisplacementComputeShader->UnbindShaderTextures(RHICmdList);

				// Debug drawing
				if (DebugTextureRef)
				{
					RHICmdList.CopyToResolveTarget(OutputSurfaceDisplacementTexture, DebugTextureRef, FResolveParams());
				}
			}
		);
	}
}

void FSurfaceDisplacementPass::ConfigurePass(const FSurfaceDisplacementPassConfig& InConfig)
{
	// Always release current resource before creating new render resources
	ReleaseRenderResource();

	Config = InConfig;

	FRHIResourceCreateInfo CreateInfo;
	uint32 TextureWidth  = InConfig.TextureWidth;
	uint32 TextureHeight = InConfig.TextureHeight;

	OutputSurfaceDisplacementTexture    = RHICreateTexture2D(TextureWidth, TextureHeight, PF_FloatRGBA, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
	OutputSurfaceDisplacementTextureUAV = RHICreateUnorderedAccessView(OutputSurfaceDisplacementTexture);
	OutputSurfaceDisplacementTextureSRV = RHICreateShaderResourceView(OutputSurfaceDisplacementTexture, 0);
}
