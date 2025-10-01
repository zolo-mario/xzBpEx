// Copyright (c) 2025 Infinito Studio. All Rights Reserved.

#include "Tools/ExporterTool.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "K2Node_Event.h"
#include "K2Node_ExecutionSequence.h"

#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

TSet<UEdGraphNode*> FExporterTool::VisitedNodes;


void FExporterTool::ExportBlueprintToText(UBlueprint* Blueprint)
{
    if (!Blueprint) return;

    VisitedNodes.Empty(); // Reset visited state

    FString ReadableOutput;
    TSharedRef<FJsonObject> RootJson = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> JsonNodes;

    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
            {
                //ReadableOutput += Blueprint->GetName() + TEXT("\n");
                TSharedPtr<FJsonObject> NodeJson = MakeShared<FJsonObject>();
                ReadableOutput += WalkNodeExecutionPath(EventNode, 0, NodeJson);
                JsonNodes.Add(MakeShared<FJsonValueObject>(NodeJson));
                ReadableOutput += TEXT("\n");
            }
        }
    }

    RootJson->SetStringField("name", Blueprint->GetName());
    RootJson->SetArrayField("nodes", JsonNodes);

    FString JsonOutput;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonOutput);
    FJsonSerializer::Serialize(RootJson, Writer);

    FString SavePath = FPaths::ProjectDir() + "/BlueprintExports/";
    IFileManager::Get().MakeDirectory(*SavePath, true);

    FFileHelper::SaveStringToFile(ReadableOutput, *(SavePath + Blueprint->GetName() + "_Readable.txt"));
    FFileHelper::SaveStringToFile(JsonOutput, *(SavePath + Blueprint->GetName() + "_Readable.json"));

    UE_LOG(LogTemp, Warning, TEXT("Exported blueprint to readable + JSON: %s"), *SavePath);
}

FString FExporterTool::WalkNodeExecutionPath(UEdGraphNode* Node, int32 IndentLevel, TSharedPtr<FJsonObject> OutJson)
{
    if (!Node || VisitedNodes.Contains(Node))
        return TEXT("");

    VisitedNodes.Add(Node);

    FString Indent = FString::ChrN(IndentLevel * 2, ' ');
    FString Title = Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();

    // --- Pure input summary (Target is Controller, etc)
    TArray<FString> PureInputs;
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->Direction == EGPD_Input && Pin->LinkedTo.Num() > 0 && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
        {
            UEdGraphNode* InputNode = Pin->LinkedTo[0]->GetOwningNode();

            // Check if it's pure (no exec pins)
            bool bIsPure = true;
            for (UEdGraphPin* InPin : InputNode->Pins)
            {
                if (InPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
                {
                    bIsPure = false;
                    break;
                }
            }

            if (bIsPure)
            {
                FString InputTitle = InputNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
                FString PinLabel = Pin->PinName.ToString();
                PureInputs.Add(PinLabel + TEXT(" is ") + InputTitle);

                VisitedNodes.Add(InputNode); // Avoid printing it again
            }
        }
    }

    // Append pure input info inline
    if (PureInputs.Num() > 0)
    {
        Title += TEXT("  : ") + FString::Join(PureInputs, TEXT(", "));
    }

    FString Readable = Indent + Title + TEXT("\n");
    OutJson->SetStringField("title", Title);

    // --- Exec pins
    TArray<UEdGraphPin*> ExecPins;
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
        {
            ExecPins.Add(Pin);
        }
    }

    TArray<TSharedPtr<FJsonValue>> Children;
    for (int32 i = 0; i < ExecPins.Num(); ++i)
    {
        UEdGraphPin* Pin = ExecPins[i];
        FString Connector = (i < ExecPins.Num() - 1) ? TEXT("├─") : TEXT("└─");
        FString Label = Pin->PinName.ToString();

        Readable += Indent + Connector + TEXT(" ") + Label + TEXT(" →\n");

        if (Pin->LinkedTo.Num() > 0)
        {
            UEdGraphNode* Next = Pin->LinkedTo[0]->GetOwningNode();
            TSharedPtr<FJsonObject> ChildJson = MakeShared<FJsonObject>();
            ChildJson->SetStringField("pin", Label);

            FString ChildReadable = WalkNodeExecutionPath(Next, IndentLevel + 1, ChildJson);
            Children.Add(MakeShared<FJsonValueObject>(ChildJson));
            Readable += ChildReadable;
        }
        else
        {
            Readable += FString::ChrN((IndentLevel + 1) * 2, ' ') + TEXT("(not connected)\n");
        }
    }

    OutJson->SetArrayField("children", Children);
    return Readable;
}
