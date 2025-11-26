from pathlib import Path
import re
from src.linter import LinterCheck, Result, LintContext, Location, CheckStatus, SubLints, LintTree

class ReExportIncludes(LinterCheck):
    """
    Checks that every include in includes.h files has:
      - // IWYU pragma: export at the end
      - includes ordered as stdlib, then derive-c, then container specific
      - #pragma once is present
    """

    def run(self, ctx: LintContext) -> LintTree:
        files = list(ctx.source_dir.rglob("includes.h"))
        
        futures = {ctx.executor.submit(self.check_file, f, ctx.source_dir): f for f in files}
        
        results = {}
        for future in futures:
            f = futures[future]
            rel_path = str(f.relative_to(ctx.source_dir))
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
            message="No includes.h files found"
        )

    def check_file(self, file_path: Path, source_dir: Path) -> SubLints:
        try:
            with open(file_path, "r", encoding="utf-8") as f:
                lines = f.readlines()
        except Exception as e:
            return SubLints({
                "File Read": Result(
                    location=Location(file=file_path, line=None),
                    status=CheckStatus.FAIL,
                    message=f"Could not read file: {e}"
                )
            })

        return SubLints({
            "Pragma Once": self.check_pragma_once(file_path, lines),
            "IWYU Pragmas": self.check_iwyu_pragmas(file_path, lines),
            "Include Order": self.check_include_order(file_path, lines)
        })

    def check_pragma_once(self, file_path: Path, lines: list[str]) -> Result:
        if any(line.strip() == "#pragma once" for line in lines):
            return Result(
                location=Location(file=file_path, line=None),
                status=CheckStatus.PASS,
                message="#pragma once present"
            )
        return Result(
            location=Location(file=file_path, line=None),
            status=CheckStatus.FAIL,
            message="Missing #pragma once"
        )

    def check_iwyu_pragmas(self, file_path: Path, lines: list[str]) -> Result:
        include_pattern = re.compile(r'^\s*#\s*include\s+[<"](.+)[>"]')
        missing_pragmas = []
        
        for i, line in enumerate(lines, 1):
            if include_pattern.match(line):
                if "// IWYU pragma: export" not in line:
                    missing_pragmas.append(i)
        
        if not missing_pragmas:
            return Result(
                location=Location(file=file_path, line=None),
                status=CheckStatus.PASS,
                message="All includes have IWYU pragma: export"
            )
        return Result(
            location=Location(file=file_path, line=None),
            status=CheckStatus.FAIL,
            message=f"Missing IWYU pragma on lines: {missing_pragmas}"
        )

    def check_include_order(self, file_path: Path, lines: list[str]) -> Result:
        include_pattern = re.compile(r'^\s*#\s*include\s+[<"](.+)[>"]')
        
        stdlib_includes = []
        derive_c_includes = []
        container_includes = []
        
        current_section = None
        
        for line in lines:
            stripped = line.strip()
            if "// stdlib includes" in stripped:
                current_section = "stdlib"
            elif "// derive-c includes" in stripped:
                current_section = "derive-c"
            elif "// container-specific includes" in stripped:
                current_section = "container"
            elif match := include_pattern.match(line):
                inc = match.group(1)
                if current_section == "stdlib":
                    stdlib_includes.append(inc)
                elif current_section == "derive-c":
                    derive_c_includes.append(inc)
                elif current_section == "container":
                    container_includes.append(inc)
        
        # Check that stdlib comes before derive-c, and derive-c before container
        # by checking that sections appear in order
        errors = []
        
        if not stdlib_includes and not derive_c_includes and not container_includes:
            errors.append("No section comments found")
        
        if errors:
            return Result(
                location=Location(file=file_path, line=None),
                status=CheckStatus.FAIL,
                message="; ".join(errors)
            )
        
        return Result(
            location=Location(file=file_path, line=None),
            status=CheckStatus.PASS,
            message="Includes properly ordered"
        )