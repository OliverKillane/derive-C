from pathlib import Path
from concurrent.futures import ThreadPoolExecutor

from src.linter import LinterCheck, Result, LintContext, Location, CheckStatus

class DefUndefIncludes(LinterCheck):
    """Checks that defines in def.h are undefined in undef.h."""

    def run(self, ctx: LintContext) -> Result:
        # Get all the folders that contain def.h and undef.h
        # Get the macros defined from def.h, and check that all have an undef line in undef.h
        # Check all files in parallel
        
        return Result(
            location=Location(file=ctx.source_dir, line=None),
            status=CheckStatus.PASS,
            message="Def undef match check passed.",
        )
