// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "FFTOceanRenderer.h"

FFFTOceanRenderer::FFFTOceanRenderer() :
	PhillipsFourierPass(new FPhillipsFourierPass()),
	FourierComponentPass(new FFourierComponentPass()),
	TwiddleFactorsPass(new FTwiddleFactorsPass()),
	InverseTransformPass(new FInverseTransformPass()),
	RenderToTargetPass(new FRenderToTargetPass())
{
}

FFFTOceanRenderer::~FFFTOceanRenderer()
{

}

void FFFTOceanRenderer::Render(float Timestamp, const FOceanRenderConfig& Config, const FOceanDebugConfig& DebugConfig)
{
	auto RenderPhillipsFourierPass = [&Config, &DebugConfig, this]()
	{
		FPhillipsFourierPassConfig PassConfig;
		PassConfig.TextureWidth = Config.RenderTextureWidth;
		PassConfig.TextureHeight = Config.RenderTextureHeight;

		const FVector2D KWindDefaultDirection(1, 0);

		FPhillipsFourierPassParam Param;
		Param.WaveAmplitude = Config.WaveAmplitude;
		Param.WindSpeed = KWindDefaultDirection.GetRotated(Config.WindDirection) * Config.WindVelocity;

		FRHITexture* DebugTextureRef = FFTOcean::GetRHITextureFromRenderTarget(DebugConfig.PhillipsFourierPassDebugTexture);

		PhillipsFourierPass->Render(PassConfig, Param, DebugTextureRef);
	};

	auto RenderFourierComponentPass = [Timestamp, &Config, &DebugConfig, this]()
	{
		FFourierComponentPassConfig PassConfig;
		PassConfig.TextureWidth = Config.RenderTextureWidth;
		PassConfig.TextureHeight = Config.RenderTextureHeight;

		FFourierComponentPassParam Param;
		Param.Time = Timestamp;
		Param.PhillipsFourierTextureSRV = PhillipsFourierPass->GetPhillipsFourierTextureSRV();

		FRHITexture* DebugTextureXRef = FFTOcean::GetRHITextureFromRenderTarget(DebugConfig.SurfaceDebugTextureX);
		FRHITexture* DebugTextureYRef = FFTOcean::GetRHITextureFromRenderTarget(DebugConfig.SurfaceDebugTextureY);
		FRHITexture* DebugTextureZRef = FFTOcean::GetRHITextureFromRenderTarget(DebugConfig.SurfaceDebugTextureZ);

		FourierComponentPass->Render(PassConfig, Param, DebugTextureXRef, DebugTextureYRef, DebugTextureZRef);
	};

	auto RenderTwiddleFactorsPass = [&Config, &DebugConfig, this]()
	{
		FTwiddleFactorsPassConfig PassConfig;
		PassConfig.TextureWidth = Config.RenderTextureWidth;
		PassConfig.TextureHeight = Config.RenderTextureHeight;

		FTwiddleFactorsPassParam Param;

		FRHITexture* DebugTextureRef = FFTOcean::GetRHITextureFromRenderTarget(DebugConfig.TwiddleFactorsDebugTexture);

		TwiddleFactorsPass->Render(PassConfig, Param, DebugTextureRef);
	};

	auto RenderInverseTransformPass = [&Config, &DebugConfig, this](int32 Index, UTextureRenderTarget2D* DebugTexture)
	{
		FInverseTransformPassConfig PassConfig;
		PassConfig.TextureWidth = Config.RenderTextureWidth;
		PassConfig.TextureHeight = Config.RenderTextureHeight;

		FInverseTransformPassParam Param;
		Param.FourierComponentTexture = FourierComponentPass->GetSurfaceTexture(Index);
		Param.FourierComponentTextureSRV = FourierComponentPass->GetSurfaceTextureSRV(Index);
		Param.FourierComponentTextureUAV = FourierComponentPass->GetSurfaceTextureUAV(Index);
		Param.TwiddleFactorsTextureSRV = TwiddleFactorsPass->GetTwiddleFactorsTextureSRV();

		FRHITexture* DebugTextureRef = FFTOcean::GetRHITextureFromRenderTarget(DebugTexture);

		InverseTransformPass->Render(PassConfig, Param, DebugTextureRef);
	};

	auto RenderRenderToTargetPass = [&Config, &DebugConfig, this](UTextureRenderTarget2D* TargetTexture)
	{
		FRenderToTargetPassConfig PassConfig;
		PassConfig.TextureWidth = Config.RenderTextureWidth;
		PassConfig.TextureHeight = Config.RenderTextureHeight;

		FRenderToTargetPassParam Param;
		Param.InverseTransformTextureSRV = InverseTransformPass->GetInverseTransformTextureSRV();

		RenderToTargetPass->Render(PassConfig, Param, TargetTexture);
	};

	RenderPhillipsFourierPass();
	RenderFourierComponentPass();
	RenderTwiddleFactorsPass();

	RenderInverseTransformPass(0, DebugConfig.TransformDebugTextureX);
	RenderRenderToTargetPass(Config.DisplacementMapX);

	RenderInverseTransformPass(1, DebugConfig.TransformDebugTextureY);
	RenderRenderToTargetPass(Config.DisplacementMapY);

	RenderInverseTransformPass(2, DebugConfig.TransformDebugTextureZ);
	RenderRenderToTargetPass(Config.DisplacementMapZ);
}
