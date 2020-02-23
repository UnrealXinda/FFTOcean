// Fill out your copyright notice in the Description page of Project Settings.


#include "OceanMeshComponent.h"
#include "Kismet/GameplayStatics.h"

UOceanMeshComponent::UOceanMeshComponent(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer),
	OceanRenderer(new FFFTOceanRenderer)
{
	bTickInEditor = true;
	bAutoActivate = true;

	RenderConfig.RenderTextureWidth = 512;
	RenderConfig.RenderTextureHeight = 512;
}

void UOceanMeshComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	float Timestamp = UGameplayStatics::GetRealTimeSeconds(GetWorld()) * RenderConfig.TimeMultiply;
	OceanRenderer->Render(Timestamp, RenderConfig, DebugConfig);
}
