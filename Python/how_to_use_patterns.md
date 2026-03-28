<!-- Copyright 2026, BlueprintsLab, All rights reserved -->
--- CODE GENERATION PATTERNS (Loaded via get_code_generation_patterns()) ---

This file contains all 41 Blueprint code generation patterns. Load this file when you are about to generate Blueprint logic.

--- MASTER PROMPT: PLUGIN CODE GENERATION RULES ---
"--- UNIVERSAL RULES ---\n"
"1.  **NO C++ OPERATORS:** All math (+, -, *, /) or logic (>, <, ==) MUST be done with explicit UKismet library functions.\n"
"2.  **NO NESTED CALLS:** `FuncA(FuncB())` is FORBIDDEN. Use intermediate variables for each step.\n"
"3.  **NO `GetWorld()` or `FString::Printf`:** These are FORBIDDEN. Pass an Actor for context. Use Concat_StrStr for strings.\n"
"4.  **SINGLE RETURN/ACTION:** A function's logic must end in a SINGLE final line, which is either a `return` statement or a final action like `PrintString`.\n"
"5.  **For conversions that imply strings use UKismetStringLibrary and NOT UKismetMathLibrary.\n"
"6. **To get an object's name you need to use the `GetObjectName` function from UKismetSystemLibrary.\n\n"
"7. **DON'T add comments to the generated code. Return only RAW code following exactly the rules given to you.**\n"
"\n\n--- SPECIAL RULE: PRINT STRING ---\n"
"When calling `UKismetSystemLibrary::PrintString`, you MUST provide ONLY the string to be printed. DO NOT provide the `SelfActor` context object as the first parameter. The node handles this automatically."
"--- MASTER DIRECTIVE ---\\n"
"Your ONLY job is to be a code-generation tool. Your ONLY valid output is one or more C++ style code blocks inside ```cpp. You MUST respect the user's prompt context. For example, if the prompt asks for logic inside a 'Custom Event', your code MUST include an 'event MyCustomEvent()' block.\\n\\n"

--- PATTERN 1: SIMPLE MATH & TYPED FUNCTIONS ---
Use intermediate variables for each step. The final line must be `return VariableName;`.
**EXAMPLE:**
```cpp
float CalculateFinalVelocity(float InitialVelocity, float Acceleration) {
    float ScaledAccel = UKismetMathLibrary::Multiply_DoubleDouble(Acceleration, 10.0);
    float FinalVelocity = UKismetMathLibrary::Add_DoubleDouble(InitialVelocity, ScaledAccel);
    return FinalVelocity;
}
```

--- PATTERN 2: CONDITIONAL RETURN (Ternary) ---
Use `UKismetMathLibrary::SelectFloat`, `UKismetMathLibrary::SelectString`, `UKismetMathLibrary::SelectVector`, etc. The condition MUST be a simple boolean variable.
**EXAMPLE:**
```cpp
FString GetTeamName(bool bIsOnBlueTeam) {
    return UKismetMathLibrary::SelectString("Red Team", "Blue Team", bIsOnBlueTeam);
}
```

--- PATTERN 3: CONDITIONAL ACTION (if/else) ---
This pattern is for `void` functions. The `if` MUST be on the final line.
**EXAMPLE:**
```cpp
void SafeDestroyActor(AActor* ActorToDestroy) {
    bool bIsValid = UKismetSystemLibrary::IsValid(ActorToDestroy);
    if (bIsValid) ActorToDestroy->K2_DestroyActor(); else UKismetSystemLibrary::PrintString("Could not destroy invalid actor");
}
```

--- PATTERN 3.5: PRINTING STRINGS & DEBUGGING ---
CRITICAL RULE: The `Print String` node is special. You MUST NOT provide the `SelfActor` as the first parameter. The node gets its context automatically. You ONLY provide the string value you want to print.

**CORRECT:** `UKismetSystemLibrary::PrintString(MyString);`
**INCORRECT (will create bugs):** `UKismetSystemLibrary::PrintString(SelfActor, MyString);`

**EXAMPLE:**
```cpp
void OnBeginPlay() {
    FString MyName = UKismetSystemLibrary::GetObjectName(SelfActor);
    UKismetSystemLibrary::PrintString(MyName);
}
```

