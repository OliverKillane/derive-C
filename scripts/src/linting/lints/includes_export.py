from pathlib import Path
import re
from typing import Self
from dataclasses import dataclass
from src.linting.linter import LinterCheck, ResultSingle, LintContext, Location, ResultMultiple, Result, Error

EXPORT_COMMENT: str = r"// IWYU pragma: export"
STDLIB_INCLUDES: str = r"// stdlib includes"
DERIVE_C_INCLUDES: str = r"// derive-c includes"
CONTAINER_SPECIFIC_INCLUDES: str = r"// container-specific includes"
INCLUDE_PATTERN: re.Pattern[str] = re.compile(r'#include\s+(<[^>]+>|"[^"]+")')

@dataclass
class Include:
    path: str
    export: bool

@dataclass
class Section:
    header_comment: str | None
    includes: list[Include]

@dataclass
class IncludeOrder:
    pragma_once: bool
    sections: list[Section]

    @classmethod
    def from_string(cls, lines: list[str]) -> Self:
        pragma_once = False
        sections: list[Section] = []
        current_section: Section | None = None
        for line in lines:
            stripped_line = line.strip()
            if stripped_line == "#pragma once":
                pragma_once = True
            elif stripped_line.startswith("//") and stripped_line != EXPORT_COMMENT:
                if current_section is not None and not current_section.includes and current_section.header_comment is None:
                    pass
                current_section = Section(header_comment=stripped_line, includes=[])
                sections.append(current_section)
            elif include_match := INCLUDE_PATTERN.match(stripped_line):
                if current_section is None:
                    # If an include appears before any section header, create an implicit section for it.
                    # This handles cases where headers might be missing or files start directly with includes.
                    current_section = Section(header_comment=None, includes=[])
                    sections.append(current_section)
                current_section.includes.append(Include(path=include_match.group(1), export=current_section.header_comment))

        return cls(pragma_once=pragma_once, sections=sections)

    @classmethod
    def from_file(cls, file: Path) -> Self:
        with open(file, "r", encoding="utf-8") as f:
            return cls.from_string(f.read())

    def validate(self) -> ResultSingle:
        pass

@dataclass
class MissingPragma(Error):
    def describe(self) -> str:
        return "Missing '#pragma once'"

@dataclass 
class UnexpectedSectionOrder(Error):
    expected: list[str]
    actual: list[str]
    
    def describe(self) -> str:
        return f"Section headers are out of order. Expected '{self.expected}' but found '{self.actual}'."

@dataclass
class MissingExport(Error):
    include: Include
    
    def describe(self) -> str:
        return f"Include '{self.include.path}' is missing {EXPORT_COMMENT}."

class IncludesExport(LinterCheck):
    """
    Checks that every include in includes.h files has:
      - // IWYU pragma: export at the end of each include
      - includes ordered as stdlib, then derive-c, then container specific
      - #pragma once is present
    """

    def run(self, ctx: LintContext) -> Result:
        files = list(ctx.source_dir.rglob("includes.h"))
        futures = {f: ctx.executor.submit(self.check_file, f, ctx.source_dir) for f in files}
        return ResultMultiple({f: future.result() for f, future in futures.items()})

    def check_file(self, file_path: Path, source_dir: Path) -> ResultMultiple:
        with open(file_path, "r", encoding="utf-8") as f:
            lines = f.readlines()
            includes = IncludeOrder.from_string(lines)
        return self.validate_includes(includes, file_path)

    def validate_includes(self, include_order: IncludeOrder, file_path: Path) -> ResultSingle:
        errors: list[Error] = []

        if not include_order.pragma_once:
            errors.append(MissingPragma())
        
        expected_section_order = [STDLIB_INCLUDES, DERIVE_C_INCLUDES, CONTAINER_SPECIFIC_INCLUDES]
        actual_section_order = []
        
        for section_idx, section in enumerate(include_order.sections):
            if section.header_comment:
                actual_section_order.append(section.header_comment)
        
        if actual_section_order == expected_section_order:
            pass
        elif actual_section_order == [STDLIB_INCLUDES, DERIVE_C_INCLUDES]:
            pass
        else:
            errors.append(UnexpectedSectionOrder(expected=expected_section_order, actual=actual_section_order))
        
        for section_idx, section in enumerate(include_order.sections):
            for include_idx, include in enumerate(section.includes):
                if not include.export:
                    errors.append(MissingExport(include=include))
        
        return ResultSingle(errors)
