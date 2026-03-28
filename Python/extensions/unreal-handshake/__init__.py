# Copyright 2026, BlueprintsLab, All rights reserved
# ========================================================================================
# mcp_main_server.py (Complete and Final Version)
# ========================================================================================

from pathlib import Path
import socket
import json
import sys
import re
import random
import argparse

from mcp.server.fastmcp import FastMCP

mcp = FastMCP("unreal-handshake")


def _read_file_with_fallback(file_path: Path, is_json: bool = False):
    """
    Internal helper to read a file with UTF-8, falling back to UTF-16-LE.
    Handles both plain text and JSON parsing.
    Uses utf-8-sig to automatically handle UTF-8 BOM (Byte Order Mark).
    """
    try:
        # Use utf-8-sig which automatically strips UTF-8 BOM if present
        with open(file_path, 'r', encoding='utf-8-sig') as f:
            content = f.read()
            # Additional safety: strip any remaining BOM character
            if content.startswith('\ufeff'):
                content = content[1:]
            return json.loads(content) if is_json else content
    except UnicodeDecodeError:
        # If UTF-8 fails, log a warning and try UTF-16-LE
        print(f"MCP Warning: Could not decode {file_path} as UTF-8. Retrying with UTF-16-LE.", file=sys.stderr)
        with open(file_path, 'r', encoding='utf-16-le') as f:
            content = f.read()
            # Strip UTF-16 LE BOM if present
            if content.startswith('\ufeff'):
                content = content[1:]
            return json.loads(content) if is_json else content
    # Allow other exceptions like FileNotFoundError to be handled by the caller.

def get_project_root() -> Path | None:
    """
    Asks the Unreal Editor directly for the project root path.
    This is the most reliable method and works regardless of where the plugin is installed.
    """
    print("MCP: Asking Unreal Editor for project root path...", file=sys.stderr)
    command = {"type": "get_project_root_path"}
    response = send_to_unreal(command)

    if response and response.get("success"):
        project_file_path_str = response.get("path")
        if project_file_path_str:
            project_file_path = Path(project_file_path_str)
            project_root = project_file_path.parent
            print(f"MCP: Received project root from Unreal: {project_root}", file=sys.stderr)
            return project_root

    print("MCP: ERROR - Failed to get project root path from Unreal Editor.", file=sys.stderr)
    return None

@mcp.tool()
def how_to_use() -> str:
    """
    Call this first in a new session. Returns base instructions for using the Unreal Editor tools.
    MANDATORY: Always call classify_intent(user_message) FIRST before any other tool to determine the correct workflow.
    """
    try:
        current_dir = Path(__file__).parent
        md_file_path = current_dir / "how_to_use_base.md"
        if not md_file_path.exists():
            return "Error: The instruction file 'how_to_use_base.md' was not found."
        return _read_file_with_fallback(md_file_path, is_json=False)
    except Exception as e:
        return f"Error reading instruction file: {str(e)}"

@mcp.tool()
def get_code_generation_patterns() -> str:
    """
    Returns the Blueprint code generation patterns (PATTERN 1-41).
    ONLY call this after classify_intent returns "CODE". Do NOT call for TEXTURE/MODEL/CHAT intents.
    These patterns teach you the correct syntax for UE5 Blueprint code generation.
    """
    try:
        current_dir = Path(__file__).parent
        patterns_file_path = current_dir / "how_to_use_patterns.md"
        if not patterns_file_path.exists():
            return "Error: The patterns file 'how_to_use_patterns.md' was not found."
        return _read_file_with_fallback(patterns_file_path, is_json=False)
    except Exception as e:
        return f"Error reading patterns file: {str(e)}"

def send_to_unreal(command):
    """Send a command to Unreal Engine MCP server, trying multiple ports if needed."""
    base_port = 9877
    max_attempts = 10  # Try ports 9877-9886

    for port_offset in range(max_attempts):
        port = base_port + port_offset
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.settimeout(5.0)  # 5 second timeout
                s.connect(('localhost', port))
                json_str = json.dumps(command)
                s.sendall(json_str.encode('utf-8'))

                response_data = b""
                while True:
                    chunk = s.recv(4096)
                    if not chunk:
                        break
                    response_data += chunk
                    try:
                        return json.loads(response_data.decode('utf-8'))
                    except json.JSONDecodeError:
                        continue
                return {"success": False, "error": "Incomplete or empty response from Unreal"}

        except ConnectionRefusedError:
            # Port is not listening, try next port
            if port_offset < max_attempts - 1:
                continue
            return {"success": False, "error": "Connection refused. Is Unreal Editor running and the MCP Subsystem active? (Tried ports 9877-9886)"}
        except socket.timeout:
            return {"success": False, "error": f"Connection timed out on port {port}"}
        except Exception as e:
            return {"success": False, "error": str(e)}

    return {"success": False, "error": "Could not connect to Unreal MCP server on any port (9877-9886)"}
        
@mcp.tool()
def check_request_feasibility(user_prompt: str) -> str:
    """
    Call this AFTER classify_intent returns "CODE" to check if the request is within capabilities.
    It checks if the user's request is within the known capabilities of the code generator.

    Args:
        user_prompt (str): The user's original, natural language request.

    Returns:
        str: A success message if the request is feasible, or a specific error message explaining why it is not.
    """
    try:
        current_dir = Path(__file__).parent
        capabilities_path = current_dir / "capabilities.json"
        if not capabilities_path.exists():
            return "Feasibility Check Passed: Warning - The capabilities.json file was not found, so the check could not be performed."

        capabilities = _read_file_with_fallback(capabilities_path, is_json=True)

    except Exception as e:
        return f"Feasibility Check Passed: Warning - Could not read capabilities file: {str(e)}"

    prompt_lower = user_prompt.lower()
    unsupported_topics = capabilities.get("unsupported_topics", [])

    for topic in unsupported_topics:
        for keyword in topic.get("keywords", []):
            # Use regex for whole-word matching to avoid partial matches (e.g., 'anim' in 'animal')
            if re.search(r'\b' + re.escape(keyword) + r'\b', prompt_lower):
                print(f"MCP: Feasibility check failed. Detected unsupported keyword: '{keyword}'", file=sys.stderr)
                # This specific prefix helps the AI know for sure that it should stop.
                return f"Feasibility Check Failed: {topic.get('message', 'This request involves unsupported functionality.')}"

    return "Feasibility Check Passed: This request appears to be within my capabilities. You may now proceed with the standard code generation workflow (get_api_context -> generate_blueprint_logic)."

@mcp.tool()
def classify_intent(user_message: str) -> str:
    """
    *** MANDATORY FIRST TOOL - ALWAYS CALL THIS BEFORE ANY OTHER TOOL ***

    Classifies the user's intent using AI to determine if they want code generation, asset creation,
    3D model generation, texture/material generation, or general chat. This determines which workflow to follow.

    Returns "CODE", "ASSET", "TEXTURE", "MODEL", or "CHAT":
    - CODE: Blueprint code/logic generation (load patterns, generate code)
    - ASSET: Create new assets like Input Actions, Input Mapping Contexts, Actors, Widgets, Data Tables
    - TEXTURE: Texture/material generation
    - MODEL: 3D model/mesh generation
    - CHAT: General conversation/questions

    Only load code patterns if intent is "CODE".

    Args:
        user_message (str): The user's original message/request.

    Returns:
        str: Intent classification with recommendation on what tools to call next.
    """
    command = {"type": "classify_intent", "user_message": user_message}
    response = send_to_unreal(command)
    if response and response.get("success"):
        intent = response.get("intent", "CHAT")
        needs_patterns = response.get("needs_patterns", False)
        recommendation = response.get("recommendation", "")

        result = f"Intent Classification: {intent}\n"
        result += f"Needs Code Patterns: {needs_patterns}\n"
        result += f"Recommendation: {recommendation}"
        return result
    else:
        # Fallback to CHAT if classification fails
        return "Intent Classification: CHAT (fallback - classification failed)\nNeeds Code Patterns: false\nRecommendation: Proceed with conversation."

def get_context_path(blueprint_path: str = None) -> tuple[str, str]:
    """Helper function to determine the target Blueprint path."""
    if blueprint_path and blueprint_path.strip():
        return blueprint_path, None

    print("MCP: No explicit path provided. Asking Unreal for selected asset...", file=sys.stderr)
    context_command = {"type": "get_selected_blueprint_path"}
    context_response = send_to_unreal(context_command)
    
    if not context_response or not context_response.get("success"):
        return None, f"Error getting context: {context_response.get('error', 'Unknown error')}"
    
    path = context_response.get("path")
    if not path:
        return None, "Error: No Blueprint path was provided, and no Blueprint is currently selected in the Content Browser."
        
    return path, None


