from src.linting.linter import (
    ResultSingle,
    ResultMultiple,
    LintResults,
    LinterCheck,
    Error,
    LintContext,
    Result,
)
from src.linting.output.console import ConsoleOutput


class DummyError(Error):
    def __init__(self, msg: str):
        self.msg = msg

    def describe(self) -> str:
        return self.msg


class DummyCheck(LinterCheck):
    def __init__(self, name: str, description: str):
        self._name = name
        self._description = description

    def run(self, ctx: LintContext) -> Result:
        return ResultSingle(errors=[])

    @property
    def name(self) -> str:
        return self._name

    @property
    def description(self) -> str:
        return self._description


def test_console_output_format():
    # Create dummy checks
    check1 = DummyCheck("Check1", "Description for Check1")
    check2 = DummyCheck("Check2", "Description for Check2\nLine 2")

    # Create results
    # Check 1: Pass
    result1 = ResultSingle(errors=[])

    # Check 2: Fail with multiple errors
    result2 = ResultSingle(errors=[DummyError("Error 1"), DummyError("Error 2")])

    # Check 3: Nested results (ResultMultiple)
    check3 = DummyCheck("Check3", "Nested Check")
    result3 = ResultMultiple(
        results={
            "SubCheck1": ResultSingle(errors=[]),
            "SubCheck2": ResultSingle(errors=[DummyError("SubError 1")]),
        }
    )

    results = LintResults(results={check1: result1, check2: result2, check3: result3})

    console = ConsoleOutput()
    output = console.format(results)

    # Expected output construction
    expected_lines = [
        "results:",
        "╞═ Check1",
        "│  │ Description for Check1",
        "│  \033[92m[PASS]\033[0m",
        "│",
        "╞═ Check2",
        "│  │ Description for Check2",
        "│  │ Line 2",
        "│  \033[91m[FAIL]\033[0m",
        "│    Error 1",
        "│    Error 2",
        "│",
        "╰─ Check3",
        "   │ Nested Check",
        "   ╞═ \033[92m[PASS]\033[0m SubCheck1",
        "   ╰─ \033[91m[FAIL]\033[0m SubCheck2",
        "        SubError 1",
    ]

    expected_output = "\n".join(expected_lines)

    # Debug print if assertion fails
    if output != expected_output:
        print("\nExpected:")
        print(expected_output)
        print("\nActual:")
        print(output)

    assert output == expected_output
