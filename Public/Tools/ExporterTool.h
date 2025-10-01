// Copyright (c) 2025 Infinito Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UBlueprint;
class FExporterTool
{
public:
    static void ExportBlueprintToText(UBlueprint* Blueprint);
    static FString WalkNodeExecutionPath(UEdGraphNode* Node, int32 IndentLevel, TSharedPtr<FJsonObject> OutJson);

private:
    static TSet<UEdGraphNode*> VisitedNodes;
};
