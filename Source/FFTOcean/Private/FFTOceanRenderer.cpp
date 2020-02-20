// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "FFTOceanRenderer.h"

FFFTOceanRenderer::FFFTOceanRenderer() :
	PhillipsFourierPass(new FPhillipsFourierPass()),
	FourierComponentPass(new FFourierComponentPass()),
	TwiddleFactorsPass(new FTwiddleFactorsPass()),
	InverseTransformPass(new FInverseTransformPass())
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

		FPhillipsFourierPassParam Param;
		Param.WaveAmplitude = Config.WaveAmplitude;
		Param.WindSpeed = Config.WindSpeed;

		FRHITexture* DebugTexture = FFTOcean::GetRHITextureFromRenderTarget(DebugConfig.PhillipsFourierPassDebugTexture);

		PhillipsFourierPass->Render(PassConfig, Param, DebugTexture);
	};

	auto RenderFourierComponentPass = [Timestamp, &Config, &DebugConfig, this]()
	{
		FFourierComponentPassConfig PassConfig;
		PassConfig.TextureWidth = Config.RenderTextureWidth;
		PassConfig.TextureHeight = Config.RenderTextureHeight;

		FFourierComponentPassParam Param;
		Param.Time = Timestamp;
		Param.PhillipsFourierTextureSRV = PhillipsFourierPass->GetPhillipsFourierTextureSRV();

		FRHITexture* DebugTextureX = FFTOcean::GetRHITextureFromRenderTarget(DebugConfig.SurfaceDebugTextureX);
		FRHITexture* DebugTextureY = FFTOcean::GetRHITextureFromRenderTarget(DebugConfig.SurfaceDebugTextureY);
		FRHITexture* DebugTextureZ = FFTOcean::GetRHITextureFromRenderTarget(DebugConfig.SurfaceDebugTextureZ);

		FourierComponentPass->Render(PassConfig, Param, DebugTextureX, DebugTextureY, DebugTextureZ);
	};

	auto RenderTwiddleFactorsPass = [&Config, &DebugConfig, this]()
	{
		FTwiddleFactorsPassConfig PassConfig;
		PassConfig.TextureWidth = Config.RenderTextureWidth;
		PassConfig.TextureHeight = Config.RenderTextureHeight;

		FTwiddleFactorsPassParam Param;

		FRHITexture* DebugTexture = FFTOcean::GetRHITextureFromRenderTarget(DebugConfig.TwiddleFactorsDebugTexture);

		TwiddleFactorsPass->Render(PassConfig, Param, DebugTexture);
	};

	auto RenderInverseTransformPass = [&Config, &DebugConfig, this](int32 Index, UTextureRenderTarget2D* TargetTexture)
	{
		FInverseTransformPassConfig PassConfig;
		PassConfig.TextureWidth = Config.RenderTextureWidth;
		PassConfig.TextureHeight = Config.RenderTextureHeight;

		FInverseTransformPassParam Param;
		Param.FourierComponentTexture = FourierComponentPass->GetSurfaceTexture(Index);
		Param.FourierComponentTextureSRV = FourierComponentPass->GetSurfaceTextureSRV(Index);
		Param.FourierComponentTextureUAV = FourierComponentPass->GetSurfaceTextureUAV(Index);
		Param.TwiddleFactorsTextureSRV = TwiddleFactorsPass->GetTwiddleFactorsTextureSRV();

		InverseTransformPass->Render(PassConfig, Param, TargetTexture);
	};

	RenderPhillipsFourierPass();
	RenderFourierComponentPass();
	RenderTwiddleFactorsPass();
	RenderInverseTransformPass(0, Config.DisplacementMapX);
	RenderInverseTransformPass(1, Config.DisplacementMapY);
	RenderInverseTransformPass(2, Config.DisplacementMapZ);
}
