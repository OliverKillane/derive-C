"""CLI tool for running benchmarks with multiple compilers."""

import argparse
import logging
import shutil
import sys
from pathlib import Path

from .types import BuildConfig, BuildType, Compiler


logger = logging.getLogger(__name__)


TOOLCHAINS = {
    Compiler.GCC: "toolchain/gcc.nix",
    Compiler.CLANG: "toolchain/clang_dev.nix",
}


def get_build_configs(
    compiler: str,
    build_type: str,
) -> list[BuildConfig]:
    configs: list[BuildConfig] = []

    compilers = (
        [Compiler.GCC, Compiler.CLANG] if compiler == "both" else [Compiler(compiler)]
    )
    build_type_enum = BuildType(build_type)

    for comp in compilers:
        configs.append(
            BuildConfig(
                compiler=comp,
                build_type=build_type_enum,
                toolchain_nix=TOOLCHAINS[comp],
                build_dir=f"build-{comp.value}-{build_type_enum.value}",
            )
        )

    return configs


def cmd_run(args: argparse.Namespace) -> int:
    repo_root = Path(args.repo_root).resolve()
    configs = get_build_configs(args.compiler, args.build_type)

    for config in configs:
        if not config.configure(repo_root):
            logger.error(
                "❌ Configuration failed for %s-%s",
                config.compiler.value,
                config.build_type.value,
            )
            return 1

        if not config.build(repo_root):
            logger.error(
                "❌ Build failed for %s-%s",
                config.compiler.value,
                config.build_type.value,
            )
            return 1

        if not config.run_benchmarks(repo_root):
            logger.error(
                "❌ Benchmark execution failed for %s-%s",
                config.compiler.value,
                config.build_type.value,
            )
            return 1

    logger.info("✅ All benchmarks completed successfully!")
    return 0


def cmd_clean(args: argparse.Namespace) -> int:
    repo_root = Path(args.repo_root).resolve()

    patterns = ["build-gcc-*", "build-clang-*"]

    for pattern in patterns:
        for path in repo_root.glob(pattern):
            if path.is_dir():
                logger.info("🗑️  Removing %s", path)
                shutil.rmtree(path)

    logger.info("✅ Cleaned benchmark build directories")
    return 0


def cmd_list(args: argparse.Namespace) -> int:
    repo_root = Path(args.repo_root).resolve()
    bench_source_dir = repo_root / "bench"

    if not bench_source_dir.exists():
        logger.error("❌ Benchmark directory not found")
        return 1

    bench_files = list(bench_source_dir.rglob("bench.cpp"))

    logger.info("Available benchmark targets:")
    for bench_file in sorted(bench_files):
        rel_path = bench_file.relative_to(bench_source_dir)
        target_name = str(rel_path.parent).replace("/", "_")
        logger.info("  - %s", target_name)

    return 0


def main() -> int:
    logging.basicConfig(
        level=logging.INFO,
        format="%(message)s",
    )

    parser = argparse.ArgumentParser(
        description="Benchmark runner for derive-C",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )

    parser.add_argument(
        "--repo-root",
        type=str,
        default=".",
        help="Path to repository root (default: current directory)",
    )

    subparsers = parser.add_subparsers(dest="command", required=True)

    run_parser = subparsers.add_parser(
        "run",
        help="Build and run benchmarks",
    )
    run_parser.add_argument(
        "--compiler",
        choices=["gcc", "clang", "both"],
        default="both",
        help="Compiler to use (default: both)",
    )
    run_parser.add_argument(
        "--build-type",
        choices=["release", "relwithdebinfo", "debug"],
        default="release",
        help="Build type (default: release)",
    )

    subparsers.add_parser(
        "clean",
        help="Clean benchmark build directories",
    )

    subparsers.add_parser(
        "list",
        help="List available benchmark targets",
    )

    args = parser.parse_args()

    if args.command == "run":
        return cmd_run(args)
    elif args.command == "clean":
        return cmd_clean(args)
    elif args.command == "list":
        return cmd_list(args)

    return 1


if __name__ == "__main__":
    sys.exit(main())
