# Copyright 2026, BlueprintsLab, All rights reserved
"""
Filesystem Access MCP Extension for Claude Desktop.
Provides file system operations (read, write, list) for use with Claude Desktop.
"""

import sys
import json
import argparse
from pathlib import Path
from mcp.server.fastmcp import FastMCP

# ==================================================================
# HELPER FUNCTIONS
# ==================================================================

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
        print(f"MCP (Filesystem): Could not decode {file_path} as UTF-8. Retrying with UTF-16-LE.", file=sys.stderr)
        with open(file_path, 'r', encoding='utf-16-le') as f:
            content = f.read()
            # Strip UTF-16 LE BOM if present
            if content.startswith('\ufeff'):
                content = content[1:]
            return json.loads(content) if is_json else content
    # Allow other exceptions like FileNotFoundError to be handled by the caller.

# ==================================================================
# SERVER 2: FILESYSTEM ACCESS
# ==================================================================
fs_mcp = FastMCP("filesystem-access")

@fs_mcp.tool()
def how_to_use() -> str:
    """
    Call this first in a new session. It returns master prompt that explains all available filesystem tools.
    """
    try:
        # This will correctly find the file inside the extension package
        current_dir = Path(__file__).parent
        md_file_path = current_dir / "filesystem_how_to_use.md"
        if not md_file_path.exists():
            return "Error: The instruction file 'filesystem_how_to_use.md' was not found."
        return _read_file_with_fallback(md_file_path, is_json=False)
    except Exception as e:
        return f"Error reading filesystem instruction file: {str(e)}"

def list_files_in_directory(directory_path: str, extensions: list[str] = None) -> str:
    """
    Recursively lists all files in a given directory, filtered by a list of extensions.

    Args:
        directory_path (str): The absolute path to directory to scan.
        extensions (list[str], optional): A list of file extensions to include (e.g., ['.cpp', '.h']).
                                          If None, it defaults to ['.cpp', '.h'].

    Returns:
        str: A JSON string of a list of file paths, or an error message.
    """
    if extensions is None:
        extensions = ['.cpp', '.h']

    try:
        path = Path(directory_path)
        if not path.is_dir():
            return json.dumps({"success": False, "error": f"Path is not a valid directory: {directory_path}"})

        found_files = []
        for file_path in path.rglob('*'):
            if file_path.is_file() and file_path.suffix.lower() in extensions:
                found_files.append(str(file_path))

        return json.dumps({"success": True, "files": found_files})
    except Exception as e:
        return json.dumps({"success": False, "error": f"An error occurred: {str(e)}"})

@fs_mcp.tool()
def read_file_contents(file_path: str) -> str:
    """
    Reads the entire content of a specified text file.

    Args:
        file_path (str): The absolute path to the file to read.

    Returns:
        str: The content of the file, or an error message if it fails.
    """
    try:
        path = Path(file_path)
        if not path.is_file():
            return f"Error: File not found at path: {file_path}"

        # Use helper for consistent, safe encoding handling
        return _read_file_with_fallback(path, is_json=False)
    except Exception as e:
        return f"Error reading file: {str(e)}"

@fs_mcp.tool()
def write_to_file(file_path: str, content: str, append: bool = False) -> str:
    """
    Writes or appends content to a specified text file. Creates the file and any necessary parent directories if they don't exist.
    CRITICAL SAFETY: This is a destructive operation. The AI must ask for user confirmation before calling this tool.

    Args:
        file_path (str): The absolute path to the file to write to.
        content (str): The content to write to the file.
        append (bool, optional): If True, appends content to the end of the file.
                                 If False (default), overwrites the entire file.

    Returns:
        str: A message indicating success or failure.
    """
    try:
        path = Path(file_path)
        # Ensure parent directories exist
        path.parent.mkdir(parents=True, exist_ok=True)

        mode = 'a' if append else 'w'

        with open(path, mode, encoding='utf-8') as f:
            f.write(content)

        action = "Appended to" if append else "Wrote to"
        print(f"MCP (Filesystem): DANGEROUS OPERATION - {action} file at {file_path}", file=sys.stderr)
        return f"Success: {action} file at {file_path}"
    except Exception as e:
        return f"Error writing to file: {str(e)}"


# ==================================================================
# MAIN SCRIPT EXECUTION
# ==================================================================
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run the filesystem-access MCP server.")
    parser.add_argument('--server-name', required=False, default="filesystem-access", help="The name of the server to run.")
    args = parser.parse_args()

    print(f"Python MCP script started. Launching server: '{args.server_name}'", file=sys.stderr)
    fs_mcp.run()

