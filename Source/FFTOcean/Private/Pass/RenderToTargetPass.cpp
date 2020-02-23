// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Pass/RenderToTargetPass.h"
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

struct FRenderToTargetPassVertex
{
	FVector4  Position;
	FVector2D UV;
};

class FRenderToTargetPassVertexDeclaration : public FRenderResource
{
public:

	FRHIVertexDeclaration* VertexDeclarationRHI;

	virtual void InitRHI() override
	{
		FVertexDeclarationElementList Elements;
		uint32 Stride = sizeof(FRenderToTargetPassVertex);
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FRenderToTargetPassVertex, Position), VET_Float4, 0, Stride));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FRenderToTargetPassVertex, UV),       VET_Float2, 1, Stride));

		VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
	}

	virtual void ReleaseRHI()
	{
		if (VertexDeclarationRHI)
		{
			VertexDeclarationRHI->Release();
		}
	}
};


class FRenderToTargetPassVertexBuffer : public FVertexBuffer
{
public:

	void InitRHI() override
	{
		TResourceArray<FRenderToTargetPassVertex, VERTEXBUFFER_ALIGNMENT> Vertices;
		Vertices.SetNumUninitialized(4);

		Vertices[0] = { FVector4(1, 1, 0, 1),   FVector2D(1, 1) };
		Vertices[1] = { FVector4(-1, 1, 0, 1),  FVector2D(0, 1) };
		Vertices[2] = { FVector4(1, -1, 0, 1),  FVector2D(1, 0) };
		Vertices[3] = { FVector4(-1, -1, 0, 1), FVector2D(0, 0) };

		FRHIResourceCreateInfo CreateInfo(&Vertices);
		VertexBufferRHI = RHICreateVertexBuffer(Vertices.GetResourceDataSize(), BUF_Static, CreateInfo);
	}
};

class FRenderToTargetPassIndexBuffer : public FIndexBuffer
{
public:

	void InitRHI() override
	{
		const uint16 Indices[] = { 0, 1, 2, 2, 1, 3};
		TResourceArray<uint16, INDEXBUFFER_ALIGNMENT> IndexBuffer;
		uint32 NumIndices = UE_ARRAY_COUNT(Indices);
		IndexBuffer.AddUninitialized(NumIndices);
		FMemory::Memcpy(IndexBuffer.GetData(), Indices, NumIndices * sizeof(uint16));

		// Create index buffer. Fill buffer with initial data upon creation
		FRHIResourceCreateInfo CreateInfo(&IndexBuffer);
		IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint16), IndexBuffer.GetResourceDataSize(), BUF_Static, CreateInfo);
	}
};

class FRenderToTargetPassVertexShader : public FGlobalShader
{

	DECLARE_SHADER_TYPE(FRenderToTargetPassVertexShader, Global);

public:

	FRenderToTargetPassVertexShader() {}
	FRenderToTargetPassVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer) {}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		return bShaderHasOutdatedParameters;
	}
};

class FRenderToTargetPassPixelShader : public FGlobalShader
{

	DECLARE_SHADER_TYPE(FRenderToTargetPassPixelShader, Global);

public:

	FRenderToTargetPassPixelShader() {}
	FRenderToTargetPassPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
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

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);

		Ar << InputDisplacementTexture;

		return bShaderHasOutdatedParameters;
	}

	void BindShaderTextures(FRHICommandList& RHICmdList, FShaderResourceViewRHIRef InputTextureSRV)
	{
		FRHIPixelShader* PixelShaderRHI = GetPixelShader();
		SetSRVParameter(RHICmdList, PixelShaderRHI, InputDisplacementTexture, InputTextureSRV);
	}

	void UnbindShaderTextures(FRHICommandList& RHICmdList)
	{
		FRHIPixelShader* PixelShaderRHI = GetPixelShader();
		SetSRVParameter(RHICmdList, PixelShaderRHI, InputDisplacementTexture, FShaderResourceViewRHIRef());
	}

private:

	FShaderResourceParameter InputDisplacementTexture;
};

IMPLEMENT_SHADER_TYPE(, FRenderToTargetPassVertexShader, TEXT("/Plugin/FFTOcean/RenderToTargetPassShader.usf"), TEXT("MainVS"), SF_Vertex)
IMPLEMENT_SHADER_TYPE(, FRenderToTargetPassPixelShader,  TEXT("/Plugin/FFTOcean/RenderToTargetPassShader.usf"), TEXT("MainPS"), SF_Pixel)

