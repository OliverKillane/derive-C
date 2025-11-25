from __future__ import annotations

from abc import ABC, abstractmethod
from dataclasses import dataclass
from enum import Enum
from pathlib import Path
from typing import Dict, Self
from concurrent.futures import ThreadPoolExecutor

@dataclass(frozen=True)
class Location:
    file: Path
    line: int | None

    def __str__(self) -> str:
        if self.line is None:
            return str(self.file)
        return f"{self.file}:{self.line}"

class CheckStatus(Enum):
    PASS = "PASS"
    FAIL = "FAIL"

@dataclass(frozen=True)
class Result:
    location: Location
    status: CheckStatus
    message: str

    def __str__(self) -> str:
        return f"{self.location} {self.status}: {self.message}"

    @property
    def successful(self) -> bool:
        return self.status == CheckStatus.PASS

@dataclass(frozen=True)
class SubLints:
    results: dict[str, LintTree]

    def __str__(self) -> str:
        return "\n".join(str(result) for result in self.results.values())
    
    @property
    def successful(self) -> bool:
        return all(result.successful for result in self.results.values())

LintTree: TypeAlias = Result | SubLints

@dataclass
class LintContext:
    source_dir: Path
    executor: ThreadPoolExecutor

class LinterCheck(ABC):
    def execute(self, ctx: LintContext) -> LintTree:
        try:
            return self.run(ctx)
        except Exception as e:
            return Result(
                location=Location(file=ctx.source_dir, line=None),
                status=CheckStatus.FAIL,
                message=str(e)
            )

    @abstractmethod
    def run(self, ctx: LintContext) -> Result:
        pass

    @property
    def name(self) -> str:
        return self.__class__.__name__

    @property
    def description(self) -> str:
        return self.__doc__ or "No description provided."

@dataclass
class LintResults:
    results: dict[LinterCheck, LintTree]

    @classmethod
    def from_context(
        cls,
        ctx: LintContext,
        checks: list[LinterCheck]
    ) -> Self:
        with ctx.executor as executor:
            futures = {executor.submit(check.run, ctx): check for check in checks}
            results = {check: future.result() for future, check in futures.items()}
        return cls(results)

    def __str__(self) -> str:
        return "\n".join(str(result) for result in self.results.values())
    
    @property
    def successful(self) -> bool:
        return all(result.successful for result in self.results.values())
