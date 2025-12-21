from pathlib import Path
from src.linting.lints.template_defines import Event, Tracker, Location
from src.linting.linter import ResultSingle


def check_code(code: str) -> ResultSingle:
    lines = code.splitlines(keepends=True)
    path = Path("test_template.h")
    events = []
    for i, line in enumerate(lines):
        event = Event.from_line(Location(path, i + 1), line)
        if event:
            events.append(event)

    errors = Tracker.process_events(events)
    return ResultSingle(errors=errors)


def assert_pass(result: ResultSingle):
    assert result.successful, (
        f"Failed with errors: {[e.describe() for e in result.errors]}"
    )


def test_simple_define_undef():
    code = """
    #define FOO
    #undef FOO
    """
    results = check_code(code)
    assert_pass(results)


def test_duplicate_define():
    code = """
    #define FOO
    #define FOO
    #undef FOO
    """
    results = check_code(code)
    assert_pass(results)


def test_redefine_undef_redefine():
    code = """
    #define FOO
    #undef FOO
    #define FOO
    #undef FOO
    """
    results = check_code(code)
    assert_pass(results)


def test_nested_defines():
    code = """
    #define BAR
    #define FOO
    #undef FOO
    #undef BAR
    """
    results = check_code(code)
    assert_pass(results)


def test_mixed_nesting():
    code = """
    #define BAR
    #define FOO
    #undef FOO
    #define BING
    #undef BING
    #undef BAR
    """
    results = check_code(code)
    assert_pass(results)


def test_include_defines():
    code = """
    #include <foo/def.h>

    #include <foo/undef.h>
    """
    results = check_code(code)
    assert_pass(results)


def test_redefinitions():
    code = """
    #define FOO
    #undef FOO
    #define FOO
    #undef FOO
    """
    results = check_code(code)
    assert_pass(results)


def test_repeat_definitions():
    code = """
    #define FOO
    #define FOO
    #undef FOO
    #define FOO
    #undef FOO
    """
    results = check_code(code)
    assert_pass(results)


def test_include_defines_and_defines():
    code = """
    #include <foo/def.h>
    #define FOO
    #undef FOO
    #include <foo/undef.h>
    """
    results = check_code(code)
    assert_pass(results)


def test_includes_for_templates():
    code = """
    #define FOO

    #define BAR // [DERIVE-C] for template
    #define NAME // [DERIVE-C] for template
    #include <bar/template.h>

    #undef FOO
    """
    results = check_code(code)
    assert_pass(results)
