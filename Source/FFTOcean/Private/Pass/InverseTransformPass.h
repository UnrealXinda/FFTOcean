// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Pass/PassUtil.h"

struct FInverseTransformPassConfig
{
	uint32       TextureWidth;
	uint32       TextureHeight;
};

struct FInverseTransformPassParam
{
	FTexture2DRHIRef           FourierComponentTexture;
	FShaderResourceViewRHIRef  FourierComponentTextureSRV;
	FUnorderedAccessViewRHIRef FourierComponentTextureUAV;

	FShaderResourceViewRHIRef  TwiddleFactorsTextureSRV;
};

class FInverseTransformPass final : public FOceanRenderPass
{
public:

	FInverseTransformPass();
	~FInverseTransformPass();

	virtual bool IsValidPass() const override;
	virtual void ReleaseRenderResource() override;

	void Render(const FInverseTransformPassConfig& InConfig, const FInverseTransformPassParam& Param, UTextureRenderTarget2D* TargetTexture);

private:

	FTexture2DRHIRef           OutputInverseTransformTexture;
	FUnorderedAccessViewRHIRef OutputInverseTransformTextureUAV;
	FShaderResourceViewRHIRef  OutputInverseTransformTextureSRV;

	FInverseTransformPassConfig Config;

	void ConfigurePass(const FInverseTransformPassConfig& InConfig);
};