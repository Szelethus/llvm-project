#!/usr/bin/env python3
import json
import argparse
import os
import sys

def main():
    parser = argparse.ArgumentParser(description="Annotate slicing criteria in source files based on JSON config.")
    parser.add_argument("json_path", help="Path to the JSON configuration file")
    args = parser.parse_args()

    if not os.path.isfile(args.json_path):
        print(f"Error: JSON file '{args.json_path}' not found.", file=sys.stderr)
        sys.exit(1)

    with open(args.json_path, 'r', encoding='utf-8') as f:
        try:
            data = json.load(f)
        except json.JSONDecodeError as e:
            print(f"Error parsing JSON: {e}", file=sys.stderr)
            sys.exit(1)

    analyses = data.get("analyses", [])
    if not analyses:
        print("No analyses found in the JSON file.")
        sys.exit(0)

    tag = "/*slicing criterion*/"

    for idx, job in enumerate(analyses, start=1):
        if "input_file" not in job or "line_number" not in job:
            print(f"Warning: Job {idx} is missing 'input_file' or 'line_number'. Skipping.", file=sys.stderr)
            continue
        
        filepath = os.path.expanduser(job["input_file"])
        line_no = int(job["line_number"])

        if not os.path.isfile(filepath):
            print(f"Warning: File '{filepath}' not found. Skipping.", file=sys.stderr)
            continue

        with open(filepath, 'r', encoding='utf-8') as f:
            lines = f.readlines()

        if line_no < 1 or line_no > len(lines):
            print(f"Warning: Line number {line_no} out of bounds for '{filepath}'. Skipping.", file=sys.stderr)
            continue

        target_idx = line_no - 1
        original_line = lines[target_idx].rstrip('\n\r')
        
        # Check if the tag is already present at the end of the line
        if original_line.strip().endswith(tag):
            print(f"[{idx}/{len(analyses)}] Already annotated: {filepath}:{line_no}")
            continue

        # Append the tag
        lines[target_idx] = f"{original_line} {tag}\n"

        with open(filepath, 'w', encoding='utf-8') as f:
            f.writelines(lines)
        
        print(f"[{idx}/{len(analyses)}] Annotated: {filepath}:{line_no}")

    print("\nAnnotation process completed.")

if __name__ == "__main__":
    main()
