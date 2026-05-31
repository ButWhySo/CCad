#!/usr/bin/env python3
import argparse
import json
import os
import shutil
import subprocess
import tempfile
import time

def main():
    parser = argparse.ArgumentParser(description="CCad Agent Benchmark Harness")
    parser.add_argument("--agent-cmd", required=True, help="Command to run the agent. The agent should accept a project file path as its last argument.")
    parser.add_argument("--test-dir", required=True, help="Directory containing .ccad.json test files")
    parser.add_argument("--ccad-binary", required=True, help="Path to ccad executable")
    args = parser.parse_args()

    test_files = [f for f in os.listdir(args.test_dir) if f.endswith(".json")]
    if not test_files:
        print("No test files found in", args.test_dir)
        return 1

    results = []
    passed = 0

    with tempfile.TemporaryDirectory() as tmpdir:
        for test_file in test_files:
            print(f"Running benchmark: {test_file}...")
            src_path = os.path.join(args.test_dir, test_file)
            dst_path = os.path.join(tmpdir, test_file)
            shutil.copy2(src_path, dst_path)

            start_time = time.time()
            
            # Run the agent
            agent_cmd = f"{args.agent_cmd} {dst_path}"
            try:
                subprocess.run(agent_cmd, shell=True, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            except subprocess.CalledProcessError as e:
                print(f"  Agent failed with exit code {e.returncode}")
            
            duration = time.time() - start_time

            # Verify with ccad drc
            drc_cmd = [args.ccad_binary, "drc", dst_path]
            try:
                drc_res = subprocess.run(drc_cmd, capture_output=True, text=True)
                drc_passed = (drc_res.returncode == 0)
                
                # Check if there are no errors in the diagnostics
                if drc_passed:
                    print("  PASS")
                    passed += 1
                else:
                    print("  FAIL (DRC errors)")
                
                results.append({
                    "test": test_file,
                    "passed": drc_passed,
                    "duration_seconds": round(duration, 2),
                    "drc_output": drc_res.stdout
                })
            except Exception as e:
                print(f"  Failed to run DRC: {e}")
                results.append({
                    "test": test_file,
                    "passed": False,
                    "duration_seconds": round(duration, 2),
                    "error": str(e)
                })

    print(f"\nBenchmark Complete. {passed}/{len(test_files)} passed.")
    
    with open("benchmark_results.json", "w") as f:
        json.dump({"total": len(test_files), "passed": passed, "results": results}, f, indent=2)
    
    return 0 if passed == len(test_files) else 1

if __name__ == "__main__":
    exit(main())