@mcp.tool()
def get_api_context(query: str) -> str:
    """
    Searches a local database of Unreal Engine API functions to find context relevant to a user's question.
    The AI MUST call this tool BEFORE attempting to generate code for a non-trivial request to ensure it uses
    correct class and function names.

    Args:
        query (str): A simple, keyword-focused question. Examples: "get actor location", "character jump", "hide component".

    Returns:
        str: A formatted string with the top 3-5 most relevant function signatures, or a message if no results are found.
    """
    try:
        current_dir = Path(__file__).parent
        cheatsheet_path = current_dir / "api_cheatsheet.json"
        
        if not cheatsheet_path.exists():
            return "Error: The API cheatsheet 'api_cheatsheet.json' was not found. The scraping process may need to be run."

        api_data = _read_file_with_fallback(cheatsheet_path, is_json=True)
            
    except Exception as e:
        return f"Error reading API cheatsheet: {str(e)}"

    keywords = set(re.findall(r'\b\w+\b', query.lower()))
    if not keywords:
        return "No valid keywords provided in the query."

    scored_functions = []
    for func in api_data:
        score = 0
        # Combine all relevant text into a single string for searching
        search_text = f"{func['full_signature']} {func['return_type']}".lower()
        for param in func.get('parameters', []):
            search_text += f" {param['type']} {param['name']}"

        for keyword in keywords:
            if keyword in search_text:
                score += 1
                # Boost score for matches in the function name itself
                if keyword in func['function_name'].lower():
                    score += 2
        
        if score > 0:
            scored_functions.append({"score": score, "data": func})

    if not scored_functions:
        return "No relevant API functions found for that query. The function may be in a class that hasn't been scraped yet."

    # Sort by score and get the top 5
    scored_functions.sort(key=lambda x: x['score'], reverse=True)
    top_functions = scored_functions[:5]

    # Format the output for the AI to easily understand
    context = "--- RELEVANT API CONTEXT ---\n\n"
    for item in top_functions:
        func_data = item['data']
        context += f"Function: {func_data['full_signature']}\n"
        context += f"Returns: {func_data['return_type']}\n"
        
        if func_data['parameters']:
            context += "Parameters:\n"
            for param in func_data['parameters']:
                context += f"  - {param['type']} {param['name']}\n"
        else:
            context += "Parameters: None\n"
        context += "\n"
        
    return context

@mcp.tool()
def get_data_asset_details(asset_path: str) -> str:
    """
    Gets the internal members of a data asset (User Defined Enum or User Defined Struct).
    The AI MUST call this tool before generating code that uses an Enum or Struct to ensure it uses the
    correct enumerator names or member variable names, preventing "hallucination" errors.

    Args:
        asset_path (str): The full asset path of the Enum or Struct to inspect (e.g., "/Game/Enums/E_MyEnum.E_MyEnum").

    Returns:
        str: A JSON string detailing the asset's members.
             For Enums: {"type": "Enum", "values": ["Idle", "Ready", "Attacking"]}
             For Structs: {"type": "Struct", "members": [{"name": "Duration", "type": "float"}, {"name": "Damage", "type": "float"}]}
             Returns an error message on failure.
    """
    command = {
        "type": "get_data_asset_details",
        "asset_path": asset_path
    }
    response = send_to_unreal(command)
    if response and response.get("success"):
        return response.get("details", "Error: Tool succeeded but returned no details.")
    else:
        return f"Failed to get data asset details: {response.get('error', 'Unknown error.')}"

@mcp.tool()
def insert_blueprint_code_at_selection(code: str, blueprint_path: str = None) -> str:
    """
    Inserts a raw code snippet immediately after the single selected node in the active Blueprint Editor.
    This is for adding to existing logic. The AI is responsible for generating the code snippet.

    IMPORTANT: You MUST call get_code_generation_patterns() before using this tool to ensure correct syntax.

    Args:
        code (str): The raw C++ style code snippet to be inserted. CRITICAL: This snippet MUST NOT have a
                    function signature (e.g., "void MyFunction() { ... }"). It should only be the body of the logic.
        blueprint_path (str, optional): The path to the target Blueprint. If omitted, uses the selected asset.

    Returns:
        str: A message indicating success or failure.
    """
    path, error = get_context_path(blueprint_path)
    if error:
        return f"Error: {error}"

    command = {
        "type": "insert_code",
        "blueprint_path": path,
        "code": code
    }
    response = send_to_unreal(command)
    return response.get('message', f"Failed: {response.get('error')}") if response and response.get("success") else f"Failed: {response.get('error')}"


@mcp.tool()
def get_asset_summary(asset_path: str = None) -> str:
    """
    Gets a detailed, raw text summary of a single asset. This is a universal tool that automatically detects
    the asset type and works for Blueprints, Materials, and Behavior Trees. If asset_path is omitted,
    it uses the currently selected asset in the Content Browser. The AI assistant is responsible for formatting
    the raw output into a clean, user-friendly explanation.

    Args:
        asset_path (str, optional): The full asset path (e.g., "/Game/AI/BT_Enemy.BT_Enemy").
                                    If omitted, uses the selected asset.

    Returns:
        str: A raw text summary of the asset's contents, or an error message if the asset is unsupported.
    """
    path, error = get_context_path(asset_path)
    if error:
        if "No Blueprint path" in error: # Keep error generic for user
            return "Error: No asset path was provided, and no asset is currently selected in the Content Browser."
        return f"Error: {error}"
    
    # Send the new, unified command to the C++ backend
    command = {"type": "get_asset_summary", "asset_path": path}
    response = send_to_unreal(command)

    if response and response.get("success"):
        return response.get("summary", "Error: C++ handler succeeded but returned an empty summary.")
    else:
        return f"Error getting asset summary: {response.get('error', 'Unknown error from Unreal.')}"    

@mcp.tool()
def generate_blueprint_logic(code: str, blueprint_path: str = None) -> str:
    """
    Takes a syntactically valid C++ style code block and compiles it into Blueprint nodes.
    CRITICAL: This tool takes a 'code' block, not a user prompt. The AI is responsible for generating the code.

    IMPORTANT: You MUST call get_code_generation_patterns() before using this tool to ensure correct syntax.
    The patterns file contains all 41 code generation patterns with examples.
    """
    path, error = get_context_path(blueprint_path)
    if error: 
        return f"Failed: {error}"
    
    # --- THIS IS THE FIX ---
    # The "type" now matches the new C++ handler, and the key is "code" instead of "user_prompt".
    command = {"type": "generate_blueprint_logic", "blueprint_path": path, "code": code}
    response = send_to_unreal(command)
    
    # This response handling will now correctly receive the success/fail message from your new C++ logic.
    if response and response.get("success"):
        return response.get("message", "Success! The code was sent to the Unreal compiler.")
    else:
        return f"Failed: {response.get('error', 'The Unreal Editor did not respond.')}"

@mcp.tool()
def create_struct(struct_name: str, save_path: str, variables: list[dict]) -> str:
    """
    Creates a new User Defined Struct asset and populates it with the specified variables.

    Args:
        struct_name (str): The name for the new Struct asset (e.g., "S_ItemData").
        save_path (str): The full Content Browser path where the asset should be saved (e.g., "/Game/Data/").
        variables (list[dict]): A list of dictionaries, where each dictionary represents a variable.
                                Each dictionary MUST have a "name" (str) and a "type" (str) key.
                                Example: [{"name": "Health", "type": "float"}, {"name": "ItemName", "type": "string"}]
    
    Returns:
        str: The asset path of the newly created struct, or an error message.
    """
    command = {
        "type": "create_struct",
        "struct_name": struct_name,
        "save_path": save_path,
        "variables": variables
    }
    response = send_to_unreal(command)
    return response.get('asset_path', f"Failed: {response.get('error')}") if response and response.get("success") else f"Failed: {response.get('error')}"

@mcp.tool()
def create_enum(enum_name: str, save_path: str, enumerators: list[str]) -> str:
    """
    Creates a new User Defined Enum asset and populates it with the specified enumerators.

    Args:
        enum_name (str): The name for the new Enum asset (e.g., "E_WeaponType").
        save_path (str): The full Content Browser path where the asset should be saved (e.g., "/Game/Data/").
        enumerators (list[str]): A list of strings, where each string is a new entry in the enum.
                                 Example: ["None", "Sword", "Axe", "Bow"]
    
    Returns:
        str: The asset path of the newly created enum, or an error message.
    """
    command = {
        "type": "create_enum",
        "enum_name": enum_name,
        "save_path": save_path,
        "enumerators": enumerators
    }
    response = send_to_unreal(command)
    return response.get('asset_path', f"Failed: {response.get('error')}") if response and response.get("success") else f"Failed: {response.get('error')}"


