// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Pass/PassUtil.h"

struct FPhillipsFourierPassConfig
{
	uint32       TextureWidth;
	uint32       TextureHeight;
	FRHITexture* GaussianNoiseTextureRef;
};

struct FPhillipsFourierPassParam
{
	float     Time;
	float     WaveAmplitude;
	FVector2D WindSpeed;      // Tilde W * v
	FVector2D WaveDirection;  // Tilde K * k
};

class FPhillipsFourierPass final : public FOceanRenderPass
{
public:

	FPhillipsFourierPass();
	~FPhillipsFourierPass();

	virtual bool IsValidPass() const override;
	virtual void ReleaseRenderResource() override;

	void ConfigurePass(const FPhillipsFourierPassConfig& InConfig);
	void Render(const FPhillipsFourierPassParam& Param, FRHITexture* DebugTextureRef);

	FORCEINLINE FShaderResourceViewRHIRef GetPhillipsFourierTextureSRV() const
	{
		return OutputPhillipsFourierTextureSRV;
	}

private:

	FTexture2DRHIRef           OutputPhillipsFourierTexture;
	FUnorderedAccessViewRHIRef OutputPhillipsFourierTextureUAV;
	FShaderResourceViewRHIRef  OutputPhillipsFourierTextureSRV;

	FTexture2DRHIRef           InputGaussianNoiseTexture;
	FShaderResourceViewRHIRef  InputGaussianNoiseTextureSRV;

	FPhillipsFourierPassConfig Config;
};