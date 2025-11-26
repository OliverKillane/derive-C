from pathlib import Path
import subprocess
from src.linter import LinterCheck, Result, LintContext, Location, CheckStatus, SubLints, LintTree

class TemplateClangd(LinterCheck):
    """
    Runs clangd check to ensure changes don't break intellisense.
    """

    def run(self, ctx: LintContext) -> LintTree:
        files = list(ctx.source_dir.rglob("template.h"))
        
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
            message="No template.h files found"
        )

    def check_file(self, file_path: Path, source_dir: Path) -> Result:
        # Run clang with settings from .clangd config
        # The .clangd config defines -D__clang_daemon__ for src/* files
        try:
            cmd = [
                "clang",
                "-fsyntax-only",
                "-D__clang_daemon__",  # From .clangd config
                "-std=c2x",
                f"-I{source_dir}",  # Include src/ so <derive-c/...> paths work
                str(file_path)
            ]
            
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=10
            )
            
            if result.returncode == 0:
                return Result(
                    location=Location(file=file_path, line=None),
                    status=CheckStatus.PASS,
                    message="No clang errors"
                )
            else:
                # Extract error messages
                errors = result.stderr.strip()
                return Result(
                    location=Location(file=file_path, line=None),
                    status=CheckStatus.FAIL,
                    message=f"Clang errors:\n{errors}"
                )
        except subprocess.TimeoutExpired:
            return Result(
                location=Location(file=file_path, line=None),
                status=CheckStatus.FAIL,
                message="Clang check timed out"
            )
        except FileNotFoundError:
            return Result(
                location=Location(file=file_path, line=None),
                status=CheckStatus.FAIL,
                message="clang not found in PATH"
            )
        except Exception as e:
            return Result(
                location=Location(file=file_path, line=None),
                status=CheckStatus.FAIL,
                message=f"Error running clang: {e}"
            )