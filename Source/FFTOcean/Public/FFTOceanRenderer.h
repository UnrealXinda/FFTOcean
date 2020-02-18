// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Pass/PhillipsFourierPass.h"
#include "Pass/FourierComponentPass.h"
#include "FFTOceanRenderer.generated.h"

USTRUCT(BlueprintType)
struct FOceanRenderConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 64, ClampMax = 1024))
	int32 RenderTextureWidth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 64, ClampMax = 1024))
	int32 RenderTextureHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	float TimeMultiply;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	float WaveAmplitude;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D WindSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ocean Rendering")
	class UTexture2D* GaussianNoiseTexture;
};


USTRUCT(BlueprintType)
struct FOceanDebugConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class UTextureRenderTarget2D* PhillipsFourierPassDebugTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class UTextureRenderTarget2D* SurfaceDebugTextureX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class UTextureRenderTarget2D* SurfaceDebugTextureY;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class UTextureRenderTarget2D* SurfaceDebugTextureZ;
};

class FFFTOceanRenderer final
{
public:

	FFFTOceanRenderer();
	~FFFTOceanRenderer();

	void Render(float Timestamp, const FOceanRenderConfig& Config, const FOceanDebugConfig& DebugConfig);

private:

	FPhillipsFourierPass  PhillipsFourierPass;
	FFourierComponentPass FourierComponentPass;
};
