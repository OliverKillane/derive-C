from __future__ import annotations

from pathlib import Path
import re
from typing import Self
from dataclasses import dataclass, field

from src.linting.linter import (
    DERIVE_C_TAG,
    LinterCheck,
    ResultSingle,
    LintContext,
    ResultMultiple,
    Result,
    Error,
)

EXPORT_COMMENT: str = r"// IWYU pragma: export"
STDLIB_INCLUDES: str = rf"// {DERIVE_C_TAG} stdlib includes"
DERIVE_C_INCLUDES: str = rf"// {DERIVE_C_TAG} lib includes"
USED_TEMPLATE_INCLUDES: str = rf"// {DERIVE_C_TAG} used template includes"
CONTAINER_SPECIFIC_INCLUDES: str = rf"// {DERIVE_C_TAG} container includes"

# Match lines like:
#   // [DERIVE-C] stdlib includes
# and keep the section name (e.g. "stdlib includes").
SECTION_PATTERN: re.Pattern[str] = re.compile(
    r"^\s*//\s*\[DERIVE-C\]\s*(?P<name>.+?)\s*$"
)

# Match #include <...> or #include "..."
INCLUDE_PATTERN: re.Pattern[str] = re.compile(
    r'^\s*#\s*include\s*(?P<delim><|")(?P<path>[^>"]+)(?P=delim)\s*(?P<tail>.*)$'
)

# Detect "IWYU pragma: export" anywhere after the include.
EXPORT_PATTERN: re.Pattern[str] = re.compile(r"\bIWYU\s+pragma:\s*export\b")


@dataclass(frozen=True, slots=True)
class Include:
    path: str
    export: bool

    @classmethod
    def from_line(cls, line: str) -> "Include | None":
        m = INCLUDE_PATTERN.match(line)
        if not m:
            return None
        path = m.group("path").strip()
        tail = m.group("tail") or ""
        export = EXPORT_PATTERN.search(tail) is not None
        return cls(path=path, export=export)


@dataclass(slots=True)
class Section:
    """
    A [DERIVE-C] section.

    - name: the part after '// [DERIVE-C]' (e.g. 'stdlib includes')
    - header_comment: the full header comment line (preserved for ordering checks)
    """

    name: str
    header_comment: str
    includes: list[Include] = field(default_factory=list)

    @classmethod
    def from_header_comment(cls, line: str) -> "Section | None":
        m = SECTION_PATTERN.match(line)
        if not m:
            return None
        name = m.group("name").strip()
        header = line.strip()
        return cls(name=name, header_comment=header)

    def add_include_line(self, line: str) -> bool:
        inc = Include.from_line(line)
        if inc is None:
            return False
        self.includes.append(inc)
        return True


@dataclass(slots=True)
class IncludeFiles:
    """
    Parsed representation of an includes.h file:
      - pragma_once present?
      - list of [DERIVE-C] sections in file order

    Note: this parser only considers includes that appear *inside* a [DERIVE-C] section.
    """

    pragma_once: bool
    sections: list[Section] = field(default_factory=list)

    @classmethod
    def from_lines(cls, lines: list[str]) -> Self:
        pragma_once = False
        sections: list[Section] = []
        current: Section | None = None

        for raw in lines:
            line = raw.rstrip("\n")
            stripped = line.strip()

            if stripped == "#pragma once":
                pragma_once = True
                continue

            sec = Section.from_header_comment(line)
            if sec is not None:
                sections.append(sec)
                current = sec
                continue

            if current is not None:
                current.add_include_line(line)

        return cls(pragma_once=pragma_once, sections=sections)

    @classmethod
    def from_file(cls, file: Path, encoding: str = "utf-8") -> Self:
        return cls.from_lines(file.read_text(encoding=encoding).splitlines())


# ---------- Errors ----------


@dataclass
class MissingPragma(Error):
    def describe(self) -> str:
        return "Missing '#pragma once'"


@dataclass
class UnexpectedSectionOrder(Error):
    expected: list[str]
    actual: list[str]

    def describe(self) -> str:
        return (
            f"Section headers are out of order. Expected '{self.expected}' "
            f"but found '{self.actual}'."
        )


@dataclass
class MissingExport(Error):
    include: Include

    def describe(self) -> str:
        return f"Include '{self.include.path}' is missing {EXPORT_COMMENT}."


# ---------- Check ----------


class IncludesExport(LinterCheck):
    """
    Checks that every include in includes.h files has:
      - // IWYU pragma: export at the end of each include
      - includes ordered as stdlib, then derive-c, then used-template, then container specific
      - #pragma once is present
    """

    def run(self, ctx: LintContext) -> Result:
        files = list(ctx.source_dir.rglob("includes.h"))
        futures = {
            f: ctx.executor.submit(self.check_file, f, ctx.source_dir) for f in files
        }
        return ResultMultiple({f: future.result() for f, future in futures.items()})

    def check_file(self, file_path: Path, source_dir: Path) -> ResultSingle:
        lines = file_path.read_text(encoding="utf-8").splitlines()
        includes = IncludeFiles.from_lines(lines)
        return self.validate_includes(includes, file_path)

    def validate_includes(
        self, include_files: IncludeFiles, file_path: Path
    ) -> ResultSingle:
        errors: list[Error] = []

        if not include_files.pragma_once:
            errors.append(MissingPragma())

        expected_section_order = [
            STDLIB_INCLUDES,
            DERIVE_C_INCLUDES,
            USED_TEMPLATE_INCLUDES,
            CONTAINER_SPECIFIC_INCLUDES,
        ]

        actual_section_order: list[str] = [
            s.header_comment for s in include_files.sections
        ]

        # Allow "used template includes" to be omitted.
        allowed_orders = {
            tuple(expected_section_order),
            (STDLIB_INCLUDES, DERIVE_C_INCLUDES, CONTAINER_SPECIFIC_INCLUDES),
            (STDLIB_INCLUDES, DERIVE_C_INCLUDES, USED_TEMPLATE_INCLUDES),
            (STDLIB_INCLUDES, DERIVE_C_INCLUDES),
        }

        if tuple(actual_section_order) not in allowed_orders:
            errors.append(
                UnexpectedSectionOrder(
                    expected=expected_section_order,
                    actual=actual_section_order,
                )
            )

        for section in include_files.sections:
            for include in section.includes:
                if not include.export:
                    errors.append(MissingExport(include=include))

        return ResultSingle(errors)
