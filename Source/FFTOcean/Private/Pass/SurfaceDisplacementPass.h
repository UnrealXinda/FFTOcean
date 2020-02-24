// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Pass/PassUtil.h"

struct FSurfaceDisplacementPassConfig
{
	uint32 TextureWidth;
	uint32 TextureHeight;
};

struct FSurfaceDisplacementPassParam
{
	FShaderResourceViewRHIRef InverseTransformTextureSRVs[3];
};

class FSurfaceDisplacementPass final : public FOceanRenderPass
{
public:

	FSurfaceDisplacementPass();
	~FSurfaceDisplacementPass();

	virtual bool IsValidPass() const override;
	virtual void ReleaseRenderResource() override;

	void Render(const FSurfaceDisplacementPassConfig& InConfig, const FSurfaceDisplacementPassParam& Param, FRHITexture* DebugTextureRef);

	FORCEINLINE FShaderResourceViewRHIRef GetSurfaceDisplacementTextureSRV() const
	{
		return OutputSurfaceDisplacementTextureSRV;
	}

private:

	FSurfaceDisplacementPassConfig Config;

	FTexture2DRHIRef           OutputSurfaceDisplacementTexture;
	FUnorderedAccessViewRHIRef OutputSurfaceDisplacementTextureUAV;
	FShaderResourceViewRHIRef  OutputSurfaceDisplacementTextureSRV;

	void ConfigurePass(const FSurfaceDisplacementPassConfig& InConfig);
};