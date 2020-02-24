// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Pass/PassUtil.h"

struct FSurfaceNormalPassConfig
{
	uint32 TextureWidth;
	uint32 TextureHeight;
};

struct FSurfaceNormalPassParam
{
	FShaderResourceViewRHIRef DisplacementTextureSRV;
	float NormalStrength;
};

class FSurfaceNormalPass final : public FOceanRenderPass
{
public:

	FSurfaceNormalPass();
	~FSurfaceNormalPass();

	virtual bool IsValidPass() const override;
	virtual void ReleaseRenderResource() override;

	void Render(const FSurfaceNormalPassConfig& InConfig, const FSurfaceNormalPassParam& Param, FRHITexture* DebugTextureRef);

private:

	FSurfaceNormalPassConfig Config;

	FTexture2DRHIRef           OutputSurfaceNormalTexture;
	FUnorderedAccessViewRHIRef OutputSurfaceNormalTextureUAV;
	FShaderResourceViewRHIRef  OutputSurfaceNormalTextureSRV;

	void ConfigurePass(const FSurfaceNormalPassConfig& InConfig);
};