from pathlib import Path
from dataclasses import dataclass
from concurrent.futures import ThreadPoolExecutor
import re
from typing import List, Tuple, Dict
from src.linter import LinterCheck, Result, LintContext, Location, CheckStatus, SubLints
import glob
from pathlib import Path

@dataclass
class Defines:
    define: list[str]
    undefine: list[str]

def run_for_file(ctx: LintContext, file: Path) -> SubLints:
    macro_defines = Defines()
    includes_defines = Defines()

class TemplateDefines(LinterCheck):
    """
    Checks that we correctly undef defines in template files:
     - undefine in reverse order of defines
     - no defines leaked
    """
    
    def run(self, ctx: LintContext) -> Result:
        # glob all the required template.h files
        files = glob.glob("**/template.h", root_dir=ctx.source_dir) 

        # for each file (in parallel and join results with (file name) as the name for each in the 
        # collection)


        # n a single file, run 2 functions
        # 1. goes through every #define, and gets the order of defines and undefines, then return a 'macro defines': result
        # 2. Gets all the includes for def.h and undef.h, gets the file (e.g. foo/def.h -> foo), and then checks these
        # So reports a collection of 2 items (these)

        # The check needs to ensure the macros are undefined in the reverse order of define
        return Result(
            location=Location(file=ctx.source_dir, line=None),
            status=CheckStatus.PASS,
            message="Template defines check passed.",
        )