@mcp.tool()
def create_new_blueprint(blueprint_name: str, parent_class: str = "Actor", save_path: str = "/Game/") -> str:
    """Creates a new Blueprint asset and returns its path."""
    command = {"type": "create_blueprint", "blueprint_name": blueprint_name, "parent_class": parent_class, "save_path": save_path}
    response = send_to_unreal(command)
    return response.get('asset_path', f"Failed: {response.get('error')}") if response and response.get("success") else f"Failed: {response.get('error')}"

@mcp.tool()
def create_project_folder(folder_path: str) -> str:
    """
    Creates a new folder in the project's Content directory.

    Args:
        folder_path (str): The path for the new folder, relative to /Game/. For example, to create a folder named 'Characters'
                           inside '/Game/Blueprints/', you would provide 'Blueprints/Characters'.
    
    Returns:
        str: A message indicating success or failure.
    """
    command = {"type": "create_project_folder", "folder_path": folder_path}
    response = send_to_unreal(command)
    if response and response.get("success"):
        return response.get("message", "Folder created successfully.")
    else:
        return f"Failed to create folder: {response.get('error', 'Unknown error.')}"

@mcp.tool()
def get_focused_content_browser_path() -> str:
    """
    Gets the folder path of the currently focused Content Browser in the Unreal Editor.

    Returns:
        str: The path of the focused folder (e.g., '/Game/Blueprints/Characters/'), or an error message.
    """
    command = {"type": "get_focused_folder_path"}
    response = send_to_unreal(command)
    if response and response.get("success"):
        return response.get("path", "/Game/") # Default to /Game/ if path is missing
    else:
        # If it fails, returning a default path is safer than returning an error
        # because the AI can still proceed with a creation request.
        print(f"MCP Warning: Could not get focused folder. Reason: {response.get('error', 'Unknown')}. Defaulting to /Game/.", file=sys.stderr)
        return "/Game/"

@mcp.tool()
def list_assets_in_folder(folder_path: str = None) -> str:
    """
    Lists all assets within a specified folder in the Content Browser.

    Args:
        folder_path (str, optional): The path of the folder to inspect (e.g., "/Game/Characters/").
                                     If not provided, uses the currently focused folder in Content Browser.

    Returns:
        str: A JSON string representing a list of asset objects, each with 'name', 'path', and 'class'.
    """
    # If no folder path provided, get the current focused folder
    if folder_path is None:
        command = {"type": "get_focused_folder_path"}
        response = send_to_unreal(command)
        if response and response.get("success"):
            folder_path = response.get("path", "/Game/")
        else:
            folder_path = "/Game/"

    command = {"type": "list_assets_in_folder", "folder_path": folder_path}
    response = send_to_unreal(command)
    if response and response.get("success"):
        return json.dumps(response.get("assets", []))
    else:
        return f"Failed to list assets: {response.get('error', 'Unknown error.')}"

@mcp.tool()
def get_selected_content_browser_assets() -> str:
    """
    Gets a list of all assets currently selected by the user in the Content Browser.

    Returns:
        str: A JSON string representing a list of the selected asset objects, each with 'name', 'path', and 'class'.
    """
    command = {"type": "get_selected_content_browser_assets"}
    response = send_to_unreal(command)
    if response and response.get("success"):
        return json.dumps(response.get("assets", []))
    else:
        return f"Failed to get selected assets: {response.get('error', 'Unknown error.')}"

@mcp.tool()
def edit_data_asset_defaults(asset_path: str, edits: list[dict]) -> str:
    """
    Edits one or more "Class Default" properties on any asset, especially Data Assets.
    This is the correct tool for programmatically changing default values like colors, numbers, text, or struct properties.

    Args:
        asset_path (str): The full path to the asset to modify (e.g., "/Game/Data/DA_WeaponStats").
        edits (list[dict]): A list of edit operations. Each dictionary must contain:
                            - "property_name" (str): The name of the property (e.g., "Damage", "UIData.IconColor").
                            - "value" (str): The new property value, formatted as a string.
    
    CRITICAL VALUE FORMATTING:
    - Text/String: MUST be wrapped in escaped quotes, e.g., "'\"New Name\"'"
    - Color Struct: "'(R=1.0,G=0.5,B=0.0,A=1.0)'"
    - Vector Struct: "'(X=100,Y=50,Z=0)'"
    - Enum: "'E_MyEnum::NewValue'" (with outer quotes)
    - Boolean: "'true'" or "'false'"
    
    Returns:
        str: A message indicating success or failure.
    """
    command = {
        "type": "edit_data_asset_defaults",
        "asset_path": asset_path,
        "edits": edits
    }
    response = send_to_unreal(command)
    return response.get("message", f"Failed: {response.get('error')}") if response and response.get("success") else f"Failed: {response.get('error')}"

@mcp.tool()
def get_all_scene_actors() -> str:
    """
    Retrieves a list of all actors currently in the editor level.

    Returns:
        str: A JSON string representing a list of actor objects. Each object contains the actor's 'label', 'class', and 'location'.
    """
    command = {"type": "get_all_scene_actors"}
    response = send_to_unreal(command)
    if response and response.get("success"):
        return json.dumps(response.get("actors", []))
    else:
        return f"Failed to get scene actors: {response.get('error', 'Unknown error')}"

@mcp.tool()
def select_actors_by_label(actor_labels: list[str]) -> str:
    """
    Selects actors in the editor level that match the provided labels (names).

    Args:
        actor_labels (list[str]): A list of actor labels to select. An empty list will clear the selection.
    
    Returns:
        str: A message indicating the result of the selection.
    """
    command = {"type": "set_selected_actors", "actor_labels": actor_labels}
    response = send_to_unreal(command)
    if response and response.get("success"):
        if not actor_labels:
            return "Cleared actor selection in the level."
        return f"Successfully selected {len(actor_labels)} actor(s) in the level."
    else:
        return f"Failed to select actors: {response.get('error', 'Unknown error.')}"

@mcp.tool()
def add_component_to_blueprint(component_class: str, component_name: str, blueprint_path: str = None) -> str:
    """Adds a component. If blueprint_path is omitted, uses the selected asset."""
    path, error = get_context_path(blueprint_path)
    if error: return error

    command = {"type": "add_component", "blueprint_path": path, "component_class": component_class, "component_name": component_name}
    response = send_to_unreal(command)
    return f"Success! Added '{component_name}' to '{path}'." if response and response.get("success") else f"Failed: {response.get('error')}"

@mcp.tool()
def edit_component_property(component_name: str, property_name: str, property_value: str, blueprint_path: str = None) -> str:
    """Edits a component property. If blueprint_path is omitted, uses the selected asset."""
    path, error = get_context_path(blueprint_path)
    if error: return error

    command = {"type": "edit_component_property", "blueprint_path": path, "component_name": component_name, "property_name": property_name, "property_value": property_value}
    response = send_to_unreal(command)
    return response.get('message', 'Success!') if response and response.get("success") else f"Failed: {response.get('error')}"

@mcp.tool()
def add_variable_to_blueprint(variable_name: str, variable_type: str, default_value: str = None, category: str = None, blueprint_path: str = None) -> str:
    """
    Adds a variable to a Blueprint. If blueprint_path is omitted, uses the selected asset.

    Supported variable types:
    - Basic types: bool, byte, int, int64, float, double, string, text, name
    - Engine structs: Vector, Rotator, Transform, Color, LinearColor
    - Arrays: TArray<Type> or Array<Type> (e.g., "TArray<AActor>", "Array<int>")
    - Subclass references: TSubclassOf<ClassName> (e.g., "TSubclassOf<AActor>")
    - Soft object references: TSoftObjectPtr<Path> (e.g., "TSoftObjectPtr</Game/Blueprints/BP_Enemy>")
    - Soft class references: TSoftClassPtr<Path>
    - Asset paths: /Game/... or /Engine/... for blueprints, structs, enums, etc.
    - Native classes: AActor, UUserWidget, etc. (with or without A/U prefix)

    Args:
        variable_name (str): The name of the variable to add.
        variable_type (str): The type of the variable. See supported types above.
        default_value (str, optional): The default value for the variable.
        category (str, optional): The category name to organize variable in MyBlueprint panel.
        blueprint_path (str, optional): The path to the target Blueprint. If omitted, uses the selected asset.

    Returns:
        str: Success message or error message.
    """
    path, error = get_context_path(blueprint_path)
    if error: return error

    command = {"type": "add_variable", "blueprint_path": path, "variable_name": variable_name, "variable_type": variable_type, "default_value": default_value or "", "category": category or ""}
    response = send_to_unreal(command)
    return f"Success! Added variable '{variable_name}' to '{path}'." if response and response.get("success") else f"Failed: {response.get('error')}"

