// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "FFTOcean.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FFFTOceanModule"

void FFFTOceanModule::StartupModule()
{
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("FFTOcean"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/FFTOcean"), PluginShaderDir);
}

void FFFTOceanModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FFFTOceanModule, FFTOcean)