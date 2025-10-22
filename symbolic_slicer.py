#!/usr/bin/env python3
import sys
import subprocess
import shutil
from pathlib import Path

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

def check_file_exists(file_path):
    if not Path(file_path).is_file():
        eprint(f"Error: File '{file_path}' does not exist.")
        sys.exit(1)

def check_positive_int(value, name):
    if not value.isdigit() or int(value) <= 0:
        eprint(f"Error: {name} must be a positive integer.")
        sys.exit(1)

def check_line_in_file(file_path, line_number):
    total_lines = sum(1 for _ in open(file_path, "r"))
    if line_number > total_lines:
        eprint(f"Error: Line number {line_number} exceeds total lines ({total_lines}) in file.")
        sys.exit(1)

def check_variable_on_line(file_path, line_number, variable):
    lines = open(file_path, "r").read().splitlines()

    # Safety: ensure line number is valid
    if line_number < 1 or line_number > len(lines):
        eprint(f"Error: Line number {line_number} is out of range for '{file_path}'.")
        sys.exit(1)

    # Check variable on the specified line
    line = lines[line_number - 1].rstrip()
    if variable not in line:
        eprint(f"Error: Variable '{variable}' not found on line {line_number}.")
        sys.exit(1)

    # Check preceding line for the slice pragma comment
    if line_number == 1:
        eprint(f"Error: No preceding line for line {line_number}; missing slice pragma comment.")
        sys.exit(1)

    preceding_line = lines[line_number - 2].strip()
    expected_comment = f"/*@ slice pragma expr {variable}; */"

    if preceding_line != expected_comment:
        eprint(
            f"Error: Missing or incorrect slice pragma before line {line_number}.\n"
            f"Expected (ignoring leading/trailing spaces):\n    {expected_comment}"
        )
        sys.exit(1)

    eprint(f"Variable '{variable}' and required slice pragma found correctly at line {line_number}.")
    return line

import tempfile

def check_clang_format(file_path):
    if shutil.which("clang-format") is None:
        eprint("Error: clang-format is not installed.")
        sys.exit(1)

    # Define the desired style
    style_content = """\
BasedOnStyle: LLVM
IndentWidth: 4
BinPackArguments: false
BinPackParameters: false
AllowShortFunctionsOnASingleLine: false
AllowShortIfStatementsOnASingleLine: false
AllowShortLoopsOnASingleLine: false
AllowShortBlocksOnASingleLine: false
AllowShortCaseLabelsOnASingleLine: false
AllowShortLambdasOnASingleLine: false
"""

    # Create a temporary file with the style
    with tempfile.NamedTemporaryFile(mode="w+", delete=False) as style_file:
        style_file.write(style_content)
        style_file_path = style_file.name

    # Run clang-format with the temporary style
    result = subprocess.run(
        ["clang-format", f"-style=file:{style_file_path}", file_path],
        capture_output=True, text=True
    )

    # Read the original file
    original_lines = open(file_path, "r").readlines()
    formatted_lines = result.stdout.splitlines(keepends=True)

    # Compare formatted vs original
    if original_lines != formatted_lines:
        eprint(f"Error: File '{file_path}' is not properly clang-formatted with the required style.")
        eprint("To fix it, run:")
        eprint(f"    clang-format -i -style=file:{style_file_path} {file_path}")
        sys.exit(1)

    eprint(f"File '{file_path}' is properly clang-formatted with the required style.")


def check_clang_and_checker(clang_bin):
    if shutil.which(clang_bin) is None:
        eprint(f"Error: Clang binary '{clang_bin}' not found.")
        sys.exit(1)
    result = subprocess.run(
        [clang_bin, "-cc1", "-analyzer-checker-help-alpha"],
        capture_output=True, text=True
    )
    if "alpha.core.SlicingCriterion" not in result.stdout:
        eprint(f"Error: alpha.core.SlicingCriterion checker is not available in '{clang_bin}'.")
        sys.exit(1)

def run_clang_analyzer(clang_bin, file_path, line_number, variable):
    cmd = [
        clang_bin, "-c", "--analyze", file_path,
        "-Xclang", "-analyzer-checker=alpha.core.SlicingCriterion",
        "-Xclang", "-analyzer-config",
        f"-Xclang", f"alpha.core.SlicingCriterion:LineNumber={line_number}",
        "-Xclang", "-analyzer-config",
        f"-Xclang", f"alpha.core.SlicingCriterion:ExpressionName={variable}",
        "-Xclang", "-analyzer-output=html",
        "-o", "htmloutput",
        "-Xclang", "-analyzer-disable-checker=optin"
    ]
    result = subprocess.run(cmd, capture_output=True, text=True)
    # Combine stdout and stderr to capture the slice output
    analyzer_output = (result.stdout or "") + (result.stderr or "")
    if "SLICING CRITERION FOUND" not in analyzer_output:
        eprint("Error: Slicing criterion not found.")
        eprint(result.stdout)
        eprint("Command: " + ' '.join(cmd))
        sys.exit(1)
    eprint("Slicing criterion found by Clang static analyzer.")
    return analyzer_output

def extract_slice_locations(analyzer_output):
    lines = [line.replace("Slicing loc: ", "") for line in analyzer_output.splitlines() if "Slicing loc:" in line]
    unique_sorted = sorted(set(lines), key=lambda x: (x.split()[0], int(x.split()[1])))
    eprint("Program slice locations:")
    for loc in unique_sorted:
        eprint(loc)
    return unique_sorted

def print_slice_content(slice_locations):
    for loc in slice_locations:
        file_name, line_number = loc.split()
        line_number = int(line_number)
        file_path = Path(file_name)
        if not file_path.is_file():
            eprint(f"Warning: File '{file_name}' not found, skipping.")
            continue
        line_content = list(open(file_path, "r"))[line_number - 1].rstrip()
        print(f"{file_name}:{line_number}: {line_content}")

def main():
    if len(sys.argv) < 4 or len(sys.argv) > 5:
        eprint("Usage: slice.py <file_location> <line_number> <variable_name> [clang_binary]")
        sys.exit(1)

    file_path = sys.argv[1]
    line_number = sys.argv[2]
    variable = sys.argv[3]
    clang_bin = sys.argv[4] if len(sys.argv) == 5 else "clang"

    check_file_exists(file_path)
    check_positive_int(line_number, "Line number")
    line_number = int(line_number)
    check_line_in_file(file_path, line_number)
    check_variable_on_line(file_path, line_number, variable)
    check_clang_format(file_path)
    check_clang_and_checker(clang_bin)
    analyzer_output = run_clang_analyzer(clang_bin, file_path, line_number, variable)
    slice_locations = extract_slice_locations(analyzer_output)
    print_slice_content(slice_locations)

if __name__ == "__main__":
    main()

