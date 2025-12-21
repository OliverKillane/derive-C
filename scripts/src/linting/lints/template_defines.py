from enum import Enum, auto
from pathlib import Path
import re
from dataclasses import dataclass
from typing import Self

from src.linting.linter import (
    DERIVE_C_TAG,
    LinterCheck,
    ResultSingle,
    LintContext,
    Location,
    ResultMultiple,
    Result,
    Error,
)
from src.linting.utils import RED, RESET

FOR_TEMPLATE: str = f"// {DERIVE_C_TAG} for template"
FOR_INPUT_ARG: str = f"// {DERIVE_C_TAG} for input arg"


class Action(Enum):
    DEFINE = auto()
    UNDEF = auto()


class Kind(Enum):
    MACRO = auto()
    INCLUDE = auto()


class Attrib(Enum):
    NORMAL = auto()
    TEMPLATE = auto()
    INPUT_ARG = auto()


@dataclass(frozen=True)
class Define:
    name: str
    kind: Kind

    def describe(self, action: Action) -> str:
        match self.kind:
            case Kind.MACRO:
                match action:
                    case Action.DEFINE:
                        return f"#define {self.name}"
                    case Action.UNDEF:
                        return f"#undef {self.name}"
            case Kind.INCLUDE:
                match action:
                    case Action.DEFINE:
                        return f"#include <{self.name}/def.h>"
                    case Action.UNDEF:
                        return f"#include <{self.name}/undef.h>"


@dataclass(frozen=True)
class Event:
    action: Action
    attrib: Attrib
    location: Location
    define: Define

    def describe(self) -> str:
        output: str = self.define.describe(self.action)
        if self.attrib == Attrib.TEMPLATE:
            output += f" {FOR_TEMPLATE}"
        return output

    @classmethod
    def from_line(cls, location: Location, line: str) -> Self | None:
        s = line.strip()
        if not s:
            return None

        if s.endswith(FOR_TEMPLATE):
            attrib = Attrib.TEMPLATE
        elif s.endswith(FOR_INPUT_ARG):
            attrib = Attrib.INPUT_ARG
        else:
            attrib = Attrib.NORMAL
        if attrib is Attrib.TEMPLATE:
            s = s[: -len(FOR_TEMPLATE)].rstrip()

        b = s.lstrip()
        action = kind = None
        name = ""

        if b.startswith("#define "):
            action, kind = Action.DEFINE, Kind.MACRO
            tail = b[8:].lstrip()
            name = re.split(r"[ \t(]", tail, 1)[0]
        elif b.startswith("#undef "):
            action, kind = Action.UNDEF, Kind.MACRO
            name = b[7:].split(" ", 1)[0].strip()
        elif b.startswith("#include <") and b.endswith(">"):
            inner = b[10:-1].strip()
            if inner.endswith("/def.h"):
                action, kind = Action.DEFINE, Kind.INCLUDE
                name = inner[:-6]
            elif inner.endswith("/undef.h"):
                action, kind = Action.UNDEF, Kind.INCLUDE
                name = inner[:-8]

        if not action or not kind or not name:
            return None

        return cls(action, attrib, location, Define(name, kind))

    @classmethod
    def from_file(cls, file: Path) -> list[Self]:
        events = []
        with open(file) as f:
            for line_num, line in enumerate(f):
                event = cls.from_line(Location(file=file, line=line_num), line)
                if event:
                    events.append(event)
        return events


@dataclass
class UnexpectedUndef(Error):
    expected: Define
    actual: Event

    def describe(self) -> str:
        return f"{RED}{self.actual.describe()} unexpected, next undef could only be: {self.expected.describe(Action.UNDEF)}{RESET}"


@dataclass
class ImpossibleUndef(Error):
    actual: Event

    def describe(self) -> str:
        return f"{RED}{self.actual.describe()} impossible as it is not defined at this point{RESET}"


@dataclass
class MissingUndef(Error):
    expected: Define

    def describe(self) -> str:
        return f"{RED}{self.expected.describe(Action.UNDEF)} expected{RESET}"


@dataclass
class Tracker:
    stack: list[Define]
    diagnostics: list[Error]

    @classmethod
    def process_events(cls, events: list[Event]) -> list[Error]:
        state = cls(stack=[], diagnostics=[])
        for event in events:
            state._process_event(event)
        for define in state.stack[::-1]:
            state.diagnostics.append(MissingUndef(define))
        return state.diagnostics

    def _process_event(self, event: Event) -> None:
        match event.action:
            case Action.DEFINE:
                if event.define not in self.stack:
                    if event.attrib == Attrib.NORMAL:
                        self.stack.append(event.define)
                    else:
                        # ignore for templates
                        pass
                else:
                    # duplicate define, we ignore these
                    pass
            case Action.UNDEF:
                if len(self.stack) > 0:
                    if event.attrib == Attrib.NORMAL:
                        if self.stack[-1] == event.define:
                            self.stack.pop()
                        else:
                            self.diagnostics.append(
                                UnexpectedUndef(self.stack[-1], event)
                            )
                else:
                    self.diagnostics.append(ImpossibleUndef(event))


class TemplateDefines(LinterCheck):
    """
    Checks that we correctly undef defines in template files:
     - undefine in reverse order of defines
     - no defines leaked
    """

    def run(self, ctx: LintContext) -> Result:
        files = list(ctx.source_dir.rglob("template.h"))
        futures = {f: ctx.executor.submit(self.check_file, f) for f in files}
        return ResultMultiple({f: future.result() for f, future in futures.items()})

    def check_file(self, file_path: Path) -> ResultSingle:
        events = Event.from_file(file_path)
        return ResultSingle(
            errors=Tracker.process_events(events),
        )