@mcp.tool()
def delete_variable(variable_name: str, blueprint_path: str = None) -> str:
    """
    Deletes a variable from a Blueprint.

    Args:
        variable_name (str): The name of the variable to delete.
        blueprint_path (str, optional): The path to the target Blueprint. If omitted, uses the selected asset.

    Returns:
        str: A message indicating success or failure.
    """
    path, error = get_context_path(blueprint_path)
    if error: return error

    command = {"type": "delete_variable", "blueprint_path": path, "var_name": variable_name}
    response = send_to_unreal(command)
    return f"Success! Deleted variable '{variable_name}' from '{path}'." if response and response.get("success") else f"Failed: {response.get('error')}"

@mcp.tool()
def categorize_variables(categories: dict = None, blueprint_path: str = None) -> str:
    """
    Organizes Blueprint variables into categories. Without arguments, returns uncategorized variables.
    With 'categories', applies categories to variables (only affects uncategorized variables).

    Args:
        categories (dict, optional): A dictionary mapping variable names to category names.
                                   Example: {"Health": "Gameplay", "Ammo": "Inventory"}
        blueprint_path (str, optional): The path to the target Blueprint. If omitted, uses the selected asset.

    Returns:
        str: JSON string containing the list of uncategorized variables, or the result of categorization.
    """
    path, error = get_context_path(blueprint_path)
    if error: return error

    command = {"type": "categorize_variables", "blueprint_path": path}
    if categories:
        command["categories"] = categories

    response = send_to_unreal(command)
    if response and response.get("success"):
        if categories:
            return response.get("message", "Variables categorized successfully.")
        else:
            return f"Uncategorized variables: {response.get('variables', [])}"
    else:
        return f"Failed: {response.get('error')}"

@mcp.tool()
def delete_component(component_name: str, blueprint_path: str = None) -> str:
    """
    Deletes a component from a Blueprint.

    Args:
        component_name (str): The name of the component to delete.
        blueprint_path (str, optional): The path to the target Blueprint. If omitted, uses the selected asset.

    Returns:
        str: A message indicating success or failure.
    """
    path, error = get_context_path(blueprint_path)
    if error: return error

    command = {"type": "delete_component", "blueprint_path": path, "component_name": component_name}
    response = send_to_unreal(command)
    return f"Success! Deleted component '{component_name}' from '{path}'." if response and response.get("success") else f"Failed: {response.get('error')}"

@mcp.tool()
def delete_selected_nodes() -> str:
    """
    Deletes all currently selected nodes from the active Blueprint graph editor.

    Returns:
        str: A message indicating how many nodes were deleted, or an error message.
    """
    command = {"type": "delete_nodes"}
    response = send_to_unreal(command)
    if response and response.get("success"):
        return response.get("message", "Nodes deleted successfully.")
    else:
        return f"Failed: {response.get('error', 'Unknown error')}"

@mcp.tool()
def delete_widget(widget_path: str, widget_name: str) -> str:
    """
    Deletes a widget from a User Widget Blueprint.

    Args:
        widget_path (str): Path to the User Widget Blueprint.
        widget_name (str): The name of the widget to delete.

    Returns:
        str: A message indicating success or failure.
    """
    command = {"type": "delete_widget", "widget_path": widget_path, "widget_name": widget_name}
    response = send_to_unreal(command)
    return f"Success! Deleted widget '{widget_name}'." if response and response.get("success") else f"Failed: {response.get('error')}"

@mcp.tool()
def delete_unused_variables(blueprint_path: str = None) -> str:
    """
    Deletes all unused (unreferenced) variables from a Blueprint.
    Note: This tool is not yet fully implemented and will return guidance for manual deletion.

    Args:
        blueprint_path (str, optional): The path to the target Blueprint. If omitted, uses the selected asset.

    Returns:
        str: A message with guidance on manual deletion.
    """
    path, error = get_context_path(blueprint_path)
    if error: return error

    command = {"type": "delete_unused_variables", "blueprint_path": path}
    response = send_to_unreal(command)
    if response and response.get("success"):
        return response.get("info", response.get("message", ""))
    else:
        return f"Failed: {response.get('error')}"

@mcp.tool()
def delete_function(function_name: str, blueprint_path: str = None) -> str:
    """
    Deletes a function from a Blueprint.

    Args:
        function_name (str): The name of the function to delete.
        blueprint_path (str, optional): The path to the target Blueprint. If omitted, uses the selected asset.

    Returns:
        str: A message indicating success or failure.
    """
    path, error = get_context_path(blueprint_path)
    if error: return error

    command = {"type": "delete_function", "blueprint_path": path, "function_name": function_name}
    response = send_to_unreal(command)
    return f"Success! Deleted function '{function_name}' from '{path}'." if response and response.get("success") else f"Failed: {response.get('error')}"

@mcp.tool()
def undo_last_generated() -> str:
    """
    Undoes the last code generation by deleting all nodes created in the last compilation.
    This is useful for reverting incorrect AI-generated code.

    Returns:
        str: A message indicating how many nodes were deleted, or an error message.
    """
    command = {"type": "undo_last_generated"}
    response = send_to_unreal(command)
    if response and response.get("success"):
        return response.get("message", "Undo successful.")
    else:
        return f"Failed: {response.get('error', 'Unknown error')}"

# ============================================================================
# C++ File Operations Tools
# ============================================================================

@mcp.tool()
def list_plugins() -> str:
    """
    Lists all enabled plugins in the current Unreal Engine project.

    This tool returns information about all plugins including their name,
    friendly name, version, type (Engine/Project/External), and directory.

    Returns:
        str: JSON string containing array of plugins with their details.
    """
    command = {"type": "list_plugins"}
    response = send_to_unreal(command)
    if response and response.get("success"):
        plugins = response.get("plugins", [])
        return f"Found {len(plugins)} plugin(s):\n" + "\n".join([
            f"- {p.get('friendly_name', p['name'])} v{p.get('version', 'N/A')} ({p.get('type', 'Unknown')})"
            for p in plugins
        ])
    else:
        return f"Failed: {response.get('error', 'Unknown error')}"

@mcp.tool()
def find_plugin(search_pattern: str) -> str:
    """
    Finds a plugin by name pattern. Supports partial matching.

    Args:
        search_pattern (str): The name or partial name to search for.

    Returns:
        str: JSON string containing matching plugins with their details.
    """
    command = {"type": "find_plugin", "search_pattern": search_pattern}
    response = send_to_unreal(command)
    if response and response.get("success"):
        plugins = response.get("plugins", [])
        if not plugins:
            return f"No plugins found matching: {search_pattern}"
        return f"Found {len(plugins)} plugin(s):\n" + "\n".join([
            f"- {p.get('friendly_name', p['name'])} ({p.get('directory', 'N/A')})"
            for p in plugins
        ])
    else:
        return f"Failed: {response.get('error', 'Unknown error')}"

@mcp.tool()
def read_cpp_file(file_path: str) -> str:
    """
    Reads the content of a C++ source file (.h, .cpp, .hpp, .c, .cc).

    Args:
        file_path (str): Path to the C++ file. Can be relative to project or absolute.

    Returns:
        str: The file content or an error message.
    """
    command = {"type": "read_cpp_file", "file_path": file_path}
    response = send_to_unreal(command)
    if response and response.get("success"):
        content = response.get("content", "")
        return f"Successfully read file ({response.get('content_length', 0)} chars):\n\n{content}"
    else:
        return f"Failed: {response.get('error', 'Unknown error')}"

@mcp.tool()
def write_cpp_file(file_path: str, content: str) -> str:
    """
    Writes content to a C++ source file (.h, .cpp, .hpp, .c, .cc).
    WARNING: This is a destructive operation that overwrites the file.

    Args:
        file_path (str): Path to the C++ file. Can be relative to project or absolute.
        content (str): The C++ code content to write.

    Returns:
        str: Success message or an error message.
    """
    command = {"type": "write_cpp_file", "file_path": file_path, "content": content}
    response = send_to_unreal(command)
    if response and response.get("success"):
        return f"Successfully wrote C++ file: {file_path}"
    else:
        return f"Failed: {response.get('error', 'Unknown error')}"

@mcp.tool()
def list_plugin_files(plugin_name: str) -> str:
    """
    Lists all C++ source files (.h, .cpp, .hpp) in a plugin.

    Args:
        plugin_name (str): Name or friendly name of the plugin.

    Returns:
        str: JSON string containing array of files with their paths and types.
    """
    command = {"type": "list_plugin_files", "plugin_name": plugin_name}
    response = send_to_unreal(command)
    if response and response.get("success"):
        files = response.get("files", [])
        return f"Found {len(files)} C++ file(s) in plugin '{plugin_name}':\n" + "\n".join([
            f"- {f.get('file_type', 'unknown').upper()}: {f.get('relative_path', 'N/A')}"
            for f in files
        ])
    else:
        return f"Failed: {response.get('error', 'Unknown error')}"

