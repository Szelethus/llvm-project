#!/usr/bin/env python3
import sys
import subprocess
import shutil
import argparse
import tempfile
import os
import html
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

    eprint(f"Variable '{variable}' found correctly at line {line_number}.")
    return line

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
    style_file_path = None
    try:
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
            # intentionally keep the style file so user can run the exact command shown
            sys.exit(1)

        eprint(f"File '{file_path}' is properly clang-formatted with the required style.")
    finally:
        if style_file_path and os.path.exists(style_file_path):
            try:
                if 'result' in locals() and original_lines == formatted_lines:
                    os.unlink(style_file_path)
            except Exception:
                pass

def check_clang_and_checker(clang_bin):
    if shutil.which("CodeChecker") is None:
        eprint("Error: CodeChecker is not installed or not in PATH.")
        sys.exit(1)
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

def run_clang_analyzer(clang_bin, file_path, line_number, variable, path_sensitive, extra_args):
    # Assemble the build command, interpolating the file path, extra args, and previous warnings
    extra_flags = " ".join(extra_args)
    build_cmd = f"g++ {file_path} -c -Wno-incompatible-function-pointer-types {extra_flags}".strip()

    cmd = [
        "CodeChecker", "check",
        "-b", build_cmd,
        "-e", "alpha.core.SlicingCriterion",
        "--checker-config", f"clangsa:alpha.core.SlicingCriterion:LineNumber={line_number}",
        "--checker-config", f"clangsa:alpha.core.SlicingCriterion:ExpressionName={variable}",
        "--analyzer-config", f"clangsa:path-sensitive={path_sensitive}",
        "-d", "optin",
        "-d", "unix",
        "--analyzers", "clangsa",
        "--verbose=debug_analyzer"
    ]
    
    # Pass the clang binary via the environment variable
    env = os.environ.copy()
    env["CC_ANALYZER_BIN"] = f"clangsa:{clang_bin}"

    result = subprocess.run(cmd, capture_output=True, text=True, env=env)
    
    analyzer_output = (result.stdout or "") + (result.stderr or "")
    if "SLICING CRITERION FOUND" not in analyzer_output:
        eprint("Error: Slicing criterion not found.")
        eprint(analyzer_output)
        eprint("Command: " + ' '.join(cmd))
        sys.exit(1)
        
    eprint(f"Slicing criterion found by CodeChecker (path-sensitive={path_sensitive}).")
    return analyzer_output

def extract_slice_locations(analyzer_output):
    lines = [line.replace("Slicing loc: ", "") for line in analyzer_output.splitlines() if "Slicing loc:" in line]
    unique_sorted = sorted(set(lines), key=lambda x: (x.split()[0], int(x.split()[1])))
    return unique_sorted

def print_slice_content(slice_locations, output_handle):
    for loc in slice_locations:
        file_name, line_number = loc.split()
        line_number = int(line_number)
        file_path = Path(file_name)
        if not file_path.is_file():
            eprint(f"Warning: File '{file_name}' not found, skipping.")
            continue
        line_content = list(open(file_path, "r"))[line_number - 1].rstrip()
        print(f"{line_number}: {line_content}", file=output_handle)

def run_and_write_clang(clang_bin, input_path, line_no, var_name, clang_output_file, path_sensitive, extra_args):
    analyzer_output = run_clang_analyzer(clang_bin, input_path, line_no, var_name, path_sensitive, extra_args)
    slice_locations = extract_slice_locations(analyzer_output)
    try:
        with open(clang_output_file, "w", encoding="utf-8") as outf:
            print_slice_content(slice_locations, outf)
        eprint(f"CodeChecker program slice written to '{clang_output_file}'.")
    except Exception as exc:
        eprint(f"Error: Could not write slice to '{clang_output_file}': {exc}")
        sys.exit(1)

def get_slice_line_numbers(slice_file_path):
    lines = set()
    if not os.path.exists(slice_file_path):
        return lines
    with open(slice_file_path, "r", encoding="utf-8") as f:
        for line in f:
            if ":" in line:
                try:
                    lines.add(int(line.split(":")[0]))
                except ValueError:
                    continue
    return lines

