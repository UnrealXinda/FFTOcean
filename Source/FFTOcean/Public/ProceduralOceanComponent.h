// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "ProceduralOceanComponent.generated.h"

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

public:

	UProceduralOceanComponent(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable)
	void InitOceanGeometry();

	virtual void OnRegister() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
};