@mcp.tool()
def spawn_actor_in_level(actor_class: str, location: list = [0, 0, 0], rotation: list = [0, 0, 0], scale: list = [1, 1, 1], actor_label: str = None) -> str:
    """
    Spawns an actor in the current Unreal Engine editor level.

    Args:
        actor_class (str): The class of the actor to spawn. This can be:
                           - A basic shape: "Cube", "Sphere", "Cylinder", "Cone".
                           - A C++ class name: "PointLight", "CameraActor".
                           - A full Blueprint asset path: "/Game/Blueprints/BP_MyCharacter.BP_MyCharacter".
        location (list, optional): [X, Y, Z] world coordinates. Defaults to [0, 0, 0].
        rotation (list, optional): [Pitch, Yaw, Roll] in degrees. Defaults to [0, 0, 0].
        scale (list, optional): [X, Y, Z] scale factors. Defaults to [1, 1, 1].
        actor_label (str, optional): A custom name for the actor in the World Outliner. Defaults to None.
        
    Returns:
        str: A message indicating success or failure.
    """
    command = {
        "type": "spawn_actor",
        "actor_class": actor_class,
        "location": location,
        "rotation": rotation,
        "scale": scale,
        "actor_label": actor_label or "" # Send empty string instead of None
    }

    response = send_to_unreal(command)
    if response and response.get("success"):
        label_part = f" with label '{actor_label}'" if actor_label else ""
        return f"Successfully spawned {actor_class}{label_part} at location {location}."
    else:
        error = response.get('error', 'Unknown error')
        # Add a hint to help the AI self-correct on future attempts
        if "not found" in error or "Could not find" in error:
            hint = "\nHint: For basic shapes, use 'Cube' or 'Sphere'. For C++ classes, use names like 'PointLight'. For Blueprints, you must provide the full asset path like '/Game/Path/To/BP_MyAsset.BP_MyAsset'."
            error += hint
        return f"Failed to spawn actor: {error}"

@mcp.tool()
def spawn_multiple_actors_in_level(
    count: int, 
    actor_class: str, 
    bounding_actor_label: str = None,
    random_location_min: list = [-1000, -1000, 0], 
    random_location_max: list = [1000, 1000, 0],
    random_scale_min: list = [1, 1, 1],
    random_scale_max: list = [1, 1, 1],
    random_rotation_min: list = [0, 0, 0],
    random_rotation_max: list = [0, 0, 0]
) -> str:
    """
    Spawns multiple actors with random transforms, optionally within the bounds of another actor.

    Args:
        count (int): The number of actors to spawn.
        actor_class (str): The class of the actor to spawn (e.g., "Cube", "/Game/BP_MyAsset.BP_MyAsset").
        bounding_actor_label (str, optional): The name (label) of an actor in the level to use as a spawn volume.
                                           CRITICAL: Use the special value '_SELECTED_' to use the user's currently selected actor.
        random_location_min/max (list, optional): Min/max [X, Y, Z] bounds. Ignored if bounding_actor_label is used.
        random_scale_min/max (list, optional): Min/max [X, Y, Z] bounds for random scale.
        random_rotation_min/max (list, optional): Min/max [Pitch, Yaw, Roll] bounds for random rotation.

    Returns:
        str: A message indicating the result of the bulk spawn operation.
    """
    if count > 1000: # Increased safety limit
        return "Error: Cannot spawn more than 1000 actors at once to ensure editor performance."

    # This function is now much simpler. It just packages all parameters and sends them
    # to the new C++ 'spawn_advanced' handler, which does all the heavy lifting.
    command = {
        "type": "spawn_advanced",
        "count": count,
        "actor_class": actor_class,
        "bounding_actor_label": bounding_actor_label or "",
        "random_location_min": random_location_min,
        "random_location_max": random_location_max,
        "random_scale_min": random_scale_min,
        "random_scale_max": random_scale_max,
        "random_rotation_min": random_rotation_min,
        "random_rotation_max": random_rotation_max,
    }

    response = send_to_unreal(command)
    if response and response.get("success"):
        location_desc = f"within the bounds of '{bounding_actor_label}'" if bounding_actor_label else "in the specified area"
        return f"Successfully processed an advanced spawn request for {count} instances of '{actor_class}' {location_desc}."
    else:
        return f"Advanced spawn request failed: {response.get('error', 'Unknown error.')}"

@mcp.tool()
def get_widget_properties(widget_classes: list[str]) -> str:
    """
    Retrieves all valid, editable properties, including nested ones (e.g., 'Font.Size'), for a list of UMG classes.
    The AI MUST call this tool ONCE with ALL required classes before constructing a UMG layout.

    Args:
        widget_classes (list[str]): A list of C++ class names to inspect (e.g., ["Button", "TextBlock", "CanvasPanelSlot"]).

    Returns:
        str: A JSON string of a dictionary where keys are class names and values are lists of their properties.
    """
    command = {
        "type": "get_widget_properties",
        "widget_classes": widget_classes
    }
    response = send_to_unreal(command)
    if response and response.get("success"):
        return json.dumps(response.get("properties", {}))
    else:
        return f"Failed to get properties: {response.get('error', 'Unknown error.')}"

@mcp.tool()
def export_text_to_file(file_name: str, content: str, format: str = "md") -> str:
    """
    Writes the given string content to a specified file in the project's 'Saved/Exported Explanations' folder.
    This is used to save summaries or explanations generated by other tools.

    Args:
        file_name (str): The desired name of the file, without extension.
        content (str): The text content to write to the file.
        format (str, optional): The file format/extension, e.g., 'txt' or 'md'. Defaults to "md".

    Returns:
        str: A message indicating success or failure, including the final file path.
    """
    command = {
        "type": "export_to_file",
        "file_name": file_name,
        "content": content,
        "format": format
    }
    response = send_to_unreal(command)
    if response and response.get("success"):
        return response.get("message", "Export successful.")
    else:
        return f"Export failed: {response.get('error', 'Unknown error.')}"

@mcp.tool()
def create_material(material_path: str, color: list = [1.0, 1.0, 1.0, 1.0]) -> str:
    """
    Creates a new, simple Material asset with a specified base color.

    Args:
        material_path (str): The full asset path for the new material, including the name (e.g., "/Game/Materials/M_Red").
        color (list, optional): The base color of the material as [R, G, B, A] values from 0.0 to 1.0. Defaults to white.
    
    Returns:
        str: A message indicating success or failure.
    """
    command = {"type": "create_material", "material_path": material_path, "color": color}
    response = send_to_unreal(command)
    if response and response.get("success"):
        return response.get("message", "Material created successfully.")
    else:
        return f"Failed to create material: {response.get('error', 'Unknown error.')}"

@mcp.tool()
def set_actor_material(actor_label: str, material_path: str) -> str:
    """
    Applies a material to an actor in the level.

    Args:
        actor_label (str): The exact label (name) of the actor in the World Outliner.
        material_path (str): The full asset path of the material to apply (e.g., "/Game/Materials/M_Red").
    
    Returns:
        str: A message indicating success or failure.
    """
    command = {"type": "set_actor_material", "actor_label": actor_label, "material_path": material_path}
    response = send_to_unreal(command)
    if response and response.get("success"):
        return response.get("message", "Material applied successfully.")
    else:
        return f"Failed to set material: {response.get('error', 'Unknown error.')}"

@mcp.tool()
def set_multiple_actor_materials(actor_labels: list[str], material_path: str) -> str:
    """
    Applies a single material to multiple actors in the level in one efficient operation.

    Args:
        actor_labels (list[str]): A list of exact labels (names) of the actors in the World Outliner.
        material_path (str): The full asset path of the material to apply (e.g., "/Game/Materials/M_Blue").
    
    Returns:
        str: A message indicating the result of the bulk operation.
    """
    command = {
        "type": "set_multiple_actor_materials", 
        "actor_labels": actor_labels, 
        "material_path": material_path
    }
    response = send_to_unreal(command)
    if response and response.get("success"):
        return response.get("message", "Bulk material operation completed.")
    else:
        return f"Failed to set multiple materials: {response.get('error', 'Unknown error.')}"

@mcp.tool()
def get_selected_graph_nodes_as_text() -> str:
    """
    Gets a text description of the currently selected nodes in the active graph editor. This works for
    Blueprint, Material, and Behavior Tree editors. This is the first step for any code explanation or
    refactoring task involving selected nodes.
    """
    try:
        command = {"type": "get_selected_nodes"}
        response = send_to_unreal(command)

        if response and response.get("success"):
            return response.get('nodes_text', 'No nodes selected or unable to describe.')
        else:
            return f"Failed to get selected nodes: {response.get('error', 'Unknown error.')}"
            
    except Exception as e:
        return f"A fatal Python error occurred: {str(e)}"

