from __future__ import annotations

from abc import ABC, abstractmethod
from dataclasses import dataclass
from enum import Enum
from pathlib import Path
from typing import Dict, Self, TypeAlias
from concurrent.futures import ThreadPoolExecutor

@dataclass(frozen=True)
class Location:
    file: Path
    line: int | None

    def __str__(self) -> str:
        if self.line is None:
            return str(self.file)
        return f"{self.file}:{self.line}"

class Error(ABC):
    @abstractmethod
    def describe(self) -> str:
        pass

@dataclass(frozen=True)
class ResultSingle:
    errors: list[Error]

    @property
    def successful(self) -> bool:
        return len(self.errors) == 0


@dataclass(frozen=True)
class ResultMultiple:
    results: dict[str, Result]
    
    @property
    def successful(self) -> bool:
        return all(result.successful for result in self.results.values())

Result: TypeAlias = ResultSingle | ResultMultiple

@dataclass
class LintContext:
    source_dir: Path
    executor: ThreadPoolExecutor

class LinterCheck(ABC):
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
    results: dict[LinterCheck, Result]

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
    
    @property
    def successful(self) -> bool:
        return all(result.successful for result in self.results.values())

class LintOutput(ABC):
    @abstractmethod
    def write(self, results: LintResults) -> None:
        pass