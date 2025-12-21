from dataclasses import dataclass
from typing import Self
from pathlib import Path
from enum import Enum, auto
from src.linting.linter import (
    DERIVE_C_TAG,
    LinterCheck,
    ResultSingle,
    LintContext,
    ResultMultiple,
    Result,
    Error,
)
import re

ARGUMENT = f"// {DERIVE_C_TAG} argument"


class Kind(Enum):
    NORMAL = auto()
    FOR_ARG = auto()


@dataclass(frozen=True)
class Define:
    name: str
    kind: Kind


@dataclass
class Defines:
    defs: set[Define]
    undefs: set[Define]

    @classmethod
    def from_strings(cls, defs: list[str], undefs: list[str]) -> Self:
        defined_defines = set[Define]()
        undefined_defines = set[Define]()

        define_pattern = re.compile(r"^\s*#define\s+([a-zA-Z_]\w*).*")
        undef_pattern = re.compile(r"^\s*#undef\s+([a-zA-Z_]\w*).*")

        for line in defs:
            match = define_pattern.match(line)
            if match:
                name = match.group(1)
                kind = Kind.FOR_ARG if ARGUMENT in line else Kind.NORMAL
                defined_defines.add(Define(name, kind))

        for line in undefs:
            match = undef_pattern.match(line)
            if match:
                name = match.group(1)
                kind = Kind.FOR_ARG if ARGUMENT in line else Kind.NORMAL
                undefined_defines.add(Define(name, kind))

        return cls(defs=defined_defines, undefs=undefined_defines)

    def errors(self) -> list[Error]:
        errors: list[Error] = []
        for define in self.defs - self.undefs:
            errors.append(LeakedDefine(define.name))

        # Filter undefs that are not in defs
        extra_undefs = self.undefs - self.defs

        for undef_define in extra_undefs:
            # Only report ExtraUndef if it's not a FOR_ARG kind, or if it's FOR_ARG but still has no corresponding define
            # The logic "when no def in errors()" implies that if it's a FOR_ARG undef, and there's no normal def, it's fine.
            # We already have `extra_undefs` which means there is no exact `Define` match in `self.defs`.
            # So, if it's `FOR_ARG`, we skip the error.
            if undef_define.kind == Kind.NORMAL:
                errors.append(ExtraUndef(undef_define.name))
        return errors


@dataclass
class LeakedDefine(Error):
    name: str

    def describe(self) -> str:
        return f"Leaked define: {self.name}"


@dataclass
class ExtraUndef(Error):
    name: str

    def describe(self) -> str:
        return f"Extra undef: {self.name}"


@dataclass
class MissingUndefFile(Error):
    file: Path

    def describe(self) -> str:
        return f"No corresponding undef.h found for {self.file}"


class DefUndefIncludes(LinterCheck):
    """Checks that defines in def.h are undefined in undef.h."""

    def run(self, ctx: LintContext) -> Result:
        def_files = list(ctx.source_dir.rglob("def.h"))
        futures = {ctx.executor.submit(self.check_pair, f): f for f in def_files}
        return ResultMultiple({f: future.result() for future, f in futures.items()})

    def check_pair(self, def_file: Path) -> ResultSingle:
        undef_file = def_file.parent / "undef.h"

        if not undef_file.exists():
            return ResultSingle(errors=[MissingUndefFile(def_file)])

        with open(def_file, "r", encoding="utf-8") as f:
            def_lines = f.readlines()
        with open(undef_file, "r", encoding="utf-8") as f:
            undef_lines = f.readlines()

        defines = Defines.from_strings(def_lines, undef_lines)
        return ResultSingle(defines.errors())
