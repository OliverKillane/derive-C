"""Console output formatting for linter results."""

from src.linter import Result, SubLints, LintTree, LintResults, CheckStatus, LinterCheck, LintOutput

class ConsoleOutput(LintOutput):
    """Console output formatter with tree structure and colors."""
    
    def write(self, results: LintResults) -> None:
        """Write formatted results to console."""
        print(f"results:\n{self._format_lint_results(results)}")
    
    def _format_result(self, result: Result) -> str:
        """Format a single Result with colored status."""
        color = "\033[92m" if result.status == CheckStatus.PASS else "\033[91m"
        reset = "\033[0m"
        return f"{color}[{result.status.value}]{reset}\n  {result.message}"

    def _format_sublints(self, sublints: SubLints) -> str:
        """Format SubLints with tree structure."""
        lines = []
        items = list(sublints.results.items())
        for i, (key, result) in enumerate(items):
            is_last = (i == len(items) - 1)
            prefix = "╰─ " if is_last else "╞═ "
            continuation = "   " if is_last else "│  "
            
            lines.append(f"{prefix}{key}")
            for line in self._format_lint_tree(result).splitlines():
                lines.append(f"{continuation}{line}")
        return "\n".join(lines)

    def _format_lint_tree(self, tree: LintTree) -> str:
        """Format a LintTree (either Result or SubLints)."""
        if isinstance(tree, Result):
            return self._format_result(tree)
        else:
            return self._format_sublints(tree)

    def _format_lint_results(self, results: LintResults) -> str:
        """Format complete LintResults with check names and descriptions."""
        lines = []
        items = list(results.results.items())
        for i, (check, result) in enumerate(items):
            is_last = (i == len(items) - 1)
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
