// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Pass/PassUtil.h"

class FTildeZeroPass : public FOceanRenderPass()
{
	FTildeZeroPass()
	~FTildeZeroPass();

	virtual bool IsValidPass() const override;
};