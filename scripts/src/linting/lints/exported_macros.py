from dataclasses import dataclass
from pathlib import Path
import re

from src.linting.linter import (
    Error,
    LintContext,
    LinterCheck,
    Result,
    ResultSingle,
    ResultMultiple,
)
from src.linting.tags import FOR_TEMPLATE, MACRO_PREFIX, MACRO_FIX_PRIVATE

EXPECTED_NON_PREFIXED: set[str] = {
    "NS",
    "PRIV",
}


@dataclass
class UnexpectedNonNamespacedMacro(Error):
    macro: str

    def describe(self) -> str:
        return f"{self.macro} was defined, but is not prefixed with `{MACRO_PREFIX}` or `{MACRO_FIX_PRIVATE}`"


class MacroExports(LinterCheck):
    f"""
    Checks all defines that are not:
     - Defined in a template.h file
     - Defined in a def.h/undef.h file
    
    This check aims to ensure all names are either prefixed with `{MACRO_PREFIX}`, `{MACRO_FIX_PRIVATE}` expected to not be.
    """

    def run(self, ctx: LintContext) -> Result:
        files = [f for f in ctx.source_dir.rglob("*") if self._should_check_file(f)]
        futures = {f: ctx.executor.submit(self.check_file, f) for f in files}
        return ResultMultiple({f: future.result() for f, future in futures.items()})

    def _should_check_file(self, file_path: Path) -> bool:
        if not file_path.is_file():
            return False
        return file_path.name not in {"template.h", "def.h", "undef.h"}

    def check_file(self, file_path: Path) -> ResultSingle:
        define_pattern = re.compile(r"^\s*#\s*define\s+([a-zA-Z_]\w*)\b")
        undef_pattern = re.compile(r"^\s*#\s*undef\s+([a-zA-Z_]\w*)\b")
        errors: list[Error] = []
        defines: list[str] = []
        undefs: set[str] = set()

        for line in file_path.read_text(encoding="utf-8").splitlines():
            match = define_pattern.match(line)
            if not match:
                undef_match = undef_pattern.match(line)
                if undef_match:
                    undefs.add(undef_match.group(1))
                continue
            if line.rstrip().endswith(FOR_TEMPLATE):
                continue
            defines.append(match.group(1))

        for name in defines:
            if name in undefs:
                continue
            if name.startswith(MACRO_PREFIX) or name.startswith(MACRO_FIX_PRIVATE):
                continue
            if name in EXPECTED_NON_PREFIXED:
                continue
            errors.append(UnexpectedNonNamespacedMacro(name))

        return ResultSingle(errors)