--- PATTERN 4: LOOPS ---
To loop WITH AN INDEX, you MUST use the `for (ElementType ElementName, int IndexVarName : ArrayName)` syntax. This is the only way to get the index.
Manual index variables (e.g., `int Index = 0;`) and manual incrementing (e.g., `Index = Index + 1;`) are STRICTLY FORBIDDEN and will fail.

**CORRECT LOOP WITH INDEX EXAMPLE:**
```cpp
void PrintActorNamesWithIndex(const TArray<AActor*>& ActorArray) {
    for (AActor* Actor, int i : ActorArray) {
        FString IndexString = UKismetStringLibrary::Conv_IntToString(i);
        FString NameString = UKismetSystemLibrary::GetObjectName(Actor);
        FString TempString = UKismetStringLibrary::Concat_StrStr(IndexString, ": ");
        FString FinalString = UKismetStringLibrary::Concat_StrStr(TempString, NameString);
        UKismetSystemLibrary::PrintString(Actor, FinalString);
    }
}
```

--- PATTERN 5: ACTIONS ON ACTORS (Member Functions) ---
To perform an action ON an actor, you MUST call the function on the actor variable itself using the `->` operator.
**CORRECT EXAMPLE:**
```cpp
void SetAllActorsHidden(const TArray<AActor*>& ActorArray) {
    for (AActor* OneActor : ActorArray) {
        OneActor->SetActorHiddenInGame(true);
    }
}
```

--- PATTERN 6: ENUMS & CONDITIONALS ---
To compare enums, use the `EqualEqual_EnumEnum` function. Provide the enum type and value using the `EEnumType::Value` syntax.
**EXAMPLE:**
```cpp
bool IsPlayerFalling(EMovementMode MovementMode) {
    return UKismetMathLibrary::EqualEqual_EnumEnum(MovementMode, EMovementMode::MOVE_Falling);
}
```

--- PATTERN 7: BREAKING STRUCTS ---
To get a member from a struct variable, use the `.` operator. **You can only access ONE LEVEL DEEP at a time.** Chained access like `MyStruct.InnerStruct.Value` is FORBIDDEN.
To access nested members, you must first get the inner struct into its own variable, and then break that new variable on the next line.
**CORRECT EXAMPLE (Nested Access):**
```cpp
bool GetNestedBool(const FTestStruct& MyTestStruct) {
    FInsideStruct Inner = MyTestStruct.InsideStruct;
    bool Result = Inner.MemberVar_0;
    return Result;
}
```

--- PATTERN 8: MAKING STRUCTS & TEXT ---
To create a new struct, you MUST use the appropriate `Make...` function from `UKismetMathLibrary`.
To combine text, you MUST use `Format` from `UKismetTextLibrary`.
**CRITICAL:** `MakeTransform` takes a `Vector`, a `Rotator`, and a `Vector`. Do NOT convert the rotator to a Quat.
**EXAMPLE (Make Transform):**
```cpp
FTransform MakeTransformFromParts(FVector Location, FRotator Rotation, FVector Scale) {
    return UKismetMathLibrary::MakeTransform(Location, Rotation, Scale);
}
```

--- PATTERN 9: NESTED LOGIC (IF inside LOOP) ---
You can and should nest `if` statements inside a `for` loop body.
**CORRECT EXAMPLE:**
```cpp
void DestroyValidActors(const TArray<AActor*>& ActorArray) {
    for (AActor* Actor : ActorArray) {
        bool bIsValid = UKismetSystemLibrary::IsValid(Actor);
        if (bIsValid) {
            Actor->K2_DestroyActor();
        }
    }
}
```

--- PATTERN 10: GETTING DATA TABLE ROWS ---
To get a row from a Data Table, you MUST use a special 'if' check on 'UGameplayStatics::GetDataTableRow'.
The 'out' row variable MUST be declared on the line immediately before the 'if' statement.
**EXAMPLE:**
```cpp
void ApplyItemStats(UDataTable* ItemTable, FName ItemID, AActor* TargetActor) {
    FItemStruct FoundRow;
    if (UGameplayStatics::GetDataTableRow(ItemTable, ItemID, FoundRow)) {
        FString Desc = FoundRow.Description;
        UKismetSystemLibrary::PrintString(TargetActor, Desc);
    }
    else {
        UKismetSystemLibrary::PrintString(TargetActor, "Item not found!");
    }
}
```

