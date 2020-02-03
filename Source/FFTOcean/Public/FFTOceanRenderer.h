// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FFFTOceanRenderer
{
public:

	FFFTOceanRenderer();
	~FFFTOceanRenderer();

	void Render(
		class UTextureRenderTarget2D* DisplacementMapTexture,
		class UTextureRenderTarget2D* NormalMapTexture,
		class UTextureRenderTarget2D* IFFTDebugTexture = nullptr,
		class UTextureRenderTarget2D* TwiddleDebugTexture = nullptr,
		class UTextureRenderTarget2D* TildeZeroDebugTexture = nullptr);
};