@mcp.tool()
def add_widgets_to_layout(user_widget_path: str, layout_definition: list[dict]) -> str:
    """
    Adds one or more new widgets to an EXISTING User Widget layout without deleting existing content.
    This is the correct tool for "add a button," "insert a panel," etc.

    Args:
        user_widget_path (str): Path to the User Widget Blueprint to modify.
        layout_definition (list[dict]): A list of dictionaries describing the new widgets to add.
                                       Every widget in this list MUST have a valid 'parent_name'.
    """
    # This logic can be almost identical to the create tool, just pointing to a different C++ handler.
    parsed_layout = None
    if isinstance(layout_definition, str):
        try:
            parsed_layout = json.loads(layout_definition)
        except json.JSONDecodeError as e:
            return f"Error: Malformed JSON string in layout_definition. {e}"
    else:
        parsed_layout = layout_definition

    if not isinstance(parsed_layout, list):
         return f"Error: layout_definition is not a list. Type: {type(parsed_layout).__name__}"

    command = {
        "type": "add_widgets_to_layout", # This type points to our new C++ handler
        "user_widget_path": user_widget_path,
        "layout_definition": parsed_layout
    }
    response = send_to_unreal(command)
    return response.get("message", f"Failed: {response.get('error')}") if response and response.get("success") else f"Failed: {response.get('error')}"

@mcp.tool()
def edit_widget_properties(user_widget_path: str, edits: list[dict]) -> str:
    """
    Edits one or more properties on existing widgets in a single, efficient batch operation.
    This is the correct tool for "change the color," "set the text," "adjust the padding," etc.

    Args:
        user_widget_path (str): Path to the User Widget Blueprint to modify.
        edits (list[dict]): A list of edit operations. Each dictionary must contain:
                            - "widget_name" (str): The name of the widget to edit.
                            - "property_name" (str): The name of the property to change (e.g., "Slot.bAutoSize", "Font.Size").
                            - "value" (str): The new property value, correctly formatted.
    """
    command = {
        "type": "edit_widget_properties", # This type points to our new C++ handler
        "user_widget_path": user_widget_path,
        "edits": edits
    }
    response = send_to_unreal(command)
    return response.get("message", f"Failed: {response.get('error')}") if response and response.get("success") else f"Failed: {response.get('error')}"

@mcp.tool()
def create_widget_from_layout(user_widget_path: str, layout_definition: list[dict]) -> str:
    """
    Creates and configures a complex widget hierarchy in a single, high-performance operation.
    This is the PREFERRED tool for any multi-widget UI creation.

    Args:
        user_widget_path (str): Path to the User Widget Blueprint (e.g., "/Game/UI/WBP_MainMenu").
        layout_definition (list[dict]): A list of dictionaries, each describing a widget to create.
                                       This can also be a JSON string representation of the list.
    """
    
    # --- THIS IS THE FIX ---
    # The AI is sometimes passing the layout as a JSON string instead of a direct list.
    # This guard makes the tool more robust by parsing the string if necessary.
    parsed_layout = None
    if isinstance(layout_definition, str):
        try:
            print("MCP Server: layout_definition was a string. Attempting to parse from JSON.", file=sys.stderr)
            parsed_layout = json.loads(layout_definition)
        except json.JSONDecodeError as e:
            # Give the AI a clear error message if the string is not valid JSON
            return f"Error: The provided layout_definition was a malformed JSON string and could not be parsed. Please ensure it is a valid JSON array. Error: {e}"
    else:
        # It was already a valid list, so we can use it directly.
        parsed_layout = layout_definition

    # Final validation to ensure we have a list to send to Unreal
    if not isinstance(parsed_layout, list):
         return f"Error: The final layout_definition is not a list. Type received: {type(parsed_layout).__name__}"
    # --- END OF FIX ---

    command = {
        "type": "create_widget_from_layout",
        "user_widget_path": user_widget_path,
        "layout_definition": parsed_layout # Use the now-guaranteed-to-be-a-list layout
    }
    
    response = send_to_unreal(command)
    if response and response.get("success"):
        return response.get("message", "Widget layout created successfully.")
    else:
        return f"Failed to create widget layout: {response.get('error', 'Unknown error from Unreal.')}"

@mcp.tool()
def scan_and_index_project_blueprints() -> str:
    """Scans all Blueprint assets in the project and creates a searchable index file."""
    try:
        print("MCP: Received request to scan project. Sending command to Unreal...", file=sys.stderr)
        command = {"type": "scan_project"}
        response = send_to_unreal(command)
        if response and response.get("success"):
            return "Project scan completed successfully. The index is now up-to-date."
        else:
            return f"Project scan failed: {response.get('error', 'Unknown error.')}"
    except Exception as e:
        return f"A fatal Python error occurred during project scan: {str(e)}"

@mcp.tool()
def get_relevant_blueprint_context(question: str) -> str:
    """Searches the project's Blueprint index to find context relevant to a user's question."""
    project_root = get_project_root()
    if not project_root:
        return "Error: Could not find the Unreal project root directory (.uproject file)."
    index_path = project_root / "Saved" / "AI" / "ProjectIndex.json"
    if not index_path.exists():
        return "Error: The project index file does not exist. Please run 'scan_and_index_project_blueprints()' first."
    try:
        project_index = _read_file_with_fallback(index_path, is_json=True)
    except Exception as e:
        return f"Error reading project index file: {str(e)}"
    
    keywords = set(re.findall(r'\b\w+\b', question.lower()))
    if not keywords:
        return "Error: Could not extract keywords from the question."

    scored_blueprints = []
    for path, summary in project_index.items():
        # Skip metadata fields (start with _) and non-string values
        if path.startswith("_") or not isinstance(summary, str):
            continue
        score = 0
        summary_lower = summary.lower()
        for keyword in keywords:
            if keyword in summary_lower:
                score += 1
        if score > 0:
            scored_blueprints.append({"path": path, "summary": summary, "score": score})

    if not scored_blueprints:
        return "No relevant Blueprints found for that question. The system might be implemented in C++ or the index needs updating."

    scored_blueprints.sort(key=lambda x: x['score'], reverse=True)
    top_blueprints = scored_blueprints[:3]

    context = "--- RELEVANT BLUEPRINT CONTEXT ---\n\n"
    for bp in top_blueprints:
        context += f"--- Path: {bp['path']} ---\n"
        context += bp['summary']
        context += "\n\n"
        
    return context

@mcp.tool()
def get_project_performance_report() -> str:
    """
    Reads the project index file and returns a formatted list of all Blueprints that have potential performance issues.
    The project must be scanned with 'scan_and_index_project_blueprints()' first.
    """
    project_root = get_project_root()
    if not project_root:
        return "Error: Could not find the Unreal project root directory (.uproject file)."

    index_path = project_root / "Saved" / "AI" / "ProjectIndex.json"
    if not index_path.exists():
        return "Error: The project index file does not exist. Please run 'scan_and_index_project_blueprints()' first."

    try:
        project_index = _read_file_with_fallback(index_path, is_json=True)
    except Exception as e:
        return f"Error reading project index file: {str(e)}"
    
    problem_blueprints = []
    tick_users = []
    cast_in_tick = []
    spawn_in_tick = []
    string_in_tick = []
    getall_in_tick = []
    performance_tag = "--- POTENTIAL PERFORMANCE ISSUES ---"

    for path, summary in project_index.items():
        # Skip metadata fields (start with _) and non-string values
        if path.startswith("_") or not isinstance(summary, str):
            continue
        if performance_tag in summary:
            issues_section = summary.split(performance_tag, 1)[1].strip()
            issues_lower = issues_section.lower()

            # Track all tick users
            tick_users.append(path)

            # Parse specific issues
            for line in issues_section.split('\n'):
                line_lower = line.lower().strip()
                if line_lower.startswith('!'):
                    if "cast" in line_lower and "tick" in line_lower:
                        cast_in_tick.append(f"{path}: {line.strip('! ')}")
                    elif "spawn" in line_lower:
                        spawn_in_tick.append(f"{path}: {line.strip('! ')}")
                    elif "string" in line_lower or "concat" in line_lower or "buildstring" in line_lower:
                        string_in_tick.append(f"{path}: {line.strip('! ')}")
                    elif "getallactorsofclass" in line_lower:
                        getall_in_tick.append(f"{path}: {line.strip('! ')}")

            if issues_section:
                problem_blueprints.append({"path": path, "issues": issues_section})

    if not problem_blueprints:
        return """--- Project Performance Report ---

✅ No Blueprints with common performance issues detected.

All scanned Blueprints appear to follow best practices:
- No Event Tick usage with expensive operations
- No casting in Tick loops
- No spawning actors every frame
- No string operations in Tick

Great job! Your project follows good performance patterns."""

    # Build detailed report
    report_lines = ["--- Project Performance Report ---\n"]
    report_lines.append(f"📊 Analyzed {len(problem_blueprints)} Blueprint(s) with potential issues\n")
    report_lines.append(f"⚡ {len(tick_users)} Blueprints use Event Tick\n\n")

    # Event Tick category
    if tick_users:
        report_lines.append("## 🔄 Event Tick Usage\n")
        report_lines.append("The following Blueprints run logic every frame. Consider Timeline, Timer, or Event-based logic:\n")
        for bp in tick_users[:10]:
            report_lines.append(f"- `{bp}`")
        if len(tick_users) > 10:
            report_lines.append(f"  ... and {len(tick_users) - 10} more")
        report_lines.append("")

    # Cast in Tick
    if cast_in_tick:
        report_lines.append("\n## 🔍 Cast<> in Event Tick\n")
        report_lines.append("Casting every frame is expensive. Cache the reference instead:\n")
        for issue in cast_in_tick[:5]:
                report_lines.append(f"- {issue}")
        report_lines.append("")

    # GetAllActorsOfClass in Tick
    if getall_in_tick:
        report_lines.append("\n## 🔍 GetAllActorsOfClass in Event Tick\n")
        report_lines.append("Extremely expensive. Use interfaces, tags, or cache results:\n")
        for issue in getall_in_tick[:5]:
                report_lines.append(f"- {issue}")
        report_lines.append("")

    # Spawn in Tick
    if spawn_in_tick:
        report_lines.append("\n## 🎯 SpawnActor in Event Tick\n")
        report_lines.append("Spawning every frame causes GC pressure. Use object pooling:\n")
        for issue in spawn_in_tick[:5]:
                report_lines.append(f"- {issue}")
        report_lines.append("")

    # String operations in Tick
    if string_in_tick:
        report_lines.append("\n## 📝 String Operations in Event Tick\n")
        report_lines.append("String ops cause GC pressure. Cache strings or use FName:\n")
        for issue in string_in_tick[:5]:
                report_lines.append(f"- {issue}")
        report_lines.append("")

    # Recommendations
    report_lines.append("\n--- Recommendations ---\n")
    if tick_users:
        report_lines.append("1. Replace Tick with Timeline for smooth animations")
        report_lines.append("2. Use Timer events instead of Tick for periodic checks")
        report_lines.append("3. Disable Tick when not needed (e.g., when hidden)")
    else:
        report_lines.append("✅ Your project follows good performance patterns!")

    return "\n".join(report_lines)

