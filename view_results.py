#!/usr/bin/env python3
import json
import argparse
import sys
import os
import re
import shlex
import subprocess
from pathlib import Path

def format_stats(stat_dict):
    """Formats the metrics into a space-separated string for the terminal."""
    if not stat_dict:
        return f"{'-':>4} {'-':>3} {'-':>3}"
    
    sz = stat_dict.get('size', 0)
    fi = stat_dict.get('files', 0)
    fu = stat_dict.get('functions', 0)
    return f"{sz:>4} {fi:>3} {fu:>3}"

def get_project_name(filepath):
    """Heuristic to extract the project name from the file path."""
    parts = filepath.split(os.sep)
    if "test_projects" in parts:
        idx = parts.index("test_projects")
        if idx + 1 < len(parts):
            return parts[idx + 1]
    elif "llvm-project" in parts:
        return "llvm-project"
    
    if len(parts) >= 2:
        return parts[-2]
    return "Unknown"

def escape_latex(text):
    """Escapes problematic characters for LaTeX."""
    if not text:
        return ""
    return str(text).replace("_", "\\_").replace("%", "\\%")

def get_file_metrics(filepath, variable):
    """Opens the source file to count LOC and find the criterion line number."""
    loc = 0
    crit_line = "-"
    expanded_path = os.path.expanduser(filepath)
    
    if os.path.isfile(expanded_path):
        try:
            with open(expanded_path, 'r', encoding='utf-8') as f:
                lines = f.readlines()
                loc = len(lines)
                
                tag_re = re.compile(r'/\*\s*slicing\s+criterion\s*\*/', re.IGNORECASE)
                found_line = None
                
                for i, line in enumerate(lines):
                    if tag_re.search(line):
                        if variable in line:
                            found_line = i + 1
                            break
                        elif i > 0 and variable in lines[i - 1]:
                            found_line = i
                            break
                
                if found_line is None:
                    for i, line in enumerate(lines):
                        if tag_re.search(line):
                            found_line = i + 1
                            break
                            
                if found_line is not None:
                    crit_line = str(found_line)
                    
        except Exception as e:
            print(f"Warning: Could not read {expanded_path} for metrics: {e}", file=sys.stderr)
            
    return loc, crit_line

def get_tu_metrics(filepath, compile_commands_path):
    """Runs the compiler preprocessor to count TU LOC with and without std headers."""
    if not compile_commands_path:
        return "-", "-"

    cc_exp = os.path.expanduser(compile_commands_path)
    if not os.path.isfile(cc_exp):
        return "-", "-"

    try:
        with open(cc_exp, 'r', encoding='utf-8') as f:
            cc_data = json.load(f)
    except Exception:
        return "-", "-"

    target_path = os.path.realpath(os.path.expanduser(filepath))
    entry = None
    for item in cc_data:
        item_file = os.path.realpath(os.path.join(item.get('directory', ''), item.get('file', '')))
        if item_file == target_path:
            entry = item
            break

    if not entry:
        return "-", "-"

    if 'arguments' in entry:
        cmd = entry['arguments']
    elif 'command' in entry:
        cmd = shlex.split(entry['command'])
    else:
        return "-", "-"

    new_cmd = []
    skip_next = False
    for arg in cmd:
        if skip_next:
            skip_next = False
            continue
        if arg == '-c':
            continue
        if arg == '-o':
            skip_next = True
            continue
        if arg.startswith('-o'):
            continue
        new_cmd.append(arg)
    
    # -E: Preprocess only
    # -w: Suppress warnings
    # -C: Preserve comments in preprocessor output to keep line counts accurate
    new_cmd.extend(['-E', '-w', '-C'])

    try:
        res = subprocess.run(new_cmd, cwd=entry.get('directory'), capture_output=True, text=True, check=True)
        stdout = res.stdout
    except subprocess.CalledProcessError as e:
        print(f"Warning: Preprocessor call failed for {filepath}: {e.stderr.strip()[:100]}...", file=sys.stderr)
        return "-", "-"
    except Exception as e:
        print(f"Warning: Could not execute preprocessor for {filepath}: {e}", file=sys.stderr)
        return "-", "-"

    loc_with_std = 0
    loc_no_std = 0
    in_system_header = False

    for line in stdout.splitlines():
        # Match standard preprocessor line markers (e.g., # 1 "/usr/include/stdio.h" 1 3)
        if line.startswith('# '):
            parts = line.split('"')
            if len(parts) >= 3:
                flags = parts[2]
                # ' 3' means the following text comes from a system header
                if ' 3' in flags:
                    in_system_header = True
                else:
                    in_system_header = False
            continue
        
        if not line.strip():
            continue

        loc_with_std += 1
        if not in_system_header:
            loc_no_std += 1

    return loc_with_std, loc_no_std

