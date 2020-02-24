// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "RHI/Public/RHIResources.h"
#include "RHI/Public/RHICommandList.h"
#include "RenderCore/Public/GlobalShader.h"
#include "RenderCore/Public/ShaderParameterUtils.h"
#include "RenderCore/Public/ShaderParameterMacros.h"
#include "Engine/Classes/Engine/TextureRenderTarget2D.h"

#define SafeReleaseTextureResource(Texture)  \
	do {                                     \
		if (Texture.IsValid()) {             \
			Texture->Release();              \
		}                                    \
	} while(0);

class FOceanRenderPass
{
public:

	FOceanRenderPass() = default;
	virtual ~FOceanRenderPass() { }

	FOceanRenderPass(const FOceanRenderPass&) = delete;
	FOceanRenderPass(FOceanRenderPass&&)      = delete;

	FOceanRenderPass& operator=(const FOceanRenderPass&) = delete;
	FOceanRenderPass& operator=(FOceanRenderPass&&)      = delete;

	virtual bool IsValidPass() const     = 0;
	virtual void ReleaseRenderResource() = 0;
};

namespace FFTOcean
{
	inline FRHITexture* GetRHITextureFromRenderTarget(const UTextureRenderTarget2D* RenderTarget)
	{
		if (RenderTarget)
		{
			FTextureReferenceRHIRef OutputRenderTargetTextureRHI = RenderTarget->TextureReference.TextureReferenceRHI;

			checkf(OutputRenderTargetTextureRHI, TEXT("Can't get render target %d texture"));

			FRHITexture* RenderTargetTextureRef = OutputRenderTargetTextureRHI->GetTextureReference()->GetReferencedTexture();

			return RenderTargetTextureRef;
		}

		return nullptr;
	}

	inline FRHITexture* GetRHITextureFromTexture2D(const UTexture2D* Texture)
	{
		return Texture ? Texture->TextureReference.TextureReferenceRHI->GetReferencedTexture() : nullptr;
	}
}