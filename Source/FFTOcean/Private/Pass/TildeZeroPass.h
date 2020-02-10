// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Pass/PassUtil.h"

struct FTildeZeroPassConfig
{
	uint32       TextureWidth;
	uint32       TextureHeight;
	FRHITexture* GaussianNoiseTextureRef;
};

inline bool operator==(const FTildeZeroPassConfig& A, const FTildeZeroPassConfig& B)
{
	return FMemory::Memcmp(&A, &B, sizeof(FTildeZeroPassConfig)) == 0;
}

inline bool operator!=(const FTildeZeroPassConfig& A, const FTildeZeroPassConfig& B) 
{
	return !(A == B);
}

struct FTildeZeroPassParam
{
	float     Time;
	float     WaveAmplitude;
	FVector2D WindSpeed;      // Tilde W * v
	FVector2D WaveDirection;  // Tilde K * k
};

class FTildeZeroPass final : public FOceanRenderPass
{
public:

	FTildeZeroPass();
	~FTildeZeroPass();

	virtual bool IsValidPass() const override;
	virtual void ReleaseRenderResource() override;

	void InitPass(const FTildeZeroPassConfig& InConfig);
	void Render(const FTildeZeroPassParam& Param, FRHITexture* DebugTextureRef);

	FORCEINLINE FShaderResourceViewRHIRef GetTildeZeroTextureSRV() const
	{
		return OutputTildeZeroTextureSRV;
	}

private:

	FTexture2DRHIRef           OutputTildeZeroTexture;
	FUnorderedAccessViewRHIRef OutputTildeZeroTextureUAV;
	FShaderResourceViewRHIRef  OutputTildeZeroTextureSRV;

	FTexture2DRHIRef           InputGaussianNoiseTexture;
	FShaderResourceViewRHIRef  InputGaussianNoiseTextureSRV;

	FTildeZeroPassConfig       Config;
};