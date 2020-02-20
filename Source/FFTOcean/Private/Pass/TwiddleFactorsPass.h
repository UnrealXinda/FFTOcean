// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Pass/PassUtil.h"

struct FTwiddleFactorsPassConfig
{
	uint32       TextureWidth;
	uint32       TextureHeight;
};

struct FTwiddleFactorsPassParam
{
};

class FTwiddleFactorsPass final : public FOceanRenderPass
{
public:

	FTwiddleFactorsPass();
	~FTwiddleFactorsPass();

	virtual bool IsValidPass() const override;
	virtual void ReleaseRenderResource() override;

	void Render(const FTwiddleFactorsPassConfig& InConfig, const FTwiddleFactorsPassParam& Param, FRHITexture* DebugTextureRef);

	FORCEINLINE FShaderResourceViewRHIRef GetTwiddleFactorsTextureSRV() const
	{
		return OutputTwiddleFactorsTextureSRV;
	}

private:

	FTexture2DRHIRef           OutputTwiddleFactorsTexture;
	FUnorderedAccessViewRHIRef OutputTwiddleFactorsTextureUAV;
	FShaderResourceViewRHIRef  OutputTwiddleFactorsTextureSRV;

	FTwiddleFactorsPassConfig Config;

	void ConfigurePass(const FTwiddleFactorsPassConfig& InConfig);
};