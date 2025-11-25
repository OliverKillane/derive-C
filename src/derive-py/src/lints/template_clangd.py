from pathlib import Path
from dataclasses import dataclass
from concurrent.futures import ThreadPoolExecutor
import re
from typing import List, Tuple, Dict
from src.linter import LinterCheck, Result, LintContext, Location, CheckStatus

class TemplateClangd(LinterCheck):
    """
    Runs clangd with check to check for any errors in the file.
     - Lints to ensure changes do not break intellisense
    """

    def run(self, ctx: LintContext) -> Result:
        pass