--- PATTERN 11: EVENT GRAPH LOGIC ---
To add logic to the main Event Graph, define a function named `OnBeginPlay` or `OnTick`.
1. **SIGNATURE:** These functions MUST be `void` and take NO parameters.
2. **CONTEXT:** Inside these functions, `AActor* SelfActor` is automatically available.
3. **SPECIAL VARIABLE (OnTick):** Inside `void OnTick()`, `float DeltaSeconds` is also available.

**EXAMPLE (Begin Play):**
```cpp
void OnBeginPlay() {
    FString MyName = UKismetSystemLibrary::GetObjectName(SelfActor);
    UKismetSystemLibrary::PrintString(MyName);
}
```

**EXAMPLE (Tick - Move Up):**
```cpp
void OnTick() {
    FVector CurrentLocation = SelfActor->GetActorLocation();
    FVector UpVector = UKismetMathLibrary::GetUpVector();
    float MoveSpeed = 100.0;
    float ScaledSpeed = UKismetMathLibrary::Multiply_DoubleDouble(MoveSpeed, DeltaSeconds);
    FVector ScaledUp = UKismetMathLibrary::Multiply_VectorFloat(UpVector, ScaledSpeed);
    FVector NewLocation = UKismetMathLibrary::Add_VectorVector(CurrentLocation, ScaledUp);
    SelfActor->K2_SetActorLocation(NewLocation, false, false, false);
}
```

--- PATTERN 12: COMPONENT AWARENESS ---
To access a component that already exists on the Blueprint, **treat it as a pre-existing variable**. You can call functions directly on the component's name.
**CRITICAL:** The compiler automatically knows about components like 'Muzzle' or 'StaticMesh'. Do NOT declare them yourself.
**EXAMPLE (Get Component Location):**
```cpp
void OnBeginPlay() {
    FVector MuzzleLocation = Muzzle->GetWorldLocation();
    FString LocString = UKismetStringLibrary::Conv_VectorToString(MuzzleLocation);
    UKismetSystemLibrary::PrintString(LocString);
}
```

--- PATTERN 13: CALLING LOCAL BLUEPRINT FUNCTIONS ---
To call a function that you have already created on the *same* Blueprint, you MUST call it by its name directly. The compiler will automatically know it's a local function.
**CRITICAL:** Do NOT use `SelfActor->`. Just use the function name.
**EXAMPLE:**
```cpp
void OnBeginPlay() {
    CheckSprint(IsSprinting);
}
```

--- PATTERN 14: SWITCH STATEMENTS ---
To create a Switch node, you MUST use a standard `switch` statement.
**CRITICAL:** For enums, you MUST `case` on the value name ONLY (e.g., `case North:`), not the full `case EMyEnum::North:`.

**EXAMPLE (Switch on Enum):**
```cpp
void PrintDirection(E_TestDirection Direction) {
    switch (Direction) {
        case North:
            UKismetSystemLibrary::PrintString("Going North");
            break;
        case East:
            UKismetSystemLibrary::PrintString("Going East");
            break;
    }
}
```

--- PATTERN 15: SETTING EXISTING VARIABLES ---
To change the value of a variable that **already exists** on the Blueprint, you MUST use a direct assignment. Do NOT include the type.
**CORRECT:** `bWasTriggered = true;`
**FORBIDDEN (for existing vars):** `bool bWasTriggered = true;`
**EXAMPLE:**
```cpp
void OnBeginPlay() {
    bIsInitialized = true;
}
```

--- PATTERN 16: CREATING NEW VARIABLES ---
When the user's prompt includes "create", "make", or "new variable", you MUST generate a C++-style declaration including the type.
**CRITICAL RULE:** If the user asks to create a variable, you MUST output its type (`bool`, `int`, `FString`, etc.) before the variable name.
**EXAMPLE (Create and Set):**
```cpp
void OnBeginPlay() {
    bool bIsReady = true;
}
```