def generate_html_report(input_path, ps_lines, pi_lines, html_output_path):
    with open(input_path, "r", encoding="utf-8", errors="replace") as f:
        source_lines = f.readlines()

    html_content = [
        "<!DOCTYPE html>",
        "<html>",
        "<head>",
        "<meta charset=\"utf-8\">",
        "<title>Slice Comparison Report</title>",
        "<style>",
        "  body { font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, monospace; background-color: #f5f5f5; padding: 20px; }",
        "  .code-container { background: white; border: 1px solid #ddd; border-radius: 4px; padding: 10px; overflow-x: auto; }",
        "  .line-row { display: flex; }",
        "  .line-num { width: 45px; flex-shrink: 0; text-align: right; padding-right: 15px; color: #888; user-select: none; border-right: 1px solid #eee; margin-right: 15px; }",
        "  .code { white-space: pre; margin: 0; }",
        "  .pi-only { background-color: #ffcdd2; } /* Soft Red */",
        "  .ps-only { background-color: #bbdefb; } /* Soft Blue */",
        "  .both { background-color: #81c784; } /* Stronger Green */",
        "  .legend span { padding: 4px 8px; border-radius: 3px; margin-right: 10px; border: 1px solid #ccc; font-size: 14px; }",
        "</style>",
        "</head>",
        "<body>",
        f"<h2>Slice Comparison: {html.escape(Path(input_path).name)}</h2>",
        "<div class='legend' style='margin-bottom: 20px;'>",
        "  <span class='pi-only'>Path-Insensitive Only</span>",
        "  <span class='ps-only'>Path-Sensitive Only</span>",
        "  <span class='both'>Both Slices</span>",
        "</div>",
        "<div class='code-container'>"
    ]

    for i, line_content in enumerate(source_lines, start=1):
        in_ps = i in ps_lines
        in_pi = i in pi_lines

        css_class = ""
        if in_ps and in_pi:
            css_class = "both"
        elif in_ps:
            css_class = "ps-only"
        elif in_pi:
            css_class = "pi-only"

        escaped_code = html.escape(line_content.rstrip('\n\r'))
        if not escaped_code:
            escaped_code = " "

        row = f"<div class='line-row {css_class}'><div class='line-num'>{i}</div><div class='code'>{escaped_code}</div></div>"
        html_content.append(row)

    html_content.extend(["</div>", "</body>", "</html>"])

    with open(html_output_path, "w", encoding="utf-8") as f:
        f.write("\n".join(html_content))
    
    eprint(f"HTML comparison report written to '{html_output_path}'.")

def parse_args():
    parser = argparse.ArgumentParser(description="Program slicing driver using CodeChecker")
    parser.add_argument("-i", "--input", dest="input", required=True,
                        help="Path to input source file")
    parser.add_argument("-l", "--line_no", dest="line_no", required=True,
                        help="Line number (1-based) for slicing criterion")
    parser.add_argument("-v", "--var_name", dest="var_name", required=True,
                        help="Variable name for slicing criterion")
    parser.add_argument("-c", "--clang_bin", dest="clang_bin", default="clang",
                        help="Clang binary to use (default: clang)")
    parser.add_argument("-o", "--output", dest="output", required=True,
                        help="Output directory where slicer outputs will be written")
    return parser.parse_known_args()

def main():
    args, extra_args = parse_args()

    input_path = args.input
    line_no_str = args.line_no
    var_name = args.var_name
    clang_bin = args.clang_bin
    output_dir = args.output

    # --- initial checks ---
    check_file_exists(input_path)
    check_positive_int(line_no_str, "Line number")
    line_no = int(line_no_str)
    check_line_in_file(input_path, line_no)
    check_variable_on_line(input_path, line_no, var_name)
    check_clang_format(input_path)
    check_clang_and_checker(clang_bin)

    # Ensure output directory exists
    outdir_path = Path(output_dir)
    try:
        outdir_path.mkdir(parents=True, exist_ok=True)
    except Exception as exc:
        eprint(f"Error: Could not create output directory '{output_dir}': {exc}")
        sys.exit(1)

    # Prepare output file paths
    input_basename = Path(input_path).name
    clang_output_file_ps = outdir_path / f"{input_basename}_clang_slice_path_sensitive.txt"
    clang_output_file_pi = outdir_path / f"{input_basename}_clang_slice_path_insensitive.txt"

    # Run CodeChecker twice (path-sensitive and path-insensitive), passing extra_args
    run_and_write_clang(clang_bin, input_path, line_no, var_name, clang_output_file_ps, "true", extra_args)
    run_and_write_clang(clang_bin, input_path, line_no, var_name, clang_output_file_pi, "false", extra_args)

    # --- Analysis Output ---
    eprint("\n--- Analysis ---")
    for out_file in [clang_output_file_ps, clang_output_file_pi]:
        if out_file.exists():
            with open(out_file, "r", encoding="utf-8") as f:
                line_count = sum(1 for _ in f)
            eprint(f"{out_file.name}: {line_count} lines")
        else:
            eprint(f"{out_file.name}: File not found.")

    # --- Generate HTML Report ---
    ps_lines = get_slice_line_numbers(clang_output_file_ps)
    pi_lines = get_slice_line_numbers(clang_output_file_pi)
    html_report_file = outdir_path / f"{input_basename}_comparison.html"
    generate_html_report(input_path, ps_lines, pi_lines, html_report_file)

if __name__ == "__main__":
    main()
