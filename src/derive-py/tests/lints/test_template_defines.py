from pathlib import Path
import pytest
from src.lints.template_defines import TemplateDefines
from src.linter import CheckStatus, SubLints, Result

def check_code(code: str) -> SubLints:
    linter = TemplateDefines()
    lines = code.splitlines(keepends=True)
    path = Path("test_template.h")
    parsed = linter.parse_file(lines, path)
    return linter.validate_file(parsed, path)

def assert_pass(results: SubLints):
    for key, result in results.results.items():
        if isinstance(result, Result):
            assert result.status == CheckStatus.PASS, f"{key}: {result.message}"
        else:
            # Recurse if needed, but TemplateDefines returns flat SubLints
            pass

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

    #define BAR // for template
    #define NAME // for template
    #include <bar/template.h>

    #undef FOO
    """
    results = check_code(code)
    assert_pass(results)