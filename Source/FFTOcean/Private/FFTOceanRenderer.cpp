// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "FFTOceanRenderer.h"

FFFTOceanRenderer::FFFTOceanRenderer() :
	PhillipsFourierPass(),
	FourierComponentPass()
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
		PassConfig.GaussianNoiseTextureRef = FFTOcean::GetRHITextureFromTexture2D(Config.GaussianNoiseTexture);

		FPhillipsFourierPassParam Param;
		Param.WaveAmplitude = Config.WaveAmplitude;
		Param.WindSpeed = Config.WindSpeed;

		FRHITexture* DebugTexture = FFTOcean::GetRHITextureFromRenderTarget(DebugConfig.PhillipsFourierPassDebugTexture);

		PhillipsFourierPass.ConfigurePass(PassConfig);
		PhillipsFourierPass.Render(Param, DebugTexture);
	};

	auto RenderFourierComponentPass = [Timestamp, &Config, &DebugConfig, this]()
	{
		FFourierComponentPassConfig PassConfig;
		PassConfig.TextureWidth = Config.RenderTextureWidth;
		PassConfig.TextureHeight = Config.RenderTextureHeight;

		FFourierComponentPassParam Param;
		Param.Time = Timestamp;
		Param.PhillipsFourierTextureSRV = PhillipsFourierPass.GetPhillipsFourierTextureSRV();

		FRHITexture* DebugTextureX = FFTOcean::GetRHITextureFromRenderTarget(DebugConfig.SurfaceDebugTextureX);
		FRHITexture* DebugTextureY = FFTOcean::GetRHITextureFromRenderTarget(DebugConfig.SurfaceDebugTextureY);
		FRHITexture* DebugTextureZ = FFTOcean::GetRHITextureFromRenderTarget(DebugConfig.SurfaceDebugTextureZ);

		FourierComponentPass.ConfigurePass(PassConfig);
		FourierComponentPass.Render(Param, DebugTextureX, DebugTextureY, DebugTextureZ);
	};

	RenderPhillipsFourierPass();
	RenderFourierComponentPass();
}
