// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Pass/PassUtil.h"

struct FRenderToTargetPassConfig
{
	uint32       TextureWidth;
	uint32       TextureHeight;
};

struct FRenderToTargetPassParam
{
	FShaderResourceViewRHIRef InverseTransformTextureSRV;
};

class FRenderToTargetPass final : public FOceanRenderPass
{
public:

	FRenderToTargetPass();
	~FRenderToTargetPass();

	virtual bool IsValidPass() const override;
	virtual void ReleaseRenderResource() override;

	void Render(const FRenderToTargetPassConfig& InConfig, const FRenderToTargetPassParam& Param, UTextureRenderTarget2D* TargetTexture);

private:

	FRenderToTargetPassConfig Config;

	TUniquePtr<class FRenderToTargetPassVertexBuffer> RenderToTargetVertexBuffer;
	TUniquePtr<class FRenderToTargetPassIndexBuffer>  RenderToTargetIndexBuffer;

	void ConfigurePass(const FRenderToTargetPassConfig& InConfig);
};