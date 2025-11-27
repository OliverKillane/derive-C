from enum import Enum, auto
from pathlib import Path
import re
from dataclasses import dataclass
from typing import Self

from src.linter import LinterCheck, Result, LintContext, Location, CheckStatus, SubLints, LintTree
from src.utils import RED, GREEN, RESET

FOR_TEMPLATE: str = "// for template"

class Action(Enum):
    DEFINE = auto()
    UNDEF = auto()

class Kind(Enum):
    MACRO = auto()
    INCLUDE = auto()

class Attrib(Enum):
    NORMAL = auto()
    TEMPLATE = auto()

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

        attrib = Attrib.TEMPLATE if s.endswith(FOR_TEMPLATE) else Attrib.NORMAL
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
class UnexpectedUndef:
    expected: Define
    actual: Event

    def describe(self) -> str:
        return f"{RED}{self.actual.describe()} unexpected, next undef could only be: {self.expected.describe(Action.UNDEF)}{RESET}"

@dataclass
class ImpossibleUndef:
    actual: Event

    def describe(self) -> str:
        return f"{RED}{self.actual.describe()} impossible as it is not defined at this point{RESET}"

@dataclass
class Correct:
    event: Event

    def describe(self) -> str:
        return f"{GREEN}{self.event.describe()}{RESET}"

@dataclass
class MissingUndef:
    expected: Define

    def describe(self) -> str:
        return f"{RED}{self.expected.describe(Action.UNDEF)} expected{RESET}"

@dataclass
class Tracker:
    stack: list[Define]
    diagnostics: list[Correct | ImpossibleUndef | UnexpectedUndef | MissingUndef]

    @classmethod
    def process_events(cls, events: list[Event]) -> list[Correct | ImpossibleUndef | UnexpectedUndef | MissingUndef]: 
        state = cls(
            stack=[],
            diagnostics=[],
        )

        for event in events:
            state._process_event(event)

        for define in state.stack[::-1]:
            state.diagnostics.append(MissingUndef(define))

        return state.diagnostics

    def _process_event(self, event: Event) -> None:
        match event.action:
            case Action.DEFINE:
                if event.define not in self.stack:
                    if event.attrib != Attrib.TEMPLATE:
                        self.stack.append(event.define)
                    else:
                        # ignore for templates
                        pass
                else:
                    # duplicate define, we ignore these
                    pass
                self.diagnostics.append(Correct(event))
            case Action.UNDEF:
                if len(self.stack) > 0:
                    if self.stack[-1] == event.define:
                        self.stack.pop()
                        self.diagnostics.append(Correct(event))
                    else:
                        self.diagnostics.append(UnexpectedUndef(self.stack[-1], event))
                else:
                    self.diagnostics.append(ImpossibleUndef(event))

class TemplateDefines(LinterCheck):
    """
    Checks that we correctly undef defines in template files:
     - undefine in reverse order of defines
     - no defines leaked
    """
    
    def run(self, ctx: LintContext) -> LintTree:
        files = list(ctx.source_dir.rglob("template.h"))
        futures = {ctx.executor.submit(self.check_file, f): f for f in files}
        
        results = {}
        for future in futures:
            f = futures[future]
            rel_path = str(f.relative_to(ctx.source_dir))
            results[rel_path] = future.result()
        
        return SubLints(results) if results else Result(
            location=Location(file=ctx.source_dir, line=None),
            status=CheckStatus.PASS,
            message="No template.h files found"
        )

    def check_file(self, file_path: Path) -> SubLints:
        with open(file_path, "r", encoding="utf-8") as f:
            events = Event.from_file(file_path)
            diagnostics = Tracker.process_events(events)
            
            if all(isinstance(d, Correct) for d in diagnostics):
                return Result(
                    location=Location(file=file_path, line=None),
                    status=CheckStatus.PASS,
                    message="No errors"
                )
            else:
                return Result(
                        location=Location(file=file_path, line=None),
                        status=CheckStatus.FAIL,
                        message=f"""
Failed to validate, with diagnostics:
{'\n'.join(d.describe() for d in diagnostics)}""")
