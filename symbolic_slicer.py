#!/usr/bin/env python3
import sys
import subprocess
import shutil
import argparse
import tempfile
import os
import html
import json
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

def check_codechecker_analyzer_path(clang_bin):
    resolved_clang = shutil.which(clang_bin)
    if not resolved_clang:
        return

    env = os.environ.copy()
    env["CC_ANALYZER_BIN"] = f"clangsa:{resolved_clang}"

    cmd = ["CodeChecker", "analyzers", "-o", "json"]
    result = subprocess.run(cmd, capture_output=True, text=True, env=env)
    
    try:
        analyzers = json.loads(result.stdout)
        for analyzer in analyzers:
            if analyzer.get("name") == "clangsa":
                cc_path = analyzer.get("path")
                if Path(cc_path).resolve() != Path(resolved_clang).resolve():
                    eprint(f"Error: CodeChecker 'clangsa' path '{cc_path}' does not match requested '{resolved_clang}'.")
                    sys.exit(1)
                eprint(f"Sanity check passed: CodeChecker matches provided clangsa binary at '{cc_path}'.")
                return
        eprint("Error: 'clangsa' analyzer not found in CodeChecker output.")
        sys.exit(1)
    except Exception as e:
        eprint(f"Error checking CodeChecker analyzers: {e}")
        eprint(f"Raw output: {result.stdout}")
        sys.exit(1)

def run_clang_analyzer(clang_bin, file_path, line_number, variable, path_sensitive, compile_commands, extra_args):
    resolved_clang = shutil.which(clang_bin)
    
    env = os.environ.copy()
    env["CC_ANALYZER_BIN"] = f"clangsa:{resolved_clang}"

    with tempfile.TemporaryDirectory() as temp_out_dir:
        common_args = [
            "-o", temp_out_dir,
            "-e", "alpha.core.SlicingCriterion",
            "--checker-config", f"clangsa:alpha.core.SlicingCriterion:LineNumber={line_number}",
            "--checker-config", f"clangsa:alpha.core.SlicingCriterion:ExpressionName={variable}",
            "--analyzer-config", f"clangsa:path-sensitive={path_sensitive}",
            "-d", "optin",
            "-d", "unix",
            "--analyzers", "clangsa",
            "--verbose=debug_analyzer"
        ]

        if compile_commands:
            cmd = ["CodeChecker", "analyze", compile_commands, "--file", file_path] + common_args
        else:
            extra_flags = " ".join(extra_args)
            build_cmd = f"g++ {file_path} -c -Wno-incompatible-function-pointer-types {extra_flags}".strip()
            cmd = ["CodeChecker", "check", "-b", build_cmd] + common_args

        result = subprocess.run(cmd, capture_output=True, text=True, env=env)
    
    if result.returncode == 1:
        eprint(f"Error: CodeChecker command failed with exit code {result.returncode}.")
        eprint("Command: " + ' '.join(cmd))
        eprint("\n--- stderr ---")
        eprint(result.stderr.strip() if result.stderr else "(no stderr output)")
        eprint("\n--- stdout ---")
        eprint(result.stdout.strip() if result.stdout else "(no stdout output)")
        sys.exit(result.returncode)
    
    analyzer_output = (result.stdout or "") + (result.stderr or "")
    if "SLICING CRITERION FOUND" not in analyzer_output:
        eprint("Error: Slicing criterion not found.")
        eprint(analyzer_output)
        eprint("Command: " + ' '.join(cmd))
        sys.exit(1)
        
    eprint(f"Slicing criterion found by CodeChecker (path-sensitive={path_sensitive}).")
    return analyzer_output

def extract_locations(analyzer_output, marker):
    lines = [line.strip().replace(marker, "") for line in analyzer_output.splitlines() if line.strip().startswith(marker)]
    unique_sorted = sorted(set(lines), key=lambda x: (x.split()[0], int(x.split()[1])))
    return unique_sorted

