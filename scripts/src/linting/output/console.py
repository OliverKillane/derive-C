"""Console output formatting for linter results."""

from src.linting.linter import (
    ResultSingle,
    ResultMultiple,
    Result,
    LintResults,
    LintOutput,
)


class ConsoleOutput(LintOutput):
    """Console output formatter with tree structure and colors."""

    def write(self, results: LintResults) -> None:
        """Write formatted results to console."""
        print(self.format(results))

    def format(self, results: LintResults) -> str:
        """Format results to string."""
        return f"results:\n{self._format_lint_results(results)}"

    def _format_result(self, result: ResultSingle) -> str:
        """Format a single Result with colored status."""
        if result.successful:
            return "\033[92m[PASS]\033[0m"

        lines = ["\033[91m[FAIL]\033[0m"]
        for error in result.errors:
            lines.append(f"  {error.describe()}")
        return "\n".join(lines)

    def _format_sublints(self, sublints: ResultMultiple) -> str:
        """Format SubLints with tree structure."""
        lines = []
        items = list(sublints.results.items())
        for i, (key, result) in enumerate(items):
            is_last = i == len(items) - 1
            prefix = "╰─ " if is_last else "╞═ "
            continuation = "   " if is_last else "│  "

            # Get status indicator
            if isinstance(result, ResultSingle):
                status = (
                    "\033[92m[PASS]\033[0m"
                    if result.successful
                    else "\033[91m[FAIL]\033[0m"
                )
                # For ResultSingle, we show status on the key line, not in nested output
                lines.append(f"{prefix}{status} {key}")
                # Only show error details if failed
                if not result.successful:
                    for error in result.errors:
                        lines.append(f"{continuation}  {error.describe()}")
            else:
                # For nested ResultMultiple, determine status from all sub-results
                status = (
                    "\033[92m[PASS]\033[0m"
                    if result.successful
                    else "\033[91m[FAIL]\033[0m"
                )
                lines.append(f"{prefix}{status} {key}")
                for line in self._format_lint_tree(result).splitlines():
                    lines.append(f"{continuation}{line}")
        return "\n".join(lines)

    def _format_lint_tree(self, tree: Result) -> str:
        """Format a LintTree (either Result or SubLints)."""
        if isinstance(tree, ResultSingle):
            return self._format_result(tree)
        else:
            return self._format_sublints(tree)

    def _format_lint_results(self, results: LintResults) -> str:
        """Format complete LintResults with check names and descriptions."""
        lines = []
        items = list(results.results.items())
        for i, (check, result) in enumerate(items):
            is_last = i == len(items) - 1
            prefix = "╰─ " if is_last else "╞═ "
            continuation = "   " if is_last else "│  "

            # Print check name
            lines.append(f"{prefix}{check.name}")
            # Print description with vertical bar continuation
            desc = check.description.strip()
            for desc_line in desc.splitlines():
                lines.append(f"{continuation}│ {desc_line.strip()}")
            # Print results tree
            for line in self._format_lint_tree(result).splitlines():
                lines.append(f"{continuation}{line}")
            # Add blank line between checks (except after last)
            if not is_last:
                lines.append("│")
        return "\n".join(lines)