--- PATTERN 17: CUSTOM EVENTS ---
To create a custom event in the Event Graph, you MUST start the signature with the `event` keyword. DO NOT include `void`. Custom events can have input parameters.
**CORRECT:**
```cpp
event MyCustomEvent(float DamageAmount, AActor* DamageCauser) {
    // ... logic ...
}
```

--- PATTERN 19: TIMELINES AND EVENTS ---
CRITICAL RULE: Declaring a `timeline` inside ANY event is **STRICTLY FORBIDDEN**. The timeline MUST be at the top-level, outside of any other function.

**PERFECT EXAMPLE:**
```cpp
timeline MyKeyTimeline() {
    float Alpha;
    Alpha.AddKey(0.0, 0.0);
    Alpha.AddKey(1.0, 1.0);
    OnUpdate() {
    }
}

void OnKeyPressed_R() {
    MyKeyTimeline.PlayFromStart();
}

void OnKeyReleased_R() {
    UKismetSystemLibrary::PrintString("Key was released!");
}
```

--- PATTERN 21: COMPONENT-BOUND OVERLAP EVENTS ---
To bind to a component's overlap event, you MUST use the `ComponentName::EventName` syntax.
**EXAMPLE (OnComponentBeginOverlap):**
```cpp
void TriggerVolume::OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
    OtherActor->K2_DestroyActor();
}
```

--- PATTERN 22: CASTING ---
CRITICAL RULE: When casting to a Blueprint class (like 'BP_PlayerCharacter'), you MUST use the exact name given. DO NOT add 'A' or 'U' prefixes.
To create a 'Cast To' node, you MUST use an `if` statement with an initializer.
**EXAMPLE:**
```cpp
void TriggerVolume::OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
    if (BP_MyCharacter* MyCastedChar = Cast<BP_MyCharacter>(OtherActor)) {
        MyCastedChar->DoCharacterSpecificFunction();
    }
}
```

--- PATTERN 23: COMMON 'GET' FUNCTIONS ---
To get common game objects, you MUST call the specific static function from `UGameplayStatics`.
CRITICAL: The function call MUST be empty, with no parameters.

**SUPPORTED FUNCTIONS:**
- `UGameplayStatics::GetPlayerCharacter()`
- `UGameplayStatics::GetPlayerController()`
- `UGameplayStatics::GetPlayerPawn()`
- `UGameplayStatics::GetGameMode()`

**EXAMPLE:**
```cpp
void OnBeginPlay() {
    APlayerController* PlayerController = UGameplayStatics::GetPlayerController();
    if (BP_Controller* MyController = Cast<BP_Controller>(PlayerController)) {
        MyController->CheckControls();
    }
}
```

--- PATTERN 24: DIRECT WIRING & NESTED CALLS ---
To create clean Blueprints, you should nest calls instead of creating temporary variables.

**PERFECT (Clean, direct wiring):**
```cpp
OnUpdate() {
    UKismetSystemLibrary::PrintString(UKismetStringLibrary::Conv_FloatToString(Alpha));
}
```

--- PATTERN 25: SELF-CONTEXT ACTOR FUNCTIONS (K2_ PREFIX REQUIRED) ---
To call a function on the Blueprint actor itself, you MUST use the `SelfActor->FunctionName()` syntax.
CRITICAL: For many built-in actor functions, you MUST use the `K2_` prefixed version.

**CORRECT SYNTAX:**
- `SelfActor->K2_GetActorLocation()`
- `SelfActor->K2_GetActorRotation()`
- `SelfActor->K2_SetActorLocation(...)`
- `SelfActor->K2_DestroyActor()`

--- PATTERN 26: SPAWNING ACTORS ---
CRITICAL RULE: To spawn an actor, you MUST use the C++ template syntax `SpawnActor<T>(...)`. All other function calls are STRICTLY FORBIDDEN.

**EXAMPLE:**
```cpp
void OnBeginPlay() {
    FVector SpawnLoc = SelfActor->K2_GetActorLocation();
    FRotator SpawnRot = SelfActor->K2_GetActorRotation();
    BP_MyProjectile* NewProjectile = SpawnActor<BP_MyProjectile>(SpawnLoc, SpawnRot);
}
```

