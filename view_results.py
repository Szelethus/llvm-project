#!/usr/bin/env python3
import json
import argparse
import sys
import os

def format_stats(stat_dict):
    """Formats the metrics into a space-separated string."""
    if not stat_dict:
        return f"{'-':>4} {'-':>3} {'-':>3}"
    
    sz = stat_dict.get('size', 0)
    fi = stat_dict.get('files', 0)
    fu = stat_dict.get('functions', 0)
    return f"{sz:>4} {fi:>3} {fu:>3}"

def main():
    parser = argparse.ArgumentParser(description="View slicing benchmark results in a multicolumn table.")
    parser.add_argument("json_path", help="Path to benchmark_results.json (or the directory containing it)")
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

    # Dynamic column widths
    max_fname_len = max([len(r.get("file", "")) for r in results] + [10])
    file_w = min(max_fname_len, 45)  
    
    col_w = 12                       
    group_w = (col_w * 2) + 3        
    ratio_w = 11                     # Width for the new reduction percentage column
    total_w = file_w + 3 + group_w + 3 + group_w + 3 + col_w + 3 + ratio_w

    # Table separators
    sep_main = "=" * total_w
    sep_t1 = f"{'':<{file_w}} | {'-' * group_w} | {'-' * group_w} | {'-' * col_w} | {'-' * ratio_w}"
    sep_t2 = f"{'':<{file_w}} | {'-' * col_w} | {'-' * col_w} | {'-' * col_w} | {'-' * col_w} | {'-' * col_w} | {'-' * ratio_w}"
    
    # Sub-header for the metrics
    t3_col = f"{'Sz':>4} {'Fi':>3} {'Fu':>3}"

    # Print Headers (3-Tier Multicolumn)
    print(f"\n{sep_main}")
    print(f"{'':<{file_w}} | {'Path Insensitive (PI)':^{group_w}} | {'Path Sensitive (PS)':^{group_w}} | {'Union':^{col_w}} | {'Reduction':^{ratio_w}}")
    print(sep_t1)
    print(f"{'':<{file_w}} | {'Exec':^{col_w}} | {'Slice':^{col_w}} | {'Exec':^{col_w}} | {'Slice':^{col_w}} | {'Slice':^{col_w}} | {'PS % Less':^{ratio_w}}")
    print(sep_t2)
    print(f"{'File':^{file_w}} | {t3_col} | {t3_col} | {t3_col} | {t3_col} | {t3_col} | {'Size %':^{ratio_w}}")
    print(sep_main)

    # Print Data Rows
    for r in results:
        fname = r.get("file", "Unknown")
        if len(fname) > file_w:
            fname = fname[:file_w - 3] + "..."
            
        pi_e = format_stats(r.get("pi_exec"))
        pi_s = format_stats(r.get("pi_slice"))
        ps_e = format_stats(r.get("ps_exec"))
        ps_s = format_stats(r.get("ps_slice"))
        uni  = format_stats(r.get("union"))
        
        # Calculate how much smaller PS Slice Size is compared to Union Slice Size
        ps_size = r.get("ps_slice", {}).get("size", 0)
        uni_size = r.get("union", {}).get("size", 0)
        
        if uni_size > 0:
            pct_smaller = ((uni_size - ps_size) / uni_size) * 100
            ratio_str = f"{pct_smaller:>7.1f}% "
        else:
            ratio_str = f"{'-':>8} "
        
        print(f"{fname:<{file_w}} | {pi_e:^{col_w}} | {pi_s:^{col_w}} | {ps_e:^{col_w}} | {ps_s:^{col_w}} | {uni:^{col_w}} | {ratio_str:^{ratio_w}}")

    print(sep_main)
    print(" * Legend: Sz (Size)  Fi (Files)  Fu (Functions)\n")

if __name__ == "__main__":
    main()
