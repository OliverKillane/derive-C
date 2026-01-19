"""Core types for benchmark configuration and execution."""

import logging
import subprocess
from dataclasses import dataclass
from enum import Enum
from pathlib import Path


logger = logging.getLogger(__name__)


class Compiler(Enum):
    GCC = "gcc"
    CLANG = "clang"


class BuildType(Enum):
    RELEASE = "release"
    RELWITHDEBINFO = "relwithdebinfo"
    DEBUG = "debug"

    def to_cmake_type(self) -> str:
        return {
            BuildType.RELEASE: "Release",
            BuildType.RELWITHDEBINFO: "RelWithDebInfo",
            BuildType.DEBUG: "Debug",
        }[self]


@dataclass(frozen=True)
class BuildConfig:
    compiler: Compiler
    build_type: BuildType
    toolchain_nix: str
    build_dir: str

    def configure(self, repo_root: Path) -> bool:
        cmake_build_type = self.build_type.to_cmake_type()

        cmd = [
            "nix-shell",
            self.toolchain_nix,
            "--run",
            f"cmake -S . -B {self.build_dir} -GNinja -DCMAKE_BUILD_TYPE={cmake_build_type} -DBENCH=ON -DTESTS=OFF -DDOCS=OFF",
        ]

        logger.info(
            "⚙️  Configuring %s-%s...", self.compiler.value, self.build_type.value
        )
        result = subprocess.run(cmd, cwd=repo_root)

        return result.returncode == 0

    def build(self, repo_root: Path) -> bool:
        cmd = [
            "nix-shell",
            self.toolchain_nix,
            "--run",
            f"ninja -C {self.build_dir}",
        ]

        logger.info("🔨 Building %s-%s...", self.compiler.value, self.build_type.value)
        result = subprocess.run(cmd, cwd=repo_root)

        return result.returncode == 0

    def run_benchmarks(self, repo_root: Path) -> bool:
        build_path = repo_root / self.build_dir
        results_dir = (
            build_path
            / "benchmark_results"
            / f"{self.compiler.value}-{self.build_type.value}"
        )

        results_dir.mkdir(parents=True, exist_ok=True)

        bench_dir = build_path / "bench"
        if not bench_dir.exists():
            logger.error("❌ Benchmark directory not found: %s", bench_dir)
            return False

        bench_executables = list(bench_dir.glob("*"))
        if not bench_executables:
            logger.error("❌ No benchmark executables found in %s", bench_dir)
            return False

        logger.info(
            "📊 Running benchmarks for %s-%s...",
            self.compiler.value,
            self.build_type.value,
        )

        for bench_exe in bench_executables:
            if not bench_exe.is_file() or not bench_exe.stat().st_mode & 0o111:
                continue

            bench_name = bench_exe.name
            output_json = results_dir / f"{bench_name}.json"

            cmd = [
                "nix-shell",
                self.toolchain_nix,
                "--run",
                f"{bench_exe} --benchmark_format=json --benchmark_out={output_json} --benchmark_min_time=0.1s",
            ]

            logger.info("  Running %s...", bench_name)
            result = subprocess.run(cmd, cwd=repo_root, capture_output=True)

            if result.returncode != 0:
                logger.warning("  ⚠️  %s failed", bench_name)
                return False

        logger.info("✅ Results saved to %s", results_dir)
        return True
