// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Pass/PassUtil.h"

struct FFourierComponentPassConfig
{
	uint32 TextureWidth;
	uint32 TextureHeight;
};

struct FFourierComponentPassParam
{
	float                     Time;
	FShaderResourceViewRHIRef PhillipsFourierTextureSRV;
};

class FFourierComponentPass final : public FOceanRenderPass
{
public:

	FFourierComponentPass();
	~FFourierComponentPass();

	virtual bool IsValidPass() const override;
	virtual void ReleaseRenderResource() override;

	void Render(
		const FFourierComponentPassConfig& InConfig,
		const FFourierComponentPassParam& Param,
		FRHITexture* XDebugTextureRef,
		FRHITexture* YDebugTextureRef,
		FRHITexture* ZDebugTextureRef);

	FORCEINLINE FTexture2DRHIRef GetSurfaceTexture(int Index) const
	{
		return OutputSurfaceTextures[Index];
	}

	FORCEINLINE FShaderResourceViewRHIRef GetSurfaceTextureSRV(int Index) const
	{
		return OutputSurfaceTexturesSRV[Index];
	}

	FORCEINLINE FUnorderedAccessViewRHIRef GetSurfaceTextureUAV(int Index) const
	{
		return OutputSurfaceTexturesUAV[Index];
	}

private:

	FTexture2DRHIRef            OutputSurfaceTextures[3];
	FShaderResourceViewRHIRef   OutputSurfaceTexturesSRV[3];
	FUnorderedAccessViewRHIRef  OutputSurfaceTexturesUAV[3];

	FFourierComponentPassConfig Config;

	void ConfigurePass(const FFourierComponentPassConfig& InConfig);
};