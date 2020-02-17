// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "FFTOceanRenderer.h"

FFFTOceanRenderer::FFFTOceanRenderer() :
	PhillipsFourierPass()
{

}

FFFTOceanRenderer::~FFFTOceanRenderer()
{

}

void FFFTOceanRenderer::Render(
	UTextureRenderTarget2D* DisplacementMapTexture,
	UTextureRenderTarget2D* NormalMapTexture,
	UTextureRenderTarget2D* IFFTDebugTexture /*= nullptr*/,
	UTextureRenderTarget2D* TwiddleDebugTexture /*= nullptr*/,
	UTextureRenderTarget2D* TildeZeroDebugTexture /*= nullptr*/)
{

}
