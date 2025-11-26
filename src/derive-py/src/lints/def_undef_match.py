from pathlib import Path
import re
from src.linter import LinterCheck, Result, LintContext, Location, CheckStatus, SubLints, LintTree

class DefUndefIncludes(LinterCheck):
    """Checks that defines in def.h are undefined in undef.h."""

    def run(self, ctx: LintContext) -> LintTree:
        # Find all directories containing both def.h and undef.h
        def_files = list(ctx.source_dir.rglob("def.h"))
        
        futures = {ctx.executor.submit(self.check_pair, f, ctx.source_dir): f for f in def_files}
        
        results = {}
        for future in futures:
            f = futures[future]
            rel_path = str(f.parent.relative_to(ctx.source_dir))
            try:
                results[rel_path] = future.result()
            except Exception as e:
                results[rel_path] = Result(
                    location=Location(file=f, line=None),
                    status=CheckStatus.FAIL,
                    message=f"Exception: {e}"
                )
        
        return SubLints(results) if results else Result(
            location=Location(file=ctx.source_dir, line=None),
            status=CheckStatus.PASS,
            message="No def.h files found"
        )

    def check_pair(self, def_file: Path, source_dir: Path) -> Result:
        undef_file = def_file.parent / "undef.h"
        
        if not undef_file.exists():
            return Result(
                location=Location(file=def_file, line=None),
                status=CheckStatus.FAIL,
                message=f"No corresponding undef.h found"
            )
        
        try:
            with open(def_file, "r", encoding="utf-8") as f:
                def_lines = f.readlines()
            with open(undef_file, "r", encoding="utf-8") as f:
                undef_lines = f.readlines()
        except Exception as e:
            return Result(
                location=Location(file=def_file, line=None),
                status=CheckStatus.FAIL,
                message=f"Could not read files: {e}"
            )
        
        # Extract defined macros from def.h
        define_pattern = re.compile(r'^\s*#\s*define\s+(\w+)')
        undef_pattern = re.compile(r'^\s*#\s*undef\s+(\w+)')
        
        defined_macros = set()
        for line in def_lines:
            if match := define_pattern.match(line):
                defined_macros.add(match.group(1))
        
        undefined_macros = set()
        for line in undef_lines:
            if match := undef_pattern.match(line):
                undefined_macros.add(match.group(1))
        
        # Check that all defined macros are undefined
        missing_undefs = defined_macros - undefined_macros
        extra_undefs = undefined_macros - defined_macros
        
        if not missing_undefs and not extra_undefs:
            return Result(
                location=Location(file=def_file, line=None),
                status=CheckStatus.PASS,
                message="All defines have corresponding undefs"
            )
        
        msgs = []
        if missing_undefs:
            msgs.append(f"Missing undefs: {', '.join(sorted(missing_undefs))}")
        if extra_undefs:
            msgs.append(f"Extra undefs: {', '.join(sorted(extra_undefs))}")
        
        return Result(
            location=Location(file=def_file, line=None),
            status=CheckStatus.FAIL,
            message="; ".join(msgs)
        )
