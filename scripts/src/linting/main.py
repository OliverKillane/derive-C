import argparse
import sys
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from typing import List, Dict

from src.linting.linter import LinterCheck, LintContext, ResultMultiple, LintResults
from src.linting.lints.template_defines import TemplateDefines
from src.linting.lints.def_undef_match import DefUndefIncludes
from src.linting.lints.includes_export import IncludesExport
from src.linting.output.console import ConsoleOutput

LINTS: list[LinterCheck] = [
    TemplateDefines(),
    DefUndefIncludes(),
    IncludesExport(),
]

def main():
    parser = argparse.ArgumentParser(description="Custom Linter for Derive-C")
    parser.add_argument("source_dir", type=Path, help="Path to the source directory")
    args = parser.parse_args()

    if not args.source_dir.exists():
        print(f"Error: Source directory '{args.source_dir}' does not exist.")
        sys.exit(1)

    with ThreadPoolExecutor() as executor:
        results = LintResults.from_context(
            LintContext(
                source_dir=args.source_dir,
                executor=executor,
            ),
            LINTS,
        )
    
    ConsoleOutput().write(results)

    if not results.successful:
        sys.exit(1)

    sys.exit(0)

if __name__ == "__main__":
    main()
