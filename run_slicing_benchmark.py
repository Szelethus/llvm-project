#!/usr/bin/env python3
import json
import subprocess
import argparse
import sys
import os
from pathlib import Path

def main():
    parser = argparse.ArgumentParser(description="Batch run symbolic slicing analyses from JSON configuration.")
    parser.add_argument("config_file", help="Path to the JSON configuration file")
    parser.add_argument("-o", "--out_dir", required=True, help="Output directory to save the aggregated benchmark JSON")
    parser.add_argument("--slicer_script", default="./symbolic_slicer.py", help="Path to the slicer script (default: ./symbolic_slicer.py)")
    args = parser.parse_args()

    if not os.path.isfile(args.config_file):
        print(f"Error: Configuration file '{args.config_file}' not found.", file=sys.stderr)
        sys.exit(1)

    with open(args.config_file, "r", encoding="utf-8") as f:
        try:
            config = json.load(f)
        except json.JSONDecodeError as e:
            print(f"Error parsing JSON: {e}", file=sys.stderr)
            sys.exit(1)

    analyses = config.get("analyses", [])
    if not analyses:
        print("No analyses found in the configuration file.", file=sys.stderr)
        sys.exit(0)

    total_jobs = len(analyses)
    results = []
    
    for idx, job in enumerate(analyses, start=1):
        cmd = [args.slicer_script]

        if "input_file" in job:
            cmd.extend(["-i", os.path.expanduser(str(job["input_file"]))])
        if "line_number" in job:
            cmd.extend(["-l", str(job["line_number"])])
        if "variable" in job:
            cmd.extend(["-v", str(job["variable"])])
        if "output_dir" in job:
            cmd.extend(["-o", os.path.expanduser(str(job["output_dir"]))])
            
        if "compile_commands" in job:
            cmd.extend(["--compile_commands", os.path.expanduser(str(job["compile_commands"]))])
        if "clang_bin" in job:
            cmd.extend(["-c", os.path.expanduser(str(job["clang_bin"]))])
        if "slice_function" in job:
            cmd.extend(["-f", str(job["slice_function"])])

        print(f"\n{'='*60}")
        print(f"Executing Job [{idx}/{total_jobs}]: {job.get('input_file', 'Unknown')}")
        print(f"Command: {' '.join(cmd)}")
        print(f"{'='*60}\n")

        # Capture stdout for the JSON payload, but let stderr passthrough for live logs
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        if result.stderr:
            print(result.stderr, file=sys.stderr)

        if result.returncode != 0:
            print(f"\nWarning: Job [{idx}/{total_jobs}] exited with non-zero code ({result.returncode}).", file=sys.stderr)
        elif result.stdout:
            try:
                # The JSON payload is guaranteed to be the last line printed to stdout
                last_line = result.stdout.strip().splitlines()[-1]
                stats = json.loads(last_line)
                results.append(stats)
            except Exception as e:
                print(f"Warning: Could not parse structured output from job [{idx}]: {e}", file=sys.stderr)

    print("\nBatch execution completed.\n")
    
    # Save aggregated results to the output directory
    out_dir_path = Path(args.out_dir)
    try:
        out_dir_path.mkdir(parents=True, exist_ok=True)
    except Exception as exc:
        print(f"Error: Could not create output directory '{args.out_dir}': {exc}", file=sys.stderr)
        sys.exit(1)

    output_file = out_dir_path / "benchmark_results.json"
    try:
        with open(output_file, "w", encoding="utf-8") as f:
            json.dump({"benchmark_results": results}, f, indent=2)
        print(f"Aggregated benchmark results successfully saved to: {output_file}")
    except Exception as e:
        print(f"Error saving benchmark results: {e}", file=sys.stderr)

if __name__ == "__main__":
    main()
