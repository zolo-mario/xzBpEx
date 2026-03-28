// Copyright 2026, BlueprintsLab, All rights reserved
#include "BpGeneratorCompiler.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CallArrayFunction.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_MacroInstance.h"
#include "EdGraph/EdGraph.h"
#include "ScopedTransaction.h"
#include "Editor/EditorEngine.h"
#include "Editor/UnrealEdEngine.h"
const TCHAR* GpluginState = TEXT("0_#");
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Engine/GameInstance.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetStringLibrary.h"
TArray<TArray<FGuid>> FBpGeneratorCompiler::NodeCreationStack;
FCriticalSection FBpGeneratorCompiler::NodeCreationStackLock;
TArray<FGuid> FBpGeneratorCompiler::PopLastCreatedNodes()
{
    FScopeLock Lock(&NodeCreationStackLock);
    TArray<FGuid> Result;
    if (NodeCreationStack.Num() > 0)
    {
        Result = NodeCreationStack.Last();
        NodeCreationStack.RemoveAt(NodeCreationStack.Num() - 1);
    }
    return Result;
}
bool FBpGeneratorCompiler::DeleteNodesByGUIDs(UBlueprint* Blueprint, const TArray<FGuid>& NodeGUIDs)
{
    if (!Blueprint || NodeGUIDs.Num() == 0)
    {
        return false;
    }
    const FScopedTransaction Transaction(FText::FromString("Undo Last Generated Code"));
    for (const FGuid& NodeGuid : NodeGUIDs)
    {
        UEdGraphNode* Node = FBlueprintEditorUtils::GetNodeByGUID(Blueprint, NodeGuid);
        if (Node)
        {
            Node->Modify();
            Node->DestroyNode();
        }
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    return true;
}
#include "Kismet/KismetTextLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "K2Node_Select.h"
#include "Internationalization/Regex.h"
#include "EdGraphNode_Comment.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/KismetArrayLibrary.h"
#include "K2Node_EnumLiteral.h"
#include "K2Node_SwitchEnum.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_MakeArray.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_GetSubsystem.h"
const TCHAR* GpluginModule = TEXT("_G3n!pr");
#include "K2Node_FormatText.h"
#include "Engine/DataTable.h"
#include "K2Node_GetDataTableRow.h"
#include "Engine/DataTable.h"
#include "K2Node_Event.h"
#include "K2Node_Self.h"
#include "K2Node_VariableGet.h"
#include "Components/ActorComponent.h"
#include "K2Node_SpawnActorFromClass.h"
#include "K2Node_SwitchInteger.h"
#include "K2Node_SwitchString.h"
#include "K2Node_SwitchName.h"
#include "K2Node_SwitchEnum.h"
#include "K2Node_VariableSet.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_Timeline.h"
#include "Engine/TimelineTemplate.h"
#include "Curves/CurveFloat.h"
#include "K2Node_ComponentBoundEvent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_InputKey.h"
#include "K2Node_GetClassDefaults.h"
#include "Blueprint/WidgetTree.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "EnhancedInputLibrary.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/InputComponent.h"
DEFINE_LOG_CATEGORY(LogBpGenCompiler);
TArray<FString> SplitParameters(const FString& ParamsStr)
{
    TArray<FString> Params;
    FString CurrentParam;
    int32 ParenNesting = 0;
    for (const TCHAR Char : ParamsStr)
    {
        if (Char == '(') ParenNesting++;
        if (Char == ')') ParenNesting--;
        if (Char == ',' && ParenNesting == 0)
        {
            Params.Add(CurrentParam.TrimStartAndEnd());
            CurrentParam.Empty();
        }
        else
        {
            CurrentParam += Char;
        }
    }
    if (!CurrentParam.IsEmpty())
    {
        Params.Add(CurrentParam.TrimStartAndEnd());
    }
    return Params;
}
FString StripCStyleCasts(const FString& Expression)
{
    FString Result = Expression;
    const FRegexPattern Pattern(TEXT("(?:\\bfloat\\b|\\bint\\b|\\bconst AActor\\*\\b)\\s*\\(([^\\)]+)\\)"));
    FRegexMatcher Matcher(Pattern, Result);
    FString CleanedResult;
    int32 LastMatchEnd = 0;
    while (Matcher.FindNext())
    {
        CleanedResult.Append(Result.Mid(LastMatchEnd, Matcher.GetCaptureGroupBeginning(0) - LastMatchEnd));
        CleanedResult.Append(Matcher.GetCaptureGroup(1));
        LastMatchEnd = Matcher.GetCaptureGroupEnding(0);
    }
    CleanedResult.Append(Result.Mid(LastMatchEnd));
    return CleanedResult.IsEmpty() ? Result : CleanedResult;
}
static void GatherEntireConnectedChain(UEdGraphNode* StartNode, TSet<UEdGraphNode*>& VisitedNodes)
{
    if (!StartNode || VisitedNodes.Contains(StartNode))
    {
        return;
    }
    VisitedNodes.Add(StartNode);
    for (UEdGraphPin* Pin : StartNode->Pins)
    {
        if (Pin && Pin->Direction == EGPD_Input && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
        {
            for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
            {
                if (LinkedPin)
                {
                    GatherEntireConnectedChain(LinkedPin->GetOwningNode(), VisitedNodes);
                }
            }
        }
    }
    for (UEdGraphPin* Pin : StartNode->Pins)
    {
        if (Pin && Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
        {
            for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
            {
                if (LinkedPin)
                {
                    GatherEntireConnectedChain(LinkedPin->GetOwningNode(), VisitedNodes);
                }
            }
        }
    }
}
struct FDownstreamConnectionInfo
{
    UEdGraphNode* DownstreamNode;
    FGuid DownstreamNodeGuid;
    FName DownstreamPinName;
};
static FDownstreamConnectionInfo MoveDownstreamNodesRight(UEdGraphPin* ExecPin)
{
    FDownstreamConnectionInfo Result;
    Result.DownstreamNode = nullptr;
    if (!ExecPin) return Result;
    if (ExecPin->LinkedTo.Num() > 0)
    {
        UEdGraphPin* DownstreamPin = ExecPin->LinkedTo[0];
        Result.DownstreamNode = DownstreamPin->GetOwningNode();
        Result.DownstreamNodeGuid = Result.DownstreamNode->NodeGuid;
        Result.DownstreamPinName = DownstreamPin->GetFName();
        ExecPin->BreakAllPinLinks();
        TSet<UEdGraphNode*> DownstreamNodes;
        GatherEntireConnectedChain(Result.DownstreamNode, DownstreamNodes);
        for (UEdGraphNode* NodeToMove : DownstreamNodes)
        {
            NodeToMove->Modify();
            NodeToMove->NodePosX += 1200;
        }
    }
    return Result;
}
static void ReconnectDownstreamNodes(UEdGraphPin* LastNewExecPin, const FDownstreamConnectionInfo& ConnectionInfo, UBlueprint* Blueprint)
{
    if (!LastNewExecPin || !ConnectionInfo.DownstreamNodeGuid.IsValid()) return;
    UE_LOG(LogBpGenCompiler, Warning, TEXT("ReconnectDownstreamNodes called. Last pin: %s, Downstream: %s"),
        *LastNewExecPin->GetName(), *ConnectionInfo.DownstreamPinName.ToString());
    UEdGraphNode* RefreshedDownstreamNode = FBlueprintEditorUtils::GetNodeByGUID(Blueprint, ConnectionInfo.DownstreamNodeGuid);
    if (RefreshedDownstreamNode)
    {
        UEdGraphPin* RefreshedDownstreamPin = RefreshedDownstreamNode->FindPin(ConnectionInfo.DownstreamPinName);
        if (RefreshedDownstreamPin)
        {
            LastNewExecPin->GetSchema()->TryCreateConnection(LastNewExecPin, RefreshedDownstreamPin);
        }
    }
}
void FBpGeneratorCompiler::SetPinDefaultValue(UEdGraphPin* Pin, const FString& LiteralValue)
{
    if (!Pin) return;
    if (LiteralValue.Equals(TEXT("nullptr"), ESearchCase::IgnoreCase))
    {
        Pin->DefaultValue = TEXT("None");
        Pin->GetSchema()->TrySetDefaultValue(*Pin, Pin->DefaultValue);
        return;
    }
    FString CleanValue = LiteralValue.TrimQuotes();
    if (LiteralValue.Equals(TEXT("FLinearColor()"), ESearchCase::IgnoreCase))
    {
        Pin->DefaultValue = TEXT("(R=0.0,G=0.0,B=0.0,A=0.0)");
        Pin->GetSchema()->TrySetDefaultValue(*Pin, Pin->DefaultValue);
        return;
    }
    if (LiteralValue.StartsWith(TEXT("TArray<")))
    {
        return;
    }
    if (CleanValue.Contains(TEXT("::")))
    {
        FString EnumClassName, EnumValueName;
        CleanValue.Split(TEXT("::"), &EnumClassName, &EnumValueName);
        UEnum* Enum = FindEnumByName(EnumClassName);
        if (Enum)
        {
            Pin->DefaultValue = EnumValueName;
        }
        else
        {
            UE_LOG(LogBpGenCompiler, Error, TEXT("SetPinDefaultValue failed to find Enum class: %s"), *EnumClassName);
            Pin->DefaultValue = TEXT("");
        }
    }
    else if (CleanValue.Equals(TEXT("true"), ESearchCase::IgnoreCase) || CleanValue.Equals(TEXT("false"), ESearchCase::IgnoreCase))
    {
        Pin->DefaultValue = CleanValue.ToLower();
    }
    else if (CleanValue.StartsWith(TEXT("FLinearColor")))
    {
        FString Temp = CleanValue.RightChop(FString(TEXT("FLinearColor")).Len()).TrimStartAndEnd();
        Temp.RemoveAt(0);
        Temp.RemoveAt(Temp.Len() - 1);
        TArray<FString> Components;
        Temp.ParseIntoArray(Components, TEXT(","), true);
        if (Components.Num() == 4)
        {
            Pin->DefaultValue = FString::Printf(TEXT("(R=%s,G=%s,B=%s,A=%s)"), *Components[0], *Components[1], *Components[2], *Components[3]);
        }
    }
    else
    {
        Pin->DefaultValue = CleanValue;
    }
    Pin->GetSchema()->TrySetDefaultValue(*Pin, Pin->DefaultValue);
}
void FBpGeneratorCompiler::FinalizeAllCommentBoxes()
{
    if (CommentMap.Num() == 0)
    {
        return;
    }
    UE_LOG(LogBpGenCompiler, Log, TEXT("All nodes placed. Now creating %d comment boxes."), CommentMap.Num());
    for (const auto& CommentPair : CommentMap)
    {
        const FString& CommentText = CommentPair.Key;
        const TArray<UEdGraphNode*>& NodesInBlock = CommentPair.Value;
        if (NodesInBlock.Num() == 0) continue;
        int32 MinX = MAX_int32, MinY = MAX_int32;
        int32 MaxX = MIN_int32, MaxY = MIN_int32;
        for (UEdGraphNode* Node : NodesInBlock)
        {
            MinX = FMath::Min(MinX, Node->NodePosX);
            MinY = FMath::Min(MinY, Node->NodePosY);
            MaxX = FMath::Max(MaxX, Node->NodePosX + Node->NodeWidth);
            MaxY = FMath::Max(MaxY, Node->NodePosY + Node->NodeHeight);
        }
        UEdGraphNode_Comment* CommentNode = NewObject<UEdGraphNode_Comment>(FunctionGraph);
        FunctionGraph->AddNode(CommentNode, true, false);
        const int32 Padding = 30;
        CommentNode->NodePosX = MinX - Padding;
        CommentNode->NodePosY = MinY - Padding;
        CommentNode->NodeWidth = (MaxX - MinX) + (Padding * 2);
        CommentNode->NodeHeight = (MaxY - MinY) + (Padding * 2);
        CommentNode->NodeComment = CommentText;
        CommentNode->CommentColor = FLinearColor(0.05f, 0.05f, 0.05f, 0.8f);
    }
}
FBpGeneratorCompiler::FBpGeneratorCompiler(UBlueprint* InTargetBlueprint)
    : TargetBlueprint(InTargetBlueprint), FunctionGraph(nullptr), EntryNode(nullptr), ResultNode(nullptr), bIsEventGraphLogic(false), bIsComponentEvent(false), bIsKeyEvent(false), bIsKeyReleasedEvent(false)
{
    CurrentPureNodeYOffset = 80;
    CurrentBaseY = 0;
    KnownLibraries.Add(TEXT("UKismetMathLibrary"), UKismetMathLibrary::StaticClass());
    KnownLibraries.Add(TEXT("UKismetSystemLibrary"), UKismetSystemLibrary::StaticClass());
    KnownLibraries.Add(TEXT("UKismetStringLibrary"), UKismetStringLibrary::StaticClass());
    KnownLibraries.Add(TEXT("UGameplayStatics"), UGameplayStatics::StaticClass());
    KnownLibraries.Add(TEXT("UKismetTextLibrary"), UKismetTextLibrary::StaticClass());
    KnownLibraries.Add(TEXT("UKismetRenderingLibrary"), UKismetRenderingLibrary::StaticClass());
    KnownLibraries.Add(TEXT("UKismetArrayLibrary"), UKismetArrayLibrary::StaticClass());
    KnownLibraries.Add(TEXT("UWidgetBlueprintLibrary"), UWidgetBlueprintLibrary::StaticClass());
    KnownLibraries.Add(TEXT("UEnhancedInputLibrary"), UEnhancedInputLibrary::StaticClass());
    KnownLibraries.Add(TEXT("UEnhancedInputLocalPlayerSubsystem"), UEnhancedInputLocalPlayerSubsystem::StaticClass());
    FunctionAliases.Add(TEXT("MaxOfFloat"), TEXT("Max"));
    FunctionAliases.Add(TEXT("MinOfFloat"), TEXT("Min"));
    FunctionAliases.Add(TEXT("Max_IntInt"), TEXT("Max"));
    FunctionAliases.Add(TEXT("Min_IntInt"), TEXT("Min"));
    FunctionAliases.Add(TEXT("Not_Bool"), TEXT("Not_PreBool"));
    FunctionAliases.Add(TEXT("Max_Int"), TEXT("Max"));
    FunctionAliases.Add(TEXT("VectorLength"), TEXT("VSize"));
    FunctionAliases.Add(TEXT("GetName"), TEXT("GetObjectName"));
    FunctionAliases.Add(TEXT("GetActorLabel"), TEXT("GetObjectName"));
    FunctionAliases.Add(TEXT("SelectBool"), TEXT("Select"));
    FunctionAliases.Add(TEXT("Add_FloatFloat"), TEXT("Add_DoubleDouble"));
    FunctionAliases.Add(TEXT("Min_Int"), TEXT("Min"));
    FunctionAliases.Add(TEXT("NormalizeTo"), TEXT("Normal"));
    FunctionAliases.Add(TEXT("Normalized2D"), TEXT("Normal"));
    FunctionAliases.Add(TEXT("Normalize"), TEXT("Normal"));
    FunctionAliases.Add(TEXT("K2_GetActorLabel"), TEXT("GetObjectName"));
    FunctionAliases.Add(TEXT("NotEqual_FloatFloat"), TEXT("NotEqual_DoubleDouble"));
    FunctionAliases.Add(TEXT("Multiply_FloatFloat"), TEXT("Multiply_DoubleDouble"));
    FunctionAliases.Add(TEXT("Div_IntInt"), TEXT("Divide_IntInt"));
    FunctionAliases.Add(TEXT("Greater_FloatFloat"), TEXT("Greater_DoubleDouble"));
    FunctionAliases.Add(TEXT("GreaterGreater_FloatFloat"), TEXT("Greater_DoubleDouble"));
    FunctionAliases.Add(TEXT("Conv_FloatToString"), TEXT("Conv_DoubleToString"));
    FunctionAliases.Add(TEXT("LessEqual_FloatFloat"), TEXT("LessEqual_DoubleDouble"));
    FunctionAliases.Add(TEXT("Div_FloatFloat"), TEXT("Divide_DoubleDouble"));
    FunctionAliases.Add(TEXT("Divide_FloatFloat"), TEXT("Divide_DoubleDouble"));
    FunctionAliases.Add(TEXT("Less_FloatFloat"), TEXT("Less_DoubleDouble"));
    FunctionAliases.Add(TEXT("DestroyActor"), TEXT("K2_DestroyActor"));
    FunctionAliases.Add(TEXT("Subtract_FloatFloat"), TEXT("Subtract_DoubleDouble"));
    FunctionAliases.Add(TEXT("SpawnActorFromClass"), TEXT("K2_SpawnActorFromClass"));
    FunctionAliases.Add(TEXT("GetEnhancedInputLocalPlayerSubSystem"), TEXT("GetEnhancedInputLocalPlayerSubsystem"));
    FunctionAliases.Add(TEXT("GetEnhancedInputLocalPlayerSubSystemFromPC"), TEXT("GetEnhancedInputLocalPlayerSubsystem"));
    UScriptStruct* HitResultStruct = TBaseStructure<FHitResult>::Get();
    if(HitResultStruct)
    {
        UClass* GameplayStaticsClass = UGameplayStatics::StaticClass();
        if(GameplayStaticsClass)
        {
            UFunction* BreakFn = GameplayStaticsClass->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameplayStatics, BreakHitResult));
            if(BreakFn)
            {
                NativeBreakFunctions.Add(HitResultStruct, BreakFn);
                UE_LOG(LogBpGenCompiler, Log, TEXT("Cached native break function for FHitResult."));
            }
        }
    }
    UClass* ActorClass = AActor::StaticClass();
    if (ActorClass)
    {
        for (TFieldIterator<UFunction> FuncIt(ActorClass, EFieldIteratorFlags::IncludeSuper); FuncIt; ++FuncIt)
        {
            UFunction* Function = *FuncIt;
            if (Function->HasAnyFunctionFlags(FUNC_BlueprintCallable))
            {
                KnownMemberFunctions.Add(Function->GetName());
                KnownMemberFunctions.Add(Function->GetName().Replace(TEXT("K2_"), TEXT("")));
            }
        }
    }
    EventNameMap.Add(TEXT("OnBeginPlay"), FName(TEXT("ReceiveBeginPlay")));
    EventNameMap.Add(TEXT("OnTick"), FName(TEXT("ReceiveTick")));
    EventNameMap.Add(TEXT("OnUpdateAnimation"), FName(TEXT("BlueprintUpdateAnimation")));
    EventNameMap.Add(TEXT("OnInitializeAnimation"), FName(TEXT("BlueprintInitializeAnimation")));
    EventNameMap.Add(TEXT("OnPostEvaluateAnimation"), FName(TEXT("BlueprintPostEvaluateAnimation")));
    EventNameMap.Add(TEXT("OnConstruct"), FName(TEXT("Construct")));
    EventNameMap.Add(TEXT("OnPreConstruct"), FName(TEXT("PreConstruct")));
    EventNameMap.Add(TEXT("OnDestruct"), FName(TEXT("Destruct")));
    EventNameMap.Add(TEXT("OnInitialized"), FName(TEXT("Initialized")));
}
FString PreProcessKnownConversions(const FString& Expression)
{
    if (Expression.Contains(TEXT("FString::FromInt")))
    {
        return Expression.Replace(TEXT("FString::FromInt"), TEXT("UKismetStringLibrary::Conv_IntToString"));
    }
    if (Expression.Contains(TEXT("FText::FromString")))
    {
        return Expression.Replace(TEXT("FText::FromString"), TEXT("UKismetTextLibrary::Conv_StringToText"));
    }
    return Expression;
}
bool FBpGeneratorCompiler::Compile(const FString& InCode)
{
    try
    {
        TSet<FGuid> ExistingNodeGUIDs;
        UEdGraph* EventGraph = FBlueprintEditorUtils::FindEventGraph(TargetBlueprint);
        if (EventGraph)
        {
            for (UEdGraphNode* Node : EventGraph->Nodes)
            {
                if (Node)
                {
                    ExistingNodeGUIDs.Add(Node->NodeGuid);
                }
            }
        }
        FunctionGraph = FBlueprintEditorUtils::FindEventGraph(TargetBlueprint);
        if (!FunctionGraph)
        {
            UE_LOG(LogBpGenCompiler, Error, TEXT("Compile FAILED: Could not find a valid Event Graph."));
            return false;
        }
        const FScopedTransaction Transaction(FText::FromString("Generate Blueprint Logic via AI"));
        TargetBlueprint->Modify();
        FunctionGraph->Modify();
        if (FunctionGraph->Nodes.Num() == 0)
        {
            UK2Node_Self* PrewarmSelfNode = NewObject<UK2Node_Self>(FunctionGraph);
            PrewarmSelfNode->CreateNewGuid(); PrewarmSelfNode->PostPlacedNewNode(); PrewarmSelfNode->AllocateDefaultPins();
            FunctionGraph->AddNode(PrewarmSelfNode, true, false);
            PrewarmSelfNode->NodePosX = -2000; PrewarmSelfNode->NodePosY = 0;
        }
        UK2Node_Self* SelfNode = NewObject<UK2Node_Self>(FunctionGraph);
        SelfNode->CreateNewGuid(); SelfNode->PostPlacedNewNode(); SelfNode->AllocateDefaultPins();
        FunctionGraph->AddNode(SelfNode, true, false);
        SelfPin = SelfNode->FindPin(UEdGraphSchema_K2::PN_Self);
        SelfNode->NodePosX = -2000; SelfNode->NodePosY = 0;
        FString RemainingCode = InCode;
        int32 CurrentGraphYOffset = 0;
        const int32 VerticalSpacing = 800;
        while (!RemainingCode.TrimStartAndEnd().IsEmpty())
        {
            FString TrimmedCode = RemainingCode.TrimStartAndEnd();
            PinMap.Empty();
            InputPins.Empty();
            LiteralMap.Empty();
            CurrentNodeX = 400;
            CommentMap.Empty();
            CurrentCommentText.Empty();
            if (SelfPin)
            {
                PinMap.Add(TEXT("SelfActor"), SelfPin);
            }
            int32 BodyStart = TrimmedCode.Find(TEXT("{"));
            if (BodyStart == -1) break;
            int32 BodyEnd = FindMatchingClosingBrace(TrimmedCode, BodyStart);
            if (BodyEnd == -1) break;
            if (TrimmedCode.StartsWith(TEXT("timeline")))
            {
                UE_LOG(LogBpGenCompiler, Warning, TEXT("DISPATCHER: Detected top-level timeline declaration."));
                FString TimelineBlock = TrimmedCode.Left(BodyEnd + 1);
                ProcessTopLevelTimeline(TimelineBlock);
            }
            else
            {
                if (!ParseFunctionSignature(TrimmedCode))
                {
                    UE_LOG(LogBpGenCompiler, Error, TEXT("Dispatcher failed: Could not parse a valid signature from the remaining code. Halting."));
                    break;
                }
                if (bIsWidgetEvent)
                {
                    UE_LOG(LogBpGenCompiler, Warning, TEXT("DISPATCHER: Detected Widget Event for '%s::%s'"), *ParsedWidgetName, *ParsedFunctionName);
                    UEdGraphPin* LastExecPin = FindAndConfigureWidgetEvent(FName(*ParsedWidgetName), FName(*ParsedFunctionName));
                    if (LastExecPin)
                    {
                        FDownstreamConnectionInfo DownstreamInfo = MoveDownstreamNodesRight(LastExecPin);
                        ResetNodePlacementForChain(LastExecPin, CurrentGraphYOffset);
                        ProcessScopedCodeBlock(CodeLines, LastExecPin);
                        ReconnectDownstreamNodes(LastExecPin, DownstreamInfo, TargetBlueprint);
                    }
                }
                else if (bIsComponentEvent)
                {
                    UE_LOG(LogBpGenCompiler, Warning, TEXT("DISPATCHER: Detected Component Event for '%s::%s'"), *ParsedComponentName, *ParsedFunctionName);
                    UEdGraphPin* LastExecPin = FindAndConfigureComponentEvent(FName(*ParsedComponentName), FName(*ParsedFunctionName));
                    if (LastExecPin)
                    {
                        FDownstreamConnectionInfo DownstreamInfo = MoveDownstreamNodesRight(LastExecPin);
                        ResetNodePlacementForChain(LastExecPin, CurrentGraphYOffset);
                        ProcessScopedCodeBlock(CodeLines, LastExecPin);
                        ReconnectDownstreamNodes(LastExecPin, DownstreamInfo, TargetBlueprint);
                    }
                }
                else if (bIsKeyEvent)
                {
                    UE_LOG(LogBpGenCompiler, Warning, TEXT("DISPATCHER: Detected Key Event for '%s'"), *ParsedKeyName);
                    UEdGraphPin* LastExecPin = FindOrCreateKeyEventNode(FName(*ParsedKeyName), bIsKeyReleasedEvent);
                    if (LastExecPin)
                    {
                        FDownstreamConnectionInfo DownstreamInfo = MoveDownstreamNodesRight(LastExecPin);
                        ResetNodePlacementForChain(LastExecPin, CurrentGraphYOffset);
                        ProcessScopedCodeBlock(CodeLines, LastExecPin);
                        ReconnectDownstreamNodes(LastExecPin, DownstreamInfo, TargetBlueprint);
                    }
                }
                else if (ParsedFunctionName == TEXT("ConstructionScript"))
                {
                    UE_LOG(LogBpGenCompiler, Warning, TEXT("DISPATCHER: Detected Construction Script logic"));
                    UEdGraph* ConstructionScriptGraph = FBlueprintEditorUtils::FindUserConstructionScript(TargetBlueprint);
                    if (ConstructionScriptGraph)
                    {
                        FunctionGraph = ConstructionScriptGraph;
                        TArray<UK2Node_FunctionEntry*> EntryNodes;
                        ConstructionScriptGraph->GetNodesOfClass<UK2Node_FunctionEntry>(EntryNodes);
                        if (EntryNodes.Num() > 0)
                        {
                            UEdGraphPin* LastExecPin = EntryNodes[0]->GetThenPin();
                            FDownstreamConnectionInfo DownstreamInfo = MoveDownstreamNodesRight(LastExecPin);
                            ResetNodePlacementForChain(LastExecPin, CurrentGraphYOffset);
                            ProcessScopedCodeBlock(CodeLines, LastExecPin);
                            ReconnectDownstreamNodes(LastExecPin, DownstreamInfo, TargetBlueprint);
                        }
                    }
                }
                else if (bIsCustomEvent)
                {
                    UE_LOG(LogBpGenCompiler, Warning, TEXT("DISPATCHER: Detected Custom Event '%s'"), *ParsedFunctionName);
                    UEdGraphPin* LastExecPin = FindOrCreateCustomEventNode(FName(*ParsedFunctionName));
                    if (LastExecPin)
                    {
                        FDownstreamConnectionInfo DownstreamInfo = MoveDownstreamNodesRight(LastExecPin);
                        ResetNodePlacementForChain(LastExecPin, CurrentGraphYOffset);
                        ProcessScopedCodeBlock(CodeLines, LastExecPin);
                        ReconnectDownstreamNodes(LastExecPin, DownstreamInfo, TargetBlueprint);
                    }
                }
                else if (bIsEventGraphLogic)
                {
                    UE_LOG(LogBpGenCompiler, Warning, TEXT("DISPATCHER: Detected Built-in Event '%s'"), *ParsedFunctionName);
                    FName EventName = EventNameMap.FindRef(ParsedFunctionName);
                    UEdGraphPin* LastExecPin = FindOrCreateEventNode(EventName);
                    if (LastExecPin)
                    {
                        UE_LOG(LogBpGenCompiler, Warning, TEXT("DISPATCHER: About to MoveDownstreamNodesRight for event '%s'"), *ParsedFunctionName);
                        FDownstreamConnectionInfo DownstreamInfo = MoveDownstreamNodesRight(LastExecPin);
                        ResetNodePlacementForChain(LastExecPin, CurrentGraphYOffset);
                        UE_LOG(LogBpGenCompiler, Warning, TEXT("DISPATCHER: About to ProcessScopedCodeBlock for event '%s'. Line count: %d"), *ParsedFunctionName, CodeLines.Num());
                        ProcessScopedCodeBlock(CodeLines, LastExecPin);
                        UE_LOG(LogBpGenCompiler, Warning, TEXT("DISPATCHER: ProcessScopedCodeBlock DONE for event '%s'"), *ParsedFunctionName);
                        ReconnectDownstreamNodes(LastExecPin, DownstreamInfo, TargetBlueprint);
                    }
                }
                else
                {
                    UE_LOG(LogBpGenCompiler, Warning, TEXT("DISPATCHER: Detected regular function '%s'"), *ParsedFunctionName);
                    UEdGraph* TargetGraph = nullptr;
                    for (UEdGraph* Graph : TargetBlueprint->FunctionGraphs) { if (Graph->GetName() == ParsedFunctionName) { TargetGraph = Graph; break; } }
                    if (TargetGraph) {
                        UE_LOG(LogBpGenCompiler, Error, TEXT("Function modification is not yet implemented."));
                    }
                    else {
                        if (!CreateNewFunction()) { return false; }
                    }
                }
            }
            RemainingCode = TrimmedCode.RightChop(BodyEnd + 1);
            CurrentGraphYOffset += VerticalSpacing;
        }
        FinalizeAllCommentBoxes();
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(TargetBlueprint);
        TargetBlueprint->PostEditChange();
        TArray<FGuid> CreatedNodeGUIDs;
        if (EventGraph)
        {
            for (UEdGraphNode* Node : EventGraph->Nodes)
            {
                if (Node && !ExistingNodeGUIDs.Contains(Node->NodeGuid))
                {
                    CreatedNodeGUIDs.Add(Node->NodeGuid);
                }
            }
        }
        if (CreatedNodeGUIDs.Num() > 0)
        {
            NodeCreationStack.Add(CreatedNodeGUIDs);
            UE_LOG(LogBpGenCompiler, Log, TEXT("Tracked %d newly created nodes for undo"), CreatedNodeGUIDs.Num());
        }
        return true;
    }
    catch (...)
    {
        UE_LOG(LogBpGenCompiler, Fatal, TEXT("A critical, unhandled exception was caught during the compilation process. The editor was saved from a crash."));
        return false;
    }
}
bool FBpGeneratorCompiler::InsertCode(UEdGraphNode* InsertionPointNode, const FString& DummyFunctionCode)
{
    try{
    if (!InsertionPointNode || !TargetBlueprint || DummyFunctionCode.IsEmpty()) { return false; }
    FunctionGraph = InsertionPointNode->GetGraph();
    if (!FunctionGraph) return false;
    TSet<FGuid> ExistingNodeGUIDs;
    for (UEdGraphNode* Node : FunctionGraph->Nodes)
    {
        if (Node)
        {
            ExistingNodeGUIDs.Add(Node->NodeGuid);
        }
    }
    const FScopedTransaction Transaction(FText::FromString("Insert Blueprint Logic via AI"));
    TargetBlueprint->Modify();
    FunctionGraph->Modify();
    const FGuid InsertionNodeGuid = InsertionPointNode->NodeGuid;
    UEdGraphPin* OutputExecPin = nullptr;
    for (UEdGraphPin* Pin : InsertionPointNode->Pins) { if (Pin && Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec) { OutputExecPin = Pin; break; } }
    if (!OutputExecPin) return false;
    UEdGraphPin* OriginalDownstreamPin = nullptr;
    FGuid OriginalDownstreamNodeGuid;
    FName OriginalDownstreamPinName;
    if (OutputExecPin->LinkedTo.Num() > 0)
    {
        OriginalDownstreamPin = OutputExecPin->LinkedTo[0];
        OriginalDownstreamNodeGuid = OriginalDownstreamPin->GetOwningNode()->NodeGuid;
        OriginalDownstreamPinName = OriginalDownstreamPin->GetFName();
        TSet<UEdGraphNode*> DownstreamNodes;
        GatherEntireConnectedChain(OriginalDownstreamPin->GetOwningNode(), DownstreamNodes);
        for (UEdGraphNode* NodeToMove : DownstreamNodes)
        {
            NodeToMove->Modify();
            NodeToMove->NodePosX += 1200;
        }
        OutputExecPin->BreakAllPinLinks();
    }
    int32 BodyStart = DummyFunctionCode.Find(TEXT("{"));
    if (BodyStart == -1) return false;
    int32 BodyEnd = FindMatchingClosingBrace(DummyFunctionCode, BodyStart);
    if (BodyEnd == -1) return false;
    FString CodeBody = DummyFunctionCode.Mid(BodyStart + 1, BodyEnd - BodyStart - 1).TrimStartAndEnd();
    if (CodeBody.StartsWith(TEXT("void ")) || CodeBody.StartsWith(TEXT("bool ")) || CodeBody.StartsWith(TEXT("int ")) || CodeBody.StartsWith(TEXT("float ")) || CodeBody.StartsWith(TEXT("FString ")) || CodeBody.StartsWith(TEXT("event ")))
    {
        int32 InnerBodyStart = CodeBody.Find(TEXT("{"));
        if (InnerBodyStart != -1)
        {
            int32 InnerBodyEnd = FindMatchingClosingBrace(CodeBody, InnerBodyStart);
            if (InnerBodyEnd != -1)
            {
                UE_LOG(LogBpGenCompiler, Warning, TEXT("Detected nested function signature from AI. Performing double-unwrap to extract the core logic."));
                CodeBody = CodeBody.Mid(InnerBodyStart + 1, InnerBodyEnd - InnerBodyStart - 1);
            }
        }
    }
    PinMap.Empty(); LiteralMap.Empty(); CommentMap.Empty(); CurrentCommentText.Empty(); TimelineMap.Empty(); PendingTimelineLogic.Empty();
    TArray<UK2Node_Self*> SelfNodes;
    FunctionGraph->GetNodesOfClass<UK2Node_Self>(SelfNodes);
    if (SelfNodes.Num() > 0) { SelfPin = SelfNodes[0]->FindPin(UEdGraphSchema_K2::PN_Self); }
    else
    {
        UK2Node_Self* SelfNode = NewObject<UK2Node_Self>(FunctionGraph);
        SelfNode->CreateNewGuid(); SelfNode->PostPlacedNewNode(); SelfNode->AllocateDefaultPins();
        FunctionGraph->AddNode(SelfNode, true, false);
        SelfPin = SelfNode->FindPin(UEdGraphSchema_K2::PN_Self);
        SelfNode->NodePosX = InsertionPointNode->NodePosX - 400; SelfNode->NodePosY = InsertionPointNode->NodePosY;
    }
    PinMap.Add(TEXT("SelfActor"), SelfPin);
    UEdGraphPin* UpstreamDataPin = nullptr;
    for (UEdGraphPin* Pin : InsertionPointNode->Pins) { if (Pin && Pin->Direction == EGPD_Input && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec && Pin->LinkedTo.Num() > 0) { UpstreamDataPin = Pin->LinkedTo[0]; break; } }
    if (UpstreamDataPin) { PinMap.Add(TEXT("$input"), UpstreamDataPin); }
    TArray<FString> AllCodeLines;
    CodeBody.ParseIntoArrayLines(AllCodeLines, true);
    bool bDidCompile = false;
    TArray<FString> ExecutionLines;
    for (int32 i = 0; i < AllCodeLines.Num(); ++i)
    {
        if (AllCodeLines[i].TrimStartAndEnd().StartsWith(TEXT("timeline")))
        {
            UEdGraphPin* DummyExecPin = nullptr;
            ProcessTimelineDeclaration(AllCodeLines, i, DummyExecPin);
            bDidCompile = true;
        }
        else { ExecutionLines.Add(AllCodeLines[i]); }
    }
    if (bDidCompile)
    {
        UEdGraphNode* RefreshedInsertionNode = FBlueprintEditorUtils::GetNodeByGUID(TargetBlueprint, InsertionNodeGuid);
        if (!RefreshedInsertionNode) return false;
        OutputExecPin = nullptr;
        for (UEdGraphPin* Pin : RefreshedInsertionNode->Pins) { if (Pin && Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec) { OutputExecPin = Pin; break; } }
        if (!OutputExecPin) return false;
        PinMap.Empty();
        TArray<UK2Node_Self*> RefreshedSelfNodes;
        FunctionGraph->GetNodesOfClass<UK2Node_Self>(RefreshedSelfNodes);
        if (RefreshedSelfNodes.Num() > 0) { SelfPin = RefreshedSelfNodes[0]->FindPin(UEdGraphSchema_K2::PN_Self); }
        if (SelfPin) { PinMap.Add(TEXT("SelfActor"), SelfPin); }
        TMap<FString, UK2Node_Timeline*> RefreshedTimelineMap;
        for (auto const& [Name, StaleNode] : TimelineMap)
        {
            TArray<UK2Node_Timeline*> AllTimelineNodes;
            FBlueprintEditorUtils::GetAllNodesOfClass(TargetBlueprint, AllTimelineNodes);
            for (UK2Node_Timeline* Node : AllTimelineNodes) { if (Node && Node->TimelineName == StaleNode->TimelineName) { RefreshedTimelineMap.Add(Name, Node); break; } }
        }
        TimelineMap = RefreshedTimelineMap;
    }
    UEdGraphPin* LastExecPinOfNewChain = OutputExecPin;
    ResetNodePlacementForChain(LastExecPinOfNewChain);
    ProcessScopedCodeBlock(ExecutionLines, LastExecPinOfNewChain);
    if (OriginalDownstreamNodeGuid.IsValid() && LastExecPinOfNewChain)
    {
        UEdGraphNode* RefreshedDownstreamNode = FBlueprintEditorUtils::GetNodeByGUID(TargetBlueprint, OriginalDownstreamNodeGuid);
        if (RefreshedDownstreamNode)
        {
            UEdGraphPin* RefreshedDownstreamPin = RefreshedDownstreamNode->FindPin(OriginalDownstreamPinName);
            if (RefreshedDownstreamPin)
            {
                LastExecPinOfNewChain->GetSchema()->TryCreateConnection(LastExecPinOfNewChain, RefreshedDownstreamPin);
            }
        }
    }
    FinalizeAllCommentBoxes();
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(TargetBlueprint);
    TArray<FGuid> CreatedNodeGUIDs;
    for (UEdGraphNode* Node : FunctionGraph->Nodes)
    {
        if (Node && !ExistingNodeGUIDs.Contains(Node->NodeGuid))
        {
            CreatedNodeGUIDs.Add(Node->NodeGuid);
        }
    }
    if (CreatedNodeGUIDs.Num() > 0)
    {
        NodeCreationStack.Add(CreatedNodeGUIDs);
        UE_LOG(LogBpGenCompiler, Log, TEXT("Tracked %d newly created nodes for undo (InsertCode)"), CreatedNodeGUIDs.Num());
    }
    return true;
        }
        catch (...)
        {
            UE_LOG(LogBpGenCompiler, Fatal, TEXT("A critical, unhandled exception was caught during the code insertion process. The editor was saved from a crash."));
            return false;
        }
}
void FBpGeneratorCompiler::ProcessIfStatement(const TArray<FString>& AllLines, int32& InOutLineIndex, UEdGraphPin*& InOutLastExecPin)
{
    UE_LOG(LogBpGenCompiler, Warning, TEXT("      >>>>> ENTERING Multi-line ProcessIfStatement"));
    const FRegexPattern CastPattern(TEXT("if\\s*\\(\\s*([\\w\\d_]+)\\*\\s+([\\w\\d_]+)\\s*=\\s*Cast<([\\w\\d_]+)>\\s*\\(\\s*([\\w\\d_]+)\\s*\\)\\)"));
    FRegexMatcher CastMatcher(CastPattern, AllLines[InOutLineIndex]);
    if (CastMatcher.FindNext())
    {
        UE_LOG(LogBpGenCompiler, Log, TEXT("Cast pattern detected. Rerouting to specialized handler."));
        FString TargetType = CastMatcher.GetCaptureGroup(1);
        FString NewVarName = CastMatcher.GetCaptureGroup(2);
        FString ObjectToCast = CastMatcher.GetCaptureGroup(4);
        UClass* ClassToCastTo = FindFirstObjectSafe<UClass>(*TargetType);
        if (!ClassToCastTo)
        {
            ClassToCastTo = FindBlueprintClassByName(TargetType);
        }
        if (!ClassToCastTo)
        {
            UE_LOG(LogBpGenCompiler, Error, TEXT("Cast failed: Could not find class '%s' to cast to."), *TargetType);
            for (int32 j = InOutLineIndex; j < AllLines.Num(); ++j) { if (AllLines[j].Contains("}")) { InOutLineIndex = j; break; } }
            return;
        }
        UK2Node_DynamicCast* CastNode = NewObject<UK2Node_DynamicCast>(FunctionGraph);
        CastNode->TargetType = ClassToCastTo;
        CastNode->CreateNewGuid();
        FunctionGraph->AddNode(CastNode, true, false);
        CastNode->PostPlacedNewNode();
        CastNode->AllocateDefaultPins();
        PlaceNode(CastNode, InOutLastExecPin);
        if (InOutLastExecPin)
        {
            InOutLastExecPin->GetSchema()->TryCreateConnection(InOutLastExecPin, CastNode->GetExecPin());
        }
        UEdGraphPin* SourceObjectPin = ProcessExpression(ObjectToCast, InOutLastExecPin);
        if (SourceObjectPin)
        {
            CastNode->GetCastSourcePin()->GetSchema()->TryCreateConnection(SourceObjectPin, CastNode->GetCastSourcePin());
        }
        FString FullIfBlock;
        int32 NestingLevel = 0;
        int32 StartLine = InOutLineIndex;
        for (int32 j = StartLine; j < AllLines.Num(); ++j)
        {
            FString CurrentLine = AllLines[j];
            FullIfBlock += CurrentLine + TEXT("\n");
            for (const TCHAR& Char : CurrentLine) { if (Char == '{') NestingLevel++; else if (Char == '}') NestingLevel--; }
            if (NestingLevel == 0 && !FullIfBlock.IsEmpty())
            {
                if (j + 1 >= AllLines.Num() || !AllLines[j + 1].TrimStartAndEnd().StartsWith(TEXT("else"))) { InOutLineIndex = j; break; }
            }
        }
        int32 TrueBodyStart = FullIfBlock.Find(TEXT("{"));
        int32 TrueBodyEnd = FindMatchingClosingBrace(FullIfBlock, TrueBodyStart);
        FString TrueBlockContent = FullIfBlock.Mid(TrueBodyStart + 1, TrueBodyEnd - TrueBodyStart - 1);
        TArray<FString> TrueBlockLines;
        TrueBlockContent.ParseIntoArrayLines(TrueBlockLines, true);
        UEdGraphPin* CastSucceededPin = CastNode->GetValidCastPin();
        PinMap.Add(NewVarName, CastNode->GetCastResultPin());
        try {
            ProcessScopedCodeBlock(TrueBlockLines, CastSucceededPin);
        }
        catch (...)
        {
            PinMap.Remove(NewVarName);
            return;
        }
        int32 ElsePos = FullIfBlock.Find(TEXT("else"), ESearchCase::CaseSensitive, ESearchDir::FromStart, TrueBodyEnd);
        if (ElsePos != INDEX_NONE)
        {
            int32 FalseBodyStart = FullIfBlock.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, ElsePos);
            int32 FalseBodyEnd = FindMatchingClosingBrace(FullIfBlock, FalseBodyStart);
            FString FalseBlockContent = FullIfBlock.Mid(FalseBodyStart + 1, FalseBodyEnd - FalseBodyStart - 1);
            TArray<FString> FalseBlockLines;
            FalseBlockContent.ParseIntoArrayLines(FalseBlockLines, true);
            UEdGraphPin* CastFailedPin = CastNode->GetInvalidCastPin();
            ProcessScopedCodeBlock(FalseBlockLines, CastFailedPin, true);
        }
        PinMap.Remove(NewVarName);
        UE_LOG(LogBpGenCompiler, Warning, TEXT("      <<<<< EXITING Multi-line ProcessIfStatement (Casting Path)."));
        return;
    }
    if (AllLines[InOutLineIndex].Contains(TEXT("GetDataTableRow")) && InOutLineIndex > 0)
    {
        FString PrevLine = AllLines[InOutLineIndex - 1].TrimStartAndEnd();
        if (PrevLine.Contains(TEXT(" ")) && !PrevLine.Contains(TEXT("=")) && !PrevLine.Contains(TEXT("(")))
        {
            UE_LOG(LogBpGenCompiler, Log, TEXT("GetDataTableRow pattern detected. Rerouting to specialized handler."));
            ProcessGetDataTableRow(AllLines, InOutLineIndex, InOutLastExecPin);
            return;
        }
    }
    FString FullIfBlock;
    int32 NestingLevel = 0;
    int32 StartLine = InOutLineIndex;
    bool bFoundEnd = false;
    for (int32 j = StartLine; j < AllLines.Num(); ++j)
    {
        FString CurrentLine = AllLines[j];
        FullIfBlock += CurrentLine + TEXT("\n");
        for (const TCHAR& Char : CurrentLine)
        {
            if (Char == '{') NestingLevel++;
            else if (Char == '}') NestingLevel--;
        }
        if (NestingLevel == 0 && !bFoundEnd)
        {
            if (j + 1 >= AllLines.Num() || !AllLines[j + 1].TrimStartAndEnd().StartsWith(TEXT("else")))
            {
                InOutLineIndex = j;
                bFoundEnd = true;
                break;
            }
        }
    }
    UK2Node_IfThenElse* BranchNode = NewObject<UK2Node_IfThenElse>(FunctionGraph);
    BranchNode->CreateNewGuid();
    BranchNode->PostPlacedNewNode();
    BranchNode->AllocateDefaultPins();
    FunctionGraph->AddNode(BranchNode, true, false);
    PlaceNode(BranchNode, InOutLastExecPin);
    if (InOutLastExecPin)
    {
        InOutLastExecPin->GetSchema()->TryCreateConnection(InOutLastExecPin, BranchNode->GetExecPin());
    }
    int32 ConditionStart = FullIfBlock.Find(TEXT("("));
    if (ConditionStart == INDEX_NONE) return;
    int32 ConditionEnd = FindMatchingClosingParen(FullIfBlock, ConditionStart);
    if (ConditionEnd == INDEX_NONE) return;
    FString ConditionString = FullIfBlock.Mid(ConditionStart + 1, ConditionEnd - ConditionStart - 1);
    UE_LOG(LogBpGenCompiler, Error, TEXT("IF_BUG_LOG: About to process condition '%s'. ConditionExecPin is on node: %s"),
        *ConditionString,
        InOutLastExecPin ? *InOutLastExecPin->GetOwningNode()->GetName() : TEXT("NULL")
    );
    UEdGraphPin* ConditionExecPin = InOutLastExecPin;
    UEdGraphPin* ConditionResultPin = ProcessExpression(ConditionString, ConditionExecPin);
    if (ConditionResultPin)
    {
        BranchNode->GetConditionPin()->GetSchema()->TryCreateConnection(ConditionResultPin, BranchNode->GetConditionPin());
    }
    int32 TrueBodyStart = FullIfBlock.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, ConditionEnd);
    if (TrueBodyStart == INDEX_NONE) return;
    int32 TrueBodyEnd = FindMatchingClosingBrace(FullIfBlock, TrueBodyStart);
    if (TrueBodyEnd == INDEX_NONE) return;
    FString TrueBlockContent = FullIfBlock.Mid(TrueBodyStart + 1, TrueBodyEnd - TrueBodyStart - 1);
    TArray<FString> TrueBlockLines;
    TrueBlockContent.ParseIntoArrayLines(TrueBlockLines, true);
    UEdGraphPin* ThenPin = BranchNode->GetThenPin();
    ProcessScopedCodeBlock(TrueBlockLines, ThenPin);
    int32 ElsePos = FullIfBlock.Find(TEXT("else"), ESearchCase::CaseSensitive, ESearchDir::FromStart, TrueBodyEnd);
    if (ElsePos != INDEX_NONE)
    {
        int32 FalseBodyStart = FullIfBlock.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, ElsePos);
        if (FalseBodyStart != INDEX_NONE)
        {
            int32 FalseBodyEnd = FindMatchingClosingBrace(FullIfBlock, FalseBodyStart);
            if (FalseBodyEnd != INDEX_NONE)
            {
                FString FalseBlockContent = FullIfBlock.Mid(FalseBodyStart + 1, FalseBodyEnd - FalseBodyStart - 1);
                TArray<FString> FalseBlockLines;
                FalseBlockContent.ParseIntoArrayLines(FalseBlockLines, true);
                UEdGraphPin* ElsePin = BranchNode->GetElsePin();
                ProcessScopedCodeBlock(FalseBlockLines, ElsePin, true);
            }
        }
    }
    UE_LOG(LogBpGenCompiler, Warning, TEXT("      <<<<< EXITING Multi-line ProcessIfStatement (Boolean Path)."));
}
UEdGraphPin* FBpGeneratorCompiler::ProcessExpression(const FString& Expression, UEdGraphPin*& InOutLastExecPin)
{
    UE_LOG(LogBpGenCompiler, Display, TEXT("----> ENTERING ProcessExpression for: '%s'"), *Expression);
    UE_LOG(LogBpGenCompiler, Display, TEXT("      SelfPin is currently: %s"), SelfPin ? TEXT("VALID") : TEXT("!!! NULL !!!"));
    FString PreProcessedExpression = PreProcessKnownConversions(Expression);
    FString CleanedExpression = StripCStyleCasts(PreProcessedExpression.TrimStartAndEnd());
    if (CleanedExpression.Equals(TEXT("K2_GetPawnOwner()"), ESearchCase::IgnoreCase) || CleanedExpression.Equals(TEXT("TryGetPawnOwner()"), ESearchCase::IgnoreCase))
    {
        UE_LOG(LogBpGenCompiler, Log, TEXT("  -> Special Case: Detected TryGetPawnOwner(). Creating node."));
        UFunction* GetPawnOwnerFunc = UAnimInstance::StaticClass()->FindFunctionByName(FName("TryGetPawnOwner"));
        if (GetPawnOwnerFunc)
        {
            UK2Node_CallFunction* GetOwnerNode = NewObject<UK2Node_CallFunction>(FunctionGraph);
            FunctionGraph->AddNode(GetOwnerNode, true, false);
            GetOwnerNode->SetFromFunction(GetPawnOwnerFunc);
            GetOwnerNode->PostPlacedNewNode();
            GetOwnerNode->AllocateDefaultPins();
            PlaceNode(GetOwnerNode, InOutLastExecPin);
            return GetOwnerNode->GetReturnValuePin();
        }
        else
        {
            UE_LOG(LogBpGenCompiler, Error, TEXT("  -> Special Case FAILED: Could not find UAnimInstance::TryGetPawnOwner function in engine!"));
            return nullptr;
        }
    }
    int32 AddMappingContext_ParenStart = CleanedExpression.Find(TEXT("AddMappingContext("));
    if (AddMappingContext_ParenStart != INDEX_NONE && AddMappingContext_ParenStart < 2)
    {
        UE_LOG(LogBpGenCompiler, Log, TEXT("  -> SPECIAL: AddMappingContext detected, creating subsystem getter and call"));
        int32 LastParen = CleanedExpression.Find(TEXT(")"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
        FString ParamsStr = CleanedExpression.Mid(AddMappingContext_ParenStart + 18, LastParen - AddMappingContext_ParenStart - 18);
        TArray<FString> Params = SplitParameters(ParamsStr);
        UE_LOG(LogBpGenCompiler, Log, TEXT("  -> AddMappingContext params string: '%s', parsed %d params"), *ParamsStr, Params.Num());
        if (Params.Num() >= 1)
        {
            UK2Node_GetSubsystem* GetSubsystemNode = NewObject<UK2Node_GetSubsystem>(FunctionGraph);
            GetSubsystemNode->CreateNewGuid();
            FunctionGraph->AddNode(GetSubsystemNode, true, false);
            GetSubsystemNode->Initialize(UEnhancedInputLocalPlayerSubsystem::StaticClass());
            GetSubsystemNode->PostPlacedNewNode();
            GetSubsystemNode->AllocateDefaultPins();
            PlaceNode(GetSubsystemNode, InOutLastExecPin);
            UEdGraphPin* SubsystemOutput = GetSubsystemNode->GetResultPin();
            UClass* SubsystemClass = UEnhancedInputLocalPlayerSubsystem::StaticClass();
            UFunction* AddMappingContextFunc = SubsystemClass->FindFunctionByName(FName("AddMappingContext"));
            if (AddMappingContextFunc && SubsystemOutput)
            {
                UK2Node_CallFunction* CallNode = NewObject<UK2Node_CallFunction>(FunctionGraph);
                CallNode->CreateNewGuid();
                FunctionGraph->AddNode(CallNode, true, false);
                CallNode->SetFromFunction(AddMappingContextFunc);
                CallNode->PostPlacedNewNode();
                CallNode->AllocateDefaultPins();
                PlaceNode(CallNode, InOutLastExecPin);
                if (UEdGraphPin* SubsystemExecIn = GetSubsystemNode->GetExecPin())
                {
                    if (InOutLastExecPin && IsValid(InOutLastExecPin->GetOwningNodeUnchecked()))
                    {
                        InOutLastExecPin->GetSchema()->TryCreateConnection(InOutLastExecPin, SubsystemExecIn);
                    }
                }
                if (UEdGraphPin* CallExecIn = CallNode->GetExecPin())
                {
                    if (UEdGraphPin* SubsystemThen = GetSubsystemNode->GetThenPin())
                    {
                        SubsystemThen->GetSchema()->TryCreateConnection(SubsystemThen, CallExecIn);
                    }
                }
                InOutLastExecPin = CallNode->GetThenPin();
                if (UEdGraphPin* CallSelfPin = CallNode->FindPin(UEdGraphSchema_K2::PN_Self))
                {
                    if (SubsystemOutput)
                    {
                        CallSelfPin->GetSchema()->TryCreateConnection(SubsystemOutput, CallSelfPin);
                    }
                }
                TArray<FName> ParamNames;
                for (TFieldIterator<FProperty> PropIt(AddMappingContextFunc); PropIt; ++PropIt)
                {
                    if (PropIt->HasAnyPropertyFlags(CPF_Parm) && !PropIt->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm))
                    {
                        ParamNames.Add(PropIt->GetFName());
                    }
                }
                for (int32 i = 0; i < Params.Num() && i < ParamNames.Num(); ++i)
                {
                    UEdGraphPin* DestPin = CallNode->FindPin(ParamNames[i]);
                    if (DestPin && !DestPin->bHidden)
                    {
                        FString ParamExpr = Params[i].TrimStartAndEnd();
                        UEdGraphPin* SourcePin = ProcessExpression(ParamExpr, InOutLastExecPin);
                        if (SourcePin)
                        {
                            DestPin->GetSchema()->TryCreateConnection(SourcePin, DestPin);
                        }
                    }
                }
                UE_LOG(LogBpGenCompiler, Log, TEXT("  -> AddMappingContext node created successfully"));
                return nullptr;
            }
        }
    }
    if (CleanedExpression.Equals(TEXT("SelfActor"), ESearchCase::IgnoreCase))
    {
        return SelfPin;
    }
    UE_LOG(LogBpGenCompiler, Display, TEXT("ProcessExpression: Received expression '%s'"), *CleanedExpression);
    if (PinMap.Contains(CleanedExpression))
    {
        UE_LOG(LogBpGenCompiler, Log, TEXT("  -> SUCCESS: Found expression in PinMap. Returning pin from node '%s'."), *PinMap[CleanedExpression]->GetOwningNode()->GetName());
        return PinMap[CleanedExpression];
    }
    if (LiteralMap.Contains(CleanedExpression))
    {
        return nullptr;
    }
    UClass* ClassToSearch = TargetBlueprint->SkeletonGeneratedClass ? TargetBlueprint->SkeletonGeneratedClass : TargetBlueprint->GeneratedClass;
    if (ClassToSearch)
    {
        FProperty* FoundProperty = FindFProperty<FProperty>(ClassToSearch, FName(*CleanedExpression));
        if (FoundProperty)
        {
            UE_LOG(LogBpGenCompiler, Log, TEXT("  -> JIT GET: Found persistent member variable '%s'. Creating GET node."), *CleanedExpression);
            UK2Node_VariableGet* VarGetNode = NewObject<UK2Node_VariableGet>(FunctionGraph);
            VarGetNode->CreateNewGuid();
            FunctionGraph->AddNode(VarGetNode, true, false);
            VarGetNode->PostPlacedNewNode();
            VarGetNode->AllocateDefaultPins();
            VarGetNode->VariableReference.SetFromField<FProperty>(FoundProperty, true);
            VarGetNode->ReconstructNode();
            PlaceNode(VarGetNode, InOutLastExecPin);
            if (UEdGraphPin* ValuePin = VarGetNode->GetValuePin())
            {
                PinMap.Add(CleanedExpression, ValuePin);
                return ValuePin;
            }
        }
    }
    if (UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(TargetBlueprint))
    {
        FProperty* WidgetProperty = nullptr;
        if (WidgetBlueprint->SkeletonGeneratedClass)
        {
            WidgetProperty = FindFProperty<FProperty>(WidgetBlueprint->SkeletonGeneratedClass, FName(*CleanedExpression));
        }
        if (!WidgetProperty && WidgetBlueprint->GeneratedClass)
        {
            WidgetProperty = FindFProperty<FProperty>(WidgetBlueprint->GeneratedClass, FName(*CleanedExpression));
        }
        if (!WidgetProperty && WidgetBlueprint->WidgetTree)
        {
            UWidget* FoundWidget = WidgetBlueprint->WidgetTree->FindWidget(FName(*CleanedExpression));
            if (FoundWidget)
            {
                UE_LOG(LogBpGenCompiler, Log, TEXT("  -> Widget '%s' found in WidgetTree but not as property. Compiling blueprint..."), *CleanedExpression);
                FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
                if (WidgetBlueprint->SkeletonGeneratedClass)
                {
                    WidgetProperty = FindFProperty<FProperty>(WidgetBlueprint->SkeletonGeneratedClass, FName(*CleanedExpression));
                }
                if (!WidgetProperty && WidgetBlueprint->GeneratedClass)
                {
                    WidgetProperty = FindFProperty<FProperty>(WidgetBlueprint->GeneratedClass, FName(*CleanedExpression));
                }
            }
        }
        if (WidgetProperty)
        {
            UE_LOG(LogBpGenCompiler, Log, TEXT("  -> JIT Discovery: Found Widget variable '%s'."), *CleanedExpression);
            UK2Node_VariableGet* WidgetVarNode = NewObject<UK2Node_VariableGet>(FunctionGraph);
            WidgetVarNode->CreateNewGuid();
            FunctionGraph->AddNode(WidgetVarNode, true, false);
            WidgetVarNode->PostPlacedNewNode();
            WidgetVarNode->AllocateDefaultPins();
            WidgetVarNode->VariableReference.SetFromField<FProperty>(WidgetProperty, true);
            WidgetVarNode->ReconstructNode();
            PlaceNode(WidgetVarNode, InOutLastExecPin);
            if (UEdGraphPin* ValuePin = WidgetVarNode->GetValuePin())
            {
                PinMap.Add(CleanedExpression, ValuePin);
                return ValuePin;
            }
        }
    }
    if (CleanedExpression.Contains(TEXT(".")) && CleanedExpression.EndsWith(TEXT("()")))
    {
        FString ObjectName, FunctionName;
        if (CleanedExpression.Split(TEXT("."), &ObjectName, &FunctionName))
        {
            ObjectName = ObjectName.TrimStartAndEnd();
            FunctionName.RemoveFromEnd(TEXT("()"));
            UE_LOG(LogBpGenCompiler, Log, TEXT("-> Attempting to call function '%s' on object '%s'"), *FunctionName, *ObjectName);
            if (UK2Node_Timeline** FoundNodePtr = TimelineMap.Find(ObjectName))
            {
                UK2Node_Timeline* TimelineNode = *FoundNodePtr;
                UE_LOG(LogBpGenCompiler, Warning, TEXT("  -> Found Timeline Node '%s' in map."), *ObjectName);
                PlaceNode(TimelineNode, InOutLastExecPin);
                UEdGraphPin* TargetExecPin = TimelineNode->FindPin(FName(*FunctionName), EGPD_Input);
                if (TargetExecPin && InOutLastExecPin)
                {
                    InOutLastExecPin->GetSchema()->TryCreateConnection(InOutLastExecPin, TargetExecPin);
                    InOutLastExecPin = TimelineNode->GetFinishedPin();
                }
                if (FTimelineLogicBlock* Logic = PendingTimelineLogic.Find(ObjectName))
                {
                    if (Logic->OnUpdateCode.Num() > 0)
                    {
                        UE_LOG(LogBpGenCompiler, Warning, TEXT(">>>>>> PROCESSING PENDING OnUpdate LOGIC for timeline '%s'"), *ObjectName);
                        UEdGraphPin* UpdatePin = Logic->TimelineNode->GetUpdatePin();
                        TArray<FString> TempPinNames;
                        PinMap.Add(TEXT("SelfActor"), SelfPin);
                        TempPinNames.Add(TEXT("SelfActor"));
                        for (auto const& [Name, Pin] : Logic->LocalPinMap) { PinMap.Add(Name, Pin); TempPinNames.Add(Name); }
                        for (UEdGraphPin* Pin : Logic->TimelineNode->Pins) {
                            if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec) {
                                PinMap.Add(Pin->GetName(), Pin);
                                TempPinNames.Add(Pin->GetName());
                            }
                        }
                        ResetNodePlacementForChain(UpdatePin);
                        ProcessScopedCodeBlock(Logic->OnUpdateCode, UpdatePin);
                        for (const FString& PinName : TempPinNames) { PinMap.Remove(PinName); }
                    }
                    if (Logic->OnFinishedCode.Num() > 0)
                    {
                        UE_LOG(LogBpGenCompiler, Warning, TEXT(">>>>>> PROCESSING PENDING OnFinished LOGIC for timeline '%s'"), *ObjectName);
                        UEdGraphPin* FinishedPin = InOutLastExecPin;
                        TArray<FString> TempPinNames;
                        PinMap.Add(TEXT("SelfActor"), SelfPin);
                        TempPinNames.Add(TEXT("SelfActor"));
                        for (auto const& [Name, Pin] : Logic->LocalPinMap) { PinMap.Add(Name, Pin); TempPinNames.Add(Name); }
                        for (UEdGraphPin* Pin : Logic->TimelineNode->Pins) {
                            if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec) {
                                PinMap.Add(Pin->GetName(), Pin);
                                TempPinNames.Add(Pin->GetName());
                            }
                        }
                        ResetNodePlacementForChain(FinishedPin);
                        ProcessScopedCodeBlock(Logic->OnFinishedCode, FinishedPin);
                        for (const FString& PinName : TempPinNames) { PinMap.Remove(PinName); }
                    }
                    PendingTimelineLogic.Remove(ObjectName);
                }
                return nullptr;
            }
        }
    }
    if (CleanedExpression.Contains(TEXT("UKismetTextLibrary::Format(")))
    {
        UE_LOG(LogBpGenCompiler, Log, TEXT("ProcessExpression: Detected Format Text pattern."));
        int32 ParenStart = CleanedExpression.Find(TEXT("("));
        int32 ParenEnd = CleanedExpression.Find(TEXT(")"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
        FString ParamsStr = CleanedExpression.Mid(ParenStart + 1, ParenEnd - ParenStart - 1);
        TArray<FString> Params = SplitParameters(ParamsStr);
        if (Params.Num() > 1)
        {
            FString FormatPatternVarName = Params[0];
            FString FormatString = LiteralMap.FindRef(FormatPatternVarName);
            if (FormatString.IsEmpty()) FormatString = FormatPatternVarName.TrimQuotes();
            UE_LOG(LogBpGenCompiler, Log, TEXT("  - Format String retrieved: '%s'"), *FormatString);
            UK2Node_FormatText* FormatNode = NewObject<UK2Node_FormatText>(FunctionGraph);
            FormatNode->CreateNewGuid();
            FunctionGraph->AddNode(FormatNode, true, false);
            FormatNode->PostPlacedNewNode();
            FormatNode->AllocateDefaultPins();
            if (UEdGraphPin* FormatPin = FormatNode->GetFormatPin())
            {
                FormatPin->DefaultTextValue = FText::FromString(FormatString);
            }
            PlaceNode(FormatNode, InOutLastExecPin);
            for (int32 i = 1; i < Params.Num(); ++i)
            {
                FormatNode->AddArgumentPin();
                UEdGraphPin* NewDestPin = nullptr;
                for (int32 PinIdx = FormatNode->Pins.Num() - 1; PinIdx >= 0; --PinIdx)
                {
                    UEdGraphPin* CurrentPin = FormatNode->Pins[PinIdx];
                    if (CurrentPin && CurrentPin->Direction == EGPD_Input && CurrentPin->PinName != TEXT("Format") && CurrentPin->LinkedTo.Num() == 0)
                    {
                        NewDestPin = CurrentPin;
                        break;
                    }
                }
                if (NewDestPin)
                {
                    FString ParamToConnect = Params[i];
                    UEdGraphPin* SourcePin = ProcessExpression(ParamToConnect, InOutLastExecPin);
                    if (SourcePin)
                    {
                        if (NewDestPin->GetSchema()->TryCreateConnection(SourcePin, NewDestPin))
                        {
                            UE_LOG(LogBpGenCompiler, Log, TEXT("        - Connection SUCCESSFUL."));
                        }
                    }
                }
            }
            return FormatNode->FindPin(TEXT("Result"));
        }
    }
    if (CleanedExpression.Contains(TEXT(".")) && !CleanedExpression.Contains(TEXT("(")) && !CleanedExpression.IsNumeric())
    {
        FString StructVariableName, MemberName;
        if (CleanedExpression.Split(TEXT("."), &StructVariableName, &MemberName))
        {
            UEdGraphPin** FoundStructPinPtr = PinMap.Find(StructVariableName);
            if (FoundStructPinPtr && *FoundStructPinPtr)
            {
                UEdGraphPin* StructPin = *FoundStructPinPtr;
                UScriptStruct* StructType = nullptr;
                UK2Node_GetDataTableRow* SourceDataTableNode = Cast<UK2Node_GetDataTableRow>(StructPin->GetOwningNode());
                if (SourceDataTableNode && StructPin->PinName == TEXT("Out Row"))
                {
                    StructType = SourceDataTableNode->GetDataTableRowStructType();
                    UE_LOG(LogBpGenCompiler, Warning, TEXT("Special Case: Got struct type '%s' directly from GetDataTableRow node."), *StructType->GetName());
                }
                else
                {
                    StructType = Cast<UScriptStruct>(StructPin->PinType.PinSubCategoryObject.Get());
                }
                if (StructType)
                {
                    if (UFunction* const* FoundBreakFunctionPtr = NativeBreakFunctions.Find(StructType))
                    {
                        UFunction* FoundBreakFunction = *FoundBreakFunctionPtr;
                        UE_LOG(LogBpGenCompiler, Log, TEXT("Found native Break function '%s' for struct '%s'. Creating CallFunction node."), *FoundBreakFunction->GetName(), *StructType->GetName());
                        UK2Node_CallFunction* CallFuncNode = NewObject<UK2Node_CallFunction>(FunctionGraph);
                        CallFuncNode->SetFromFunction(FoundBreakFunction);
                        CallFuncNode->CreateNewGuid();
                        FunctionGraph->AddNode(CallFuncNode, true, false);
                        PlaceNode(CallFuncNode, InOutLastExecPin);
                        CallFuncNode->PostPlacedNewNode();
                        CallFuncNode->AllocateDefaultPins();
                        UEdGraphPin* DestInputPin = nullptr;
                        for (UEdGraphPin* Pin : CallFuncNode->Pins)
                        {
                            if (Pin && Pin->Direction == EGPD_Input && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
                            {
                                DestInputPin = Pin;
                                break;
                            }
                        }
                        if (DestInputPin)
                        {
                            DestInputPin->GetSchema()->TryCreateConnection(StructPin, DestInputPin);
                        }
                        PlaceNode(CallFuncNode, InOutLastExecPin);
                        return CallFuncNode->FindPin(FName(*MemberName));
                    }
                    else
                    {
                        UE_LOG(LogBpGenCompiler, Log, TEXT("Creating generic BreakStruct node for struct type '%s'"), *StructType->GetName());
                        UK2Node_BreakStruct* BreakNode = NewObject<UK2Node_BreakStruct>(FunctionGraph);
                        BreakNode->StructType = StructType;
                        BreakNode->bMadeAfterOverridePinRemoval = true;
                        BreakNode->CreateNewGuid();
                        FunctionGraph->AddNode(BreakNode, true, false);
                        BreakNode->PostPlacedNewNode();
                        BreakNode->AllocateDefaultPins();
                        BreakNode->ReconstructNode();
                        UEdGraphPin* InputStructPin = nullptr;
                        for (UEdGraphPin* Pin : BreakNode->Pins) { if (Pin && Pin->Direction == EGPD_Input) { InputStructPin = Pin; break; } }
                        if (InputStructPin) InputStructPin->GetSchema()->TryCreateConnection(StructPin, InputStructPin);
                        PlaceNode(BreakNode, InOutLastExecPin);
                        UEdGraphPin* MemberPin = nullptr;
                        for (UEdGraphPin* Pin : BreakNode->Pins)
                        {
                            if (Pin && Pin->Direction == EGPD_Output)
                            {
                                FString PinDisplayName = Pin->GetDisplayName().ToString();
                                PinDisplayName.RemoveSpacesInline();
                                if (PinDisplayName.Equals(MemberName, ESearchCase::IgnoreCase))
                                {
                                    MemberPin = Pin;
                                    break;
                                }
                            }
                        }
                        if (!MemberPin)
                        {
                            UE_LOG(LogBpGenCompiler, Error, TEXT("Could not find MemberPin '%s' on generic break node."), *MemberName);
                        }
                        return MemberPin;
                    }
                }
                else
                {
                    UE_LOG(LogBpGenCompiler, Error, TEXT("Could not determine StructType for variable '%s'."), *StructVariableName);
                }
            }
        }
    }
    int32 QuestionMarkPos = -1;
    int32 ColonPos = -1;
    if (CleanedExpression.FindLastChar('?', QuestionMarkPos) && CleanedExpression.FindLastChar(':', ColonPos) && ColonPos > QuestionMarkPos)
    {
        FString ConditionStr = CleanedExpression.Left(QuestionMarkPos).TrimStartAndEnd();
        FString TrueStr = CleanedExpression.Mid(QuestionMarkPos + 1, ColonPos - QuestionMarkPos - 1).TrimStartAndEnd();
        FString FalseStr = CleanedExpression.RightChop(ColonPos + 1).TrimStartAndEnd();
        UEdGraphPin* ConditionPin = ProcessExpression(ConditionStr, InOutLastExecPin);
        if (!ConditionPin) return nullptr;
        UK2Node_Select* SelectNode = NewObject<UK2Node_Select>(FunctionGraph);
        SelectNode->CreateNewGuid();
        SelectNode->PostPlacedNewNode();
        SelectNode->AllocateDefaultPins();
        FunctionGraph->AddNode(SelectNode, true, false);
        PlaceNode(SelectNode, InOutLastExecPin);
        if (UEdGraphPin* IndexPin = SelectNode->GetIndexPin()) IndexPin->GetSchema()->TryCreateConnection(ConditionPin, IndexPin);
        if (UEdGraphPin* Select_FalsePin = SelectNode->FindPin(TEXT("Option 0")))
        {
            UEdGraphPin* FalsePin = ProcessExpression(FalseStr, InOutLastExecPin);
            if (FalsePin) Select_FalsePin->GetSchema()->TryCreateConnection(FalsePin, Select_FalsePin);
            else SetPinDefaultValue(Select_FalsePin, FalseStr);
            SelectNode->PinConnectionListChanged(Select_FalsePin);
        }
        if (UEdGraphPin* Select_TruePin = SelectNode->FindPin(TEXT("Option 1")))
        {
            UEdGraphPin* TruePin = ProcessExpression(TrueStr, InOutLastExecPin);
            if (TruePin) Select_TruePin->GetSchema()->TryCreateConnection(TruePin, Select_TruePin);
            else SetPinDefaultValue(Select_TruePin, TrueStr);
        }
        return SelectNode->GetReturnValuePin();
    }
    if (CleanedExpression.Contains(TEXT("->")) && !CleanedExpression.Contains(TEXT("(")))
    {
        FString ObjectName, MemberName;
        if (CleanedExpression.Split(TEXT("->"), &ObjectName, &MemberName))
        {
            UE_LOG(LogBpGenCompiler, Log, TEXT("  -> Member Access Detected: Getting '%s' from object '%s'"), *MemberName, *ObjectName);
            UEdGraphPin* SourceObjectPin = ProcessExpression(ObjectName, InOutLastExecPin);
            if (!SourceObjectPin)
            {
                UE_LOG(LogBpGenCompiler, Error, TEXT("    -> FAILED: Could not find source object pin for '%s'"), *ObjectName);
                return nullptr;
            }
            UClass* SourceClass = Cast<UClass>(SourceObjectPin->PinType.PinSubCategoryObject.Get());
            if (!SourceClass) return nullptr;
            FProperty* MemberProperty = FindFProperty<FProperty>(SourceClass, FName(*MemberName));
            if (!MemberProperty)
            {
                UE_LOG(LogBpGenCompiler, Error, TEXT("    -> FAILED: Could not find member property '%s' on class '%s'."), *MemberName, *SourceClass->GetName());
                return nullptr;
            }
            UK2Node_VariableGet* GetNode = NewObject<UK2Node_VariableGet>(FunctionGraph);
            FunctionGraph->AddNode(GetNode, true, false);
            GetNode->PostPlacedNewNode();
            GetNode->VariableReference.SetFromField<FProperty>(MemberProperty, false);
            GetNode->ReconstructNode();
            PlaceNode(GetNode, InOutLastExecPin);
            if (UEdGraphPin* GetNodeSelfPin = GetNode->FindPin(UEdGraphSchema_K2::PN_Self))
            {
                GetNodeSelfPin->GetSchema()->TryCreateConnection(SourceObjectPin, GetNodeSelfPin);
            }
            return GetNode->GetValuePin();
        }
    }
    int32 ArrowOperator = CleanedExpression.Find(TEXT("->"));
    int32 FirstParen = CleanedExpression.Find(TEXT("("));
    if (ArrowOperator != INDEX_NONE && FirstParen != INDEX_NONE && ArrowOperator < FirstParen)
    {
        FString VariableName = CleanedExpression.Left(ArrowOperator).TrimStartAndEnd();
        if (VariableName.Equals(TEXT("SelfActor"), ESearchCase::IgnoreCase))
        {
            FString FunctionCall = CleanedExpression.RightChop(ArrowOperator + 2);
            UE_LOG(LogBpGenCompiler, Log, TEXT("  -> Re-routing 'SelfActor' call to local function handler for: '%s'"), *FunctionCall);
            return ProcessExpression(FunctionCall, InOutLastExecPin);
        }
        FString FunctionName = CleanedExpression.Mid(ArrowOperator + 2, FirstParen - (ArrowOperator + 2));
        UE_LOG(LogBpGenCompiler, Log, TEXT("  -> Parsing Member Function Call: Variable='%s', Function='%s'"), *VariableName, *FunctionName);
        UClass* BlueprintClass = TargetBlueprint->SkeletonGeneratedClass ? TargetBlueprint->SkeletonGeneratedClass : TargetBlueprint->GeneratedClass;
        FProperty* ComponentProperty = (BlueprintClass) ? FindFProperty<FProperty>(BlueprintClass, FName(*VariableName)) : nullptr;
        if (ComponentProperty && !PinMap.Contains(VariableName))
        {
            UE_LOG(LogBpGenCompiler, Log, TEXT("  -> JIT Discovery Path: Atomically getting component '%s' and calling function '%s'."), *VariableName, *FunctionName);
            UK2Node_VariableGet* VarGetNode = NewObject<UK2Node_VariableGet>(FunctionGraph);
            VarGetNode->CreateNewGuid();
            FunctionGraph->AddNode(VarGetNode, true, false);
            VarGetNode->PostPlacedNewNode();
            VarGetNode->AllocateDefaultPins();
            VarGetNode->VariableReference.SetFromField<FProperty>(ComponentProperty, true);
            VarGetNode->ReconstructNode();
            PlaceNode(VarGetNode, InOutLastExecPin);
            UEdGraphPin* SourceComponentPin = VarGetNode->GetValuePin();
            if (!SourceComponentPin) return nullptr;
            FObjectProperty* ObjProperty = CastField<FObjectProperty>(ComponentProperty);
            if (!ObjProperty || !ObjProperty->PropertyClass) return nullptr;
            UClass* TargetClass = ObjProperty->PropertyClass;
            UFunction* FunctionToCall = TargetClass->FindFunctionByName(FName(*FunctionName), EIncludeSuperFlag::IncludeSuper);
            if (!FunctionToCall)
            {
                UE_LOG(LogBpGenCompiler, Error, TEXT("    -> FATAL: Function '%s' could not be found on component class '%s'."), *FunctionName, *TargetClass->GetName());
                return nullptr;
            }
            UK2Node_CallFunction* CallFuncNode = NewObject<UK2Node_CallFunction>(FunctionGraph);
            CallFuncNode->SetFromFunction(FunctionToCall);
            CallFuncNode->CreateNewGuid();
            CallFuncNode->PostPlacedNewNode();
            CallFuncNode->AllocateDefaultPins();
            FunctionGraph->AddNode(CallFuncNode);
            PlaceNode(CallFuncNode, InOutLastExecPin);
            if (UEdGraphPin* TargetPin = CallFuncNode->FindPin(UEdGraphSchema_K2::PN_Self))
            {
                TargetPin->GetSchema()->TryCreateConnection(SourceComponentPin, TargetPin);
            }
            if (FunctionToCall->HasAnyFunctionFlags(FUNC_BlueprintPure) == false)
            {
                if (UEdGraphPin* NodeExecIn = CallFuncNode->GetExecPin())
                {
                    if (InOutLastExecPin && IsValid(InOutLastExecPin->GetOwningNodeUnchecked()))
                    {
                        InOutLastExecPin->GetSchema()->TryCreateConnection(InOutLastExecPin, NodeExecIn);
                    }
                    InOutLastExecPin = CallFuncNode->GetThenPin();
                }
            }
            int32 LastParen = CleanedExpression.Find(TEXT(")"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
            FString ParamsStr = CleanedExpression.Mid(FirstParen + 1, LastParen - FirstParen - 1);
            TArray<FString> Params = SplitParameters(ParamsStr);
            TArray<FName> FunctionParamNames;
            for (TFieldIterator<FProperty> PropIt(FunctionToCall); PropIt; ++PropIt) { if (PropIt->HasAnyPropertyFlags(CPF_Parm) && !PropIt->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm)) { FunctionParamNames.Add(PropIt->GetFName()); } }
            for (int32 i = 0; i < Params.Num(); ++i)
            {
                if (FunctionParamNames.IsValidIndex(i))
                {
                    UEdGraphPin* DestPin = CallFuncNode->FindPin(FunctionParamNames[i]);
                    if (DestPin && !DestPin->bHidden)
                    {
                        FString ParamExpression = Params[i].TrimStartAndEnd();
                        UEdGraphPin* SourcePin = ProcessExpression(ParamExpression, InOutLastExecPin);
                        if (SourcePin)
                        {
                            DestPin->GetSchema()->TryCreateConnection(SourcePin, DestPin);
                        }
                        else
                        {
                            SetPinDefaultValue(DestPin, ParamExpression);
                        }
                    }
                }
            }
            for (UEdGraphPin* Pin : CallFuncNode->Pins)
            {
                if (Pin && Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
                {
                    return Pin;
                }
            }
            return nullptr;
        }
        if (TimelineMap.Contains(VariableName))
        {
            UE_LOG(LogBpGenCompiler, Log, TEXT("Timeline Call detected: '%s'"), *CleanedExpression);
            UK2Node_Timeline* TimelineNode = TimelineMap[VariableName];
            PlaceNode(TimelineNode, InOutLastExecPin);
            UEdGraphPin* TargetExecPin = TimelineNode->FindPin(FName(*FunctionName), EGPD_Input);
            if (TargetExecPin && InOutLastExecPin)
            {
                InOutLastExecPin->GetSchema()->TryCreateConnection(InOutLastExecPin, TargetExecPin);
            }
            InOutLastExecPin = TimelineNode->GetFinishedPin();
            if (FTimelineLogicBlock* Logic = PendingTimelineLogic.Find(VariableName))
            {
                TMap<FString, UEdGraphPin*> TempPinsForTimeline;
                for (UEdGraphPin* Pin : TimelineNode->Pins) {
                    if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec) {
                        PinMap.Add(Pin->GetName(), Pin);
                        TempPinsForTimeline.Add(Pin->GetName(), Pin);
                    }
                }
                if (Logic->OnUpdateCode.Num() > 0)
                {
                    UEdGraphPin* UpdatePin = TimelineNode->GetUpdatePin();
                    ResetNodePlacementForChain(UpdatePin);
                    ProcessScopedCodeBlock(Logic->OnUpdateCode, UpdatePin);
                }
                for (auto const& [Name, Pin] : TempPinsForTimeline) { PinMap.Remove(Name); }
                PendingTimelineLogic.Remove(VariableName);
            }
            return nullptr;
        }
        UEdGraphPin* SourceObjectPin = ProcessExpression(VariableName, InOutLastExecPin);
        if (!SourceObjectPin)
        {
            UE_LOG(LogBpGenCompiler, Error, TEXT("    -> FATAL: Could not find or discover source variable pin for '%s'."), *VariableName);
            return nullptr;
        }
        if (FunctionName.StartsWith(TEXT("Get")) && TargetBlueprint->SkeletonGeneratedClass)
        {
            FString ComponentName = FunctionName.RightChop(3);
            if (ComponentProperty)
            {
                UE_LOG(LogBpGenCompiler, Log, TEXT("  -> Intercepted '%s'. Creating a direct 'Get' node for existing component '%s'."), *FunctionName, *ComponentName);
                UK2Node_VariableGet* VarGetNode = NewObject<UK2Node_VariableGet>(FunctionGraph);
                VarGetNode->CreateNewGuid();
                FunctionGraph->AddNode(VarGetNode, true, false);
                VarGetNode->PostPlacedNewNode();
                VarGetNode->AllocateDefaultPins();
                VarGetNode->VariableReference.SetFromField<FProperty>(ComponentProperty, true);
                VarGetNode->ReconstructNode();
                PlaceNode(VarGetNode, InOutLastExecPin);
                if (UEdGraphPin* ValuePin = VarGetNode->GetValuePin())
                {
                    PinMap.Add(ComponentName, ValuePin);
                    return ValuePin;
                }
            }
        }
        if (FunctionName.StartsWith(TEXT("Get")) && TargetBlueprint->SkeletonGeneratedClass)
        {
            FString ComponentName = FunctionName.RightChop(3);
            if (ComponentProperty)
            {
                UE_LOG(LogBpGenCompiler, Log, TEXT("  -> Intercepted '%s'. Creating a direct 'Get' node for existing component '%s'."), *FunctionName, *ComponentName);
                UK2Node_VariableGet* VarGetNode = NewObject<UK2Node_VariableGet>(FunctionGraph);
                VarGetNode->CreateNewGuid();
                FunctionGraph->AddNode(VarGetNode, true, false);
                VarGetNode->PostPlacedNewNode();
                VarGetNode->AllocateDefaultPins();
                VarGetNode->VariableReference.SetFromField<FProperty>(ComponentProperty, true);
                VarGetNode->ReconstructNode();
                PlaceNode(VarGetNode, InOutLastExecPin);
                if (UEdGraphPin* ValuePin = VarGetNode->GetValuePin())
                {
                    PinMap.Add(ComponentName, ValuePin);
                    return ValuePin;
                }
            }
        }
        UFunction* FunctionToCall = nullptr;
        UClass* TargetClass = Cast<UClass>(SourceObjectPin->PinType.PinSubCategoryObject.Get());
        if (TargetClass)
        {
            FString FinalFunctionName = FunctionName;
            FunctionToCall = TargetClass->FindFunctionByName(FName(*FinalFunctionName));
            if (!FunctionToCall)
            {
                if (const FString* CorrectedName = FunctionAliases.Find(FinalFunctionName))
                {
                    UE_LOG(LogTemp, Log, TEXT("ALIAS MATCH (Member): Found alias for '%s'. Trying corrected name '%s'."), *FinalFunctionName, **CorrectedName);
                    FinalFunctionName = *CorrectedName;
                    FunctionToCall = TargetClass->FindFunctionByName(FName(*FinalFunctionName));
                }
            }
            if (!FunctionToCall)
            {
                UE_LOG(LogTemp, Warning, TEXT("Could not find exact function or alias for '%s' on class '%s'. Beginning fuzzy search..."), *FunctionName, *TargetClass->GetName());
                UFunction* BestMatchFunction = nullptr;
                int32 MinDistance = MAX_int32;
                const int32 LevenshteinTolerance = 3;
                for (TFieldIterator<UFunction> FuncIt(TargetClass); FuncIt; ++FuncIt)
                {
                    UFunction* CurrentFunc = *FuncIt;
                    if (CurrentFunc && CurrentFunc->HasAnyFunctionFlags(FUNC_BlueprintCallable))
                    {
                        FString RealFuncName = CurrentFunc->GetName();
                        int32 Distance = CalculateLevenshteinDistance(FunctionName, RealFuncName);
                        if (Distance < MinDistance)
                        {
                            MinDistance = Distance;
                            BestMatchFunction = CurrentFunc;
                        }
                    }
                }
                if (BestMatchFunction && MinDistance <= LevenshteinTolerance)
                {
                    UE_LOG(LogTemp, Log, TEXT("FUZZY MATCH SUCCESS (Member): AI requested '%s', but found close match '%s' on class '%s' (Distance: %d). Using this instead."),
                        *FunctionName,
                        *BestMatchFunction->GetName(),
                        *TargetClass->GetName(),
                        MinDistance);
                    FunctionToCall = BestMatchFunction;
                }
            }
        }
        if (!FunctionToCall)
        {
            UE_LOG(LogBpGenCompiler, Error, TEXT("    -> FATAL: Function '%s' could not be found on class '%s' after checking for exact, alias, and fuzzy matches."), *FunctionName, *TargetClass->GetName());
            return nullptr;
        }
        UK2Node_CallFunction* CallFuncNode = NewObject<UK2Node_CallFunction>(FunctionGraph);
        CallFuncNode->SetFromFunction(FunctionToCall);
        CallFuncNode->CreateNewGuid();
        CallFuncNode->PostPlacedNewNode();
        CallFuncNode->AllocateDefaultPins();
        FunctionGraph->AddNode(CallFuncNode);
        PlaceNode(CallFuncNode, InOutLastExecPin);
        UEdGraphPin* TargetPin = CallFuncNode->FindPin(UEdGraphSchema_K2::PN_Self);
        if (TargetPin)
        {
            TargetPin->GetSchema()->TryCreateConnection(SourceObjectPin, TargetPin);
        }
        if (FunctionToCall->HasAnyFunctionFlags(FUNC_BlueprintPure) == false)
        {
            if (UEdGraphPin* NodeExecIn = CallFuncNode->GetExecPin())
            {
                if (InOutLastExecPin && IsValid(InOutLastExecPin->GetOwningNodeUnchecked()))
                {
                    InOutLastExecPin->GetSchema()->TryCreateConnection(InOutLastExecPin, NodeExecIn);
                }
                InOutLastExecPin = CallFuncNode->GetThenPin();
            }
        }
        int32 LastParen = CleanedExpression.Find(TEXT(")"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
        FString ParamsStr = CleanedExpression.Mid(FirstParen + 1, LastParen - FirstParen - 1);
        TArray<FString> Params = SplitParameters(ParamsStr);
        TArray<FName> FunctionParamNames;
        for (TFieldIterator<FProperty> PropIt(FunctionToCall); PropIt; ++PropIt) { if (PropIt->HasAnyPropertyFlags(CPF_Parm) && !PropIt->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm)) { FunctionParamNames.Add(PropIt->GetFName()); } }
        for (int32 i = 0; i < Params.Num(); ++i)
        {
            if (FunctionParamNames.IsValidIndex(i))
            {
                FName PinName = FunctionParamNames[i];
                UEdGraphPin* DestPin = CallFuncNode->FindPin(PinName);
                if (DestPin && !DestPin->bHidden)
                {
                    FString ParamExpression = Params[i].TrimStartAndEnd();
                    if (ParamExpression.EndsWith(TEXT("::StaticClass()")))
                    {
                        FString ClassParamName = ParamExpression.Left(ParamExpression.Find(TEXT("::")));
                        UClass* FoundParamClass = FindBlueprintClassByName(ClassParamName);
                        if (FoundParamClass)
                        {
                            DestPin->DefaultObject = FoundParamClass;
                        }
                    }
                    else
                    {
                        UEdGraphPin* SourcePin = ProcessExpression(ParamExpression, InOutLastExecPin);
                        if (SourcePin)
                        {
                            DestPin->GetSchema()->TryCreateConnection(SourcePin, DestPin);
                        }
                        else
                        {
                            SetPinDefaultValue(DestPin, ParamExpression);
                        }
                    }
                }
            }
        }
        CallFuncNode->ReconstructNode();
        for (UEdGraphPin* Pin : CallFuncNode->Pins)
        {
            if (Pin && Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
            {
                return Pin;
            }
        }
        return nullptr;
    }
    int32 ScopeOperator = CleanedExpression.Find(TEXT("::"));
    if (ScopeOperator != INDEX_NONE)
    {
        int32 StaticFirstParen = CleanedExpression.Find(TEXT("("));
        if (StaticFirstParen != INDEX_NONE && ScopeOperator < StaticFirstParen)
        {
            FString ClassName = CleanedExpression.Left(ScopeOperator);
            FString FunctionName = CleanedExpression.Mid(ScopeOperator + 2, StaticFirstParen - (ScopeOperator + 2));
            if (ClassName == TEXT("UGameplayStatics") &&
                (FunctionName == TEXT("GetPlayerCharacter") ||
                    FunctionName == TEXT("GetPlayerController") ||
                    FunctionName == TEXT("GetPlayerPawn") ||
                    FunctionName == TEXT("GetGameMode")))
            {
                UClass* LibClass = UGameplayStatics::StaticClass();
                UFunction* FuncToCall = LibClass->FindFunctionByName(FName(*FunctionName));
                if (FuncToCall)
                {
                    UK2Node_CallFunction* GetNode = NewObject<UK2Node_CallFunction>(FunctionGraph);
                    GetNode->SetFromFunction(FuncToCall);
                    GetNode->CreateNewGuid();
                    FunctionGraph->AddNode(GetNode, true, false);
                    GetNode->PostPlacedNewNode();
                    GetNode->AllocateDefaultPins();
                    PlaceNode(GetNode, InOutLastExecPin);
                    return GetNode->GetReturnValuePin();
                }
            }
            if (FunctionName == TEXT("EqualEqual_EnumEnum")) FunctionName = TEXT("EqualEqual_ByteByte");
            UClass* FoundClass = KnownLibraries.FindRef(ClassName);
            if (!FoundClass) { UE_LOG(LogTemp, Error, TEXT("Class '%s' not found in KnownLibraries."), *ClassName); return nullptr; }
            FString FinalFunctionName = FunctionName;
            UFunction* FunctionToCall = FoundClass->FindFunctionByName(FName(*FinalFunctionName));
            if (!FunctionToCall)
            {
                if (const FString* CorrectedName = FunctionAliases.Find(FinalFunctionName))
                {
                    UE_LOG(LogTemp, Log, TEXT("ALIAS MATCH: Found alias for '%s'. Trying corrected name '%s'."), *FinalFunctionName, **CorrectedName);
                    FinalFunctionName = *CorrectedName;
                    FunctionToCall = FoundClass->FindFunctionByName(FName(*FinalFunctionName));
                }
            }
            if (!FunctionToCall)
            {
                UE_LOG(LogTemp, Warning, TEXT("Could not find exact function or alias for '%s'. Beginning fuzzy search..."), *FunctionName);
                UFunction* BestMatchFunction = nullptr;
                UClass* BestMatchClass = nullptr;
                int32 MinDistance = MAX_int32;
                const int32 LevenshteinTolerance = 3;
                for (auto const& [LibName, LibClass] : KnownLibraries)
                {
                    if (LibClass)
                    {
                        for (TFieldIterator<UFunction> FuncIt(LibClass); FuncIt; ++FuncIt)
                        {
                            UFunction* CurrentFunc = *FuncIt;
                            if (CurrentFunc && CurrentFunc->HasAnyFunctionFlags(FUNC_BlueprintCallable))
                            {
                                FString RealFuncName = CurrentFunc->GetName();
                                int32 Distance = CalculateLevenshteinDistance(FunctionName, RealFuncName);
                                if (Distance < MinDistance)
                                {
                                    MinDistance = Distance;
                                    BestMatchFunction = CurrentFunc;
                                    BestMatchClass = LibClass;
                                }
                            }
                        }
                    }
                }
                if (BestMatchFunction && MinDistance <= LevenshteinTolerance)
                {
                    UE_LOG(LogTemp, Log, TEXT("FUZZY MATCH SUCCESS: AI requested '%s', but found close match '%s' in library '%s' (Distance: %d). Using this instead."),
                        *FunctionName,
                        *BestMatchFunction->GetName(),
                        *BestMatchClass->GetName(),
                        MinDistance);
                    FunctionToCall = BestMatchFunction;
                    FoundClass = BestMatchClass;
                }
            }
            if (!FunctionToCall)
            {
                UE_LOG(LogTemp, Error, TEXT("FATAL: Could not find function '%s' after checking for exact, alias, and fuzzy matches."), *FunctionName);
                return nullptr;
            }
            bool bIsArrayLibraryFunction = (ClassName == TEXT("UKismetArrayLibrary"));
            UK2Node_CallFunction* CallFuncNode = nullptr;
            if (bIsArrayLibraryFunction)
            {
                CallFuncNode = NewObject<UK2Node_CallArrayFunction>(FunctionGraph);
            }
            else
            {
                CallFuncNode = NewObject<UK2Node_CallFunction>(FunctionGraph);
            }
            CallFuncNode->SetFromFunction(FunctionToCall);
            CallFuncNode->CreateNewGuid();
            CallFuncNode->PostPlacedNewNode();
            CallFuncNode->AllocateDefaultPins();
            int32 LastParen = CleanedExpression.Find(TEXT(")"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
            FString ParamsStr = CleanedExpression.Mid(StaticFirstParen + 1, LastParen - StaticFirstParen - 1);
            TArray<FString> Params = SplitParameters(ParamsStr);
            TArray<UEdGraphPin*> OtherParamPins;
            for (TFieldIterator<FProperty> PropIt(FunctionToCall); PropIt; ++PropIt)
            {
                if (PropIt->HasAnyPropertyFlags(CPF_Parm) && !PropIt->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm) && PropIt->GetFName() != TEXT("WorldContextObject"))
                {
                    if (UEdGraphPin* ParamPin = CallFuncNode->FindPin(PropIt->GetFName())) { OtherParamPins.Add(ParamPin); }
                }
            }
            TArray<TTuple<UEdGraphPin*, UEdGraphPin*>> PendingConnections;
            TArray<TTuple<UEdGraphPin*, FString>> PendingLiterals;
            UE_LOG(LogBpGenCompiler, Error, TEXT("================ GROUND TRUTH LOGS START ================"));
            UE_LOG(LogBpGenCompiler, Error, TEXT("RAW PARAMS FROM AI (%d total):"), Params.Num());
            for (int32 i = 0; i < Params.Num(); ++i)
            {
                UE_LOG(LogBpGenCompiler, Warning, TEXT("  -> Raw Param [%d]: '%s'"), i, *Params[i]);
            }
            TArray<FString> InputParams;
            for (const FString& Param : Params)
            {
                const FString TrimmedParam = Param.TrimStartAndEnd();
                const FName ParamAsName(*TrimmedParam);
                if (ParamAsName.IsEqual(TEXT("SelfActor"), ENameCase::IgnoreCase) ||
                    ParamAsName.IsEqual(TEXT("ActorsToIgnore"), ENameCase::IgnoreCase) ||
                    ParamAsName.IsEqual(TEXT("OutHit"), ENameCase::IgnoreCase))
                {
                    continue;
                }
                InputParams.Add(TrimmedParam);
            }
            TArray<UEdGraphPin*> DestPins;
            for (UEdGraphPin* Pin : CallFuncNode->Pins)
            {
                if (Pin && Pin->Direction == EGPD_Input && !Pin->bHidden &&
                    Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec &&
                    Pin->PinName != UEdGraphSchema_K2::PN_Self &&
                    !Pin->PinToolTip.Contains(TEXT("World Context Object")) &&
                    !Pin->PinName.IsEqual(TEXT("ActorsToIgnore"), ENameCase::IgnoreCase))
                {
                    DestPins.Add(Pin);
                }
            }
            UE_LOG(LogBpGenCompiler, Error, TEXT("GATHERED NODE DESTINATION PINS (%d total):"), DestPins.Num());
            for (int32 i = 0; i < DestPins.Num(); ++i)
            {
                UE_LOG(LogBpGenCompiler, Warning, TEXT("  -> Dest Pin [%d]: '%s' (Type: %s)"), i, *DestPins[i]->PinName.ToString(), *DestPins[i]->PinType.PinCategory.ToString());
            }
            UE_LOG(LogBpGenCompiler, Error, TEXT("MATCHING LOOP (Will process %d pairs):"), FMath::Min(InputParams.Num(), DestPins.Num()));
            for (int32 i = 0; i < InputParams.Num() && i < DestPins.Num(); ++i)
            {
                UEdGraphPin* DestPin = DestPins[i];
                FString ParamExpression = InputParams[i];
                UE_LOG(LogBpGenCompiler, Log, TEXT("  -> MATCHING [%d]: AI Param '%s'  ==>  Node Pin '%s'"), i, *ParamExpression, *DestPin->PinName.ToString());
                if (ParamExpression.EndsWith(TEXT("::StaticClass()")) && DestPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class)
                {
                    FString ClassParamName = ParamExpression.Left(ParamExpression.Find(TEXT("::")));
                    UE_LOG(LogBpGenCompiler, Log, TEXT("  -> Special Case: Detected StaticClass() for Blueprint '%s'."), *ClassParamName);
                    UClass* FoundParamClass = FindBlueprintClassByName(ClassParamName);
                    if (FoundParamClass)
                    {
                        DestPin->DefaultObject = FoundParamClass;
                    }
                    else
                    {
                        UE_LOG(LogBpGenCompiler, Error, TEXT("    -> FAILED to find Blueprint class '%s'."), *ClassParamName);
                        PendingLiterals.Add(MakeTuple(DestPin, ParamExpression));
                    }
                }
                else
                {
                    UEdGraphPin* DummyExecPin = nullptr;
                    UEdGraphPin* SourcePin = ProcessExpression(ParamExpression, DummyExecPin);
                    if (SourcePin)
                    {
                        PendingConnections.Add(MakeTuple(SourcePin, DestPin));
                    }
                    else
                    {
                        PendingLiterals.Add(MakeTuple(DestPin, ParamExpression));
                    }
                }
            }
            UE_LOG(LogBpGenCompiler, Error, TEXT("================ GROUND TRUTH LOGS END ================"));
            if (bIsArrayLibraryFunction)
            {
                UE_LOG(LogBpGenCompiler, Warning, TEXT("  -> Array Function detected: %s"), *FunctionName);
                UEdGraphPin* TargetArrayPin = CallFuncNode->FindPin(TEXT("TargetArray"));
                if (!TargetArrayPin) TargetArrayPin = CallFuncNode->FindPin(TEXT("Target Array"));
                if (TargetArrayPin)
                {
                    UE_LOG(LogBpGenCompiler, Warning, TEXT("  -> Found TargetArray pin, current type: %s, container: %d"),
                        *TargetArrayPin->PinType.PinCategory.ToString(),
                        (int32)TargetArrayPin->PinType.ContainerType);
                    for (int32 i = 0; i < PendingConnections.Num(); ++i)
                    {
                        UEdGraphPin* DestPin = PendingConnections[i].Get<1>();
                        if (DestPin == TargetArrayPin)
                        {
                            UEdGraphPin* SourcePin = PendingConnections[i].Get<0>();
                            UE_LOG(LogBpGenCompiler, Warning, TEXT("  -> SourcePin container type: %d, category: %s"),
                                (int32)SourcePin->PinType.ContainerType,
                                *SourcePin->PinType.PinCategory.ToString());
                            bool bConnected = SourcePin->GetSchema()->TryCreateConnection(SourcePin, DestPin);
                            UE_LOG(LogBpGenCompiler, Warning, TEXT("  -> Array Function: Connected Target Array pin (success: %s)"), bConnected ? TEXT("true") : TEXT("false"));
                            if (SourcePin->PinType.ContainerType == EPinContainerType::Array)
                            {
                                FEdGraphPinType ArrayType = SourcePin->PinType;
                                FEdGraphPinType InnerType = SourcePin->PinType;
                                InnerType.ContainerType = EPinContainerType::None;
                                UE_LOG(LogBpGenCompiler, Warning, TEXT("  -> Array type: %s, Inner type: %s"),
                                    *ArrayType.PinCategory.ToString(),
                                    *InnerType.PinCategory.ToString());
                                for (UEdGraphPin* OtherPin : CallFuncNode->Pins)
                                {
                                    if (!OtherPin || OtherPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
                                    {
                                        continue;
                                    }
                                    bool bIsArrayPin = (OtherPin->PinType.ContainerType == EPinContainerType::Array);
                                    if (bIsArrayPin)
                                    {
                                        OtherPin->PinType = ArrayType;
                                        UE_LOG(LogBpGenCompiler, Warning, TEXT("  -> Array Function: Set array type on wildcard pin '%s' (dir: %d)"),
                                            *OtherPin->PinName.ToString(), (int32)OtherPin->Direction);
                                    }
                                    else
                                    {
                                        OtherPin->PinType = InnerType;
                                        UE_LOG(LogBpGenCompiler, Warning, TEXT("  -> Array Function: Set inner type on wildcard pin '%s' (dir: %d)"),
                                            *OtherPin->PinName.ToString(), (int32)OtherPin->Direction);
                                    }
                                }
                                CallFuncNode->PinConnectionListChanged(DestPin);
                                CallFuncNode->Modify();
                            }
                            PendingConnections.RemoveAt(i);
                            break;
                        }
                    }
                }
                else
                {
                    UE_LOG(LogBpGenCompiler, Warning, TEXT("  -> TargetArray pin NOT FOUND"));
                }
            }
            for (const auto& Connection : PendingConnections)
            {
                Connection.Get<0>()->GetSchema()->TryCreateConnection(Connection.Get<0>(), Connection.Get<1>());
            }
            for (UEdGraphPin* Pin : CallFuncNode->Pins) {
                if (Pin && Pin->PinToolTip.Contains(TEXT("World Context Object")) && SelfPin) {
                    Pin->GetSchema()->TryCreateConnection(SelfPin, Pin);
                    break;
                }
            }
            for (const auto& Literal : PendingLiterals)
            {
                FString FinalLiteralValue = Literal.Get<1>();
                if (const FString* MappedValue = LiteralMap.Find(Literal.Get<1>()))
                {
                    FinalLiteralValue = *MappedValue;
                }
                SetPinDefaultValue(Literal.Get<0>(), FinalLiteralValue);
            }
            if (UEdGraphPin* OutHitPin = CallFuncNode->FindPin(TEXT("OutHit")))
            {
                if (OutHitPin->Direction == EGPD_Output)
                {
                    PinMap.Add(TEXT("OutHit"), OutHitPin);
                }
            }
            if (ClassName == TEXT("UWidgetBlueprintLibrary") && FunctionName == TEXT("Create"))
            {
                UEdGraphPin* OwningPlayerPin = CallFuncNode->FindPin(TEXT("OwningPlayer"));
                if (OwningPlayerPin && OwningPlayerPin->LinkedTo.Num() == 0)
                {
                    UE_LOG(LogBpGenCompiler, Log, TEXT("Create Widget's 'Owning Player' is unconnected. Automatically creating 'Get Player Controller'."));
                    UFunction* GetControllerFunc = UGameplayStatics::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerController));
                    UK2Node_CallFunction* GetControllerNode = NewObject<UK2Node_CallFunction>(FunctionGraph);
                    GetControllerNode->SetFromFunction(GetControllerFunc);
                    GetControllerNode->CreateNewGuid();
                    FunctionGraph->AddNode(GetControllerNode, true, false);
                    GetControllerNode->PostPlacedNewNode();
                    GetControllerNode->AllocateDefaultPins();
                    PlaceNode(GetControllerNode, InOutLastExecPin, -250, 80);
                    if (UEdGraphPin* ControllerReturnPin = GetControllerNode->GetReturnValuePin())
                    {
                        OwningPlayerPin->GetSchema()->TryCreateConnection(ControllerReturnPin, OwningPlayerPin);
                    }
                }
            }
            if (!bIsArrayLibraryFunction)
            {
                CallFuncNode->ReconstructNode();
            }
            FunctionGraph->AddNode(CallFuncNode);
            PlaceNode(CallFuncNode, InOutLastExecPin);
            if (FunctionName == TEXT("EqualEqual_ByteByte"))
            {
                UEdGraphNode_Comment* CommentNode = NewObject<UEdGraphNode_Comment>(FunctionGraph);
                FunctionGraph->AddNode(CommentNode, true, false);
                CommentNode->NodeComment = TEXT("you can use the \"Equal - Utility\" - it had dropdown for enums type");
                const int32 Padding = 20;
                CommentNode->NodePosX = CallFuncNode->NodePosX - Padding;
                CommentNode->NodePosY = CallFuncNode->NodePosY - Padding * 2;
                CommentNode->NodeWidth = 250;
                CommentNode->NodeHeight = CallFuncNode->NodeHeight + Padding * 4;
                CommentNode->CommentColor = FLinearColor(0.6f, 0.6f, 0.1f, 0.8f);
            }
            if (FunctionToCall->HasAnyFunctionFlags(FUNC_BlueprintPure) == false)
            {
                if (UEdGraphPin* NodeExecIn = CallFuncNode->GetExecPin())
                {
                    if (InOutLastExecPin && IsValid(InOutLastExecPin->GetOwningNodeUnchecked()))
                    {
                        InOutLastExecPin->GetSchema()->TryCreateConnection(InOutLastExecPin, NodeExecIn);
                    }
                    InOutLastExecPin = CallFuncNode->GetThenPin();
                }
            }
            for (UEdGraphPin* Pin : CallFuncNode->Pins)
            {
                if (Pin && Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
                {
                    return Pin;
                }
            }
            return nullptr;
        }
    }
    int32 Local_ParenStart = CleanedExpression.Find(TEXT("("));
    if (Local_ParenStart != INDEX_NONE)
    {
        FString Local_FunctionName = CleanedExpression.Left(Local_ParenStart).TrimStartAndEnd();
        UFunction* FoundFunction = nullptr;
        if (ClassToSearch)
        {
            FoundFunction = ClassToSearch->FindFunctionByName(FName(*Local_FunctionName));
        }
        if (!FoundFunction && ClassToSearch)
        {
            FoundFunction = ClassToSearch->FindFunctionByName(FName(*Local_FunctionName), EIncludeSuperFlag::IncludeSuper);
        }
        if (FoundFunction)
        {
            UE_LOG(LogBpGenCompiler, Log, TEXT("  -> SUCCESS: Found local/native function '%s'. Creating node."), *Local_FunctionName);
            UK2Node_CallFunction* CallFuncNode = NewObject<UK2Node_CallFunction>(FunctionGraph);
            CallFuncNode->CreateNewGuid();
            FunctionGraph->AddNode(CallFuncNode, true, false);
            CallFuncNode->SetFromFunction(FoundFunction);
            CallFuncNode->PostPlacedNewNode();
            CallFuncNode->AllocateDefaultPins();
            PlaceNode(CallFuncNode, InOutLastExecPin);
            if (UEdGraphPin* NodeExecIn = CallFuncNode->GetExecPin())
            {
                if (InOutLastExecPin && IsValid(InOutLastExecPin->GetOwningNodeUnchecked()))
                {
                    InOutLastExecPin->GetSchema()->TryCreateConnection(InOutLastExecPin, NodeExecIn);
                }
                InOutLastExecPin = CallFuncNode->GetThenPin();
            }
            int32 Local_LastParen = CleanedExpression.Find(TEXT(")"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
            FString Local_ParamsStr = CleanedExpression.Mid(Local_ParenStart + 1, Local_LastParen - Local_ParenStart - 1);
            TArray<FString> Local_Params = SplitParameters(Local_ParamsStr);
            TArray<FName> Local_FunctionParamNames;
            for (TFieldIterator<FProperty> PropIt(FoundFunction); PropIt; ++PropIt)
            {
                if (PropIt->HasAnyPropertyFlags(CPF_Parm) && !PropIt->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm))
                {
                    Local_FunctionParamNames.Add(PropIt->GetFName());
                }
            }
            for (int32 i = 0; i < Local_Params.Num(); ++i)
            {
                if (Local_FunctionParamNames.IsValidIndex(i))
                {
                    FName PinName = Local_FunctionParamNames[i];
                    UEdGraphPin* DestPin = CallFuncNode->FindPin(PinName);
                    if (DestPin && !DestPin->bHidden)
                    {
                        FString ParamExpression = Local_Params[i].TrimStartAndEnd();
                        UEdGraphPin* SourcePin = ProcessExpression(ParamExpression, InOutLastExecPin);
                        if (SourcePin)
                        {
                            DestPin->GetSchema()->TryCreateConnection(SourcePin, DestPin);
                        }
                        else
                        {
                            SetPinDefaultValue(DestPin, ParamExpression);
                        }
                    }
                }
            }
            return CallFuncNode->GetReturnValuePin();
        }
    }
    return nullptr;
}
int32 FBpGeneratorCompiler::CalculateLevenshteinDistance(const FString& S1, const FString& S2)
{
    const int32 Len1 = S1.Len();
    const int32 Len2 = S2.Len();
    TArray<TArray<int32>> DP;
    DP.AddDefaulted(Len1 + 1);
    for (int32 i = 0; i <= Len1; ++i)
    {
        DP[i].AddDefaulted(Len2 + 1);
    }
    for (int32 i = 0; i <= Len1; ++i)
    {
        DP[i][0] = i;
    }
    for (int32 j = 0; j <= Len2; ++j)
    {
        DP[0][j] = j;
    }
    for (int32 i = 1; i <= Len1; ++i)
    {
        for (int32 j = 1; j <= Len2; ++j)
        {
            if (S1[i - 1] == S2[j - 1])
            {
                DP[i][j] = DP[i - 1][j - 1];
            }
            else
            {
                int32 CostReplace = DP[i - 1][j - 1];
                int32 CostDelete = DP[i - 1][j];
                int32 CostInsert = DP[i][j - 1];
                DP[i][j] = 1 + FMath::Min(FMath::Min(CostReplace, CostDelete), CostInsert);
            }
        }
    }
    return DP[Len1][Len2];
}
void FBpGeneratorCompiler::PlaceNode(UEdGraphNode* Node, UEdGraphPin* LastExecPin, int32 XOffset, int32 YOffset)
{
    if (!Node) return;
    bool bIsPure = true;
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
        {
            bIsPure = false;
            break;
        }
    }
    if (XOffset != 0 || YOffset != 0)
    {
        Node->NodePosX = CurrentNodeX + XOffset;
        Node->NodePosY = CurrentBaseY + YOffset;
    }
    else if (bIsPure)
    {
        Node->NodePosX = CurrentNodeX - 300;
        Node->NodePosY = CurrentBaseY + CurrentPureNodeYOffset;
        CurrentPureNodeYOffset += 130;
    }
    else
    {
        Node->NodePosX = CurrentNodeX;
        Node->NodePosY = CurrentBaseY;
        CurrentNodeX += 450;
        CurrentPureNodeYOffset = 80;
    }
    if (!CurrentCommentText.IsEmpty())
    {
        CommentMap.FindOrAdd(CurrentCommentText).Add(Node);
    }
}
void FBpGeneratorCompiler::ProcessReturnStatement(const FString& ReturnExpression, UEdGraphPin*& InOutLastExecPin)
{
    UE_LOG(LogBpGenCompiler, Log, TEXT("ProcessReturnStatement: Processing expression: '%s'"), *ReturnExpression);
    UEdGraphPin* SourcePin = ProcessExpression(ReturnExpression, InOutLastExecPin);
    if (SourcePin)
    {
        UE_LOG(LogBpGenCompiler, Log, TEXT("ProcessReturnStatement: Successfully got a source pin. Connecting to Return Node."));
        PlaceNode(SourcePin->GetOwningNode(), InOutLastExecPin);
        if (UEdGraphPin* DestPin = FindPinByName(OutputPins, "ReturnValue"))
        {
            DestPin->GetSchema()->TryCreateConnection(SourcePin, DestPin);
        }
    }
    else
    {
        UE_LOG(LogBpGenCompiler, Error, TEXT("ProcessReturnStatement: FAILED to get a source pin from expression: '%s'"), *ReturnExpression);
    }
    if (InOutLastExecPin && IsValid(InOutLastExecPin->GetOwningNodeUnchecked()) && ResultNode)
    {
        if(InOutLastExecPin->GetOwningNode() != ResultNode)
        {
            InOutLastExecPin->GetSchema()->TryCreateConnection(InOutLastExecPin, ResultNode->GetExecPin());
        }
    }
}
int32 FBpGeneratorCompiler::FindMatchingClosingParen(const FString& Haystack, int32 StartIndex)
{
    if (StartIndex >= Haystack.Len() || Haystack[StartIndex] != '(') return INDEX_NONE;
    int32 NestingLevel = 1;
    for (int32 i = StartIndex + 1; i < Haystack.Len(); ++i)
    {
        if (Haystack[i] == '(') NestingLevel++;
        else if (Haystack[i] == ')')
        {
            NestingLevel--;
            if (NestingLevel == 0) return i;
        }
    }
    return INDEX_NONE;
}
int32 FBpGeneratorCompiler::FindMatchingClosingBrace(const FString& Haystack, int32 StartIndex)
{
    if (StartIndex >= Haystack.Len() || Haystack[StartIndex] != '{') return INDEX_NONE;
    int32 NestingLevel = 1;
    for (int32 i = StartIndex + 1; i < Haystack.Len(); ++i)
    {
        if (Haystack[i] == '{') NestingLevel++;
        else if (Haystack[i] == '}')
        {
            NestingLevel--;
            if (NestingLevel == 0) return i;
        }
    }
    return INDEX_NONE;
}
UEdGraph* FBpGeneratorCompiler::FindMacroGraphByName(UBlueprint* MacroLibrary, FName MacroName)
{
    if (!MacroLibrary) return nullptr;
    for (UEdGraph* MacroGraph : MacroLibrary->MacroGraphs)
    {
        if (MacroGraph && MacroGraph->GetFName() == MacroName)
        {
            return MacroGraph;
        }
    }
    UE_LOG(LogBpGenCompiler, Error, TEXT("Could not find Macro Graph '%s' in library '%s'"), *MacroName.ToString(), *MacroLibrary->GetName());
    return nullptr;
}
void FBpGeneratorCompiler::ProcessScopedCodeBlock(const TArray<FString>& BlockLines, UEdGraphPin*& InOutLastExecPin, bool bInheritCommentContext)
{
    for (int32 i = 0; i < BlockLines.Num(); ++i)
    {
        FString TrimmedLine = BlockLines[i].TrimStartAndEnd();
        if (TrimmedLine.IsEmpty()) continue;
        if (TrimmedLine.StartsWith(TEXT("//")))
        {
            if (!bInheritCommentContext) { CurrentCommentText = TrimmedLine.RightChop(2).TrimStartAndEnd(); }
            continue;
        }
        if (!TrimmedLine.Contains(TEXT("{")) && !TrimmedLine.Contains(TEXT("}")))
        {
            while (!TrimmedLine.Contains(TEXT(";")) && i + 1 < BlockLines.Num())
            {
                i++;
                TrimmedLine += TEXT(" ") + BlockLines[i].TrimStartAndEnd();
            }
        }
        TrimmedLine.ReplaceInline(TEXT("\n"), TEXT(" "), ESearchCase::CaseSensitive);
        TrimmedLine.ReplaceInline(TEXT("\r"), TEXT(" "), ESearchCase::CaseSensitive);
        while (TrimmedLine.Contains(TEXT("  ")))
        {
            TrimmedLine.ReplaceInline(TEXT("  "), TEXT(" "), ESearchCase::CaseSensitive);
        }
        if (TrimmedLine.StartsWith(TEXT("sequence"))) { ProcessSequenceStatement(BlockLines, i, InOutLastExecPin); continue; }
        if (TrimmedLine.EndsWith(TEXT(";"))) TrimmedLine.LeftChopInline(1);
        if (TrimmedLine.StartsWith(TEXT("TArray<AActor*> ActorsToIgnore")) ||
            TrimmedLine.StartsWith(TEXT("FHitResult OutHit")))
        {
            continue;
        }
        const FRegexPattern SpawnPattern(TEXT("([A-Za-z0-9_]+\\*?)\\s*([A-Za-z0-9_]+)\\s*=\\s*SpawnActor<([A-Za-z0-9_]+)>\\((.*)\\)"));
        FRegexMatcher SpawnMatcher(SpawnPattern, TrimmedLine);
        if (SpawnMatcher.FindNext())
        {
            FString NewVarName = SpawnMatcher.GetCaptureGroup(2);
            FString ClassName = SpawnMatcher.GetCaptureGroup(3);
            FString ParamsStr = SpawnMatcher.GetCaptureGroup(4);
            TArray<FString> Params = SplitParameters(ParamsStr);
            UE_LOG(LogBpGenCompiler, Warning, TEXT("MANUAL EXPANSION: Matched spawn pattern for Class: '%s'"), *ClassName);
            UFunction* BeginSpawnFunc = UGameplayStatics::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameplayStatics, BeginDeferredActorSpawnFromClass));
            UK2Node_CallFunction* BeginSpawnNode = NewObject<UK2Node_CallFunction>(FunctionGraph);
            BeginSpawnNode->SetFromFunction(BeginSpawnFunc);
            BeginSpawnNode->CreateNewGuid();
            FunctionGraph->AddNode(BeginSpawnNode, true, false);
            BeginSpawnNode->PostPlacedNewNode();
            BeginSpawnNode->AllocateDefaultPins();
            PlaceNode(BeginSpawnNode, InOutLastExecPin);
            if (InOutLastExecPin)
            {
                InOutLastExecPin->GetSchema()->TryCreateConnection(InOutLastExecPin, BeginSpawnNode->GetExecPin());
            }
            UClass* ClassToSpawn = FindBlueprintClassByName(ClassName);
            if (ClassToSpawn)
            {
                BeginSpawnNode->FindPin(TEXT("ActorClass"))->DefaultObject = ClassToSpawn;
            }
            if (Params.Num() > 0)
            {
                FString LocationVar = Params[0];
                FString RotationVar = (Params.Num() > 1) ? Params[1] : TEXT("");
                UEdGraphPin* LocPin = ProcessExpression(LocationVar, InOutLastExecPin);
                UEdGraphPin* RotPin = ProcessExpression(RotationVar, InOutLastExecPin);
                UFunction* MakeTransformFunc = UKismetMathLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeTransform));
                UK2Node_CallFunction* MakeTransformNode = NewObject<UK2Node_CallFunction>(FunctionGraph);
                MakeTransformNode->SetFromFunction(MakeTransformFunc);
                MakeTransformNode->CreateNewGuid();
                FunctionGraph->AddNode(MakeTransformNode, true, false);
                MakeTransformNode->PostPlacedNewNode();
                MakeTransformNode->AllocateDefaultPins();
                PlaceNode(MakeTransformNode, InOutLastExecPin);
                if (LocPin) MakeTransformNode->FindPin(TEXT("Location"))->GetSchema()->TryCreateConnection(LocPin, MakeTransformNode->FindPin(TEXT("Location")));
                if (RotPin) MakeTransformNode->FindPin(TEXT("Rotation"))->GetSchema()->TryCreateConnection(RotPin, MakeTransformNode->FindPin(TEXT("Rotation")));
                UEdGraphPin* MakeTransformResultPin = MakeTransformNode->GetReturnValuePin();
                if (MakeTransformResultPin)
                {
                    BeginSpawnNode->FindPin(TEXT("SpawnTransform"))->GetSchema()->TryCreateConnection(MakeTransformResultPin, BeginSpawnNode->FindPin(TEXT("SpawnTransform")));
                }
            }
            UFunction* FinishSpawnFunc = UGameplayStatics::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameplayStatics, FinishSpawningActor));
            UK2Node_CallFunction* FinishSpawnNode = NewObject<UK2Node_CallFunction>(FunctionGraph);
            FinishSpawnNode->SetFromFunction(FinishSpawnFunc);
            FinishSpawnNode->CreateNewGuid();
            FunctionGraph->AddNode(FinishSpawnNode, true, false);
            FinishSpawnNode->PostPlacedNewNode();
            FinishSpawnNode->AllocateDefaultPins();
            PlaceNode(FinishSpawnNode, BeginSpawnNode->GetThenPin());
            if (UEdGraphPin* ThenPin = BeginSpawnNode->GetThenPin())
            {
                ThenPin->GetSchema()->TryCreateConnection(ThenPin, FinishSpawnNode->GetExecPin());
            }
            if (UEdGraphPin* ReturnPin = BeginSpawnNode->GetReturnValuePin())
            {
                ReturnPin->GetSchema()->TryCreateConnection(ReturnPin, FinishSpawnNode->FindPin(TEXT("Actor")));
            }
            UEdGraphPin* BeginTransformPin = BeginSpawnNode->FindPin(TEXT("SpawnTransform"));
            UEdGraphPin* FinishTransformPin = FinishSpawnNode->FindPin(TEXT("SpawnTransform"));
            if (BeginTransformPin && FinishTransformPin)
            {
                if (BeginTransformPin->LinkedTo.Num() > 0)
                {
                    FinishTransformPin->GetSchema()->TryCreateConnection(BeginTransformPin->LinkedTo[0], FinishTransformPin);
                }
            }
            InOutLastExecPin = FinishSpawnNode->GetThenPin();
            if (!NewVarName.IsEmpty())
            {
                PinMap.Add(NewVarName, FinishSpawnNode->GetReturnValuePin());
            }
            continue;
        }
        if (TrimmedLine.StartsWith(TEXT("return "))) { ProcessReturnStatement(TrimmedLine.Mid(7), InOutLastExecPin); }
        else if (TrimmedLine.StartsWith(TEXT("switch"))) { ProcessSwitchStatement(BlockLines, i, InOutLastExecPin); return; }
        else if (TrimmedLine.StartsWith(TEXT("if"))) { ProcessIfStatement(BlockLines, i, InOutLastExecPin); continue; }
        else if (TrimmedLine.StartsWith(TEXT("for")))
        {
            ProcessForLoop(BlockLines, i, InOutLastExecPin);
            continue;
        }
        else if (TrimmedLine.StartsWith(TEXT("while")))
        {
            ProcessWhileLoop(BlockLines, i, InOutLastExecPin);
            continue;
        }
        else if (TrimmedLine.Contains(TEXT("=")))
        {
            FString Lhs, Rhs;
            if (TrimmedLine.Split(TEXT("="), &Lhs, &Rhs))
            {
                Lhs = Lhs.TrimStartAndEnd();
                Rhs = Rhs.TrimStartAndEnd();
                const bool bIsDeclaration = Lhs.Contains(TEXT(" "));
                if (bIsDeclaration)
                {
                    FString TypeString, VarName;
                    int32 LastSpace = -1;
                    Lhs.FindLastChar(' ', LastSpace);
                    TypeString = Lhs.Left(LastSpace).TrimStartAndEnd();
                    VarName = Lhs.RightChop(LastSpace + 1).TrimStartAndEnd();
                    UEdGraphPin* ResultPin = ProcessExpression(Rhs, InOutLastExecPin);
                    if (ResultPin)
                    {
                        UE_LOG(LogBpGenCompiler, Log, TEXT("Declaration is a temporary local. Mapping '%s' to the output of node '%s'."), *VarName, *ResultPin->GetOwningNode()->GetName());
                        PinMap.Add(VarName, ResultPin);
                    }
                    else
                    {
                        UE_LOG(LogBpGenCompiler, Log, TEXT("Declaration is a persistent variable. Creating/finding '%s' and creating a SET node."), *VarName);
                        UClass* ClassToSearch = TargetBlueprint->SkeletonGeneratedClass ? TargetBlueprint->SkeletonGeneratedClass : TargetBlueprint->GeneratedClass;
                        if (ClassToSearch && !FindFProperty<FProperty>(ClassToSearch, FName(*VarName)))
                        {
                            FEdGraphPinType PinType;
                            if (ConvertCppTypeToPinType(TypeString, PinType))
                            {
                                UE_LOG(LogBpGenCompiler, Warning, TEXT("Variable '%s' not found. Creating it now."), *VarName);
                                FBlueprintEditorUtils::AddMemberVariable(TargetBlueprint, FName(*VarName), PinType);
                                FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(TargetBlueprint);
                            }
                        }
                        ProcessAssignmentStatement(VarName, Rhs, InOutLastExecPin);
                    }
                }
                else
                {
                    ProcessAssignmentStatement(Lhs, Rhs, InOutLastExecPin);
                }
            }
        }
        else if (TrimmedLine.Contains(TEXT(" ")) && !TrimmedLine.Contains(TEXT("(")))
        {
            FString TypeString, VarName;
            int32 LastSpace = -1;
            TrimmedLine.FindLastChar(' ', LastSpace);
            TypeString = TrimmedLine.Left(LastSpace).TrimStartAndEnd();
            VarName = TrimmedLine.RightChop(LastSpace + 1).TrimStartAndEnd();
            UClass* ClassToSearch = TargetBlueprint->SkeletonGeneratedClass ? TargetBlueprint->SkeletonGeneratedClass : TargetBlueprint->GeneratedClass;
            if (ClassToSearch && !FindFProperty<FProperty>(ClassToSearch, FName(*VarName)))
            {
                FEdGraphPinType PinType;
                if (ConvertCppTypeToPinType(TypeString, PinType))
                {
                    if (FBlueprintEditorUtils::AddMemberVariable(TargetBlueprint, FName(*VarName), PinType))
                    {
                        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(TargetBlueprint);
                        ProcessAssignmentStatement(VarName, TEXT(""), InOutLastExecPin);
                    }
                }
            }
        }
        else
        {
            ProcessExpression(TrimmedLine, InOutLastExecPin);
        }
    }
}
void FBpGeneratorCompiler::ProcessTopLevelTimeline(const FString& TimelineCode)
{
    TArray<FString> AllLines;
    TimelineCode.ParseIntoArrayLines(AllLines, true);
    if (AllLines.Num() == 0) return;
    int32 DummyLineIndex = 0;
    UEdGraphPin* DummyExecPin = nullptr;
    ProcessTimelineDeclaration(AllLines, DummyLineIndex, DummyExecPin);
}
void FBpGeneratorCompiler::ProcessWhileLoop(const TArray<FString>& AllLines, int32& InOutLineIndex, UEdGraphPin*& InOutLastExecPin)
{
    UE_LOG(LogBpGenCompiler, Log, TEXT("--- Processing While Loop ---"));
    UE_LOG(LogBpGenCompiler, Log, TEXT("  -> InOutLastExecPin on entry is from node: '%s'"), InOutLastExecPin ? *InOutLastExecPin->GetOwningNode()->GetName() : TEXT("NULL"));
    FString WhileLine = AllLines[InOutLineIndex];
    UBlueprint* StandardMacros = LoadObject<UBlueprint>(nullptr, TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros"));
    if (!StandardMacros) return;
    UEdGraph* WhileLoopGraph = FindMacroGraphByName(StandardMacros, FName(TEXT("WhileLoop")));
    if (!WhileLoopGraph) return;
    UK2Node_MacroInstance* WhileLoopNode = NewObject<UK2Node_MacroInstance>(FunctionGraph);
    WhileLoopNode->SetMacroGraph(WhileLoopGraph);
    WhileLoopNode->CreateNewGuid();
    FunctionGraph->AddNode(WhileLoopNode, true, false);
    WhileLoopNode->PostPlacedNewNode();
    WhileLoopNode->AllocateDefaultPins();
    WhileLoopNode->ReconstructNode();
    PlaceNode(WhileLoopNode, InOutLastExecPin);
    if (InOutLastExecPin)
    {
        if (UEdGraphPin* LoopExecIn = WhileLoopNode->FindPin(TEXT("execute")))
        {
            if (InOutLastExecPin->GetSchema()->TryCreateConnection(InOutLastExecPin, LoopExecIn))
            {
                UE_LOG(LogBpGenCompiler, Log, TEXT("  -> SUCCESS: Connected exec chain to WhileLoop 'execute' pin."));
            }
        }
        else
        {
            UE_LOG(LogBpGenCompiler, Error, TEXT("  -> FAILED: Could not find 'execute' pin on WhileLoopNode. This should not happen."));
        }
    }
    int32 ConditionStart = WhileLine.Find(TEXT("("));
    if (ConditionStart == -1) return;
    int32 ConditionEnd = FindMatchingClosingParen(WhileLine, ConditionStart);
    if (ConditionEnd == -1) return;
    FString ConditionString = WhileLine.Mid(ConditionStart + 1, ConditionEnd - ConditionStart - 1);
    UEdGraphPin* ConditionResultPin = ProcessExpression(ConditionString, InOutLastExecPin);
    if (ConditionResultPin)
    {
        if (UEdGraphPin* ConditionInputPin = WhileLoopNode->FindPin(TEXT("Condition")))
        {
            ConditionInputPin->GetSchema()->TryCreateConnection(ConditionResultPin, ConditionInputPin);
            UE_LOG(LogBpGenCompiler, Log, TEXT("  -> SUCCESS: Connected condition pin."));
        }
    }
    int32 BodyStartLine = -1;
    for (int32 i = InOutLineIndex; i < AllLines.Num(); ++i) { if (AllLines[i].Contains(TEXT("{"))) { BodyStartLine = i; break; } }
    if (BodyStartLine == -1) return;
    FString FullBodyBlock;
    for (int32 i = BodyStartLine; i < AllLines.Num(); ++i) FullBodyBlock += AllLines[i] + TEXT("\n");
    int32 BraceStart = FullBodyBlock.Find(TEXT("{"));
    int32 BraceEnd = FindMatchingClosingBrace(FullBodyBlock, BraceStart);
    if (BraceEnd == -1) return;
    FString LoopBodyContent = FullBodyBlock.Mid(BraceStart + 1, BraceEnd - BraceStart - 1);
    TArray<FString> LoopBodyLines;
    LoopBodyContent.ParseIntoArrayLines(LoopBodyLines, true);
    UEdGraphPin* LoopBodyExecPin = WhileLoopNode->FindPin(TEXT("LoopBody"));
    if (LoopBodyExecPin && LoopBodyLines.Num() > 0)
    {
        UE_LOG(LogBpGenCompiler, Log, TEXT("  -> SUCCESS: Found 'LoopBody' pin. Processing body logic..."));
        ProcessScopedCodeBlock(LoopBodyLines, LoopBodyExecPin, true);
    }
    else
    {
        UE_LOG(LogBpGenCompiler, Error, TEXT("  -> FAILED: Could not find the 'LoopBody' output pin OR the body was empty."));
    }
    if (UEdGraphPin* CompletedPin = WhileLoopNode->FindPin(TEXT("Completed")))
    {
        InOutLastExecPin = CompletedPin;
    }
    int32 NestingLevel = 0;
    int32 FinalLine = InOutLineIndex;
    for (int32 i = BodyStartLine; i < AllLines.Num(); ++i)
    {
        for (const TCHAR& Char : AllLines[i]) { if (Char == '{') NestingLevel++; else if (Char == '}') NestingLevel--; }
        if (NestingLevel == 0) { FinalLine = i; break; }
    }
    InOutLineIndex = FinalLine;
}
void FBpGeneratorCompiler::ProcessTimelineDeclaration(const TArray<FString>& AllLines, int32& InOutLineIndex, UEdGraphPin*& InOutLastExecPin)
{
    FString CurrentLine = AllLines[InOutLineIndex].TrimStartAndEnd();
    int32 NameStart = CurrentLine.Find(TEXT(" ")) + 1;
    int32 NameEnd = CurrentLine.Find(TEXT("("));
    if (NameStart >= NameEnd) return;
    FString TimelineName = CurrentLine.Mid(NameStart, NameEnd - NameStart).TrimStartAndEnd();
    if (TimelineName.IsEmpty()) return;
    UE_LOG(LogBpGenCompiler, Warning, TEXT(">>>> Processing Timeline Declaration for '%s'"), *TimelineName);
    FBlueprintEditorUtils::AddNewTimeline(TargetBlueprint, FName(*TimelineName));
    UTimelineTemplate* TimelineTemplate = nullptr;
    const int32 TimelineIndex = FBlueprintEditorUtils::FindTimelineIndex(TargetBlueprint, FName(*TimelineName));
    if (TargetBlueprint->Timelines.IsValidIndex(TimelineIndex)) { TimelineTemplate = TargetBlueprint->Timelines[TimelineIndex]; }
    if (!TimelineTemplate) { return; }
    UK2Node_Timeline* TimelineNode = NewObject<UK2Node_Timeline>(FunctionGraph);
    TimelineNode->CreateNewGuid();
    TimelineNode->PostPlacedNewNode();
    TimelineNode->AllocateDefaultPins();
    TimelineNode->TimelineName = FName(*TimelineName);
    FunctionGraph->AddNode(TimelineNode, true, false);
    TimelineMap.Add(TimelineName, TimelineNode);
    TimelineTemplate->Modify();
    TimelineTemplate->PreEditChange(nullptr);
    int32 BodyStartIndex = InOutLineIndex;
    int32 NestingLevel = 0;
    int32 BodyEndIndex = -1;
    for (int32 j = InOutLineIndex; j < AllLines.Num(); ++j) {
        for (const TCHAR& Char : AllLines[j]) { if (Char == '{') NestingLevel++; else if (Char == '}') NestingLevel--; }
        if (NestingLevel == 0) { BodyEndIndex = j; break; }
    }
    if (BodyEndIndex == -1) return;
    float MaxKeyTime = 0.0f;
    FTimelineLogicBlock LogicBlock;
    LogicBlock.TimelineNode = TimelineNode;
    for (int32 j = BodyStartIndex + 1; j < BodyEndIndex; ++j) {
        FString BodyLine = AllLines[j].TrimStartAndEnd();
        if (BodyLine.IsEmpty()) continue;
        if (BodyLine.StartsWith(TEXT("float "))) {
            FString TrackNameStr = BodyLine.Mid(6).TrimEnd();
            TrackNameStr.RemoveFromEnd(TEXT(";"));
            FTTFloatTrack NewTrack;
            NewTrack.SetTrackName(FName(*TrackNameStr), TimelineTemplate);
            NewTrack.CurveFloat = NewObject<UCurveFloat>(TimelineTemplate);
            TimelineTemplate->FloatTracks.Add(NewTrack);
        }
        else if (BodyLine.Contains(TEXT(".AddKey("))) {
            FString TrackNameStr, ParamsStr;
            BodyLine.Split(TEXT("."), &TrackNameStr, &ParamsStr);
            FName TrackName = FName(*TrackNameStr);
            FString TimeStr, ValueStr;
            ParamsStr.Mid(ParamsStr.Find(TEXT("(")) + 1, ParamsStr.Find(TEXT(")")) - ParamsStr.Find(TEXT("(")) - 1).Split(TEXT(","), &TimeStr, &ValueStr);
            float Time = FCString::Atof(*TimeStr.TrimStartAndEnd());
            float Value = FCString::Atof(*ValueStr.TrimStartAndEnd());
            MaxKeyTime = FMath::Max(MaxKeyTime, Time);
            for (FTTFloatTrack& Track : TimelineTemplate->FloatTracks) {
                if (Track.GetTrackName() == TrackName && Track.CurveFloat) {
                    Track.CurveFloat->FloatCurve.AddKey(Time, Value);
                    break;
                }
            }
        }
        else if (BodyLine.Contains(TEXT("="))) {
            FString Lhs, Rhs;
            if (BodyLine.Split(TEXT("="), &Lhs, &Rhs)) {
                FString VarName = Lhs.TrimStartAndEnd().RightChop(Lhs.TrimStartAndEnd().Find(TEXT(" ")) + 1);
                Rhs.RemoveFromEnd(TEXT(";"));
                UEdGraphPin* AnchorPin = TimelineNode->GetUpdatePin();
                UEdGraphPin* ValuePin = ProcessExpression(Rhs, AnchorPin);
                if (ValuePin) { LogicBlock.LocalPinMap.Add(VarName, ValuePin); }
            }
        }
        else if (BodyLine.StartsWith(TEXT("OnUpdate()"))) {
            int32 BlockNesting = 0;
            for (int32 k = j; k < BodyEndIndex; ++k) {
                const FString& InnerLine = AllLines[k];
                if (InnerLine.Contains(TEXT("{"))) BlockNesting++;
                if (InnerLine.Contains(TEXT("}"))) BlockNesting--;
                if (BlockNesting > 0 && !InnerLine.Contains(TEXT("OnUpdate"))) { LogicBlock.OnUpdateCode.Add(InnerLine); }
                if (BlockNesting == 0) { j = k; break; }
            }
        }
        else if (BodyLine.StartsWith(TEXT("OnFinished()"))) {
            int32 BlockNesting = 0;
            for (int32 k = j; k < BodyEndIndex; ++k) {
                const FString& InnerLine = AllLines[k];
                if (InnerLine.Contains(TEXT("{"))) BlockNesting++;
                if (InnerLine.Contains(TEXT("}"))) BlockNesting--;
                if (BlockNesting > 0 && !InnerLine.Contains(TEXT("OnFinished"))) { LogicBlock.OnFinishedCode.Add(InnerLine); }
                if (BlockNesting == 0) { j = k; break; }
            }
        }
    }
    TimelineTemplate->TimelineLength = MaxKeyTime;
    TimelineTemplate->PostEditChange();
    PendingTimelineLogic.Add(TimelineName, LogicBlock);
    TimelineNode->ReconstructNode();
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(TargetBlueprint);
    FKismetEditorUtilities::CompileBlueprint(TargetBlueprint);
    FBlueprintEditorUtils::RefreshAllNodes(TargetBlueprint);
    InOutLineIndex = BodyEndIndex;
}
UEdGraphPin* FBpGeneratorCompiler::FindOrCreateKeyEventNode(FName KeyName, bool bForReleasedEvent)
{
    TArray<UK2Node_InputKey*> EventNodes;
    FunctionGraph->GetNodesOfClass<UK2Node_InputKey>(EventNodes);
    for (UK2Node_InputKey* Node : EventNodes)
    {
        if (Node && Node->InputKey.GetFName() == KeyName)
        {
            UE_LOG(LogBpGenCompiler, Log, TEXT("Found existing InputKey node for key '%s'"), *KeyName.ToString());
            return bForReleasedEvent ? Node->GetReleasedPin() : Node->GetPressedPin();
        }
    }
    UE_LOG(LogBpGenCompiler, Log, TEXT("Creating new InputKey node for key '%s'"), *KeyName.ToString());
    UK2Node_InputKey* EventNode = NewObject<UK2Node_InputKey>(FunctionGraph);
    EventNode->InputKey = FKey(KeyName);
    FunctionGraph->AddNode(EventNode, true, false);
    EventNode->CreateNewGuid();
    EventNode->PostPlacedNewNode();
    EventNode->AllocateDefaultPins();
    const int32 NewNodeY = FindFreeYPositionForEventNode();
    EventNode->NodePosX = -600;
    EventNode->NodePosY = NewNodeY;
    return bForReleasedEvent ? EventNode->GetReleasedPin() : EventNode->GetPressedPin();
}
bool FBpGeneratorCompiler::ParseFunctionSignature(const FString& InCode)
{
    CodeLines.Empty();
    ParsedInputs.Empty();
    bIsEventGraphLogic = bIsComponentEvent = bIsCustomEvent = bIsWidgetEvent = bIsKeyEvent = bIsKeyReleasedEvent = false;
    ParsedFunctionName = ParsedReturnType = ParsedComponentName = ParsedWidgetName = ParsedKeyName = TEXT("");
    TArray<FString> AllLines;
    InCode.ParseIntoArrayLines(AllLines, true);
    FString SignatureLine;
    for (const FString& Line : AllLines)
    {
        FString TrimmedLine = Line.TrimStartAndEnd();
        if (!TrimmedLine.IsEmpty() && !TrimmedLine.StartsWith(TEXT("//")))
        {
            SignatureLine = TrimmedLine;
            break;
        }
    }
    int32 BodyStart = InCode.Find(TEXT("{"));
    if (SignatureLine.IsEmpty() || BodyStart == -1)
    {
        UE_LOG(LogBpGenCompiler, Error, TEXT("Robust Parser: Could not find a valid signature line or a starting '{' in the code block."));
        return false;
    }
    int32 BodyEnd = FindMatchingClosingBrace(InCode, BodyStart);
    if (BodyEnd == -1) return false;
    if (SignatureLine.Contains(TEXT("{")))
    {
        SignatureLine = SignatureLine.Left(SignatureLine.Find(TEXT("{"))).TrimStartAndEnd();
    }
    InCode.Mid(BodyStart + 1, BodyEnd - BodyStart - 1).ParseIntoArrayLines(CodeLines, true);
    int32 ParenPos = SignatureLine.Find(TEXT("("));
    if (ParenPos == INDEX_NONE) return false;
    FString DeclarationPart = SignatureLine.Left(ParenPos).TrimStartAndEnd();
    const FString PressedPrefix = TEXT("void OnKeyPressed_");
    const FString ReleasedPrefix = TEXT("void OnKeyReleased_");
    if (DeclarationPart.Contains(TEXT("::")))
    {
        if (Cast<UWidgetBlueprint>(TargetBlueprint))
        {
            bIsWidgetEvent = true;
            DeclarationPart.Split(TEXT("::"), &ParsedWidgetName, &ParsedFunctionName);
            ParsedReturnType = TEXT("void");
        }
        else
        {
            bIsComponentEvent = true;
            FString ComponentAndEvent;
            DeclarationPart.Split(TEXT(" "), &ParsedReturnType, &ComponentAndEvent, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
            ComponentAndEvent.Split(TEXT("::"), &ParsedComponentName, &ParsedFunctionName);
        }
    }
    else if (DeclarationPart.StartsWith(PressedPrefix))
    {
        bIsKeyEvent = true;
        bIsKeyReleasedEvent = false;
        ParsedKeyName = DeclarationPart.RightChop(PressedPrefix.Len());
        ParsedFunctionName = DeclarationPart;
        ParsedReturnType = TEXT("void");
    }
    else if (DeclarationPart.StartsWith(ReleasedPrefix))
    {
        bIsKeyEvent = true;
        bIsKeyReleasedEvent = true;
        ParsedKeyName = DeclarationPart.RightChop(ReleasedPrefix.Len());
        ParsedFunctionName = DeclarationPart;
        ParsedReturnType = TEXT("void");
    }
    else if (DeclarationPart.StartsWith(TEXT("event")))
    {
        bIsCustomEvent = true;
        ParsedReturnType = TEXT("void");
        ParsedFunctionName = DeclarationPart.RightChop(5).TrimStart();
    }
    else
    {
        int32 LastSpace = -1;
        DeclarationPart.FindLastChar(' ', LastSpace);
        if (LastSpace != -1)
        {
            ParsedReturnType = DeclarationPart.Left(LastSpace).TrimStartAndEnd();
            ParsedFunctionName = DeclarationPart.RightChop(LastSpace + 1).TrimStartAndEnd();
        }
        else
        {
            ParsedReturnType = TEXT("void");
            ParsedFunctionName = DeclarationPart;
        }
        if (EventNameMap.Contains(ParsedFunctionName))
        {
            bIsEventGraphLogic = true;
        }
    }
    ParsedFunctionName = ParsedFunctionName.TrimStartAndEnd();
    ParsedReturnType = ParsedReturnType.TrimStartAndEnd();
    ParsedComponentName = ParsedComponentName.TrimStartAndEnd();
    ParsedWidgetName = ParsedWidgetName.TrimStartAndEnd();
    if (ParsedFunctionName.StartsWith(TEXT("UFUNCTION")) ||
        ParsedFunctionName.StartsWith(TEXT("UPROPERTY")) ||
        ParsedFunctionName.StartsWith(TEXT("USTRUCT")) ||
        ParsedFunctionName.StartsWith(TEXT("UCLASS")) ||
        ParsedFunctionName.StartsWith(TEXT("UENUM")) ||
        ParsedFunctionName.StartsWith(TEXT("DECLARE_DELEGATE")) ||
        ParsedFunctionName.StartsWith(TEXT("DECLARE_MULTICAST_DELEGATE")) ||
        ParsedFunctionName.StartsWith(TEXT("DECLARE_DYNAMIC_DELEGATE")) ||
        ParsedFunctionName.StartsWith(TEXT("DECLARE_DYNAMIC_MULTICAST")) ||
        ParsedFunctionName.Contains(TEXT("Category =")) ||
        ParsedFunctionName.Contains(TEXT("BlueprintCallable")) ||
        ParsedFunctionName.Contains(TEXT("BlueprintPure")) ||
        ParsedFunctionName == TEXT("virtual") ||
        ParsedFunctionName == TEXT("override") ||
        ParsedFunctionName == TEXT("const") ||
        ParsedFunctionName.StartsWith(TEXT("//")) ||
        ParsedFunctionName.StartsWith(TEXT("/*")) ||
        ParsedFunctionName.StartsWith(TEXT("*")) ||
        ParsedFunctionName.StartsWith(TEXT("public:")) ||
        ParsedFunctionName.StartsWith(TEXT("private:")) ||
        ParsedFunctionName.StartsWith(TEXT("protected:")))
    {
        UE_LOG(LogBpGenCompiler, Warning, TEXT("Robust Parser: Rejecting parsed function name '%s' - appears to be a C++ macro or specifier"), *ParsedFunctionName);
        return false;
    }
    int32 ParenEnd = SignatureLine.Find(TEXT(")"), ESearchCase::IgnoreCase, ESearchDir::FromStart, ParenPos);
    if (ParenEnd == -1) return false;
    FString ParamsStr = SignatureLine.Mid(ParenPos + 1, ParenEnd - ParenPos - 1);
    if (!ParamsStr.IsEmpty())
    {
        TArray<FString> ParamArray;
        ParamsStr.ParseIntoArray(ParamArray, TEXT(","), true);
        for (const FString& Param : ParamArray)
        {
            FString ParamTrimmed = Param.TrimStartAndEnd();
            if (ParamTrimmed.IsEmpty()) continue;
            int32 LastSpaceInParam = -1;
            if (ParamTrimmed.FindLastChar(' ', LastSpaceInParam))
            {
                FParameterInfo Info;
                Info.Type = ParamTrimmed.Left(LastSpaceInParam).TrimStartAndEnd();
                Info.Name = ParamTrimmed.RightChop(LastSpaceInParam + 1).TrimStartAndEnd();
                ParsedInputs.Add(Info);
            }
        }
    }
    return true;
}
UEdGraphPin* FBpGeneratorCompiler::FindAndConfigureWidgetEvent(FName WidgetName, FName EventName)
{
    UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(TargetBlueprint);
    if (!WidgetBlueprint) return nullptr;
    FProperty* WidgetProperty = FindFProperty<FProperty>(WidgetBlueprint->GeneratedClass, WidgetName);
    if (!WidgetProperty)
    {
        UE_LOG(LogBpGenCompiler, Error, TEXT("WidgetEvent: Could not find a widget variable named '%s'. Make sure 'Is Variable?' is checked in the UMG Designer."), *WidgetName.ToString());
        return nullptr;
    }
    TArray<UK2Node_ComponentBoundEvent*> EventNodes;
    FunctionGraph->GetNodesOfClass(EventNodes);
    for (UK2Node_ComponentBoundEvent* Node : EventNodes)
    {
        if (Node && Node->ComponentPropertyName == WidgetName && Node->DelegatePropertyName == EventName)
        {
            UE_LOG(LogBpGenCompiler, Log, TEXT("Found existing event node for '%s::%s'"), *WidgetName.ToString(), *EventName.ToString());
            for (UEdGraphPin* Pin : Node->Pins) { if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec) { PinMap.Add(Pin->GetName(), Pin); } }
            return Node->GetThenPin();
        }
    }
    UK2Node_ComponentBoundEvent* EventNode = NewObject<UK2Node_ComponentBoundEvent>(FunctionGraph);
    EventNode->ComponentPropertyName = WidgetName;
    EventNode->DelegatePropertyName = EventName;
    if (FObjectProperty* ObjectProperty = CastField<FObjectProperty>(WidgetProperty))
    {
        EventNode->DelegateOwnerClass = ObjectProperty->PropertyClass;
    }
    FunctionGraph->AddNode(EventNode, true, false);
    EventNode->CreateNewGuid();
    EventNode->PostPlacedNewNode();
    EventNode->AllocateDefaultPins();
    EventNode->ReconstructNode();
    const int32 NewNodeY = FindFreeYPositionForEventNode();
    EventNode->NodePosX = -600;
    EventNode->NodePosY = NewNodeY;
    for (UEdGraphPin* Pin : EventNode->Pins)
    {
        if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
        {
            PinMap.Add(Pin->GetName(), Pin);
        }
    }
    return EventNode->GetThenPin();
}
UEdGraphPin* FBpGeneratorCompiler::FindAndConfigureComponentEvent(FName ComponentName, FName EventName)
{
    UActorComponent* FoundComponent = nullptr;
    if (TargetBlueprint->SimpleConstructionScript)
    {
        for (USCS_Node* Node : TargetBlueprint->SimpleConstructionScript->GetAllNodes())
        {
            if (Node && Node->GetVariableName() == ComponentName)
            {
                FoundComponent = Node->ComponentTemplate;
                break;
            }
        }
    }
    if (!FoundComponent)
    {
        UE_LOG(LogBpGenCompiler, Error, TEXT("FindAndConfigureComponentEvent failed: Could not find component '%s' on Blueprint '%s'."), *ComponentName.ToString(), *TargetBlueprint->GetName());
        return nullptr;
    }
    if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(FoundComponent))
    {
        if (!PrimitiveComponent->GetGenerateOverlapEvents())
        {
            UE_LOG(LogBpGenCompiler, Log, TEXT("Configuring component '%s' to generate overlap events."), *ComponentName.ToString());
            PrimitiveComponent->Modify();
            PrimitiveComponent->SetGenerateOverlapEvents(true);
        }
    }
    TArray<UK2Node_ComponentBoundEvent*> EventNodes;
    FunctionGraph->GetNodesOfClass<UK2Node_ComponentBoundEvent>(EventNodes);
    for (UK2Node_ComponentBoundEvent* Node : EventNodes)
    {
        if (Node && Node->ComponentPropertyName == ComponentName && Node->DelegatePropertyName == EventName)
        {
            UE_LOG(LogBpGenCompiler, Log, TEXT("Found existing component bound event node for '%s::%s'"), *ComponentName.ToString(), *EventName.ToString());
            for (UEdGraphPin* Pin : Node->Pins)
            {
                if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
                {
                    PinMap.Add(Pin->GetName(), Pin);
                }
            }
            return Node->GetThenPin();
        }
    }
    UK2Node_ComponentBoundEvent* EventNode = NewObject<UK2Node_ComponentBoundEvent>(FunctionGraph);
    EventNode->ComponentPropertyName = ComponentName;
    EventNode->DelegatePropertyName = EventName;
    EventNode->DelegateOwnerClass = FoundComponent->GetClass();
    FunctionGraph->AddNode(EventNode, true, false);
    EventNode->CreateNewGuid();
    EventNode->PostPlacedNewNode();
    EventNode->AllocateDefaultPins();
    EventNode->ReconstructNode();
    const int32 NewNodeY = FindFreeYPositionForEventNode();
    EventNode->NodePosX = -600;
    EventNode->NodePosY = NewNodeY;
    UE_LOG(LogBpGenCompiler, Log, TEXT("Created new component bound event. Mapping output pins:"));
    for (UEdGraphPin* Pin : EventNode->Pins)
    {
        if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
        {
            UE_LOG(LogBpGenCompiler, Log, TEXT("  -> Mapping pin: %s"), *Pin->GetName());
            PinMap.Add(Pin->GetName(), Pin);
        }
    }
    return EventNode->GetThenPin();
}
bool FBpGeneratorCompiler::ConvertCppTypeToPinType(const FString& CppType, FEdGraphPinType& OutPinType)
{
    FString CleanedType = CppType.Replace(TEXT("const "), TEXT("")).Replace(TEXT("&"), TEXT("")).TrimStartAndEnd();
    UE_LOG(LogBpGenCompiler, Log, TEXT("ConvertCppTypeToPinType: Processing cleaned type '%s'"), *CleanedType);
    if (CleanedType.StartsWith(TEXT("TArray<")) && CleanedType.EndsWith(TEXT(">")))
    {
        OutPinType.ContainerType = EPinContainerType::Array;
        FString InnerType = CleanedType.Mid(7, CleanedType.Len() - 8);
        return ConvertCppTypeToPinType(InnerType, OutPinType);
    }
    if (CleanedType.EndsWith(TEXT("*")))
    {
        FString ClassName = CleanedType.LeftChop(1).TrimStartAndEnd();
        FString StrippedClassName = ClassName;
        if (StrippedClassName.StartsWith(TEXT("A")) || StrippedClassName.StartsWith(TEXT("U")))
        {
            StrippedClassName.RightChopInline(1);
        }
        UClass* FoundClass = FindFirstObjectSafe<UClass>(*ClassName);
        if (!FoundClass) { FoundClass = FindFirstObjectSafe<UClass>(*StrippedClassName); }
        if(!FoundClass) { FoundClass = UClass::TryFindTypeSlow<UClass>(ClassName, EFindFirstObjectOptions::ExactClass); }
        if(!FoundClass) { FoundClass = Cast<UClass>(StaticFindObject(UClass::StaticClass(), nullptr, *(FString(TEXT("/Script/Engine.")) + StrippedClassName))); }
        if (FoundClass)
        {
            OutPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
            OutPinType.PinSubCategoryObject = FoundClass;
            return true;
        }
    }
    if (CleanedType == TEXT("float")) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Real; OutPinType.PinSubCategory = UEdGraphSchema_K2::PC_Double; return true; }
    if (CleanedType == TEXT("double")) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Real; OutPinType.PinSubCategory = UEdGraphSchema_K2::PC_Double; return true; }
    if (CleanedType == TEXT("int") || CleanedType == TEXT("int32")) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Int; return true; }
    if (CleanedType == TEXT("int64")) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Int64; return true; }
    if (CleanedType == TEXT("FString")) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_String; return true; }
    if (CleanedType == TEXT("bool")) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Boolean; return true; }
    if (CleanedType == TEXT("FText")) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Text; return true; }
    if (CleanedType == TEXT("FName")) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Name; return true; }
    if (CleanedType == TEXT("FVector")) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = TBaseStructure<FVector>::Get(); return true; }
    if (CleanedType == TEXT("FRotator")) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get(); return true; }
    if (CleanedType == TEXT("FTransform")) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get(); return true; }
    if (CleanedType == TEXT("FLinearColor")) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get(); return true; }
    if (UScriptStruct* FoundStruct = FindStructByName(CleanedType))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        OutPinType.PinSubCategoryObject = FoundStruct;
        return true;
    }
    if (UEnum* FoundEnum = FindEnumByName(CleanedType))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Byte;
        OutPinType.PinSubCategoryObject = FoundEnum;
        return true;
    }
    if (CleanedType.StartsWith(TEXT("TSubclassOf<")) && CleanedType.EndsWith(TEXT(">")))
    {
        FString ClassName = CleanedType.Mid(12, CleanedType.Len() - 13).TrimStartAndEnd();
        UClass* FoundClass = FindBlueprintClassByName(ClassName);
        if (FoundClass)
        {
            OutPinType.PinCategory = UEdGraphSchema_K2::PC_Class;
            OutPinType.PinSubCategoryObject = FoundClass;
            return true;
        }
        FoundClass = FindFirstObjectSafe<UClass>(*ClassName);
        if (!FoundClass) { FoundClass = UClass::TryFindTypeSlow<UClass>(ClassName); }
        if (FoundClass)
        {
            OutPinType.PinCategory = UEdGraphSchema_K2::PC_Class;
            OutPinType.PinSubCategoryObject = FoundClass;
            return true;
        }
    }
    return false;
}
UEnum* FBpGeneratorCompiler::FindEnumByName(const FString& EnumName)
{
    UEnum* FoundEnum = FindFirstObjectSafe<UEnum>(*EnumName);
    if (FoundEnum) return FoundEnum;
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AssetData;
    FARFilter Filter;
    Filter.ClassPaths.Add(UEnum::StaticClass()->GetClassPathName());
    Filter.bRecursiveClasses = true;
    AssetRegistryModule.Get().GetAssets(Filter, AssetData);
    for (const FAssetData& Data : AssetData)
    {
        if (Data.AssetName.ToString() == EnumName)
        {
            return Cast<UEnum>(Data.GetAsset());
        }
    }
    return nullptr;
}
UScriptStruct* FBpGeneratorCompiler::FindStructByName(const FString& StructName)
{
    FString CleanedName = StructName;
    if (CleanedName.StartsWith(TEXT("F"))) { CleanedName.RightChopInline(1); }
    if (UScriptStruct* FoundInMemory = FindFirstObjectSafe<UScriptStruct>(*StructName)) { return FoundInMemory; }
    if (UScriptStruct* FoundInMemoryClean = FindFirstObjectSafe<UScriptStruct>(*CleanedName)) { return FoundInMemoryClean; }
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AssetData;
    FARFilter Filter;
    Filter.ClassPaths.Add(UScriptStruct::StaticClass()->GetClassPathName());
    Filter.bRecursiveClasses = true;
    AssetRegistryModule.Get().GetAssets(Filter, AssetData);
    for (const FAssetData& Data : AssetData)
    {
        if (Data.AssetName.ToString() == StructName || Data.AssetName.ToString() == CleanedName)
        {
            return Cast<UScriptStruct>(Data.GetAsset());
        }
    }
    return nullptr;
}
void FBpGeneratorCompiler::ProcessGetDataTableRow(const TArray<FString>& AllLines, int32& InOutLineIndex, UEdGraphPin*& InOutLastExecPin)
{
    FString PrevLine = AllLines[InOutLineIndex - 1].TrimStartAndEnd();
    PrevLine.Split(TEXT("//"), &PrevLine, nullptr, ESearchCase::CaseSensitive);
    if (PrevLine.EndsWith(";")) PrevLine.LeftChopInline(1);
    FString RowVariableName;
    int32 LastSpace = -1;
    if (PrevLine.TrimStartAndEnd().FindLastChar(' ', LastSpace))
    {
        RowVariableName = PrevLine.RightChop(LastSpace + 1).TrimStartAndEnd();
    }
    else { return; }
    UK2Node_GetDataTableRow* GetDataNode = NewObject<UK2Node_GetDataTableRow>(FunctionGraph);
    GetDataNode->CreateNewGuid();
    FunctionGraph->AddNode(GetDataNode, true, false);
    GetDataNode->PostPlacedNewNode();
    GetDataNode->AllocateDefaultPins();
    PlaceNode(GetDataNode, InOutLastExecPin);
    if (InOutLastExecPin)
    {
        InOutLastExecPin->GetSchema()->TryCreateConnection(InOutLastExecPin, GetDataNode->GetExecPin());
    }
    FString IfLine = AllLines[InOutLineIndex];
    int32 ConditionParenStart = IfLine.Find(TEXT("("));
    int32 ConditionParenEnd = FindMatchingClosingParen(IfLine, ConditionParenStart);
    FString ConditionExpression = IfLine.Mid(ConditionParenStart + 1, ConditionParenEnd - ConditionParenStart - 1);
    int32 CallParenStart = ConditionExpression.Find(TEXT("("));
    int32 CallParenEnd = FindMatchingClosingParen(ConditionExpression, CallParenStart);
    FString ActualParamsStr = ConditionExpression.Mid(CallParenStart + 1, CallParenEnd - CallParenStart - 1);
    TArray<FString> Params = SplitParameters(ActualParamsStr);
    if (Params.Num() > 0)
    {
        FString DataTableName = Params[0];
        UDataTable* DataTableAsset = FindDataTableByName(DataTableName);
        if (DataTableAsset)
        {
            if (UEdGraphPin* DataTablePin = GetDataNode->GetDataTablePin())
            {
                DataTablePin->DefaultObject = DataTableAsset;
                GetDataNode->PinDefaultValueChanged(DataTablePin);
            }
        }
    }
    if (Params.Num() > 1)
    {
        FString RowNameParam = Params[1];
        UEdGraphPin* DummyExecPin = nullptr;
        UEdGraphPin* SourceRowPin = ProcessExpression(RowNameParam, DummyExecPin);
        if (SourceRowPin)
        {
            GetDataNode->GetRowNamePin()->GetSchema()->TryCreateConnection(SourceRowPin, GetDataNode->GetRowNamePin());
        }
        else
        {
            FString LiteralValue = RowNameParam.TrimStartAndEnd();
            if (LiteralValue.StartsWith(TEXT("FName(\"")) && LiteralValue.EndsWith(TEXT("\")")))
            {
                LiteralValue = LiteralValue.Mid(7, LiteralValue.Len() - 9);
            }
            else
            {
                LiteralValue = LiteralValue.TrimQuotes();
            }
            GetDataNode->GetRowNamePin()->DefaultValue = LiteralValue;
            GetDataNode->PinDefaultValueChanged(GetDataNode->GetRowNamePin());
        }
    }
    if (UEdGraphPin* OutRowPin = GetDataNode->FindPin(TEXT("Out Row")))
    {
        PinMap.Add(RowVariableName, OutRowPin);
    }
    UEdGraphPin* RowFoundPin = GetDataNode->GetThenPin();
    UEdGraphPin* RowNotFoundPin = GetDataNode->GetRowNotFoundPin();
    int32 CurrentSearchIndex = InOutLineIndex;
    while(CurrentSearchIndex < AllLines.Num() && !AllLines[CurrentSearchIndex].Contains(TEXT("{"))) { CurrentSearchIndex++; }
    TArray<FString> TrueBlockLines;
    int32 BraceNesting = 0, BlockStartIndex = CurrentSearchIndex;
    for (; CurrentSearchIndex < AllLines.Num(); ++CurrentSearchIndex) {
        for (const TCHAR& Char : AllLines[CurrentSearchIndex]) { if (Char == '{') BraceNesting++; else if (Char == '}') BraceNesting--; }
        if (BraceNesting == 0) {
            for (int32 lineIdx = BlockStartIndex + 1; lineIdx < CurrentSearchIndex; ++lineIdx) { TrueBlockLines.Add(AllLines[lineIdx]); }
            break;
        }
    }
    if (TrueBlockLines.Num() > 0) {
        ProcessScopedCodeBlock(TrueBlockLines, RowFoundPin);
        if(RowFoundPin && ResultNode && IsValid(RowFoundPin->GetOwningNodeUnchecked()) && RowFoundPin->GetOwningNode() != ResultNode && ResultNode->GetExecPin()) { RowFoundPin->GetSchema()->TryCreateConnection(RowFoundPin, ResultNode->GetExecPin()); }
    }
    CurrentSearchIndex++;
    if (CurrentSearchIndex < AllLines.Num() && AllLines[CurrentSearchIndex].TrimStartAndEnd().StartsWith(TEXT("else"))) {
        TArray<FString> FalseBlockLines;
        while(CurrentSearchIndex < AllLines.Num() && !AllLines[CurrentSearchIndex].Contains(TEXT("{"))) { CurrentSearchIndex++; }
        BraceNesting = 0; BlockStartIndex = CurrentSearchIndex;
        for (; CurrentSearchIndex < AllLines.Num(); ++CurrentSearchIndex) {
            for (const TCHAR& Char : AllLines[CurrentSearchIndex]) { if (Char == '{') BraceNesting++; else if (Char == '}') BraceNesting--; }
            if (BraceNesting == 0) {
                for (int32 lineIdx = BlockStartIndex + 1; lineIdx < CurrentSearchIndex; ++lineIdx) { FalseBlockLines.Add(AllLines[lineIdx]); }
                break;
            }
        }
        if (FalseBlockLines.Num() > 0) {
            ProcessScopedCodeBlock(FalseBlockLines, RowNotFoundPin);
            if(RowNotFoundPin && ResultNode && IsValid(RowNotFoundPin->GetOwningNodeUnchecked()) && RowNotFoundPin->GetOwningNode() != ResultNode && ResultNode->GetExecPin()) { RowNotFoundPin->GetSchema()->TryCreateConnection(RowNotFoundPin, ResultNode->GetExecPin()); }
        }
    }
    PinMap.Remove(RowVariableName);
    InOutLineIndex = CurrentSearchIndex;
    if (GetDataNode)
    {
        UEdGraphNode_Comment* CommentNode = NewObject<UEdGraphNode_Comment>(FunctionGraph);
        FunctionGraph->AddNode(CommentNode, true, false);
        CommentNode->NodeComment = TEXT("TODO: Break the 'Out Row' pin and connect the desired variable.");
        CommentNode->NodePosX = GetDataNode->NodePosX + GetDataNode->NodeWidth + 200;
        CommentNode->NodePosY = GetDataNode->NodePosY;
        CommentNode->NodeWidth = 250;
        CommentNode->NodeHeight = 100;
        CommentNode->CommentColor = FLinearColor(1.0f, 0.6f, 0.0f, 1.0f);
    }
}
UDataTable* FBpGeneratorCompiler::FindDataTableByName(const FString& TableName)
{
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FString> PathsToScan;
    PathsToScan.Add(TEXT("/Game/"));
    AssetRegistryModule.Get().ScanPathsSynchronous(PathsToScan, true);
    TArray<FAssetData> AssetData;
    FARFilter Filter;
    Filter.ClassPaths.Add(UDataTable::StaticClass()->GetClassPathName());
    Filter.bRecursiveClasses = true;
    AssetRegistryModule.Get().GetAssets(Filter, AssetData);
    for (const FAssetData& Data : AssetData)
    {
        if (Data.AssetName.ToString().Equals(TableName, ESearchCase::IgnoreCase))
        {
            return Cast<UDataTable>(Data.GetAsset());
        }
    }
    return nullptr;
}
UEdGraphPin* FBpGeneratorCompiler::FindPinByName(const TArray<UEdGraphPin*>& Pins, const FString& Name)
{
    for (UEdGraphPin* Pin : Pins)
    {
        if (Pin && Pin->GetFName().ToString() == Name) return Pin;
    }
    return nullptr;
}
UEdGraphPin* FBpGeneratorCompiler::FindOrCreateEventNode(FName EventName)
{
    TArray<UK2Node_Event*> EventNodes;
    FunctionGraph->GetNodesOfClass<UK2Node_Event>(EventNodes);
    for (UK2Node_Event* Node : EventNodes)
    {
        if (Node && Node->EventReference.GetMemberName() == EventName)
        {
            UE_LOG(LogBpGenCompiler, Log, TEXT("Found existing event node for '%s'"), *EventName.ToString());
            if (EventName == FName(TEXT("ReceiveTick")))
            {
                if (UEdGraphPin* DeltaSecondsPin = Node->FindPin(TEXT("DeltaSeconds")))
                {
                    PinMap.Add(TEXT("DeltaSeconds"), DeltaSecondsPin);
                }
            }
            else if (EventName == FName(TEXT("BlueprintUpdateAnimation")))
            {
                if (UEdGraphPin* DeltaSecondsPin = Node->FindPin(TEXT("DeltaTimeX")))
                {
                    PinMap.Add(TEXT("DeltaSeconds"), DeltaSecondsPin);
                }
            }
            return Node->GetThenPin();
        }
    }
    UE_LOG(LogBpGenCompiler, Log, TEXT("No existing event node for '%s' found. Creating a new one."), *EventName.ToString());
    UClass* ParentClass = TargetBlueprint->ParentClass;
    if (!ParentClass) return nullptr;
    UFunction* FunctionToOverride = ParentClass->FindFunctionByName(EventName);
    if (!FunctionToOverride) return nullptr;
    UK2Node_Event* EventNode = NewObject<UK2Node_Event>(FunctionGraph);
    FunctionGraph->AddNode(EventNode, true, false);
    EventNode->EventReference.SetFromField<UFunction>(FunctionToOverride, false);
    EventNode->bOverrideFunction = true;
    EventNode->CreateNewGuid();
    EventNode->PostPlacedNewNode();
    EventNode->AllocateDefaultPins();
    const int32 NewNodeY = FindFreeYPositionForEventNode();
    EventNode->NodePosX = -600;
    EventNode->NodePosY = NewNodeY;
    if (EventName == FName(TEXT("ReceiveTick")))
    {
        if (UEdGraphPin* DeltaSecondsPin = EventNode->FindPin(TEXT("DeltaSeconds")))
        {
            PinMap.Add(TEXT("DeltaSeconds"), DeltaSecondsPin);
        }
    }
    else if (EventName == FName(TEXT("BlueprintUpdateAnimation")))
    {
        if (UEdGraphPin* DeltaSecondsPin = EventNode->FindPin(TEXT("DeltaTimeX")))
        {
            PinMap.Add(TEXT("DeltaSeconds"), DeltaSecondsPin);
        }
    }
    return EventNode->GetThenPin();
}
UClass* FBpGeneratorCompiler::FindBlueprintClassByName(const FString& ClassName)
{
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AssetData;
    FARFilter Filter;
    Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
    Filter.bRecursiveClasses = true;
    TArray<FString> NamesToSearch;
    NamesToSearch.Add(ClassName);
    if (ClassName.StartsWith(TEXT("A")) || ClassName.StartsWith(TEXT("U")))
    {
        NamesToSearch.Add(ClassName.RightChop(1));
    }
    Filter.PackageNames.Add(*FString::Printf(TEXT("/Game/Blueprints/%s"), *ClassName));
    AssetRegistryModule.Get().GetAssets(Filter, AssetData);
    if (AssetData.Num() > 0)
    {
        if (UBlueprint* Blueprint = Cast<UBlueprint>(AssetData[0].GetAsset()))
        {
            return Blueprint->GeneratedClass;
        }
    }
    AssetRegistryModule.Get().GetAssetsByPath(FName("/Game/"), AssetData, true);
    for(const FAssetData& Data : AssetData)
    {
        if(Data.AssetName.ToString().Equals(ClassName))
        {
             if (UBlueprint* Blueprint = Cast<UBlueprint>(Data.GetAsset()))
             {
                 return Blueprint->GeneratedClass;
             }
        }
    }
    UE_LOG(LogBpGenCompiler, Error, TEXT("Could not find Blueprint asset with name '%s'."), *ClassName);
    return nullptr;
}
void FBpGeneratorCompiler::ProcessSequenceStatement(const TArray<FString>& AllLines, int32& InOutLineIndex, UEdGraphPin*& InOutLastExecPin)
{
    UE_LOG(LogBpGenCompiler, Log, TEXT("Processing Sequence statement."));
    UK2Node_ExecutionSequence* SequenceNode = NewObject<UK2Node_ExecutionSequence>(FunctionGraph);
    SequenceNode->CreateNewGuid();
    FunctionGraph->AddNode(SequenceNode, true, false);
    SequenceNode->PostPlacedNewNode();
    SequenceNode->AllocateDefaultPins();
    PlaceNode(SequenceNode, InOutLastExecPin);
    if (InOutLastExecPin)
    {
        InOutLastExecPin->GetSchema()->TryCreateConnection(InOutLastExecPin, SequenceNode->GetExecPin());
    }
    int32 PinIndex = 0;
    int32 CurrentParseLine = InOutLineIndex;
    int32 VerticalOffset = 0;
    const int32 VerticalSpacing = 150;
    while (true)
    {
        int32 BlockStartLine = -1;
        for (int32 i = CurrentParseLine; i < AllLines.Num(); ++i) { if (AllLines[i].Contains(TEXT("{"))) { BlockStartLine = i; break; } }
        if (BlockStartLine == -1) { InOutLineIndex = CurrentParseLine; break; }
        TArray<FString> BlockLines;
        int32 Nesting = 0, BlockEndLine = -1;
        for (int32 i = BlockStartLine; i < AllLines.Num(); ++i) {
            const FString& Line = AllLines[i];
            for (const TCHAR& C : Line) { if (C == '{') Nesting++; if (C == '}') Nesting--; }
            BlockLines.Add(Line);
            if (Nesting == 0) { BlockEndLine = i; break; }
        }
        if (BlockEndLine == -1) { InOutLineIndex = AllLines.Num(); break; }
        if (BlockLines.Num() > 0) {
            int32 BracePos = BlockLines[0].Find(TEXT("{")); if (BracePos != -1) BlockLines[0].RightChopInline(BracePos + 1);
            BracePos = BlockLines.Last().Find(TEXT("}")); if (BracePos != -1) BlockLines.Last().LeftInline(BracePos);
        }
        if (PinIndex > 0)
        {
            SequenceNode->AddInputPin();
        }
        UEdGraphPin* BranchPin = SequenceNode->GetThenPinGivenIndex(PinIndex);
        if (BranchPin)
        {
            ResetNodePlacementForChain(BranchPin, VerticalOffset);
            ProcessScopedCodeBlock(BlockLines, BranchPin, true);
        }
        PinIndex++;
        CurrentParseLine = BlockEndLine + 1;
        VerticalOffset += VerticalSpacing;
    }
    InOutLineIndex = CurrentParseLine - 1;
    InOutLastExecPin = nullptr;
}
void FBpGeneratorCompiler::ProcessForLoop(const TArray<FString>& AllLines, int32& InOutLineIndex, UEdGraphPin*& InOutLastExecPin)
{
    UE_LOG(LogBpGenCompiler, Log, TEXT("--- Processing For Each Loop ---"));
    UE_LOG(LogBpGenCompiler, Log, TEXT("  -> InOutLastExecPin on entry is from node: '%s'"), InOutLastExecPin ? *InOutLastExecPin->GetOwningNode()->GetName() : TEXT("NULL"));
    FString ForLine = AllLines[InOutLineIndex];
    UBlueprint* StandardMacros = LoadObject<UBlueprint>(nullptr, TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros"));
    if (!StandardMacros) return;
    UEdGraph* ForEachLoopGraph = FindMacroGraphByName(StandardMacros, FName(TEXT("ForEachLoop")));
    if (!ForEachLoopGraph) return;
    UK2Node_MacroInstance* ForEachNode = NewObject<UK2Node_MacroInstance>(FunctionGraph);
    ForEachNode->SetMacroGraph(ForEachLoopGraph);
    ForEachNode->CreateNewGuid();
    FunctionGraph->AddNode(ForEachNode, true, false);
    ForEachNode->PostPlacedNewNode();
    ForEachNode->AllocateDefaultPins();
    ForEachNode->ReconstructNode();
    PlaceNode(ForEachNode, InOutLastExecPin);
    const FGuid ForEachNodeGuid = ForEachNode->NodeGuid;
    if (InOutLastExecPin)
    {
        if (UEdGraphPin* LoopExecIn = ForEachNode->FindPin(TEXT("Exec")))
        {
            InOutLastExecPin->GetSchema()->TryCreateConnection(InOutLastExecPin, LoopExecIn);
            UE_LOG(LogBpGenCompiler, Log, TEXT("  -> SUCCESS: Connected exec chain to ForEachLoop 'Exec' pin."));
        }
    }
    int32 SigStartParen = ForLine.Find(TEXT("("));
    if (SigStartParen == -1) return;
    int32 SigEndParen = FindMatchingClosingParen(ForLine, SigStartParen);
    if (SigEndParen == -1) return;
    FString Signature = ForLine.Mid(SigStartParen + 1, SigEndParen - SigStartParen - 1);
    FString ElementAndIndexDecl, ArrayName;
    if (!Signature.Split(TEXT(":"), &ElementAndIndexDecl, &ArrayName)) return;
    ArrayName = ArrayName.TrimStartAndEnd();
    FString ElementName, IndexName;
    TArray<FString> Decls;
    ElementAndIndexDecl.ParseIntoArray(Decls, TEXT(","), true);
    if (Decls.Num() > 0)
    {
        FString ElementDecl = Decls[0].TrimStartAndEnd();
        int32 LastSpacePos = -1;
        if (ElementDecl.FindLastChar(' ', LastSpacePos)) { ElementName = ElementDecl.RightChop(LastSpacePos + 1); }
    }
    if (Decls.Num() > 1)
    {
        FString IndexDecl = Decls[1].TrimStartAndEnd();
        int32 LastSpacePos = -1;
        if (IndexDecl.FindLastChar(' ', LastSpacePos)) { IndexName = IndexDecl.RightChop(LastSpacePos + 1); }
    }
    UE_LOG(LogBpGenCompiler, Log, TEXT("  -> Parsed Loop: Array='%s', Element='%s', Index='%s'"), *ArrayName, *ElementName, *IndexName);
    UEdGraphPin* ArrayInputPin = ForEachNode->FindPin(TEXT("Array"));
    if (ArrayInputPin)
    {
        UEdGraphPin* SourceArrayPin = ProcessExpression(ArrayName, InOutLastExecPin);
        if (SourceArrayPin)
        {
            ArrayInputPin->GetSchema()->TryCreateConnection(SourceArrayPin, ArrayInputPin);
            UE_LOG(LogBpGenCompiler, Log, TEXT("  -> SUCCESS: Connected Array pin."));
        }
    }
    UK2Node_MacroInstance* RefreshedForEachNode = Cast<UK2Node_MacroInstance>(FBlueprintEditorUtils::GetNodeByGUID(TargetBlueprint, ForEachNodeGuid));
    if (!RefreshedForEachNode)
    {
        UE_LOG(LogBpGenCompiler, Error, TEXT("FATAL: Could not re-find ForEachLoop node by GUID after compile."));
        return;
    }
    UEdGraphPin* LoopBodyExecPin = RefreshedForEachNode->FindPin(TEXT("LoopBody"));
    UEdGraphPin* ArrayElementPin = RefreshedForEachNode->FindPin(TEXT("Array Element"));
    UEdGraphPin* ArrayIndexPin = RefreshedForEachNode->FindPin(TEXT("Array Index"));
    int32 BodyStartLine = -1;
    for (int32 i = InOutLineIndex; i < AllLines.Num(); ++i) { if (AllLines[i].Contains(TEXT("{"))) { BodyStartLine = i; break; } }
    if (BodyStartLine == -1) return;
    FString FullBodyBlock;
    for (int32 i = BodyStartLine; i < AllLines.Num(); ++i) FullBodyBlock += AllLines[i] + TEXT("\n");
    int32 BraceStart = FullBodyBlock.Find(TEXT("{"));
    int32 BraceEnd = FindMatchingClosingBrace(FullBodyBlock, BraceStart);
    if (BraceEnd == -1) return;
    FString LoopBodyContent = FullBodyBlock.Mid(BraceStart + 1, BraceEnd - BraceStart - 1);
    TArray<FString> LoopBodyLines;
    LoopBodyContent.ParseIntoArrayLines(LoopBodyLines, true);
    if (LoopBodyExecPin && ArrayElementPin && ArrayIndexPin && LoopBodyLines.Num() > 0)
    {
        PinMap.Add(ElementName, ArrayElementPin);
        if (!IndexName.IsEmpty()) PinMap.Add(IndexName, ArrayIndexPin);
        ProcessScopedCodeBlock(LoopBodyLines, LoopBodyExecPin, true);
        PinMap.Remove(ElementName);
        if (!IndexName.IsEmpty()) PinMap.Remove(IndexName);
    }
    InOutLastExecPin = RefreshedForEachNode->FindPin(TEXT("Completed"));
    int32 NestingLevel = 0;
    int32 FinalLine = InOutLineIndex;
    for (int32 i = BodyStartLine; i < AllLines.Num(); ++i)
    {
        for (const TCHAR& Char : AllLines[i]) { if (Char == '{') NestingLevel++; else if (Char == '}') NestingLevel--; }
        if (NestingLevel == 0) { FinalLine = i; break; }
    }
    UE_LOG(LogBpGenCompiler, Log, TEXT("  -> For loop processing finished. Updating master index from %d to %d."), InOutLineIndex, FinalLine);
    InOutLineIndex = FinalLine;
}
void FBpGeneratorCompiler::ProcessSwitchStatement(const TArray<FString>& AllLines, int32& InOutLineIndex, UEdGraphPin*& InOutLastExecPin)
{
    UE_LOG(LogBpGenCompiler, Warning, TEXT("--- Processing Switch Statement ---"));
    FString SwitchLine = AllLines[InOutLineIndex];
    int32 VarParenStart = SwitchLine.Find(TEXT("("));
    int32 VarParenEnd = FindMatchingClosingParen(SwitchLine, VarParenStart);
    if (VarParenStart == INDEX_NONE || VarParenEnd == INDEX_NONE) return;
    FString VariableName = SwitchLine.Mid(VarParenStart + 1, VarParenEnd - VarParenStart - 1).TrimStartAndEnd();
    UEdGraphPin* InputPin = nullptr;
    if (UEdGraphPin** FoundPinPtr = PinMap.Find(VariableName))
    {
        InputPin = *FoundPinPtr;
    }
    else
    {
        UClass* ClassToSearch = TargetBlueprint->SkeletonGeneratedClass ? TargetBlueprint->SkeletonGeneratedClass : TargetBlueprint->GeneratedClass;
        if (ClassToSearch)
        {
            FProperty* FoundProperty = FindFProperty<FProperty>(ClassToSearch, FName(*VariableName));
            if (FoundProperty)
            {
                UE_LOG(LogBpGenCompiler, Log, TEXT("  -> JIT GET for Switch: Found persistent member variable '%s'. Creating GET node."), *VariableName);
                UK2Node_VariableGet* VarGetNode = NewObject<UK2Node_VariableGet>(FunctionGraph);
                VarGetNode->CreateNewGuid();
                FunctionGraph->AddNode(VarGetNode, true, false);
                VarGetNode->PostPlacedNewNode();
                VarGetNode->AllocateDefaultPins();
                VarGetNode->VariableReference.SetFromField<FProperty>(FoundProperty, true);
                VarGetNode->ReconstructNode();
                PlaceNode(VarGetNode, InOutLastExecPin);
                InputPin = VarGetNode->GetValuePin();
            }
        }
    }
    if (!InputPin)
    {
        UE_LOG(LogBpGenCompiler, Error, TEXT("Switch failed: Could not find input variable '%s' in local scope or as a persistent member variable."), *VariableName);
        int32 NestingLevel = 0;
        for (int32 i = InOutLineIndex; i < AllLines.Num(); ++i) { for (const TCHAR c : AllLines[i]) { if (c == '{') NestingLevel++; if (c == '}') NestingLevel--; } if (NestingLevel == 0) { InOutLineIndex = i; break; } }
        return;
    }
    UK2Node_Switch* SwitchNode = nullptr;
    const FName PinCategory = InputPin->PinType.PinCategory;
    if (PinCategory == UEdGraphSchema_K2::PC_Byte && InputPin->PinType.PinSubCategoryObject.IsValid())
    {
        SwitchNode = NewObject<UK2Node_SwitchEnum>(FunctionGraph);
        if (UK2Node_SwitchEnum* EnumSwitch = Cast<UK2Node_SwitchEnum>(SwitchNode))
        {
            UEnum* EnumToSwitchOn = Cast<UEnum>(InputPin->PinType.PinSubCategoryObject.Get());
            EnumSwitch->Enum = EnumToSwitchOn;
        }
    }
    else if (PinCategory == UEdGraphSchema_K2::PC_Name)
    {
        SwitchNode = NewObject<UK2Node_SwitchName>(FunctionGraph);
        if (UK2Node_SwitchName* NameSwitch = Cast<UK2Node_SwitchName>(SwitchNode))
        {
            NameSwitch->PinNames.Empty();
        }
    }
    else
    {
        UE_LOG(LogBpGenCompiler, Error, TEXT("Switch failed: Input variable '%s' is not an Enum or a Name."), *VariableName);
        return;
    }
    SwitchNode->CreateNewGuid();
    FunctionGraph->AddNode(SwitchNode, true, false);
    SwitchNode->PostPlacedNewNode();
    SwitchNode->AllocateDefaultPins();
    SwitchNode->ReconstructNode();
    PlaceNode(SwitchNode, InOutLastExecPin);
    if(InOutLastExecPin && IsValid(InOutLastExecPin->GetOwningNodeUnchecked()))
    {
        InOutLastExecPin->GetSchema()->TryCreateConnection(InOutLastExecPin, SwitchNode->GetExecPin());
    }
    if(UEdGraphPin* SelectionPin = SwitchNode->GetSelectionPin())
    {
        SelectionPin->GetSchema()->TryCreateConnection(InputPin, SelectionPin);
    }
    const FRegexPattern CasePattern(TEXT("case\\s+(.+?):\\s*([\\s\\S]*?)\\s*break;"));
    FString CasesBlock;
    int32 BraceStart = AllLines[InOutLineIndex].Find(TEXT("{"));
    if (BraceStart != INDEX_NONE)
    {
        FString Remainder = AllLines[InOutLineIndex].RightChop(BraceStart);
        for(int32 i = InOutLineIndex + 1; i < AllLines.Num(); ++i) Remainder += TEXT("\n") + AllLines[i];
        int32 BraceEnd = FindMatchingClosingBrace(Remainder, 0);
        if (BraceEnd != INDEX_NONE) CasesBlock = Remainder.Mid(1, BraceEnd - 1);
    }
    FRegexMatcher CaseMatcher(CasePattern, CasesBlock);
    TArray<TPair<FString, FString>> ParsedCases;
    while(CaseMatcher.FindNext())
    {
        FString ValueStr = CaseMatcher.GetCaptureGroup(1).TrimStartAndEnd();
        FString CaseBodyStr = CaseMatcher.GetCaptureGroup(2).TrimStartAndEnd();
        ParsedCases.Add(TPair<FString, FString>(ValueStr, CaseBodyStr));
    }
    if (UK2Node_SwitchName* NameSwitch = Cast<UK2Node_SwitchName>(SwitchNode))
    {
        for (const auto& ParsedCase : ParsedCases)
        {
            NameSwitch->PinNames.Add(FName(*ParsedCase.Key.TrimQuotes()));
        }
        NameSwitch->ReconstructNode();
    }
    for (const auto& ParsedCase : ParsedCases)
    {
        UEdGraphPin* CaseExecPin = nullptr;
        UE_LOG(LogBpGenCompiler, Warning, TEXT("  -> Processing Case. Value: '%s' | Body: '%s'"), *ParsedCase.Key, *ParsedCase.Value);
        if (UK2Node_SwitchEnum* EnumSwitch = Cast<UK2Node_SwitchEnum>(SwitchNode))
        {
            for (UEdGraphPin* Pin : EnumSwitch->Pins)
            {
                if (Pin->Direction == EGPD_Output && Pin->GetDisplayName().ToString() == ParsedCase.Key)
                {
                    UE_LOG(LogBpGenCompiler, Warning, TEXT("     - Found matching pin by display name: '%s'"), *Pin->GetDisplayName().ToString());
                    CaseExecPin = Pin;
                    break;
                }
            }
        }
        else if(UK2Node_SwitchName* NameSwitch = Cast<UK2Node_SwitchName>(SwitchNode))
        {
            FName PinToFind = FName(*ParsedCase.Key.TrimQuotes());
            UE_LOG(LogBpGenCompiler, Warning, TEXT("     - Searching for Name Pin: '%s'"), *PinToFind.ToString());
            CaseExecPin = NameSwitch->FindPin(PinToFind);
        }
        if (CaseExecPin)
        {
            UE_LOG(LogBpGenCompiler, Warning, TEXT("     - Pin Found. Processing body."));
            TArray<FString> CaseBodyLines;
            ParsedCase.Value.ParseIntoArrayLines(CaseBodyLines, true);
            ProcessScopedCodeBlock(CaseBodyLines, CaseExecPin);
        }
        else
        {
             UE_LOG(LogBpGenCompiler, Error, TEXT("     - FAILED to find exec pin for case: '%s'"), *ParsedCase.Key);
        }
    }
    InOutLineIndex = AllLines.Num();
}
bool FBpGeneratorCompiler::CreateNewFunction()
{
    UE_LOG(LogBpGenCompiler, Log, TEXT("Function '%s' not found. Creating a new function."), *ParsedFunctionName);
    FName NewGraphName = FBlueprintEditorUtils::FindUniqueKismetName(TargetBlueprint, ParsedFunctionName);
    FunctionGraph = NewObject<UEdGraph>(TargetBlueprint, NewGraphName, RF_Transactional);
    FunctionGraph->Schema = UEdGraphSchema_K2::StaticClass();
    TargetBlueprint->FunctionGraphs.Add(FunctionGraph);
    EntryNode = NewObject<UK2Node_FunctionEntry>(FunctionGraph);
    EntryNode->CreateNewGuid();
    EntryNode->PostPlacedNewNode();
    EntryNode->AllocateDefaultPins();
    EntryNode->CustomGeneratedFunctionName = NewGraphName;
    EntryNode->bIsEditable = true;
    FunctionGraph->AddNode(EntryNode, false, false);
    if (ParsedReturnType == "void") { EntryNode->AddExtraFlags(FUNC_Public | FUNC_BlueprintCallable); }
    else { EntryNode->AddExtraFlags(FUNC_Public | FUNC_BlueprintCallable); }
    ResultNode = NewObject<UK2Node_FunctionResult>(FunctionGraph);
    ResultNode->CreateNewGuid();
    ResultNode->PostPlacedNewNode();
    ResultNode->AllocateDefaultPins();
    FunctionGraph->AddNode(ResultNode, false, false);
    FBlueprintEditorUtils::RenameGraph(FunctionGraph, ParsedFunctionName);
    UEdGraphPin* EntryThenPin = EntryNode->GetThenPin();
    UEdGraphPin* ResultExecPin = ResultNode->GetExecPin();
    if (EntryThenPin && ResultExecPin)
    {
        UE_LOG(LogBpGenCompiler, Warning, TEXT("Stabilizing new function graph by creating and breaking a temporary link."));
        EntryThenPin->GetSchema()->TryCreateConnection(EntryThenPin, ResultExecPin);
        EntryThenPin->BreakAllPinLinks();
    }
    for (const FParameterInfo& InputParam : ParsedInputs)
    {
        FEdGraphPinType PinType;
        if (ConvertCppTypeToPinType(InputParam.Type, PinType))
            InputPins.Add(EntryNode->CreateUserDefinedPin(FName(*InputParam.Name), PinType, EGPD_Output));
    }
    if (ParsedReturnType != "void")
    {
        FEdGraphPinType PinType;
        if (ConvertCppTypeToPinType(ParsedReturnType, PinType))
            OutputPins.Add(ResultNode->CreateUserDefinedPin(FName("ReturnValue"), PinType, EGPD_Input));
    }
    for (int32 i = 0; i < ParsedInputs.Num(); ++i)
    {
        if (InputPins.IsValidIndex(i)) PinMap.Add(ParsedInputs[i].Name, InputPins[i]);
    }
    UEdGraphPin* LastExecPin = EntryNode->GetThenPin();
    ResetNodePlacementForChain(LastExecPin);
    try {
        ProcessScopedCodeBlock(CodeLines, LastExecPin);
    }
    catch (...)
    {
        UE_LOG(LogBpGenCompiler, Fatal, TEXT("An unhandled C++ exception was caught inside ProcessScopedCodeBlock during CreateNewFunction."));
        return false;
    }
    ResultNode->NodePosX = CurrentNodeX + 400;
    ResultNode->NodePosY = EntryNode->NodePosY;
    if (LastExecPin && IsValid(LastExecPin->GetOwningNodeUnchecked()) && ResultNode)
    {
        if (LastExecPin->GetOwningNode() != ResultNode && !LastExecPin->HasAnyConnections())
        {
            LastExecPin->GetSchema()->TryCreateConnection(LastExecPin, ResultNode->GetExecPin());
        }
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(TargetBlueprint);
    TargetBlueprint->PostEditChange();
    TargetBlueprint->BroadcastChanged();
    return true;
}
void FBpGeneratorCompiler::ProcessAssignmentStatement(const FString& LhsVarName, const FString& RhsExpression, UEdGraphPin*& InOutLastExecPin)
{
    UE_LOG(LogBpGenCompiler, Log, TEXT("ProcessAssignmentStatement: Setting '%s' = '%s'"), *LhsVarName, *RhsExpression);
    if (RhsExpression.Contains(TEXT("->")) && !RhsExpression.Contains(TEXT("(")))
    {
        FString SourceObjectName, MemberName;
        if (!RhsExpression.Split(TEXT("->"), &SourceObjectName, &MemberName))
        {
            UE_LOG(LogBpGenCompiler, Error, TEXT("Assignment failed: Could not parse RHS member access '%s'"), *RhsExpression);
            return;
        }
        UEdGraphPin* SourceObjectPin = ProcessExpression(SourceObjectName, InOutLastExecPin);
        if (!SourceObjectPin)
        {
            UE_LOG(LogBpGenCompiler, Error, TEXT("Assignment failed: Could not find source object pin for '%s'"), *SourceObjectName);
            return;
        }
        UClass* SourceClass = Cast<UClass>(SourceObjectPin->PinType.PinSubCategoryObject.Get());
        if (!SourceClass)
        {
            UE_LOG(LogBpGenCompiler, Error, TEXT("Assignment failed: Could not determine UClass type from source object pin for '%s'."), *SourceObjectName);
            return;
        }
        UE_LOG(LogBpGenCompiler, Log, TEXT("Assignment Context: Will search for member '%s' inside class '%s'."), *MemberName, *SourceClass->GetName());
        FProperty* MemberProperty = FindFProperty<FProperty>(SourceClass, FName(*MemberName));
        if (!MemberProperty)
        {
            UE_LOG(LogBpGenCompiler, Error, TEXT("Assignment failed: Could not find member property '%s' on class '%s'. This can happen if the source Blueprint ('%s') has not been compiled recently."), *MemberName, *SourceClass->GetName(), *SourceClass->GetName());
            return;
        }
        UK2Node_VariableGet* GetNode = NewObject<UK2Node_VariableGet>(FunctionGraph);
        FunctionGraph->AddNode(GetNode, true, false);
        GetNode->PostPlacedNewNode();
        GetNode->VariableReference.SetFromField<FProperty>(MemberProperty, false);
        GetNode->ReconstructNode();
        PlaceNode(GetNode, InOutLastExecPin, -250, 60);
        UEdGraphPin* GetNodeSelfPin = GetNode->FindPin(UEdGraphSchema_K2::PN_Self);
        if (GetNodeSelfPin)
        {
            GetNodeSelfPin->GetSchema()->TryCreateConnection(SourceObjectPin, GetNodeSelfPin);
        }
        FProperty* LhsProperty = FindFProperty<FProperty>(TargetBlueprint->SkeletonGeneratedClass, FName(*LhsVarName));
        if (!LhsProperty)
        {
            UE_LOG(LogBpGenCompiler, Error, TEXT("Assignment failed: Could not find LHS variable '%s' on the Blueprint."), *LhsVarName);
            return;
        }
        UK2Node_VariableSet* SetNode = NewObject<UK2Node_VariableSet>(FunctionGraph);
        FunctionGraph->AddNode(SetNode, true, false);
        SetNode->PostPlacedNewNode();
        SetNode->VariableReference.SetFromField<FProperty>(LhsProperty, true);
        SetNode->ReconstructNode();
        PlaceNode(SetNode, InOutLastExecPin);
        if (InOutLastExecPin)
        {
            InOutLastExecPin->GetSchema()->TryCreateConnection(InOutLastExecPin, SetNode->GetExecPin());
        }
        UEdGraphPin* GetNodeValuePin = GetNode->GetValuePin();
        UEdGraphPin* SetNodeValuePin = SetNode->FindPin(LhsProperty->GetFName());
        if (GetNodeValuePin && SetNodeValuePin)
        {
            GetNodeValuePin->GetSchema()->TryCreateConnection(GetNodeValuePin, SetNodeValuePin);
        }
        InOutLastExecPin = SetNode->GetThenPin();
        return;
    }
    UEdGraphPin* RhsResultPin = ProcessExpression(RhsExpression, InOutLastExecPin);
    UClass* ClassToSearch = TargetBlueprint->SkeletonGeneratedClass ? TargetBlueprint->SkeletonGeneratedClass : TargetBlueprint->GeneratedClass;
    if (!ClassToSearch)
    {
        UE_LOG(LogBpGenCompiler, Error, TEXT("Assignment failed. Could not find a valid class for the target blueprint."));
        return;
    }
    FProperty* FoundProperty = FindFProperty<FProperty>(ClassToSearch, FName(*LhsVarName));
    if (!FoundProperty)
    {
        UE_LOG(LogBpGenCompiler, Error, TEXT("Assignment failed. Could not find variable '%s' on the Blueprint."), *LhsVarName);
        return;
    }
    UK2Node_VariableSet* VarSetNode = NewObject<UK2Node_VariableSet>(FunctionGraph);
    VarSetNode->CreateNewGuid();
    FunctionGraph->AddNode(VarSetNode, true, false);
    VarSetNode->PostPlacedNewNode();
    VarSetNode->AllocateDefaultPins();
    VarSetNode->VariableReference.SetFromField<FProperty>(FoundProperty, true);
    VarSetNode->ReconstructNode();
    PlaceNode(VarSetNode, InOutLastExecPin);
    if (InOutLastExecPin)
    {
        InOutLastExecPin->GetSchema()->TryCreateConnection(InOutLastExecPin, VarSetNode->GetExecPin());
    }
    UEdGraphPin* DestPin = VarSetNode->FindPin(FoundProperty->GetFName());
    if (DestPin)
    {
        if (RhsResultPin)
        {
            DestPin->GetSchema()->TryCreateConnection(RhsResultPin, DestPin);
        }
        else
        {
            SetPinDefaultValue(DestPin, RhsExpression);
        }
    }
    else
    {
        UE_LOG(LogBpGenCompiler, Error, TEXT("FATAL: Could not find input value pin on VariableSet node for '%s'"), *LhsVarName);
    }
    InOutLastExecPin = VarSetNode->GetThenPin();
}
UEdGraphPin* FBpGeneratorCompiler::FindOrCreateCustomEventNode(FName EventName)
{
    TArray<UK2Node_CustomEvent*> EventNodes;
    FunctionGraph->GetNodesOfClass<UK2Node_CustomEvent>(EventNodes);
    for (UK2Node_CustomEvent* Node : EventNodes)
    {
        if (Node && Node->CustomFunctionName == EventName)
        {
            UE_LOG(LogBpGenCompiler, Log, TEXT("Found existing custom event node for '%s'"), *EventName.ToString());
            return Node->FindPin(UEdGraphSchema_K2::PN_Then);
        }
    }
    UE_LOG(LogBpGenCompiler, Log, TEXT("No existing custom event for '%s' found. Creating a new one."), *EventName.ToString());
    UK2Node_CustomEvent* EventNode = NewObject<UK2Node_CustomEvent>(FunctionGraph);
    FunctionGraph->AddNode(EventNode, true, false);
    EventNode->CustomFunctionName = EventName;
    EventNode->CreateNewGuid();
    EventNode->PostPlacedNewNode();
    EventNode->AllocateDefaultPins();
    for (const FParameterInfo& InputParam : ParsedInputs)
    {
        FEdGraphPinType PinType;
        if (ConvertCppTypeToPinType(InputParam.Type, PinType))
        {
            UEdGraphPin* NewPin = EventNode->CreateUserDefinedPin(FName(*InputParam.Name), PinType, EGPD_Output);
            if (NewPin)
            {
                PinMap.Add(InputParam.Name, NewPin);
            }
        }
    }
    const int32 NewNodeY = FindFreeYPositionForEventNode();
    EventNode->NodePosX = -600;
    EventNode->NodePosY = NewNodeY;
    return EventNode->GetThenPin();
}
int32 FBpGeneratorCompiler::FindFreeYPositionForEventNode()
{
    int32 MaxY = -200;
    bool bFoundAnyEvent = false;
    TArray<UK2Node_Event*> EventNodes;
    FunctionGraph->GetNodesOfClass<UK2Node_Event>(EventNodes);
    for (UK2Node_Event* Node : EventNodes)
    {
        bFoundAnyEvent = true;
        MaxY = FMath::Max(MaxY, Node->NodePosY);
    }
    TArray<UK2Node_CustomEvent*> CustomEventNodes;
    FunctionGraph->GetNodesOfClass<UK2Node_CustomEvent>(CustomEventNodes);
    for (UK2Node_CustomEvent* Node : CustomEventNodes)
    {
        bFoundAnyEvent = true;
        MaxY = FMath::Max(MaxY, Node->NodePosY);
    }
    const int32 Padding = 450;
    return bFoundAnyEvent ? MaxY + Padding : 0;
}
void FBpGeneratorCompiler::ResetNodePlacementForChain(UEdGraphPin* StartOfChainPin, int32 YOffset)
{
    if (StartOfChainPin)
    {
        UEdGraphNode* StartNode = StartOfChainPin->GetOwningNode();
        const int32 Padding = 300;
        CurrentNodeX = StartNode->NodePosX + Padding;
        CurrentBaseY = StartNode->NodePosY + YOffset;
        UE_LOG(LogBpGenCompiler, Log, TEXT("Resetting node placement. Starting X: %d, Base Y: %d"), CurrentNodeX, CurrentBaseY);
    }
}
bool FBpGeneratorCompiler::ValidateSyntax(const FString& Code, FString& OutError)
{
    FString RemainingCode = Code;
    if (RemainingCode.TrimStartAndEnd().IsEmpty())
    {
        OutError = TEXT("Syntax validation failed: The provided code was empty.");
        return false;
    }
    while (!RemainingCode.TrimStartAndEnd().IsEmpty())
    {
        FString TrimmedCode = RemainingCode.TrimStartAndEnd();
        FString OriginalCodeForThisIteration = RemainingCode;
        if (!ParseFunctionSignature(TrimmedCode))
        {
            OutError = TEXT("Syntax validation failed: The code contains an invalid or incomplete function/event signature. All code must be inside a block like 'void MyFunction() { ... }'.");
            bIsEventGraphLogic = bIsComponentEvent = bIsKeyEvent = bIsCustomEvent = bIsWidgetEvent = bIsKeyReleasedEvent = false;
            return false;
        }
        int32 BodyStart = OriginalCodeForThisIteration.Find(TEXT("{"));
        if (BodyStart != -1)
        {
            int32 BodyEnd = FindMatchingClosingBrace(OriginalCodeForThisIteration, BodyStart);
            if (BodyEnd != -1)
            {
                RemainingCode = OriginalCodeForThisIteration.RightChop(BodyEnd + 1);
            }
            else
            {
                OutError = TEXT("Syntax validation failed: The code contains mismatched curly braces '{}'.");
                bIsEventGraphLogic = bIsComponentEvent = bIsKeyEvent = bIsCustomEvent = bIsWidgetEvent = bIsKeyReleasedEvent = false;
                return false;
            }
        }
        else
        {
            OutError = TEXT("Syntax validation failed: Could not find a starting '{' for a code block.");
            bIsEventGraphLogic = bIsComponentEvent = bIsKeyEvent = bIsCustomEvent = bIsWidgetEvent = bIsKeyReleasedEvent = false;
            return false;
        }
    }
    bIsEventGraphLogic = bIsComponentEvent = bIsKeyEvent = bIsCustomEvent = bIsWidgetEvent = bIsKeyReleasedEvent = false;
    return true;
}
