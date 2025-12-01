from dataclasses import dataclass
from pathlib import Path
import json
import argparse

@dataclass
class Config:
    vscode_dir: Path
    clangd_path: str
    clang_path: str
    resource_dir: str
    compile_commands_dir: str

def generate_settings(config: Config) -> None:
    settings_path = config.vscode_dir / "settings.json"
    
    # Load existing settings or create new dict
    if settings_path.exists():
        with open(settings_path, "r") as f:
            try:
                settings = json.load(f)
            except json.JSONDecodeError:
                settings = {}
    else:
        settings = {}

    # Update clangd settings
    settings["clangd.path"] = config.clangd_path
    settings["clangd.arguments"] = [
        f"--query-driver={config.clang_path}",
        f"--resource-dir={config.resource_dir}",
        f"--compile-commands-dir={config.compile_commands_dir}",
        "--log=verbose",
        "--pch-storage=memory"
    ]

    # Ensure .vscode directory exists
    config.vscode_dir.mkdir(parents=True, exist_ok=True)

    # Write settings back
    with open(settings_path, "w") as f:
        json.dump(settings, f, indent=2)
        f.write("\n") # Add newline at end of file

def main() -> None:
    parser = argparse.ArgumentParser(description="Generate .vscode/settings.json for clangd")
    parser.add_argument("--vscode-dir", type=Path, required=True, help="Path to .vscode directory")
    parser.add_argument("--clangd-path", required=True, help="Path to clangd executable")
    parser.add_argument("--clang-path", required=True, help="Path to clang executable (for query-driver)")
    parser.add_argument("--resource-dir", required=True, help="Path to clang resource directory")
    parser.add_argument("--compile-commands-dir", default="build", help="Path to compile_commands.json directory")

    args = parser.parse_args()

    config = Config(
        vscode_dir=args.vscode_dir,
        clangd_path=args.clangd_path,
        clang_path=args.clang_path,
        resource_dir=args.resource_dir,
        compile_commands_dir=args.compile_commands_dir
    )

    generate_settings(config)
