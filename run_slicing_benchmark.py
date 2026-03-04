#!/usr/bin/env python3
import json
import subprocess
import argparse
import sys
import os
import concurrent.futures
import multiprocessing
from pathlib import Path

def run_analysis(idx, total_jobs, job, slicer_script):
    """Executes a single slicing job in a subprocess."""
    cmd = [slicer_script]

    if "input_file" in job:
        cmd.extend(["-i", os.path.expanduser(str(job["input_file"]))])
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

    header = f"\n{'='*60}\nExecuting Job [{idx}/{total_jobs}]: {job.get('input_file', 'Unknown')}\nCommand: {' '.join(cmd)}\n{'='*60}\n"

    # Capture output to prevent interleaved terminal spam during parallel execution
    result = subprocess.run(cmd, capture_output=True, text=True)
    
    stats = None
    err_msg = None

    if result.returncode != 0:
        err_msg = f"\nWarning: Job [{idx}/{total_jobs}] exited with non-zero code ({result.returncode})."
    elif result.stdout:
        try:
            # The JSON payload is guaranteed to be the last line printed to stdout
            last_line = result.stdout.strip().splitlines()[-1]
            stats = json.loads(last_line)
        except Exception as e:
            err_msg = f"\nWarning: Could not parse structured output from job [{idx}]: {e}"

    return idx, total_jobs, stats, err_msg, header, result.stdout, result.stderr

def main():
    parser = argparse.ArgumentParser(description="Batch run symbolic slicing analyses from JSON configuration.")
    parser.add_argument("config_file", help="Path to the JSON configuration file")
    parser.add_argument("-o", "--out_dir", required=True, help="Output directory to save the aggregated benchmark JSON")
    parser.add_argument("--slicer_script", default="./symbolic_slicer.py", help="Path to the slicer script (default: ./symbolic_slicer.py)")
    parser.add_argument("-j", "--jobs", type=int, default=multiprocessing.cpu_count(), help="Number of parallel jobs to run (default: CPU core count)")
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
    completed_results = []

    print(f"Starting batch execution of {total_jobs} jobs with {args.jobs} workers...\n")

    # Run jobs in parallel using a ThreadPoolExecutor
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
        future_to_idx = {
            executor.submit(run_analysis, idx, total_jobs, job, args.slicer_script): idx 
            for idx, job in enumerate(analyses, start=1)
        }

        # As each job completes, cleanly print its buffered logs
        for future in concurrent.futures.as_completed(future_to_idx):
            idx = future_to_idx[future]
            try:
                res_idx, _, stats, err_msg, header, stdout, stderr = future.result()
                
                # Output sequentially so multi-line warnings aren't chopped up by other threads
                print(header, end="")
                if stderr:
                    print(stderr, file=sys.stderr, end="")
                if err_msg:
                    print(err_msg, file=sys.stderr)
                    
                if stats:
                    completed_results.append((res_idx, stats))
            except Exception as exc:
                print(f"\nJob [{idx}/{total_jobs}] generated an exception: {exc}", file=sys.stderr)

    print("\nBatch execution completed.\n")
    
    # Sort results by their original index to maintain deterministic order in the final JSON
    completed_results.sort(key=lambda x: x[0])
    results = [stats for idx, stats in completed_results]

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