# ==================================================================
# NEW MCP TOOLS - SYNCED FROM WIDGET
# ==================================================================

@mcp.tool()
def find_asset_by_name(name_pattern: str, asset_type: str = None) -> str:
    """
    Searches for assets by name pattern in the /Game directory.

    Args:
        name_pattern: The name pattern to search for (case-insensitive, partial matches allowed)
        asset_type: Optional filter by asset type (Blueprint, Material, Texture, DataTable)

    Returns:
        JSON string with matching assets (up to 20 results)
    """
    command = {
        "type": "find_asset_by_name",
        "name_pattern": name_pattern,
        "asset_type": asset_type
    }
    result = send_to_unreal(command)
    return json.dumps(result)

@mcp.tool()
def move_asset(asset_path: str, destination_folder: str) -> str:
    """
    Moves a single asset to a different folder.

    Args:
        asset_path: Full path to the asset (e.g., /Game/Blueprints/MyBP)
        destination_folder: Target folder path (e.g., /Game/NewFolder)

    Returns:
        JSON string with result including old_path and new_path
    """
    command = {
        "type": "move_asset",
        "asset_path": asset_path,
        "destination_folder": destination_folder
    }
    result = send_to_unreal(command)
    return json.dumps(result)

@mcp.tool()
def move_assets(asset_paths: list[str], destination_folder: str) -> str:
    """
    Moves multiple assets to a different folder in bulk.

    Args:
        asset_paths: List of asset paths to move
        destination_folder: Target folder path

    Returns:
        JSON string with results for each asset
    """
    command = {
        "type": "move_assets",
        "asset_paths": asset_paths,
        "destination_folder": destination_folder
    }
    result = send_to_unreal(command)
    return json.dumps(result)

@mcp.tool()
def generate_texture(prompt: str, save_path: str = None, aspect_ratio: str = "1:1", asset_name: str = None) -> str:
    """
    Generates a single texture from a text prompt using AI.

    Args:
        prompt: Description of the texture to generate
        save_path: Optional folder path (defaults to /Game/GeneratedTextures)
        aspect_ratio: Aspect ratio (1:1, 16:9, 9:16, 4:3, 3:4)
        asset_name: Optional custom asset name

    Returns:
        JSON string with asset_path on success
    """
    command = {
        "type": "generate_texture",
        "prompt": prompt,
        "save_path": save_path,
        "aspect_ratio": aspect_ratio,
        "asset_name": asset_name
    }
    result = send_to_unreal(command)
    return json.dumps(result)

@mcp.tool()
def generate_pbr_material(prompt: str, save_path: str = None, material_name: str = None) -> str:
    """
    Generates a complete PBR material with 5 texture maps (Base Color, Normal, Roughness, Metallic, AO).
    Note: This operation takes 2-5 minutes. For best results, use the Blueprint Architect widget UI.

    Args:
        prompt: Description of the material to generate
        save_path: Optional folder path for generated textures
        material_name: Optional custom material name

    Returns:
        JSON string with material_path on success
    """
    command = {
        "type": "generate_pbr_material",
        "prompt": prompt,
        "save_path": save_path,
        "material_name": material_name
    }
    result = send_to_unreal(command)
    return json.dumps(result)

@mcp.tool()
def create_material_from_textures(save_path: str, material_name: str, texture_paths: list) -> str:
    """
    Creates a material from existing textures with auto-detected texture types.
    Automatically connects textures to the correct material pins based on naming conventions.

    Supported naming patterns for auto-detection:
    - BaseColor: 'basecolor', 'base_color', 'albedo', 'diffuse', 'color', '_bc', '_d'
    - Normal: 'normal', '_nrm', '_n'
    - Roughness: 'roughness', 'rough', '_r'
    - Metallic: 'metallic', 'metal', '_m'
    - AO: 'ao', 'ambient', 'occlusion'
    - Emissive: 'emissive', 'emit', 'emission', '_e'
    - Opacity: 'opacity', 'alpha'

    Args:
        save_path: Folder path for the new material (e.g., /Game/Materials)
        material_name: Name for the new material
        texture_paths: List of texture paths (strings) or dicts with 'path' and optional 'type'
                       Example: ["/Game/Textures/T_Wood_BaseColor", {"path": "/Game/Textures/T_Wood_N", "type": "Normal"}]

    Returns:
        JSON string with material_path, connected textures, and detected types
    """
    command = {
        "type": "create_material_from_textures",
        "save_path": save_path,
        "material_name": material_name,
        "texture_paths": texture_paths
    }
    result = send_to_unreal(command)
    return json.dumps(result)

@mcp.tool()
def create_model_3d(prompt: str, model_name: str = None, save_path: str = None, auto_refine: bool = True, seed: int = None) -> str:
    """
    Generates a 3D model from a text prompt using Meshy AI.
    This operation takes 1-5 minutes and returns immediately with a task_id.

    Args:
        prompt: Description of the 3D model to generate
        model_name: Optional custom asset name
        save_path: Optional folder path (defaults to /Game/GeneratedModels)
        auto_refine: Whether to automatically refine the preview mesh
        seed: Optional random seed for reproducible generation (default -1)

    Returns:
        JSON string with task_id (check status with refine_model_3d)
    """
    command = {
        "type": "create_model_3d",
        "prompt": prompt,
        "model_name": model_name,
        "save_path": save_path,
        "auto_refine": auto_refine,
        "seed": seed if seed is not None else -1
    }
    result = send_to_unreal(command)
    return json.dumps(result)

@mcp.tool()
def refine_model_3d(task_id: str, model_name: str = None, save_path: str = None) -> str:
    """
    Refines a preview 3D model to full quality, or checks the status of a generation task.

    Args:
        task_id: The task ID from create_model_3d or a previous refine operation
        model_name: Optional custom asset name
        save_path: Optional folder path

    Returns:
        JSON string with status and model_path when complete
    """
    command = {
        "type": "refine_model_3d",
        "task_id": task_id,
        "model_name": model_name,
        "save_path": save_path
    }
    result = send_to_unreal(command)
    return json.dumps(result)

@mcp.tool()
def image_to_model_3d(image_path: str, prompt: str = None, model_name: str = None, save_path: str = None) -> str:
    """
    Converts a reference image to a 3D model using Meshy AI.
    This operation takes 1-5 minutes and returns immediately with a task_id.

    Args:
        image_path: Full path to the reference image file
        prompt: Optional description to guide the generation
        model_name: Optional custom asset name
        save_path: Optional folder path

    Returns:
        JSON string with task_id
    """
    command = {
        "type": "image_to_model_3d",
        "image_path": image_path,
        "prompt": prompt,
        "model_name": model_name,
        "save_path": save_path
    }
    result = send_to_unreal(command)
    return json.dumps(result)