def generate_latex_table(results, batch_jobs, out_dir):
    """Generates a LaTeX table with multirow grouping and TU metrics."""
    table_data = {}
    
    for r in results:
        fname = r.get("file", "Unknown")
        
        match = None
        for job in batch_jobs:
            if os.path.basename(job.get("input_file", "")) == fname and not job.get("_matched"):
                match = job
                job["_matched"] = True
                break
                
        project = "Unknown"
        var = "-"
        loc = 0
        crit_line = "-"
        tu_with = "-"
        tu_without = "-"
        
        if match:
            full_path = match.get("input_file", "")
            project = get_project_name(full_path)
            var = match.get("variable", "-")
            loc, crit_line = get_file_metrics(full_path, var)
            tu_with, tu_without = get_tu_metrics(full_path, match.get("compile_commands"))

        pi_exec = r.get("pi_exec", {}).get("size", 0)
        ps_exec = r.get("ps_exec", {}).get("size", 0)
        ps_slice = r.get("ps_slice", {}).get("size", 0)
        uni_size = r.get("union", {}).get("size", 0)
        
        if uni_size > 0:
            red_pct = ((uni_size - ps_slice) / uni_size) * 100
            red_str = f"{red_pct:.1f}\\%"
        else:
            red_str = "-"
            
        row = {
            "file": escape_latex(fname),
            "loc": loc,
            "tu_with": tu_with,
            "tu_without": tu_without,
            "crit_line": crit_line,
            "var": escape_latex(var),
            "pi_exec": pi_exec,
            "ps_exec": ps_exec,
            "ps_slice": ps_slice,
            "uni_size": uni_size,
            "red_str": red_str
        }
        
        if project not in table_data:
            table_data[project] = []
        table_data[project].append(row)

    tex = [
        "% NOTE: Ensure you have \\usepackage{multirow} in your document preamble.",
        "\\begin{table}[hbt!]",
        "\\centering",
        "\\resizebox{\\textwidth}{!}{%",
        "\\begin{tabular}{|l|l|r|r|r|r|l|r|r|r|r|r|}",
        "\\hline",
        "\\textbf{Project} & \\textbf{File} & \\textbf{LOC} & \\textbf{TU+Std} & \\textbf{TU-Std} & \\textbf{Line} & \\textbf{Var} & \\textbf{PI Exec} & \\textbf{PS Exec} & \\textbf{PS Slice} & \\textbf{Union} & \\textbf{Reduc.} \\\\ \\hline"
    ]
    
    for proj, rows in table_data.items():
        first_row = True
        n = len(rows)
        for r in rows:
            proj_col = f"\\multirow{{{n}}}{{*}}{{{escape_latex(proj)}}}" if first_row else ""
            tex.append(f"{proj_col} & {r['file']} & {r['loc']} & {r['tu_with']} & {r['tu_without']} & {r['crit_line']} & {r['var']} & {r['pi_exec']} & {r['ps_exec']} & {r['ps_slice']} & {r['uni_size']} & {r['red_str']} \\\\")
            first_row = False
        tex.append("\\hline")
        
    tex.extend([
        "\\end{tabular}%",
        "}",
        "\\caption{Slicing Benchmark Results: Translation Unit and Slice Sizes}",
        "\\label{tab:slicing_results}",
        "\\end{table}"
    ])
    
    out_path = Path(out_dir)
    out_path.mkdir(parents=True, exist_ok=True)
    tex_file = out_path / "slicing_results_table.tex"
    
    with open(tex_file, "w", encoding="utf-8") as f:
        f.write("\n".join(tex))
        
    print(f"LaTeX table generated successfully at: {tex_file}")