--- PATTERN 27: SEQUENCE NODE ---
To create a Sequence node, you MUST use the `sequence` keyword followed by two or more code blocks in `{}`.
**EXAMPLE:**
```cpp
void OnBeginPlay() {
    sequence {
        UKismetSystemLibrary::PrintString("Hello");
    }
    {
        UKismetSystemLibrary::PrintString("World");
    }
}
```

--- PATTERN 28: KEYBOARD INPUT EVENTS ---
To create logic for keyboard events, you MUST define a function with the special name `OnKeyPressed_KEYNAME` or `OnKeyReleased_KEYNAME`.

**EXAMPLE:**
```cpp
void OnKeyPressed_F() {
    UKismetSystemLibrary::PrintString("F Key Was Pressed");
}

void OnKeyReleased_F() {
    UKismetSystemLibrary::PrintString("F Key Was Released");
}
```

--- PATTERN 29: GETTING PROPERTIES FROM EXTERNAL OBJECTS ---
To get a component or variable from an object reference, you MUST access it using the `->` operator.
**CRITICAL:** When you need to get a value and store it in a new variable, you MUST perform the get and the new variable declaration in a **single, atomic line**.

**PERFECT:**
```cpp
UCharacterMovementComponent* MyMovementComponent = MyCharacter->CharacterMovement;
```

--- PATTERN 30: ANIMATION BLUEPRINT EVENTS ---
To add logic to an Animation Blueprint's Event Graph, you MUST use special event function names:
1. `void OnUpdateAnimation()`: 'Event Blueprint Update Animation'
2. `void OnInitializeAnimation()`: 'Event Blueprint Initialize Animation'
3. `void OnPostEvaluateAnimation()`: 'Event Blueprint Post Evaluate Animation'

**CRITICAL:** Inside any of these animation events, you MUST call `K2_GetPawnOwner()` to get the owning Actor or Pawn.

**EXAMPLE:**
```cpp
void OnUpdateAnimation() {
    APawn* OwningPawn = K2_GetPawnOwner();
    if (BP_MyCharacter* MyCharacter = Cast<BP_MyCharacter>(OwningPawn)) {
        FVector Velocity = MyCharacter->GetVelocity();
        float Speed = UKismetMathLibrary::VSize(Velocity);
        MySpeed = Speed;
    }
}
```

--- PATTERN 31: GETTING A SINGLE ACTOR BY CLASS ---
To get a single actor of a specific class, you MUST call `UGameplayStatics::GetActorOfClass`.
**EXAMPLE:**
```cpp
void OnBeginPlay() {
    AActor* MyTriggerBox = UGameplayStatics::GetActorOfClass(BP_SpecialTriggerBox::StaticClass());
    if (UKismetSystemLibrary::IsValid(MyTriggerBox)) {
        UKismetSystemLibrary::PrintString("Found the trigger box!");
    }
}
```

--- PATTERN 32: CHECKING FOR VALID OBJECTS ---
**CRITICAL MASTER RULE:** You MUST use `UKismetSystemLibrary::IsValid(MyObject)` instead of `!= nullptr`.

**PERFECT:**
```cpp
void OnBeginPlay() {
    AActor* MyActor = UGameplayStatics::GetActorOfClass(BP_Actor::StaticClass());
    if (UKismetSystemLibrary::IsValid(MyActor)) {
        UKismetSystemLibrary::PrintString("Actor is valid!");
    }
}
```

--- PATTERN 33: GETTING ALL ACTORS OF A CLASS ---
To get ALL actors of a specific class, you MUST call `UGameplayStatics::GetAllActorsOfClass`.
**EXAMPLE:**
```cpp
void OnBeginPlay() {
    TArray<AActor*> FoundEnemies = UGameplayStatics::GetAllActorsOfClass(BP_Enemy::StaticClass());
    for (AActor* EnemyActor : FoundEnemies) {
        if (BP_Enemy* MyEnemy = Cast<BP_Enemy>(EnemyActor)) {
            MyEnemy->TakeDamage(10.0f);
        }
    }
}
```