@mcp.tool()
def create_input_action(name: str, save_path: str = None, value_type: str = "bool") -> str:
    """
    Creates an Enhanced Input Action asset.

    Args:
        name: Name for the Input Action
        save_path: Optional folder path (defaults to current Content Browser folder)
        value_type: Value type - "bool", "float", "vector2d", or "vector3d"

    Returns:
        JSON string with asset_path
    """
    command = {
        "type": "create_input_action",
        "name": name,
        "save_path": save_path,
        "value_type": value_type
    }
    result = send_to_unreal(command)
    return json.dumps(result)

@mcp.tool()
def create_input_mapping_context(name: str, save_path: str = None) -> str:
    """
    Creates an Enhanced Input Mapping Context asset.

    Args:
        name: Name for the Input Mapping Context
        save_path: Optional folder path

    Returns:
        JSON string with asset_path
    """
    command = {
        "type": "create_input_mapping_context",
        "name": name,
        "save_path": save_path
    }
    result = send_to_unreal(command)
    return json.dumps(result)

@mcp.tool()
def add_input_mapping(context_path: str, action_path: str, key: str, modifiers: list = None, triggers: list = None) -> str:
    """
    Adds a key mapping to an Input Mapping Context.

    Args:
        context_path: Path to the Input Mapping Context asset
        action_path: Path to the Input Action asset
        key: Key name (e.g., "W", "SpaceBar", "Gamepad_FaceButton_Bottom")
        modifiers: Optional list of modifier configurations
        triggers: Optional list of trigger configurations

    Returns:
        JSON string with result
    """
    command = {
        "type": "add_input_mapping",
        "context_path": context_path,
        "action_path": action_path,
        "key": key,
        "modifiers": modifiers or [],
        "triggers": triggers or []
    }
    result = send_to_unreal(command)
    return json.dumps(result)

@mcp.tool()
def create_data_table(name: str, save_path: str = None, row_struct: str = None, rows: list[dict] = None) -> str:
    """
    Creates a DataTable asset.

    Args:
        name: Name for the DataTable
        save_path: Optional folder path
        row_struct: Optional path to the row struct (e.g., /Script/Engine.MyStruct)
        rows: Optional list of row data dicts with 'row_name' and struct fields

    Returns:
        JSON string with asset_path and rows_added count
    """
    command = {
        "type": "create_data_table",
        "name": name,
        "save_path": save_path,
        "row_struct": row_struct,
        "rows": rows or []
    }
    result = send_to_unreal(command)
    return json.dumps(result)

@mcp.tool()
def add_data_table_rows(table_path: str, rows: list[dict]) -> str:
    """
    Adds rows to an existing DataTable.

    Args:
        table_path: Path to the DataTable asset
        rows: List of row data dicts with 'row_name' and struct fields

    Returns:
        JSON string with rows_added count
    """
    command = {
        "type": "add_data_table_rows",
        "table_path": table_path,
        "rows": rows
    }
    result = send_to_unreal(command)
    return json.dumps(result)

@mcp.tool()
def get_detailed_blueprint_summary(blueprint_path: str) -> str:
    """
    Gets a detailed summary of a Blueprint including all functions, variables, and components.

    Args:
        blueprint_path: Path to the Blueprint asset

    Returns:
        JSON string with detailed summary
    """
    command = {
        "type": "get_detailed_blueprint_summary",
        "blueprint_path": blueprint_path
    }
    result = send_to_unreal(command)
    return json.dumps(result)

@mcp.tool()
def get_material_graph_summary(material_path: str) -> str:
    """
    Gets a summary of a Material's node graph.

    Args:
        material_path: Path to the Material asset

    Returns:
        JSON string with material graph summary
    """
    command = {
        "type": "get_material_graph_summary",
        "material_path": material_path
    }
    result = send_to_unreal(command)
    return json.dumps(result)

@mcp.tool()
def get_behavior_tree_summary(behavior_tree_path: str) -> str:
    """
    Gets a summary of a Behavior Tree's nodes and structure.

    Args:
        behavior_tree_path: Path to the Behavior Tree asset

    Returns:
        JSON string with behavior tree summary
    """
    command = {
        "type": "get_behavior_tree_summary",
        "behavior_tree_path": behavior_tree_path
    }
    result = send_to_unreal(command)
    return json.dumps(result)

@mcp.tool()
def get_batch_widget_properties(widget_classes: list[str]) -> str:
    """
    Gets properties for multiple widget classes at once.

    Args:
        widget_classes: List of widget class names to query

    Returns:
        JSON string with properties for each widget class
    """
    command = {
        "type": "get_batch_widget_properties",
        "widget_classes": widget_classes
    }
    result = send_to_unreal(command)
    return json.dumps(result)

@mcp.tool()
def scan_directory(directory_path: str, extensions: list[str] = None) -> str:
    """
    Scans a filesystem directory for files with specified extensions.

    Args:
        directory_path: Absolute path to the directory
        extensions: List of file extensions to include (e.g., [".cpp", ".h"]). Defaults to [".cpp", ".h", ".cs"]

    Returns:
        JSON string with list of files and count
    """
    command = {
        "type": "scan_directory",
        "directory_path": directory_path,
        "extensions": extensions
    }
    result = send_to_unreal(command)
    return json.dumps(result)

@mcp.tool()
def select_folder(folder_path: str) -> str:
    """
    Syncs the Content Browser to a specific folder.

    Args:
        folder_path: Folder path to select (e.g., /Game/Blueprints)

    Returns:
        JSON string with result
    """
    command = {
        "type": "select_folder",
        "folder_path": folder_path
    }
    result = send_to_unreal(command)
    return json.dumps(result)

@mcp.tool()
def check_project_source() -> str:
    """
    Checks if the project has C++ source code available.

    Returns:
        JSON string with has_source_code, has_project_file, has_source_control flags
    """
    command = {
        "type": "check_project_source"
    }
    result = send_to_unreal(command)
    return json.dumps(result)


# ==================================================================
# PLUGIN TOOLS
# ==================================================================

@mcp.tool()
def list_plugins() -> str:
    """
    Lists all enabled plugins in the project with metadata.

    Returns:
        str: JSON string with plugins array containing name, friendly_name, version, enabled, type, and directory for each plugin.
    """
    command = {"type": "list_plugins"}
    result = send_to_unreal(command)
    return json.dumps(result)

@mcp.tool()
def find_plugin(search_pattern: str) -> str:
    """
    Finds plugins by search pattern (fuzzy matching).

    Args:
        search_pattern (str): Pattern to search for in plugin names and friendly names.

    Returns:
        str: JSON string with matching plugins array.
    """
    command = {"type": "find_plugin", "search_pattern": search_pattern}
    result = send_to_unreal(command)
    return json.dumps(result)

@mcp.tool()
def list_plugin_files(plugin_name: str, relative_path: str = "") -> str:
    """
    Lists files in a plugin directory.

    Args:
        plugin_name (str): The name or friendly name of the plugin.
        relative_path (str, optional): Relative path within the plugin to list files from.

    Returns:
        str: JSON string with files array containing name, path, is_directory, extension, and size_bytes.
    """
    command = {"type": "list_plugin_files", "plugin_name": plugin_name, "relative_path": relative_path}
    result = send_to_unreal(command)
    return json.dumps(result)


# ==================================================================
# C++ FILE TOOLS
# ==================================================================

@mcp.tool()
def read_cpp_file(file_path: str) -> str:
    """
    Reads a C++ source file (.h, .cpp, .hpp, .c, .cc, .inl).

    Args:
        file_path (str): Path to the C++ source file (can be relative to project directory).

    Returns:
        str: JSON string with file content, absolute_path, and content_length.
    """
    command = {"type": "read_cpp_file", "file_path": file_path}
    result = send_to_unreal(command)
    return json.dumps(result)

@mcp.tool()
def write_cpp_file(file_path: str, content: str) -> str:
    """
    Writes content to a C++ source file (.h, .cpp, .hpp, .c, .cc, .inl).
    Only allows writing to project Source folder or engine plugins directory for security.

    Args:
        file_path (str): Path to the C++ source file (can be relative to project directory).
        content (str): Content to write to the file.

    Returns:
        str: JSON string with success status and bytes_written.
    """
    command = {"type": "write_cpp_file", "file_path": file_path, "content": content}
    result = send_to_unreal(command)
    return json.dumps(result)


# ==================================================================
# MAIN SCRIPT EXECUTION
# ==================================================================
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run a specific MCP server.")
    parser.add_argument('--server-name', required=False, default="unreal-handshake", help="The name of the server to run.")
    args = parser.parse_args()

    print(f"Python MCP script started. Launching server: '{args.server_name}'", file=sys.stderr)
    mcp.run()


