from pathlib import Path
from dataclasses import dataclass
from concurrent.futures import ThreadPoolExecutor
import re
from typing import List, Tuple, Dict
from src.linter import LinterCheck, Result, LintContext, Location, CheckStatus
import glob
from pathlib import path

class ReExportIncludes(LinterCheck):
    """
    Checks that every include in the `includes.h` files matches the following:
      - has `// IWYU pragma: export` at the end
      - includes ordered as stdlib, then derive-c, then container specific
    """

    def run(self, ctx: LintContext) -> Result:
        pass