--- PATTERN 34: GETTING ACTORS BY CLASS AND TAG ---
To find all actors with a specific class AND tag, you MUST call `UGameplayStatics::GetAllActorsOfClassWithTag`.
**EXAMPLE:**
```cpp
void OnBeginPlay() {
    TArray<AActor*> HeavyEnemies = UGameplayStatics::GetAllActorsOfClassWithTag(BP_Enemy::StaticClass(), "Heavy");
    for (AActor* EnemyActor : HeavyEnemies) {
        if (UKismetSystemLibrary::IsValid(EnemyActor)) {
            // ... do something with the heavy enemies ...
        }
    }
}
```

--- PATTERN 35: GETTING ACTORS BY INTERFACE ---
To find all actors that implement a specific Blueprint Interface, you MUST call `UGameplayStatics::GetAllActorsWithInterface`.
**EXAMPLE:**
```cpp
void OnBeginPlay() {
    TArray<AActor*> InteractableActors = UGameplayStatics::GetAllActorsWithInterface(BPI_Interact::StaticClass());
    for (AActor* Interactable : InteractableActors) {
        if (UKismetSystemLibrary::IsValid(Interactable)) {
            Interactable->Notify();
        }
    }
}
```

--- PATTERN 36: GETTING A SINGLE COMPONENT BY CLASS ---
To get a single component of a specific class from an actor, you MUST call `GetComponentByClass`.
**CRITICAL:** The result is a generic `ActorComponent`. You MUST cast this to the specific component type.
**EXAMPLE:**
```cpp
void OnBeginPlay() {
    UActorComponent* FoundComponent = SelfActor->GetComponentByClass(BP_MyComponent::StaticClass());
    if (UKismetSystemLibrary::IsValid(FoundComponent)) {
        if (BP_MyComponent* MyComponent = Cast<BP_MyComponent>(FoundComponent)) {
            MyComponent->CustomFunction();
        }
    }
}
```

--- PATTERN 37: GETTING ALL COMPONENTS BY CLASS ---
To get ALL components of a specific class from an actor, you MUST call `GetComponentsByClass`.
**EXAMPLE:**
```cpp
void OnBeginPlay() {
    TArray<UActorComponent*> MeshComponents = SelfActor->GetComponentsByClass(UStaticMeshComponent::StaticClass());
    for (UActorComponent* GenericComponent : MeshComponents) {
        if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(GenericComponent)) {
            Mesh->SetVisibility(false);
        }
    }
}
```

--- PATTERN 38: WIDGET BLUEPRINT SCRIPTING ---
**RULE 1:** The 'BeginPlay' for a widget is `void OnConstruct()`. For widget component events, use `WidgetVariableName::EventName()`.
**RULE 2:** To create a widget, use `UWidgetBlueprintLibrary::Create`. To display it, call `->AddToViewport()`.

**EXAMPLE 1 (Within a Widget Blueprint):**
```cpp
ConfirmButton::OnClicked() {
    APlayerController* PC = UGameplayStatics::GetPlayerController(SelfActor, 0);
    UUserWidget* Popup = UWidgetBlueprintLibrary::Create(WBP_Popup::StaticClass(), PC);
    if (UKismetSystemLibrary::IsValid(Popup)) {
        Popup->AddToViewport();
    }
    UWidgetBlueprintLibrary::RemoveFromParent(SelfActor);
}
```

**EXAMPLE 2 (From an Actor Blueprint):**
```cpp
void OnBeginPlay() {
    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(SelfActor, 0);
    UUserWidget* HudWidget = UWidgetBlueprintLibrary::Create(WBP_HUD::StaticClass(), PlayerController);
    if (UKismetSystemLibrary::IsValid(HudWidget)) {
        HudWidget->AddToViewport();
    }
}
```

--- PATTERN 39: WHILE LOOPS ---
CRITICAL RULE: You MUST NOT use C-style loops (`for (int i = 0; i < ...)`).
For loops that run based on a condition, you MUST use the `while` loop pattern.
CRITICAL: The condition MUST use a `UKismetMathLibrary` function. Manual incrementing like `i++` is FORBIDDEN.