def print_locations(locations, output_handle):
    for loc in locations:
        parts = loc.split()
        file_name = parts[0]
        line_number = int(parts[1])
        file_path = Path(file_name)
        if not file_path.is_file():
            eprint(f"Warning: File '{file_name}' not found, skipping.")
            continue
        line_content = list(open(file_path, "r"))[line_number - 1].rstrip()
        print(f"{line_number}: {line_content}", file=output_handle)

def run_and_write_clang(clang_bin, input_path, line_no, var_name, slice_out_file, exec_out_file, path_sensitive, compile_commands, extra_args):
    analyzer_output = run_clang_analyzer(clang_bin, input_path, line_no, var_name, path_sensitive, compile_commands, extra_args)
    
    slice_locations = extract_locations(analyzer_output, "Slicing loc: ")
    exec_locations = extract_locations(analyzer_output, "Executed: ")

    slice_set = set(slice_locations)
    exec_set = set(exec_locations)
    if not slice_set.issubset(exec_set):
        eprint(f"Warning: Slicing lines are NOT a strict subset of executed lines (path-sensitive={path_sensitive})!")
    else:
        eprint(f"Sanity Check Passed: Executed lines are a superset of slicing lines (path-sensitive={path_sensitive}).")

    try:
        with open(slice_out_file, "w", encoding="utf-8") as outf:
            print_locations(slice_locations, outf)
        with open(exec_out_file, "w", encoding="utf-8") as outf:
            print_locations(exec_locations, outf)
        eprint(f"CodeChecker slice & execution files written for path-sensitive={path_sensitive}.")
    except Exception as exc:
        eprint(f"Error: Could not write output files: {exc}")
        sys.exit(1)
        
    return slice_locations, exec_locations

def get_line_numbers(file_path):
    lines = set()
    if not os.path.exists(file_path):
        return lines
    with open(file_path, "r", encoding="utf-8") as f:
        for line in f:
            if ":" in line:
                try:
                    lines.add(int(line.split(":")[0]))
                except ValueError:
                    continue
    return lines