def main():
    parser = argparse.ArgumentParser(description="View slicing benchmark results and generate a LaTeX table.")
    parser.add_argument("json_path", help="Path to benchmark_results.json (or the directory containing it)")
    parser.add_argument("-b", "--batch", help="Path to the original analyses.json batch config (Required for LaTeX generation)")
    parser.add_argument("-o", "--out_dir", help="Output directory for the .tex file (Required for LaTeX generation)")
    args = parser.parse_args()

    target_path = args.json_path
    if os.path.isdir(target_path):
        target_path = os.path.join(target_path, "benchmark_results.json")

    if not os.path.isfile(target_path):
        print(f"Error: Could not find '{target_path}'.", file=sys.stderr)
        sys.exit(1)

    try:
        with open(target_path, 'r', encoding='utf-8') as f:
            data = json.load(f)
    except Exception as e:
        print(f"Error reading JSON: {e}", file=sys.stderr)
        sys.exit(1)

    results = data.get("benchmark_results", [])
    if not results:
        print("No benchmark results found in the JSON file.")
        sys.exit(0)

    # --- ASCII Terminal Output ---
    max_fname_len = max([len(r.get("file", "")) for r in results] + [10])
    file_w = min(max_fname_len, 45)  
    col_w = 12                       
    group_w = (col_w * 2) + 3        
    ratio_w = 10                     
    red_w = (ratio_w * 2) + 3        
    total_w = file_w + 3 + group_w + 3 + group_w + 3 + col_w + 3 + red_w

    sep_main = "=" * total_w
    sep_t1 = f"{'':<{file_w}} | {'-' * group_w} | {'-' * group_w} | {'-' * col_w} | {'-' * red_w}"
    sep_t2 = f"{'':<{file_w}} | {'-' * col_w} | {'-' * col_w} | {'-' * col_w} | {'-' * col_w} | {'-' * col_w} | {'-' * ratio_w} | {'-' * ratio_w}"
    sep_bot = "-" * total_w
    t3_col = f"{'Sz':>4} {'Fi':>3} {'Fu':>3}"

    print(f"\n{sep_main}")
    print(f"{'':<{file_w}} | {'Path Insensitive (PI)':^{group_w}} | {'Path Sensitive (PS)':^{group_w}} | {'Union':^{col_w}} | {'Reductions':^{red_w}}")
    print(sep_t1)
    print(f"{'':<{file_w}} | {'Exec':^{col_w}} | {'Slice':^{col_w}} | {'Exec':^{col_w}} | {'Slice':^{col_w}} | {'Slice':^{col_w}} | {'vs Union':^{ratio_w}} | {'vs PI':^{ratio_w}}")
    print(sep_t2)
    print(f"{'File':^{file_w}} | {t3_col} | {t3_col} | {t3_col} | {t3_col} | {t3_col} | {'% Less':^{ratio_w}} | {'% Less':^{ratio_w}}")
    print(sep_main)

    totals = {k: {'size': 0, 'files': 0, 'functions': 0} for k in ['pi_exec', 'pi_slice', 'ps_exec', 'ps_slice', 'union']}

    for r in results:
        fname = r.get("file", "Unknown")
        print_fname = fname[:file_w - 3] + "..." if len(fname) > file_w else fname
            
        pi_e = format_stats(r.get("pi_exec"))
        pi_s = format_stats(r.get("pi_slice"))
        ps_e = format_stats(r.get("ps_exec"))
        ps_s = format_stats(r.get("ps_slice"))
        uni  = format_stats(r.get("union"))
        
        for key in totals:
            totals[key]['size'] += r.get(key, {}).get('size', 0)
            totals[key]['files'] += r.get(key, {}).get('files', 0)
            totals[key]['functions'] += r.get(key, {}).get('functions', 0)

        ps_size = r.get("ps_slice", {}).get("size", 0)
        pi_size = r.get("pi_slice", {}).get("size", 0)
        uni_size = r.get("union", {}).get("size", 0)
        
        ratio_uni_str = f"{((uni_size - ps_size) / uni_size) * 100:>6.1f}% " if uni_size > 0 else f"{'-':>7} "
        ratio_pi_str = f"{((pi_size - ps_size) / pi_size) * 100:>6.1f}% " if pi_size > 0 else f"{'-':>7} "
        
        print(f"{print_fname:<{file_w}} | {pi_e:^{col_w}} | {pi_s:^{col_w}} | {ps_e:^{col_w}} | {ps_s:^{col_w}} | {uni:^{col_w}} | {ratio_uni_str:^{ratio_w}} | {ratio_pi_str:^{ratio_w}}")

    num_res = len(results)
    avgs = {k: {m: int(round(totals[k][m] / num_res)) for m in totals[k]} for k in totals}

    avg_ps_size = avgs['ps_slice']['size']
    avg_pi_size = avgs['pi_slice']['size']
    avg_uni_size = avgs['union']['size']

    avg_ratio_uni_str = f"{((avg_uni_size - avg_ps_size) / avg_uni_size) * 100:>6.1f}% " if avg_uni_size > 0 else f"{'-':>7} "
    avg_ratio_pi_str = f"{((avg_pi_size - avg_ps_size) / avg_pi_size) * 100:>6.1f}% " if avg_pi_size > 0 else f"{'-':>7} "

    print(sep_bot)
    print(f"{'AVERAGE':<{file_w}} | {format_stats(avgs['pi_exec']):^{col_w}} | {format_stats(avgs['pi_slice']):^{col_w}} | {format_stats(avgs['ps_exec']):^{col_w}} | {format_stats(avgs['ps_slice']):^{col_w}} | {format_stats(avgs['union']):^{col_w}} | {avg_ratio_uni_str:^{ratio_w}} | {avg_ratio_pi_str:^{ratio_w}}")
    print(sep_main)
    print(" * Legend: Sz (Size)  Fi (Files)  Fu (Functions)\n")

    # --- LaTeX Generation ---
    if args.batch and args.out_dir:
        if not os.path.isfile(args.batch):
            print(f"Error: Batch config file '{args.batch}' not found. Skipping LaTeX generation.", file=sys.stderr)
            return

        with open(args.batch, 'r', encoding='utf-8') as f:
            batch_data = json.load(f)
            batch_jobs = batch_data.get("analyses", [])

        generate_latex_table(results, batch_jobs, args.out_dir)

if __name__ == "__main__":
    main()

