// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Pass/PhillipsFourierPass.h"
#include "Pass/FourierComponentPass.h"
#include "Pass/TwiddleFactorsPass.h"
#include "Pass/InverseTransformPass.h"
#include "Pass/SurfaceDisplacementPass.h"
#include "Pass/SurfaceNormalPass.h"

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
	float StartTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	float TimeMultiply;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	float WaveAmplitude;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	float WindVelocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, ClampMax = 360))
	float WindDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	float NormalStrength;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class UTextureRenderTarget2D* DisplacementMap;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class UTextureRenderTarget2D* NormalMap;
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class UTextureRenderTarget2D* TwiddleFactorsDebugTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class UTextureRenderTarget2D* TransformDebugTextureX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class UTextureRenderTarget2D* TransformDebugTextureY;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class UTextureRenderTarget2D* TransformDebugTextureZ;
};

class FFFTOceanRenderer final
{
public:

	FFFTOceanRenderer();
	~FFFTOceanRenderer();

	void Render(float Timestamp, const FOceanRenderConfig& Config, const FOceanDebugConfig& DebugConfig);

private:

	TUniquePtr<FPhillipsFourierPass>     PhillipsFourierPass;
	TUniquePtr<FFourierComponentPass>    FourierComponentPass;
	TUniquePtr<FTwiddleFactorsPass>      TwiddleFactorsPass;
	TUniquePtr<FInverseTransformPass>    InverseTransformPass;
	TUniquePtr<FSurfaceDisplacementPass> SurfaceDisplacementPass;
	TUniquePtr<FSurfaceNormalPass>       SurfaceNormalPass;
};