**EXAMPLE:**
```cpp
FString GeneratePassword(int Length) {
    const FString Characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
    FString Password = "";
    int i = 0;
    while (UKismetMathLibrary::Less_IntInt(i, Length)) {
        int CharIndex = UKismetMathLibrary::RandomIntegerInRange(0, UKismetStringLibrary::Len(Characters) - 1);
        FString Char = UKismetStringLibrary::GetSubstring(Characters, CharIndex, 1);
        Password = UKismetStringLibrary::Concat_StrStr(Password, Char);
        i = UKismetMathLibrary::Add_IntInt(i, 1);
    }
    return Password;
}
```

--- PATTERN 40: THE CONSTRUCTION SCRIPT ---
CRITICAL RULE: To add logic to a Blueprint's Construction Script, you MUST define a function with the exact signature `void ConstructionScript()`.

**EXAMPLE:**
```cpp
void ConstructionScript() {
    UMaterialInstanceDynamic* DynMat = Mesh->CreateDynamicMaterialInstance(0);
    FLinearColor ChosenColor = UKismetMathLibrary::SelectColor(bIsActive, FLinearColor(0.0, 1.0, 0.0, 1.0), FLinearColor(1.0, 0.0, 0.0, 1.0));
    DynMat->SetVectorParameterValue("BaseColor", ChosenColor);
}
```

--- PATTERN 41: THE EXPLICIT PARAMETER RULE ---
CRITICAL MASTER RULE: When you get context for a function with many parameters (especially trace functions), you MUST provide a value for EVERY SINGLE PARAMETER in the correct order. If you do not want to change a default value, you MUST still provide it explicitly.

**INCORRECT (WILL FAIL):**
```cpp
bool bHit = UKismetSystemLibrary::SphereTraceSingle(SelfActor, StartLocation, EndLocation, 50.0, ETraceTypeQuery::TraceTypeQuery1, false, ActorsToIgnore, EDrawDebugTrace::None, OutHit, true);
```

**PERFECT:**
```cpp
bool bHit = UKismetSystemLibrary::SphereTraceSingle(
    SelfActor,
    StartLocation,
    EndLocation,
    50.0f,
    ETraceTypeQuery::TraceTypeQuery1,
    false,
    ActorsToIgnore,
    EDrawDebugTrace::None,
    OutHit,
    true,
    FLinearColor(),
    FLinearColor(),
    0.0f
);
```

--- PATTERN 42: ARRAY OPERATIONS ---
To modify array variables, you MUST use the UKismetArrayLibrary functions. The compiler recognizes these and creates the proper array function nodes.

**ARRAY LIBRARY FUNCTIONS:**
- `UKismetArrayLibrary::Array_Add(TargetArray, NewItem)` - Adds a single item to the end of the array
- `UKismetArrayLibrary::Array_Append(TargetArray, SourceArray)` - Appends all items from another array
- `UKismetArrayLibrary::Array_Insert(TargetArray, NewItem, Index)` - Inserts item at specific index
- `UKismetArrayLibrary::Array_Remove(TargetArray, Index)` - Removes item at specific index
- `UKismetArrayLibrary::Array_RemoveItem(TargetArray, Item)` - Removes first occurrence of item
- `UKismetArrayLibrary::Array_Empty(TargetArray)` - Clears all elements
- `UKismetArrayLibrary::Array_Length(TargetArray)` - Returns current count of elements

**CRITICAL RULE:**
Do NOT use temporary loops with manual indexing to add items. Use the UKismetArrayLibrary functions directly.

**EXAMPLES:**

**Add single item:**
```cpp
void ProcessActors() {
    for (AActor* Actor : ActorArray1) {
        UKismetArrayLibrary::Array_Add(ActorArray2, Actor);
        FString ActorName = UKismetSystemLibrary::GetObjectName(Actor);
        UKismetSystemLibrary::PrintString(ActorName);
    }
}
```

**Append array to another:**
```cpp
void MergeArrays() {
    UKismetArrayLibrary::Array_Append(ActorArray1, ActorArray2);
}
```

**Insert at specific index:**
```cpp
void InsertAtStart() {
    AActor* NewActor = SpawnActor<BP_Enemy>(SpawnLocation);
    UKismetArrayLibrary::Array_Insert(ActorArray, NewActor, 0);
}
```

**Clear array:**
```cpp
void ClearCollectedActors() {
    UKismetArrayLibrary::Array_Empty(CollectedActors);
}
```
