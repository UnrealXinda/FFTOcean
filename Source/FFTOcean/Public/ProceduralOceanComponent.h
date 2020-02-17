// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "FFTOceanRenderer.h"
#include "ProceduralOceanComponent.generated.h"

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
	FVector2D WaveDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D WindSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ocean Rendering")
	class UTexture2D* GaussianNoiseTexture;
};

/**
 *
 */
UCLASS(hidecategories = (Object, LOD), editinlinenew, meta = (BlueprintSpawnableComponent), ClassGroup = Rendering, DisplayName = "ProceduralOceanComponent")
class FFTOCEAN_API UProceduralOceanComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 2, ClampMax = 256), Category = "Ocean Geometry")
	int32 VertexCountX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 2, ClampMax = 256), Category = "Ocean Geometry")
	int32 VertexCountY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0), Category = "Ocean Geometry")
	float CellWidthX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0), Category = "Ocean Geometry")
	float CellWidthY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean Rendering")
	FOceanRenderConfig RenderConfig;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ocean Rendering Debug")
	class UTextureRenderTarget2D* PhillipsFourierPassDebugRenderTarget;


public:

	UProceduralOceanComponent(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable)
	void InitOceanGeometry();

	virtual void OnRegister() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	TUniquePtr<FFFTOceanRenderer> OceanRenderer;
};
