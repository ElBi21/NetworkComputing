#!/bin/bash
#SBATCH --job-name=tcn_simple
#SBATCH --nodes=1
#SBATCH --partition=g100_usr_pmem
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=4
#SBATCH --mem=16G
#SBATCH --time=3:00:00
#SBATCH --output=slurm-%j.out
#SBATCH --error=slurm-%j.err

set -euo pipefail

cd "$HOME"
IMAGE="$HOME/ns3-ecn-sharp_sandbox"
RESULT_DIR="$HOME/ecnsharp_results_${SLURM_JOB_ID:-manual}"
mkdir -p "$RESULT_DIR"


singularity exec \
    --bind "$RESULT_DIR:/results" \
    "$IMAGE" \
    bash -lc '
set -euo pipefail

cd "$HOME/ns3-ecn-sharp_sandbox/root/ns3-ecn-sharp"

rm -f Large_Scale_*.xml

LOADS=(0.1 0.3 0.5 0.7 0.9)
SEEDS=(233)

RAW=/results/tcn_summary_raw.csv
SUMMARY=/results/tcn_summary_avg.csv

echo "threshold,seed,xml,avg_fct,avg_small_99_fct" > "$RAW"

for load in "${LOADS[@]}"; do
    for seed in "${SEEDS[@]}"; do
        id="TCN_thr${load}_seed${seed}"
        xml="Large_Scale_${id}_FatTreeK4_TCN_DcTcp_${load}.xml"

        echo "Running load=${load}, seed=${seed}"
        echo "Expected XML: ${xml}"

        rm -f "$xml"

        ./waf -j "${SLURM_CPUS_PER_TASK:-4}" --run "large-scale \
            --randomSeed=${seed} \
            --load=${load} \
            --ID=${id} \
            --AQM=TCN \
            --TCNThreshold=200 \
            --leafCount=2 \
            --spineCount=2 \
            --serverCount=4 \
            --K=4 \
            --EndTime=1 \
            --FlowLaunchEndTime=0.5"

        if [ ! -s "$xml" ]; then
            echo "ERROR: expected XML not found or empty: $xml"
            echo "Nearby XML files:"
            ls -lh Large_Scale_* 2>/dev/null || true
            exit 1
        fi

        cp "$xml" "/results/$xml"

        parser_out=$(python examples/rtt-variations/fct_parser.py "$xml")

        avg_fct=$(echo "$parser_out" | awk -F": " "/^AVG FCT:/ {print \$2}")
        avg_small_99=$(echo "$parser_out" | awk -F": " "/^AVG Small flow 99 FCT:/ {print \$2}")

        if [ -z "$avg_fct" ] || [ -z "$avg_small_99" ]; then
            echo "ERROR: parser output missing expected fields"
            echo "$parser_out"
            exit 1
        fi

        echo "${load},${seed},${xml},${avg_fct},${avg_small_99}" >> "$RAW"
    done
done

python - <<PY
import csv
from collections import defaultdict

raw = "/results/tcn_summary_raw.csv"
out = "/results/tcn_summary_avg.csv"

groups = defaultdict(list)

with open(raw) as f:
    reader = csv.DictReader(f)
    for row in reader:
        groups[row["threshold"]].append((
            float(row["avg_fct"]),
            float(row["avg_small_99_fct"])
        ))

with open(out, "w") as f:
    f.write("threshold,avg_fct_mean,avg_small_99_fct_mean\\n")
    for threshold in sorted(groups, key=lambda x: float(x)):
        vals = groups[threshold]
        avg_fct_mean = sum(v[0] for v in vals) / len(vals)
        avg_small_99_mean = sum(v[1] for v in vals) / len(vals)
        f.write("{},{:.6f},{:.6f}\n".format(threshold, avg_fct_mean, avg_small_99_mean))

print("Raw results:")
print(open(raw).read())

print("Summary:")
print(open(out).read())
PY
'

echo "Done."
echo "Results directory: $RESULT_DIR"
echo "Raw results: $RESULT_DIR/tcn_summary_raw.csv"
echo "Summary: $RESULT_DIR/tcn_summary_avg.csv"