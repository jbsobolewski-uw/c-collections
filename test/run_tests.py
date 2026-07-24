#!/usr/bin/env python3
"""Orchestrates the c-collections test suite.

Runs every test case of every module test binary produced by the CMake
build and reports the results through Python's unittest framework.

Usage:
    python3 test/run_tests.py [--build-dir BUILD_DIR] [unittest args...]

The build directory defaults to ./build, falling back to
./cmake-build-debug (CLion's default output directory).
"""

import argparse
import pathlib
import subprocess
import sys
import unittest

# Keep in sync with test/CMakeLists.txt.
SUITE = {
    "list": ["api", "edge", "ownership", "stress", "memory"],
    "queue": ["api", "edge", "ownership", "stress", "memory"],
    "stack": ["api", "edge", "ownership", "stress", "memory"],
    "bst": ["api", "edge", "ownership", "stress", "memory"],
    "bst_traversals": ["orders", "early_stop", "edge", "bfs", "memory"],
    "hashmap": ["api", "edge", "ownership", "collisions", "growth", "memory"],
    "id_manager": ["api", "edge", "recycling", "exhaustion", "memory"],
}

# Printed by the harness only when a test case ran to completion.
COMPLETE_LINE = "collections-test-complete"

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent


def find_build_dir(explicit: str | None) -> pathlib.Path:
    candidates = [explicit] if explicit else ["build", "cmake-build-debug"]
    for name in candidates:
        path = (REPO_ROOT / name).resolve()
        if (path / "test").is_dir():
            return path
    sys.exit(f"error: no build directory found (tried: {', '.join(candidates)}); "
             "run cmake first or pass --build-dir")


def make_case(binary: pathlib.Path, case: str):
    def test(self):
        if not binary.exists():
            self.fail(f"test binary not built: {binary}")
        proc = subprocess.run(
            [str(binary), case], capture_output=True, text=True, timeout=300)
        self.assertIn(COMPLETE_LINE, proc.stdout,
                      f"test crashed or exited early\nstderr:\n{proc.stderr}")
        self.assertEqual(proc.returncode, 0,
                         f"test reported failure\nstderr:\n{proc.stderr}")
    return test


def build_suite(build_dir: pathlib.Path) -> None:
    for module, cases in SUITE.items():
        binary = build_dir / "test" / f"{module}_tests"
        methods = {f"test_{case}": make_case(binary, case) for case in cases}
        cls = type(f"Test_{module}", (unittest.TestCase,), methods)
        globals()[cls.__name__] = cls


def main() -> None:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--build-dir", default=None)
    args, rest = parser.parse_known_args()
    build_suite(find_build_dir(args.build_dir))
    unittest.main(argv=[sys.argv[0], "-v", *rest])

if __name__ == "__main__":
    main()
