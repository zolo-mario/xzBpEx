// Copyright 2026, BlueprintsLab, All rights reserved
#include "MaterialNodeDescriber.h"
#include "MaterialGraphDescriber.h"
#include <MaterialGraph/MaterialGraph.h>
#include "UObject/Package.h"
#include "IMaterialEditor.h"
#include "MaterialGraph/MaterialGraphNode.h"
#if WITH_EDITOR
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Engine/Texture.h"
#endif
#include <Editor/MaterialEditor/Private/MaterialEditor.h>
#include "BtGraphDescriber.h"
#include "BehaviorTreeEditor.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Components/ContentWidget.h"
#include "UObject/TextProperty.h"
#include "Components/PanelWidget.h"
FString FMaterialNodeDescriber::Describe(const TSet<UObject*>& SelectedNodes)
{
    TStringBuilder<4096> Report;
    Report.Append(TEXT("--- SELECTED MATERIAL NODES ---\n\n"));
    for (UObject* Obj : SelectedNodes)
    {
        if (UMaterialGraphNode* Node = Cast<UMaterialGraphNode>(Obj))
        {
            Report.Appendf(TEXT("- Node: %s\n"), *Node->GetNodeTitle(ENodeTitleType::ListView).ToString());
            for (UEdGraphPin* Pin : Node->Pins)
            {
                if (Pin && Pin->Direction == EGPD_Input && Pin->LinkedTo.Num() > 0)
                {
                    if (UEdGraphNode* ConnectedNode = Pin->LinkedTo[0]->GetOwningNode())
                    {
                        FString PinName = Pin->GetDisplayName().IsEmpty() ? Pin->PinName.ToString() : Pin->GetDisplayName().ToString();
                        Report.Appendf(TEXT("    - Input Pin '%s' is connected to node '%s'\n"), *PinName, *ConnectedNode->GetNodeTitle(ENodeTitleType::ListView).ToString());
                    }
                }
            }
        }
    }
    return FString(Report);
}
