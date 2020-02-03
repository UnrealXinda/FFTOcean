// Fill out your copyright notice in the Description page of Project Settings.


#include "ProceduralOceanComponent.h"
#include "Runtime/Core/Public/Async/ParallelFor.h"

UProceduralOceanComponent::UProceduralOceanComponent(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	VertexCountX = 60;
	VertexCountY = 60;

	CellWidthX = 20;
	CellWidthY = 20;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	bTickInEditor = true;
	bAutoActivate = true;
}

void UProceduralOceanComponent::InitOceanGeometry()
{
	const int32 VertexCount = VertexCountX * VertexCountY;
	const int32 TriangleCount = (VertexCountX - 1) * (VertexCountY - 1) * 6;

	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<int32> Triangles;
	TArray<FColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	Vertices.AddUninitialized(VertexCount);
	Normals.AddUninitialized(VertexCount);
	UVs.AddUninitialized(VertexCount);
	Triangles.AddUninitialized(TriangleCount);

	ParallelFor(VertexCount, [&](int32 Index)
	{
		int32 IndexX = Index % VertexCountX;
		int32 IndexY = Index / VertexCountX;
		float LocX = IndexX * CellWidthX;
		float LocY = IndexY * CellWidthY;
		float LocZ = 0.0f;
		float U = StaticCast<float>(IndexX) / (VertexCountX - 1);
		float V = StaticCast<float>(IndexY) / (VertexCountY - 1);

		Vertices[Index] = FVector(LocX, LocY, LocZ);
		Normals[Index] = FVector::UpVector;
		UVs[Index] = FVector2D(U, V);
	});

	int32 Idx = 0;

	for (int32 Y = 0; Y < VertexCountY - 1; ++Y)
	{
		for (int32 X = 0; X < VertexCountX - 1; ++X)
		{
			int A = Y * VertexCountX + X;
			int B = A + VertexCountX;
			int C = A + VertexCountX + 1;
			int D = A + 1;

			Triangles[Idx++] = A;
			Triangles[Idx++] = B;
			Triangles[Idx++] = C;

			Triangles[Idx++] = A;
			Triangles[Idx++] = C;
			Triangles[Idx++] = D;
		}
	}

	ClearMeshSection(0);
	CreateMeshSection(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, false);
}

void UProceduralOceanComponent::OnRegister()
{
	Super::OnRegister();
	InitOceanGeometry();
}

void UProceduralOceanComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{

}
