"""Load and process benchmark JSON data."""

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import polars as pl

from .types import BuildType, Compiler


@dataclass(frozen=True)
class BenchmarkConfig:
    compiler: Compiler
    build_type: BuildType
    result_file: str


def load_benchmark_json(json_path: Path) -> dict[str, Any]:
    """Load a single benchmark JSON file."""
    with open(json_path) as f:
        return json.load(f)


def extract_benchmarks_data(data: dict[str, Any]) -> pl.DataFrame:
    """Extract benchmark results into a polars DataFrame.

    Extracts:
    - name: benchmark name
    - label: implementation metadata
    - real_time: real time in nanoseconds
    - cpu_time: CPU time in nanoseconds
    - time_unit: unit of time measurement
    - iterations: number of iterations
    - items_per_second: throughput metric (if available)
    """
    benchmarks = data.get("benchmarks", [])

    if not benchmarks:
        return pl.DataFrame()

    rows = []
    for bench in benchmarks:
        row = {
            "name": bench.get("name", ""),
            "label": bench.get("label", ""),
            "real_time": bench.get("real_time", 0.0),
            "cpu_time": bench.get("cpu_time", 0.0),
            "time_unit": bench.get("time_unit", "ns"),
            "iterations": bench.get("iterations", 0),
            "items_per_second": bench.get("items_per_second", None),
        }

        counters = bench.get("counters", {})
        for key, value in counters.items():
            row[f"counter_{key}"] = value

        rows.append(row)

    return pl.DataFrame(rows)


def parse_label(df: pl.DataFrame) -> pl.DataFrame:
    """Parse label field into separate columns.

    Extracts:
    - impl: implementation name (e.g., "derive-c/swiss", "ankerl/unordered_dense")
    - key_size: key size in bytes (for maps)
    - value_size: value size in bytes (for maps)
    - item_size: item size in bytes (for sets/vectors)
    """
    df = df.with_columns(pl.col("label").str.split(" ").list.first().alias("impl"))

    df = df.with_columns(
        pl.col("label")
        .str.extract(r"key=(\d+)B", 1)
        .cast(pl.Int32, strict=False)
        .alias("key_size")
    )

    df = df.with_columns(
        pl.col("label")
        .str.extract(r"value=(\d+)B", 1)
        .cast(pl.Int32, strict=False)
        .alias("value_size")
    )

    df = df.with_columns(
        pl.col("label")
        .str.extract(r"item=(\d+)B", 1)
        .cast(pl.Int32, strict=False)
        .alias("item_size")
    )

    return df


def parse_benchmark_name(df: pl.DataFrame) -> pl.DataFrame:
    """Parse benchmark name to extract parameters.

    Extracts:
    - test_name: the test function name (e.g., "iterate", "mixed_sequential")
    - size: the size parameter (number after the last '/')
    """
    df = df.with_columns(pl.col("name").str.extract(r"^([^<]+)", 1).alias("test_name"))

    df = df.with_columns(
        pl.col("name")
        .str.extract(r"/(\d+)$", 1)
        .cast(pl.Int32, strict=False)
        .alias("size")
    )

    return df


def process_benchmark_data(df: pl.DataFrame) -> pl.DataFrame:
    """Apply all parsing transformations to a benchmark DataFrame."""
    df = parse_label(df)
    df = parse_benchmark_name(df)

    df = df.with_columns(
        [
            (pl.col("real_time") / 1000.0).alias("real_time_us"),
            (pl.col("cpu_time") / 1000.0).alias("cpu_time_us"),
        ]
    )

    if "items_per_second" in df.columns:
        df = df.with_columns(
            (pl.col("items_per_second") / 1_000_000.0).alias("items_per_second_M")
        )

    return df


def parse_result_path(path: Path) -> BenchmarkConfig | None:
    """Extract compiler and build type from result file path.

    Expected format: build/benchmark_results/{compiler}-{build_type}/{benchmark}.json
    """
    if len(path.parts) < 2:
        return None

    parent_dir = path.parts[-2]
    parts = parent_dir.split("-")

    if len(parts) < 2:
        return None

    compiler_str = parts[0]
    build_type_str = "-".join(parts[1:])

    try:
        compiler = Compiler(compiler_str)
    except ValueError:
        return None

    try:
        build_type = BuildType(build_type_str)
    except ValueError:
        return None

    return BenchmarkConfig(
        compiler=compiler,
        build_type=build_type,
        result_file=path.name,
    )


def load_all_benchmarks(
    results_dir: Path,
    benchmark_name: str,
) -> pl.DataFrame:
    """Load benchmark results from all compiler/build-type combinations."""
    all_dfs: list[pl.DataFrame] = []

    for result_path in results_dir.rglob(f"*/{benchmark_name}.json"):
        config = parse_result_path(result_path)
        if config is None:
            continue

        data = load_benchmark_json(result_path)
        df = extract_benchmarks_data(data)
        df = process_benchmark_data(df)

        df = df.with_columns(
            [
                pl.lit(config.compiler.value).alias("compiler"),
                pl.lit(config.build_type.value).alias("build_type"),
                pl.lit(config.result_file).alias("result_file"),
            ]
        )

        all_dfs.append(df)

    if not all_dfs:
        return pl.DataFrame()

    return pl.concat(all_dfs)


def load_benchmark(
    repo_path: Path | str,
    benchmark_name: str,
) -> pl.DataFrame:
    """Load a specific benchmark from the repo.
    
    Automatically searches for benchmark results in build directories.
    
    Args:
        repo_path: Path to the repository root
        benchmark_name: Name of the benchmark (e.g., "allocs", "containers_vector")
    
    Returns:
        DataFrame with benchmark results from all found builds
        
    Example:
        >>> df = load_benchmark("/path/to/derive-C", "allocs")
    """
    repo_path = Path(repo_path)
    
    # Find all build directories and their benchmark_results
    all_dfs: list[pl.DataFrame] = []
    
    # Search for any build* directories
    for build_dir in repo_path.glob("build*"):
        if not build_dir.is_dir():
            continue
            
        # Look for benchmark_results directory
        results_dir = build_dir / "benchmark_results"
        if not results_dir.exists():
            continue
            
        # Load from all compiler/build-type subdirectories
        for result_path in results_dir.glob(f"*/{benchmark_name}.json"):
            config = parse_result_path(result_path)
            if config is None:
                continue

            data = load_benchmark_json(result_path)
            df = extract_benchmarks_data(data)
            df = process_benchmark_data(df)

            df = df.with_columns(
                [
                    pl.lit(config.compiler.value).alias("compiler"),
                    pl.lit(config.build_type.value).alias("build_type"),
                    pl.lit(config.result_file).alias("result_file"),
                ]
            )

            all_dfs.append(df)

    if not all_dfs:
        return pl.DataFrame()

    return pl.concat(all_dfs)
