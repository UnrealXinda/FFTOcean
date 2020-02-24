// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "FFTOceanRenderer.h"

FFFTOceanRenderer::FFFTOceanRenderer() :
	PhillipsFourierPass(new FPhillipsFourierPass()),
	FourierComponentPass(new FFourierComponentPass()),
	TwiddleFactorsPass(new FTwiddleFactorsPass()),
	InverseTransformPass(new FInverseTransformPass()),
	SurfaceDisplacementPass(new FSurfaceDisplacementPass()),
	SurfaceNormalPass(new FSurfaceNormalPass())
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

	auto RenderInverseTransformPass = [&Config, &DebugConfig, this](UTextureRenderTarget2D* XDebugTexture, UTextureRenderTarget2D* YDebugTexture, UTextureRenderTarget2D* ZDebugTexture)
	{
		FInverseTransformPassConfig PassConfig;
		PassConfig.TextureWidth = Config.RenderTextureWidth;
		PassConfig.TextureHeight = Config.RenderTextureHeight;

		FInverseTransformPassParam Param;
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Param.FourierComponentTextures[Index] = FourierComponentPass->GetSurfaceTexture(Index);
			Param.FourierComponentTextureSRVs[Index] = FourierComponentPass->GetSurfaceTextureSRV(Index);
			Param.FourierComponentTextureUAVs[Index] = FourierComponentPass->GetSurfaceTextureUAV(Index);
		}
		Param.TwiddleFactorsTextureSRV = TwiddleFactorsPass->GetTwiddleFactorsTextureSRV();

		FRHITexture* XDebugTextureRef = FFTOcean::GetRHITextureFromRenderTarget(XDebugTexture);
		FRHITexture* YDebugTextureRef = FFTOcean::GetRHITextureFromRenderTarget(YDebugTexture);
		FRHITexture* ZDebugTextureRef = FFTOcean::GetRHITextureFromRenderTarget(ZDebugTexture);

		InverseTransformPass->Render(PassConfig, Param, XDebugTextureRef, YDebugTextureRef, ZDebugTextureRef);
	};

	auto RenderSurfaceDisplacementPass = [&Config, &DebugConfig, this](UTextureRenderTarget2D* DebugTexture)
	{
		FSurfaceDisplacementPassConfig PassConfig;
		PassConfig.TextureWidth = Config.RenderTextureWidth;
		PassConfig.TextureHeight = Config.RenderTextureHeight;

		FSurfaceDisplacementPassParam Param;
		InverseTransformPass->GetInverseTransformTextureSRVs(Param.InverseTransformTextureSRVs);

		FRHITexture* DebugTextureRef = FFTOcean::GetRHITextureFromRenderTarget(DebugTexture);

		SurfaceDisplacementPass->Render(PassConfig, Param, DebugTextureRef);
	};

	auto RenderSurfaceNormalPass = [&Config, &DebugConfig, this](UTextureRenderTarget2D* DebugTexture)
	{
		FSurfaceNormalPassConfig PassConfig;
		PassConfig.TextureWidth = Config.RenderTextureWidth;
		PassConfig.TextureHeight = Config.RenderTextureHeight;

		FSurfaceNormalPassParam Param;
		Param.DisplacementTextureSRV = SurfaceDisplacementPass->GetSurfaceDisplacementTextureSRV();
		Param.NormalStrength = Config.NormalStrength;

		FRHITexture* DebugTextureRef = FFTOcean::GetRHITextureFromRenderTarget(DebugTexture);

		SurfaceNormalPass->Render(PassConfig, Param, DebugTextureRef);
	};

	RenderPhillipsFourierPass();
	RenderFourierComponentPass();
	RenderTwiddleFactorsPass();
	RenderInverseTransformPass(DebugConfig.TransformDebugTextureX, DebugConfig.TransformDebugTextureY, DebugConfig.TransformDebugTextureZ);
	RenderSurfaceDisplacementPass(Config.DisplacementMap);
	RenderSurfaceNormalPass(Config.NormalMap);
}