def generate_html_report(input_path, ps_slice, pi_slice, ps_exec, pi_exec, html_output_path):
    with open(input_path, "r", encoding="utf-8", errors="replace") as f:
        source_lines = f.readlines()

    html_content = [
        "<!DOCTYPE html>",
        "<html>",
        "<head>",
        "<meta charset=\"utf-8\">",
        "<title>Slice Comparison Report</title>",
        "<style>",
        "  body { font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, monospace; background-color: #f5f5f5; padding: 20px; margin-right: 320px; }",
        "  .code-container { background: white; border: 1px solid #ddd; border-radius: 4px; padding: 10px; overflow-x: auto; }",
        "  .line-row { display: flex; }",
        "  .line-num { width: 45px; flex-shrink: 0; text-align: right; padding-right: 15px; color: #888; user-select: none; border-right: 1px solid #eee; margin-right: 15px; }",
        "  .code { white-space: pre; margin: 0; }",
        "  .pi-only { background-color: #ffcdd2; } /* Soft Red */",
        "  .ps-only { background-color: #bbdefb; } /* Soft Blue */",
        "  .both { background-color: #81c784; } /* Stronger Green */",
        "  .legend { position: fixed; top: 20px; right: 20px; width: 280px; background: white; padding: 15px; border: 1px solid #ccc; border-radius: 6px; box-shadow: 0 4px 12px rgba(0,0,0,0.15); display: flex; flex-direction: column; gap: 10px; }",
        "  .toggle-btn { width: 100%; padding: 10px; background: #333; color: white; border: none; border-radius: 4px; cursor: pointer; font-weight: bold; margin-bottom: 5px; }",
        "  .toggle-btn:hover { background: #555; }",
        "  .legend-item { display: flex; justify-content: space-between; align-items: center; padding: 8px 12px; border-radius: 4px; border: 1px solid rgba(0,0,0,0.1); font-size: 14px; font-weight: bold; }",
        "  .nav-btn { cursor: pointer; padding: 2px 8px; border: 1px solid #999; border-radius: 4px; background: #fff; margin-left: 6px; font-size: 12px; }",
        "  .nav-btn:hover { background: #e0e0e0; }",
        "</style>",
        "</head>",
        "<body>",
        f"<h2>Slice Comparison: {html.escape(Path(input_path).name)}</h2>",
        "<div class='legend'>",
        "  <button id='modeToggle' class='toggle-btn' onclick='toggleMode()'>Switch to Executed Lines</button>",
        "  <div class='legend-item pi-only'><span>Insensitive Only</span>",
        "    <div><button class='nav-btn' onclick='jump(\"pi-only\", -1)'>▲</button><button class='nav-btn' onclick='jump(\"pi-only\", 1)'>▼</button></div>",
        "  </div>",
        "  <div class='legend-item ps-only'><span>Sensitive Only</span>",
        "    <div><button class='nav-btn' onclick='jump(\"ps-only\", -1)'>▲</button><button class='nav-btn' onclick='jump(\"ps-only\", 1)'>▼</button></div>",
        "  </div>",
        "  <div class='legend-item both'><span>Both</span>",
        "    <div><button class='nav-btn' onclick='jump(\"both\", -1)'>▲</button><button class='nav-btn' onclick='jump(\"both\", 1)'>▼</button></div>",
        "  </div>",
        "</div>",
        "<div class='code-container'>"
    ]

    def get_css_class(ps_set, pi_set, idx):
        if idx in ps_set and idx in pi_set: return "both"
        if idx in ps_set: return "ps-only"
        if idx in pi_set: return "pi-only"
        return ""

    for i, line_content in enumerate(source_lines, start=1):
        slice_css = get_css_class(ps_slice, pi_slice, i)
        exec_css = get_css_class(ps_exec, pi_exec, i)

        escaped_code = html.escape(line_content.rstrip('\n\r'))
        if not escaped_code:
            escaped_code = " "

        row = f"<div class='line-row {slice_css}' data-slice='{slice_css}' data-exec='{exec_css}'><div class='line-num'>{i}</div><div class='code'>{escaped_code}</div></div>"
        html_content.append(row)

    html_content.extend([
        "</div>",
        "<script>",
        "let isExecMode = false;",
        "function toggleMode() {",
        "  isExecMode = !isExecMode;",
        "  document.getElementById('modeToggle').innerText = isExecMode ? 'Switch to Slicing Lines' : 'Switch to Executed Lines';",
        "  const modeAttr = isExecMode ? 'data-exec' : 'data-slice';",
        "  document.querySelectorAll('.line-row').forEach(row => {",
        "    row.className = 'line-row ' + row.getAttribute(modeAttr);",
        "  });",
        "}",
        "function jump(cls, dir) {",
        "  const elements = Array.from(document.querySelectorAll('.' + cls + '.line-row'));",
        "  if (!elements.length) return;",
        "  let target = null;",
        "  if (dir === 1) {",
        "    target = elements.find(el => el.getBoundingClientRect().top > 60);",
        "    if (!target) target = elements[0];",
        "  } else {",
        "    target = elements.slice().reverse().find(el => el.getBoundingClientRect().top < -10);",
        "    if (!target) target = elements[elements.length - 1];",
        "  }",
        "  if (target) target.scrollIntoView({ behavior: 'smooth', block: 'center' });",
        "}",
        "</script>",
        "</body>",
        "</html>"
    ])

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
    parser.add_argument("--compile_commands", dest="compile_commands", default=None,
                        help="Path to compile_commands.json (uses CodeChecker analyze if provided)")
    return parser.parse_known_args()

