// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Pass/PassUtil.h"

struct FPhillipsFourierPassConfig
{
	uint32       TextureWidth;
	uint32       TextureHeight;
};

struct FPhillipsFourierPassParam
{
	float     WaveAmplitude;
	FVector2D WindSpeed;
};

class FPhillipsFourierPass final : public FOceanRenderPass
{
public:

	FPhillipsFourierPass();
	~FPhillipsFourierPass();

	virtual bool IsValidPass() const override;
	virtual void ReleaseRenderResource() override;

	void Render(const FPhillipsFourierPassConfig& InConfig, const FPhillipsFourierPassParam& Param, FRHITexture* DebugTextureRef);

	FORCEINLINE FShaderResourceViewRHIRef GetPhillipsFourierTextureSRV() const
	{
		return OutputPhillipsFourierTextureSRV;
	}

private:

	FTexture2DRHIRef           OutputPhillipsFourierTexture;
	FUnorderedAccessViewRHIRef OutputPhillipsFourierTextureUAV;
	FShaderResourceViewRHIRef  OutputPhillipsFourierTextureSRV;

	FPhillipsFourierPassConfig Config;

	void ConfigurePass(const FPhillipsFourierPassConfig& InConfig);
};