// Copyright 2026, BlueprintsLab, All rights reserved
#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Kismet2/KismetEditorUtilities.h"
DECLARE_LOG_CATEGORY_EXTERN(LogBpGenCompiler, Log, All);
class UBlueprint;
class UEdGraph;
class UK2Node_FunctionEntry;
class UK2Node_FunctionResult;
class UEdGraphNode;
class UEdGraphPin;
struct FEdGraphPinType;
class UFunction;
class UK2Node_MacroInstance;
class UK2Node_GetDataTableRow;
class UDataTable;
class UK2Node_Event;
class UActorComponent;
class UK2Node_VariableSet;
class UEdGraphNode_Comment;
class UK2Node_CustomEvent;
class UK2Node_Timeline;
class UK2Node_ComponentBoundEvent;
class UK2Node_DynamicCast;
class UK2Node_ExecutionSequence;
class UK2Node_InputKey;
class UWidgetBlueprint;
struct FParameterInfo
{
    FString Type;
    FString Name;
};
struct FTimelineLogicBlock
{
    UK2Node_Timeline* TimelineNode = nullptr;
    TArray<FString> OnUpdateCode;
    TArray<FString> OnFinishedCode;
    TMap<FString, UEdGraphPin*> LocalPinMap;
};
class FBpGeneratorCompiler
{
public:
    FBpGeneratorCompiler(UBlueprint* InTargetBlueprint);
    bool Compile(const FString& InCode);
    bool InsertCode(UEdGraphNode* InsertionPointNode, const FString& DummyFunctionCode);
    bool ValidateSyntax(const FString& Code, FString& OutError);
    static TArray<TArray<FGuid>> NodeCreationStack;
    static FCriticalSection NodeCreationStackLock;
    static TArray<FGuid> PopLastCreatedNodes();
    static bool DeleteNodesByGUIDs(UBlueprint* Blueprint, const TArray<FGuid>& NodeGUIDs);
private:
    bool ParseFunctionSignature(const FString& InCode);
    void ProcessCodeWithLoops();
    void ProcessScopedCodeBlock(const TArray<FString>& BlockLines, UEdGraphPin*& InOutLastExecPin, bool bInheritCommentContext = false);
    void ProcessIfStatement(const TArray<FString>& AllLines, int32& InOutLineIndex, UEdGraphPin*& InOutLastExecPin);
    void ProcessGetDataTableRow(const TArray<FString>& AllLines, int32& InOutLineIndex, UEdGraphPin*& InOutLastExecPin);
    UEdGraphPin* ProcessExpression(const FString& Expression, UEdGraphPin*& InOutLastExecPin);
    UEnum* FindEnumByName(const FString& EnumName);
    class UScriptStruct* FindStructByName(const FString& StructName);
    bool ConvertCppTypeToPinType(const FString& CppType, FEdGraphPinType& OutPinType);
    UEdGraphPin* FindPinByName(const TArray<UEdGraphPin*>& Pins, const FString& Name);
    void SetPinDefaultValue(UEdGraphPin* Pin, const FString& LiteralValue);
    int32 FindMatchingClosingParen(const FString& Haystack, int32 StartIndex);
    int32 FindMatchingClosingBrace(const FString& Haystack, int32 StartIndex);
    void PlaceNode(UEdGraphNode* Node, UEdGraphPin* LastExecPin, int32 XOffset = 0, int32 YOffset = 0);
    void ProcessReturnStatement(const FString& ReturnExpression, UEdGraphPin*& InOutLastExecPin);
    UEdGraph* FindMacroGraphByName(UBlueprint* MacroLibrary, FName MacroName);
    UDataTable* FindDataTableByName(const FString& TableName);
    UEdGraphPin* FindOrCreateEventNode(FName EventName);
    UClass* FindBlueprintClassByName(const FString& ClassName);
    void ProcessForLoop(const TArray<FString>& AllLines, int32& InOutLineIndex, UEdGraphPin*& InOutLastExecPin);
    void ProcessSwitchStatement(const TArray<FString>& AllLines, int32& InOutLineIndex, UEdGraphPin*& InOutLastExecPin);
    bool CreateNewFunction();
    void ProcessAssignmentStatement(const FString& LhsVarName, const FString& RhsExpression, UEdGraphPin*& InOutLastExecPin);
    void FinalizeAllCommentBoxes();
    UEdGraphPin* FindOrCreateCustomEventNode(FName EventName);
    void ResetNodePlacementForChain(UEdGraphPin* StartOfChainPin, int32 YOffset = 0);
    void ProcessTimelineDeclaration(const TArray<FString>& AllLines, int32& InOutLineIndex, UEdGraphPin*& InOutLastExecPin);
    UEdGraphPin* FindAndConfigureComponentEvent(FName ComponentName, FName EventName);
    void ProcessSequenceStatement(const TArray<FString>& AllLines, int32& InOutLineIndex, UEdGraphPin*& InOutLastExecPin);
    UEdGraphPin* FindOrCreateKeyEventNode(FName KeyName, bool bForReleasedEvent);
    UEdGraphPin* FindAndConfigureWidgetEvent(FName WidgetName, FName EventName);
    void ProcessWhileLoop(const TArray<FString>& AllLines, int32& InOutLineIndex, UEdGraphPin*& InOutLastExecPin);
    void ProcessTopLevelTimeline(const FString& TimelineCode);
    UBlueprint* TargetBlueprint;
    UEdGraph* FunctionGraph;
    FString ParsedFunctionName;
    FString ParsedReturnType;
    FString ParsedComponentName;
    TArray<FParameterInfo> ParsedInputs;
    TArray<FString> CodeLines;
    TMap<FName, UFunction*> SpecialCaseMemberFunctions;
    FString ParsedWidgetName;
    UK2Node_FunctionEntry* EntryNode;
    UK2Node_FunctionResult* ResultNode;
    TArray<UEdGraphPin*> InputPins;
    TArray<UEdGraphPin*> OutputPins;
    TMap<FString, UEdGraphPin*> PinMap;
    TMap<FString, FString> LiteralMap;
    TMap<FString, UClass*> KnownLibraries;
    TMap<FString, FString> FunctionAliases;
    TMap<UScriptStruct*, UFunction*> NativeBreakFunctions;
    TSet<FString> KnownMemberFunctions;
    bool bIsEventGraphLogic;
    TMap<FString, FName> EventNameMap;
    bool bIsCustomEvent;
    bool bIsComponentEvent;
    bool bIsWidgetEvent;
    UEdGraphPin* SelfPin;
    int32 CurrentNodeX;
    int32 NodeCount;
    int32 CurrentBaseY;
    FString CurrentCommentText;
    TMap<FString, TArray<UEdGraphNode*>> CommentMap;
    int32 FindFreeYPositionForEventNode();
    TMap<FString, UK2Node_Timeline*> TimelineMap;
    TMap<FString, FTimelineLogicBlock> PendingTimelineLogic;
    bool bIsKeyEvent;
    FString ParsedKeyName;
    bool bIsKeyReleasedEvent;
    int32 CalculateLevenshteinDistance(const FString& S1, const FString& S2);
    int32 CurrentPureNodeYOffset;
    TArray<FGuid> NodesCreatedThisSession;
};
