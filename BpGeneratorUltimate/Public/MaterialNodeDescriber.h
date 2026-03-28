// Copyright 2026, BlueprintsLab, All rights reserved
#pragma once
#include "CoreMinimal.h"
class FMaterialNodeDescriber
{
public:
    FString Describe(const TSet<UObject*>& SelectedNodes);
};
