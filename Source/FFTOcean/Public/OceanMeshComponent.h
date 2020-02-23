// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "OceanMeshComponent.generated.h"

/**
 * 
 */
UCLASS(hidecategories = (Object), editinlinenew, meta = (BlueprintSpawnableComponent), ClassGroup = Rendering, DisplayName = "OceanMeshComponent")
class FFTOCEAN_API UOceanMeshComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean Rendering")
	FOceanRenderConfig RenderConfig;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Ocean Rendering Debug")
	FOceanDebugConfig DebugConfig;

public:

	UOceanMeshComponent(const FObjectInitializer& ObjectInitializer);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	TUniquePtr<FFFTOceanRenderer> OceanRenderer;
};
