#!/bin/bash
#SBATCH --job-name=tcn_simple
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=4
#SBATCH --mem=16G
#SBATCH --time=3:00:00
#SBATCH --output=slurm-%j.out
#SBATCH --error=slurm-%j.err

set -euo pipefail

cd "$HOME"

IMAGE="$HOME/ns3-ecn-sharp_optimized.sif"
RESULT_DIR="$HOME/ecnsharp_results_${SLURM_JOB_ID:-manual}"
mkdir -p "$RESULT_DIR"

if [ ! -f "$IMAGE" ]; then
    singularity pull "$IMAGE" docker://snowzjx/ns3-ecn-sharp:optimized
fi

singularity exec \
    --bind "$RESULT_DIR:/results" \
    "$IMAGE" \
    bash -lc '
set -euo pipefail

cd "$HOME/ns3-ecn-sharp"

THRESHOLDS=(20 40 80 120 160 200 250)
SEEDS=(233 234 235)
LOAD=0.5

RAW=/results/tcn_summary_raw.csv
SUMMARY=/results/tcn_summary_avg.csv

echo "threshold,seed,xml,avg_fct,avg_small_99_fct" > "$RAW"

for threshold in "${THRESHOLDS[@]}"; do
    for seed in "${SEEDS[@]}"; do
        id="TCN_thr${threshold}_seed${seed}"
        xml="Large_Scale_${id}_2X1_TCN_DcTcp_${LOAD}.xml"

        echo "Running threshold=${threshold}, seed=${seed}"
        echo "Expected XML: ${xml}"

        rm -f "$xml"

        ./waf -j "${SLURM_CPUS_PER_TASK:-4}" --run "large-scale \
            --randomSeed=${seed} \
            --load=${LOAD} \
            --ID=${id} \
            --AQM=TCN \
            --TCNThreshold=${threshold} \
            --leafCount=2 \
            --spineCount=1 \
            --serverCount=4 \
            --EndTime=4 \
            --FlowLaunchEndTime=3"

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

        echo "${threshold},${seed},${xml},${avg_fct},${avg_small_99}" >> "$RAW"
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