inline bool operator==(const FRenderToTargetPassConfig& A, const FRenderToTargetPassConfig& B)
{
	return FMemory::Memcmp(&A, &B, sizeof(FRenderToTargetPassConfig)) == 0;
}

inline bool operator!=(const FRenderToTargetPassConfig& A, const FRenderToTargetPassConfig& B)
{
	return !(A == B);
}

FRenderToTargetPass::FRenderToTargetPass() :
	RenderToTargetVertexBuffer(new FRenderToTargetPassVertexBuffer()),
	RenderToTargetIndexBuffer(new FRenderToTargetPassIndexBuffer())
{

}

FRenderToTargetPass::~FRenderToTargetPass()
{
	ReleaseRenderResource();
}

bool FRenderToTargetPass::IsValidPass() const
{
	return true;
}

void FRenderToTargetPass::ReleaseRenderResource()
{
	RenderToTargetVertexBuffer->ReleaseRHI();
	RenderToTargetIndexBuffer->ReleaseRHI();
}

void FRenderToTargetPass::Render(const FRenderToTargetPassConfig& InConfig, const FRenderToTargetPassParam& Param, UTextureRenderTarget2D* TargetTexture)
{
	if (Config != InConfig)
	{
		ConfigurePass(InConfig);
	}

	if (TargetTexture && IsValidPass())
	{
		ENQUEUE_RENDER_COMMAND(RenderToTargetPassCommand)
		(
			[InConfig, Param, TargetTexture, this](FRHICommandListImmediate& RHICmdList)
			{
				check(IsInRenderingThread());

				FTextureRenderTargetResource* TargetTextureResource = TargetTexture->GetRenderTargetResource();

				FRHIRenderPassInfo PassInfo(
					TargetTextureResource->GetRenderTargetTexture(),
					ERenderTargetActions::DontLoad_Store,
					nullptr
				);

				RHICmdList.BeginRenderPass(PassInfo, TEXT("RenderToTargetPass"));
				{
					FIntPoint DisplacementMapResolution(TargetTextureResource->GetSizeX(), TargetTextureResource->GetSizeY());

					// Update viewport
					RHICmdList.SetViewport(
						0.f, 0.f, 0.f,
						InConfig.TextureWidth, InConfig.TextureHeight, 1.f
					);

					// Setup shaders
					TShaderMap<FGlobalShaderType>* GlobalShaderMap = GetGlobalShaderMap(ERHIFeatureLevel::SM5);
					TShaderMapRef<FRenderToTargetPassVertexShader> VertexShader(GlobalShaderMap);
					TShaderMapRef<FRenderToTargetPassPixelShader> PixelShader(GlobalShaderMap);

					FRenderToTargetPassVertexDeclaration VertexDesc;
					VertexDesc.InitRHI();

					// Set the graphic pipeline state
					FGraphicsPipelineStateInitializer GraphicsPSOInit;
					RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
					GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
					GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
					GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
					GraphicsPSOInit.PrimitiveType = PT_TriangleList;
					GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = VertexDesc.VertexDeclarationRHI;
					GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
					GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
					SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

					// Update viewport
					RHICmdList.SetViewport(
						0.f, 0.f, 0.f,
						TargetTextureResource->GetSizeX(), TargetTextureResource->GetSizeY(), 1.f
					);

					PixelShader->BindShaderTextures(RHICmdList, Param.InverseTransformTextureSRV);
					RHICmdList.SetStreamSource(0, RenderToTargetVertexBuffer->VertexBufferRHI, 0);
					RHICmdList.DrawIndexedPrimitive(
						RenderToTargetIndexBuffer->IndexBufferRHI,
						0, // BaseVertexIndex
						0, // MinIndex
						4, // NumVertices
						0, // StartIndex
						2, // NumPrimitives
						1  // NumInstances
					);

					PixelShader->UnbindShaderTextures(RHICmdList);
				}

				RHICmdList.EndRenderPass();
			}
		);
	}
}

void FRenderToTargetPass::ConfigurePass(const FRenderToTargetPassConfig& InConfig)
{
	// Always release current resource before creating new render resources
	ReleaseRenderResource();

	Config = InConfig;

	RenderToTargetVertexBuffer->InitRHI();
	RenderToTargetIndexBuffer->InitRHI();
}
