// Copyright 2026, BlueprintsLab, All rights reserved
#include "BpGraphDescriber.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
FString FBpGraphDescriber::Describe(const TSet<UObject*>& SelectedNodes)
{
    SelectedNodeSet.Empty();
    VisitedNodes.Empty();
    for (UObject* Obj : SelectedNodes)
    {
        if (UEdGraphNode* Node = Cast<UEdGraphNode>(Obj))
        {
            SelectedNodeSet.Add(Node);
        }
    }
    if (SelectedNodeSet.Num() == 0) return TEXT("No valid nodes were selected.");
    TArray<UEdGraphNode*> EntryPoints = FindEntryPoints();
    if (EntryPoints.Num() == 0)
    {
        if (SelectedNodeSet.Num() > 0)
        {
            EntryPoints.Add(*SelectedNodeSet.begin());
        }
    }
    FString Description;
    for (UEdGraphNode* EntryNode : EntryPoints)
    {
        DescribeNodeRecursively(EntryNode, Description, 0);
    }
    return Description.IsEmpty() ? TEXT("Could not generate a description for the selected nodes.") : Description;
}
void FBpGraphDescriber::DescribeNodeRecursively(UEdGraphNode* Node, FString& OutDescription, int32 IndentLevel)
{
    if (!Node || VisitedNodes.Contains(Node)) return;
    VisitedNodes.Add(Node);
    OutDescription.Append(FString::ChrN(IndentLevel * 2, ' '));
    uint32 NodeId = PointerHash(Node);
    OutDescription.Append(FString::Printf(TEXT("#%u "), NodeId));
    bool bIsPure = true;
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
        {
            bIsPure = false;
            break;
        }
    }
    OutDescription.Append(bIsPure ? TEXT("[Pure] ") : TEXT("[Impure] "));
    OutDescription.Append(Node->GetNodeTitle(ENodeTitleType::ListView).ToString());
    if (!Node->NodeComment.IsEmpty())
    {
        OutDescription.Append(FString::Printf(TEXT(" // %s"), *Node->NodeComment));
    }
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin && Pin->Direction == EGPD_Input && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec && Pin->LinkedTo.Num() > 0)
        {
            UEdGraphNode* ConnectedNode = Pin->LinkedTo[0]->GetOwningNode();
            uint32 ConnectedNodeId = PointerHash(ConnectedNode);
            FString ConnectedNodeTitle = SelectedNodeSet.Contains(ConnectedNode)
                ? FString::Printf(TEXT("#%u %s"), ConnectedNodeId, *ConnectedNode->GetNodeTitle(ENodeTitleType::ListView).ToString())
                : TEXT("an unselected node");
            OutDescription.Append(FString::Printf(TEXT(" [Pin: %s <- '%s']"), *Pin->GetDisplayName().ToString(), *ConnectedNodeTitle));
        }
        else if (Pin && Pin->Direction == EGPD_Input && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec && !Pin->DefaultValue.IsEmpty() && Pin->DefaultValue != Pin->GetSchema()->GetPinDisplayName(Pin).ToString())
        {
            OutDescription.Append(FString::Printf(TEXT(" [Pin: %s = %s]"), *Pin->GetDisplayName().ToString(), *Pin->DefaultValue));
        }
    }
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin && Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec && Pin->LinkedTo.Num() > 0)
        {
            TArray<FString> ConnectedTargets;
            for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
            {
                UEdGraphNode* ConnectedNode = LinkedPin->GetOwningNode();
                uint32 ConnectedNodeId = PointerHash(ConnectedNode);
                FString ConnectedNodeTitle = SelectedNodeSet.Contains(ConnectedNode)
                    ? FString::Printf(TEXT("#%u %s.%s"), ConnectedNodeId, *ConnectedNode->GetNodeTitle(ENodeTitleType::ListView).ToString(), *LinkedPin->GetDisplayName().ToString())
                    : TEXT("an unselected node");
                ConnectedTargets.Add(ConnectedNodeTitle);
            }
            if (ConnectedTargets.Num() > 0)
            {
                OutDescription.Append(FString::Printf(TEXT(" [Out: %s -> '%s']"), *Pin->GetDisplayName().ToString(), *FString::Join(ConnectedTargets, TEXT("', '"))));
            }
        }
    }
    OutDescription.Append(TEXT("\n"));
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin && Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec && Pin->LinkedTo.Num() > 0)
        {
            UEdGraphNode* NextNode = Pin->LinkedTo[0]->GetOwningNode();
            if (SelectedNodeSet.Contains(NextNode))
            {
                OutDescription.Append(FString::ChrN((IndentLevel * 2) + 1, ' '));
                OutDescription.Append(FString::Printf(TEXT("-> (%s) ->\n"), *Pin->GetDisplayName().ToString()));
                DescribeNodeRecursively(NextNode, OutDescription, IndentLevel + 1);
            }
        }
    }
}
TArray<UEdGraphNode*> FBpGraphDescriber::FindEntryPoints()
{
    TArray<UEdGraphNode*> EntryPoints;
    for (UEdGraphNode* Node : SelectedNodeSet)
    {
        bool bIsEntryPoint = true;
        for (UEdGraphPin* Pin : Node->Pins)
        {
            if (Pin && Pin->Direction == EGPD_Input && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
            {
                if (Pin->LinkedTo.Num() > 0 && SelectedNodeSet.Contains(Pin->LinkedTo[0]->GetOwningNode()))
                {
                    bIsEntryPoint = false;
                    break;
                }
            }
        }
        if (bIsEntryPoint)
        {
            EntryPoints.Add(Node);
        }
    }
    return EntryPoints;
}