def main():
    args, extra_args = parse_args()

    input_path = args.input
    line_no_str = args.line_no
    var_name = args.var_name
    clang_bin = args.clang_bin
    output_dir = args.output
    compile_commands = args.compile_commands

    check_file_exists(input_path)
    if compile_commands:
        check_file_exists(compile_commands)
    check_positive_int(line_no_str, "Line number")
    line_no = int(line_no_str)
    check_line_in_file(input_path, line_no)
    check_variable_on_line(input_path, line_no, var_name)
    check_clang_format(input_path)
    check_clang_and_checker(clang_bin)
    check_codechecker_analyzer_path(clang_bin)

    outdir_path = Path(output_dir)
    try:
        outdir_path.mkdir(parents=True, exist_ok=True)
    except Exception as exc:
        eprint(f"Error: Could not create output directory '{output_dir}': {exc}")
        sys.exit(1)

    input_basename = Path(input_path).name
    clang_slice_ps = outdir_path / f"{input_basename}_clang_slice_path_sensitive.txt"
    clang_slice_pi = outdir_path / f"{input_basename}_clang_slice_path_insensitive.txt"
    clang_exec_ps = outdir_path / f"{input_basename}_clang_exec_path_sensitive.txt"
    clang_exec_pi = outdir_path / f"{input_basename}_clang_exec_path_insensitive.txt"

    ps_slice_locs, ps_exec_locs = run_and_write_clang(clang_bin, input_path, line_no, var_name, clang_slice_ps, clang_exec_ps, "true", compile_commands, extra_args)
    pi_slice_locs, pi_exec_locs = run_and_write_clang(clang_bin, input_path, line_no, var_name, clang_slice_pi, clang_exec_pi, "false", compile_commands, extra_args)

    eprint("\n--- Analysis ---")
    
    def count_stats(locations_list):
        valid_locs = [loc.split() for loc in locations_list if loc.strip()]
        size = len(valid_locs)
        file_count = len(set(parts[0] for parts in valid_locs))
        func_count = len(set(parts[2] for parts in valid_locs if len(parts) >= 3))
        return size, file_count, func_count

    pi_exec_size, pi_exec_files, pi_exec_funcs = count_stats(pi_exec_locs)
    ps_exec_size, ps_exec_files, ps_exec_funcs = count_stats(ps_exec_locs)
    pi_slice_size, pi_slice_files, pi_slice_funcs = count_stats(pi_slice_locs)
    ps_slice_size, ps_slice_files, ps_slice_funcs = count_stats(ps_slice_locs)
    
    union_slices = set(ps_slice_locs) | set(pi_slice_locs)
    union_size, union_files, union_funcs = count_stats(list(union_slices))

    eprint(f"Insensitive exec size: {pi_exec_size}, file count: {pi_exec_files}, function count: {pi_exec_funcs}")
    eprint(f"Sensitive exec size: {ps_exec_size}, file count: {ps_exec_files}, function count: {ps_exec_funcs}")
    eprint(f"Insensitive slice size: {pi_slice_size}, file count: {pi_slice_files}, function count: {pi_slice_funcs}")
    eprint(f"Sensitive slice size: {ps_slice_size}, file count: {ps_slice_files}, function count: {ps_slice_funcs}")
    eprint(f"Union of sensitive and insensitive slices size: {union_size}, file count: {union_files}, function count: {union_funcs}")

    ps_slice_lines = get_line_numbers(clang_slice_ps)
    pi_slice_lines = get_line_numbers(clang_slice_pi)
    ps_exec_lines = get_line_numbers(clang_exec_ps)
    pi_exec_lines = get_line_numbers(clang_exec_pi)
    
    html_report_file = outdir_path / f"{input_basename}_comparison.html"
    generate_html_report(input_path, ps_slice_lines, pi_slice_lines, ps_exec_lines, pi_exec_lines, html_report_file)

    output_data = {
        "file": input_basename,
        "pi_exec": {"size": pi_exec_size, "files": pi_exec_files, "functions": pi_exec_funcs},
        "ps_exec": {"size": ps_exec_size, "files": ps_exec_files, "functions": ps_exec_funcs},
        "pi_slice": {"size": pi_slice_size, "files": pi_slice_files, "functions": pi_slice_funcs},
        "ps_slice": {"size": ps_slice_size, "files": ps_slice_files, "functions": ps_slice_funcs},
        "union": {"size": union_size, "files": union_files, "functions": union_funcs}
    }
    print(json.dumps(output_data))

if __name__ == "__main__":
    main()
