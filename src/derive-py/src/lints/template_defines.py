from pathlib import Path
import re
from typing import NamedTuple

from src.linter import LinterCheck, Result, LintContext, Location, CheckStatus, SubLints, LintTree

class ParsedFile(NamedTuple):
    defines: list[str]
    undefs: list[str]
    expected_undef_stack: list[str]
    include_error: Result | None

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
            message="No template.h files found"
        )

    def check_file(self, file_path: Path) -> SubLints:
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

        parsed = self.parse_file(lines, file_path)
        return self.validate_file(parsed, file_path)

    def parse_file(self, lines: list[str], file_path: Path) -> ParsedFile:
        defines: list[str] = []
        undefs: list[str] = []
        expected_undef_stack: list[str] = []
        include_error: Result | None = None
        
        # Regex for all directives we care about
        directive_pattern = re.compile(
            r'^\s*#\s*(?:define\s+(\w+)|undef\s+(\w+)|include\s+[<"]([^>"]+)[>"])'
        )
        
        # Track defines that might be template parameters
        pending_defines: list[str] = []
        last_define_line = 0

        for i, line in enumerate(lines, 1):
            if match := directive_pattern.match(line):
                if name := match.group(1):  # define
                    # Check for gap
                    if pending_defines and (i - last_define_line > 8):
                        defines.extend(pending_defines)
                        pending_defines.clear()

                    # Deduplicate and check if already defined
                    if (not pending_defines or pending_defines[-1] != name) and name not in defines:
                        pending_defines.append(name)
                    
                    last_define_line = i
                elif name := match.group(2):  # undef
                    defines.extend(pending_defines)
                    pending_defines.clear()
                    undefs.append(name)
                elif path := match.group(3):  # include
                    if path.endswith("template.h"):
                        pending_defines.clear()
                    else:
                        defines.extend(pending_defines)
                        pending_defines.clear()

                        if path.endswith("undef.h"):
                            parent = str(Path(path).parent)
                            if not expected_undef_stack:
                                include_error = Result(
                                    location=Location(file=file_path, line=i),
                                    status=CheckStatus.FAIL,
                                    message=f"Unexpected undef.h for {parent}, no matching def.h found"
                                )
                                break
                            
                            expected = expected_undef_stack.pop()
                            if parent != expected:
                                include_error = Result(
                                    location=Location(file=file_path, line=i),
                                    status=CheckStatus.FAIL,
                                    message=f"Incorrect undef order. Expected undef for '{expected}', got '{parent}'"
                                )
                                break
                        elif path.endswith("def.h"):
                            parent = str(Path(path).parent)
                            expected_undef_stack.append(parent)
        
        defines.extend(pending_defines)
        
        return ParsedFile(defines, undefs, expected_undef_stack, include_error)

    def validate_file(self, parsed: ParsedFile, file_path: Path) -> SubLints:
        # Check macro order
        expected_undefs = list(reversed(parsed.defines))
        macro_result = Result(
            location=Location(file=file_path, line=None),
            status=CheckStatus.PASS,
            message="Defines properly undefined in reverse order"
        )
        if expected_undefs != parsed.undefs:
            macro_result = Result(
                location=Location(file=file_path, line=None),
                status=CheckStatus.FAIL,
                message=f"Mismatch: expected {expected_undefs}, got {parsed.undefs}"
            )

        # Check include order
        include_result = parsed.include_error
        if not include_result:
            if parsed.expected_undef_stack:
                missing = ", ".join(reversed(parsed.expected_undef_stack))
                include_result = Result(
                    location=Location(file=file_path, line=None),
                    status=CheckStatus.FAIL,
                    message=f"Missing undef.h for: {missing} (expected in that order)"
                )
            else:
                include_result = Result(
                    location=Location(file=file_path, line=None),
                    status=CheckStatus.PASS,
                    message="def.h and undef.h includes match and are properly ordered"
                )

        return SubLints({
            "Macro Order": macro_result,
            "Includes": include_result
        })
