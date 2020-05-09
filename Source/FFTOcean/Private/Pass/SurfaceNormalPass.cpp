// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Pass/SurfaceNormalPass.h"
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

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FSurfaceNormalComputeShaderParameters, )
	SHADER_PARAMETER(float, NormalStrength)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FSurfaceNormalComputeShaderParameters, "SurfaceNormalUniform");

class FSurfaceNormalComputeShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSurfaceNormalComputeShader, Global);
	using FParameters = FSurfaceNormalComputeShaderParameters;

public:

	FSurfaceNormalComputeShader() {}
	FSurfaceNormalComputeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		OutputNormalTexture.Bind(Initializer.ParameterMap, TEXT("OutputNormalTexture"));
		InputDisplacementTexture.Bind(Initializer.ParameterMap, TEXT("InputDisplacementTexture"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

	void BindShaderTextures(FRHICommandList& RHICmdList, FUnorderedAccessViewRHIRef OutputTextureUAV, FShaderResourceViewRHIRef InputTextureSRV)
	{
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();
		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputNormalTexture, OutputTextureUAV);
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputDisplacementTexture, InputTextureSRV);
	}

	void UnbindShaderTextures(FRHICommandList& RHICmdList)
	{
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();
		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputNormalTexture, FUnorderedAccessViewRHIRef());
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputDisplacementTexture, FShaderResourceViewRHIRef());
	}

	void SetShaderParameters(FRHICommandList& RHICmdList, const FParameters& Parameters)
	{
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();
		SetUniformBufferParameterImmediate(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FParameters>(), Parameters);
	}

private:

	LAYOUT_FIELD(FShaderResourceParameter, OutputNormalTexture);
	LAYOUT_FIELD(FShaderResourceParameter, InputDisplacementTexture);
};

IMPLEMENT_GLOBAL_SHADER(FSurfaceNormalComputeShader, "/Plugin/FFTOcean/SurfaceNormalComputeShader.usf", "ComputeSurfaceNormal", SF_Compute)

inline bool operator==(const FSurfaceNormalPassConfig& A, const FSurfaceNormalPassConfig& B)
{
	return FMemory::Memcmp(&A, &B, sizeof(FSurfaceNormalPassConfig)) == 0;
}

inline bool operator!=(const FSurfaceNormalPassConfig& A, const FSurfaceNormalPassConfig& B)
{
	return !(A == B);
}

FSurfaceNormalPass::FSurfaceNormalPass()
{

}

FSurfaceNormalPass::~FSurfaceNormalPass()
{
	ReleaseRenderResource();
}

bool FSurfaceNormalPass::IsValidPass() const
{
	bool bValid = !!OutputSurfaceNormalTexture
		&& !!OutputSurfaceNormalTextureSRV
		&& !!OutputSurfaceNormalTextureUAV;

	return bValid;
}

void FSurfaceNormalPass::ReleaseRenderResource()
{
	SafeReleaseTextureResource(OutputSurfaceNormalTexture);
	SafeReleaseTextureResource(OutputSurfaceNormalTextureSRV);
	SafeReleaseTextureResource(OutputSurfaceNormalTextureUAV);
}

void FSurfaceNormalPass::Render(const FSurfaceNormalPassConfig& InConfig, const FSurfaceNormalPassParam& Param, FRHITexture* DebugTextureRef)
{
	if (Config != InConfig)
	{
		ConfigurePass(InConfig);
	}

	if (IsValidPass())
	{
		ENQUEUE_RENDER_COMMAND(SurfaceNormalPassCommand)
		(
			[InConfig, Param, DebugTextureRef, this](FRHICommandListImmediate& RHICmdList)
			{
				check(IsInRenderingThread());

				// Set up compute shader
				TShaderMapRef<FSurfaceNormalComputeShader> SurfaceNormalComputeShader(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
				RHICmdList.SetComputeShader(SurfaceNormalComputeShader.GetComputeShader());

				// Bind shader textures
				SurfaceNormalComputeShader->BindShaderTextures(RHICmdList, OutputSurfaceNormalTextureUAV, Param.DisplacementTextureSRV);

				// Bind shader uniform
				FSurfaceNormalComputeShader::FParameters UniformParam;
				UniformParam.NormalStrength = Param.NormalStrength;
				SurfaceNormalComputeShader->SetShaderParameters(RHICmdList, UniformParam);

				// Dispatch shader
				const int ThreadGroupCountX = StaticCast<int>(Config.TextureWidth / 32);
				const int ThreadGroupCountY = StaticCast<int>(Config.TextureHeight / 32);
				DispatchComputeShader(RHICmdList, SurfaceNormalComputeShader, ThreadGroupCountX, ThreadGroupCountY, 1);

				// Unbind shader textures
				SurfaceNormalComputeShader->UnbindShaderTextures(RHICmdList);

				// Debug drawing
				if (DebugTextureRef)
				{
					RHICmdList.CopyToResolveTarget(OutputSurfaceNormalTexture, DebugTextureRef, FResolveParams());
				}
			}
		);
	}
}

void FSurfaceNormalPass::ConfigurePass(const FSurfaceNormalPassConfig& InConfig)
{
	// Always release current resource before creating new render resources
	ReleaseRenderResource();

	Config = InConfig;

	FRHIResourceCreateInfo CreateInfo;
	uint32 TextureWidth  = InConfig.TextureWidth;
	uint32 TextureHeight = InConfig.TextureHeight;

	OutputSurfaceNormalTexture    = RHICreateTexture2D(TextureWidth, TextureHeight, PF_FloatRGBA, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
	OutputSurfaceNormalTextureUAV = RHICreateUnorderedAccessView(OutputSurfaceNormalTexture);
	OutputSurfaceNormalTextureSRV = RHICreateShaderResourceView(OutputSurfaceNormalTexture, 0);
}
