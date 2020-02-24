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
	FTexture2DRHIRef           FourierComponentTextures[3];
	FShaderResourceViewRHIRef  FourierComponentTextureSRVs[3];
	FUnorderedAccessViewRHIRef FourierComponentTextureUAVs[3];

	FShaderResourceViewRHIRef  TwiddleFactorsTextureSRV;
};

class FInverseTransformPass final : public FOceanRenderPass
{
public:

	FInverseTransformPass();
	~FInverseTransformPass();

	virtual bool IsValidPass() const override;
	virtual void ReleaseRenderResource() override;

	void Render(
		const FInverseTransformPassConfig& InConfig,
		const FInverseTransformPassParam& Param,
		FRHITexture* XDebugTextureRef,
		FRHITexture* YDebugTextureRef,
		FRHITexture* ZDebugTextureRef);

	FORCEINLINE void GetInverseTransformTextureSRVs(FShaderResourceViewRHIRef (&Array)[3]) const
	{
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Array[Index] = OutputInverseTransformTextureSRVs[Index];
		}
	}

private:

	FTexture2DRHIRef           OutputInverseTransformTextures[3];
	FUnorderedAccessViewRHIRef OutputInverseTransformTextureUAVs[3];
	FShaderResourceViewRHIRef  OutputInverseTransformTextureSRVs[3];

	FInverseTransformPassConfig Config;

	void ConfigurePass(const FInverseTransformPassConfig& InConfig);

	void RenderInverseTransform(
		FRHICommandListImmediate& RHICmdList,
		const FInverseTransformPassParam& Param,
		int32 TextureIndex,
		FRHITexture* DebugTextureRef);
};