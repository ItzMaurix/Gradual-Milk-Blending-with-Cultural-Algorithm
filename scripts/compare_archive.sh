#!/bin/bash
# Baseline (sin archive) vs Archive (archive_size=5, w_attract=1.5)
# 5 instancias × 5 seeds × 2 versiones = 50 corridas

set -e
INSTANCES="eil22 eil30 a37 a46 c50"
INSTANCES_DIR="data/instances"
SUFFIX="-DB-2distribution.txt"
BINARY="./gmb"
OUTPUT_DIR="results"

SEEDS="1 2 3 4 5"
MAX_EVALS=100000
GENS_CAP=5000
POP=30
ELITE=3
PM=0.3

if [ ! -x "$BINARY" ]; then
    echo "ERROR: $BINARY no existe. Corre 'make' primero." >&2
    exit 1
fi

mkdir -p "$OUTPUT_DIR"
BASE_CSV="$OUTPUT_DIR/arch_base.csv"
ARCH_CSV="$OUTPUT_DIR/arch_treatment.csv"
echo "instance,seed,profit,evals_total,evals_best,time" > "$BASE_CSV"
echo "instance,seed,profit,evals_total,evals_best,time" > "$ARCH_CSV"

parse_result() {
    echo "$1" | grep "^RESULT," | head -1 | cut -d',' -f2,6,7,8
}

for inst in $INSTANCES; do
    inst_file="$INSTANCES_DIR/instancia${inst}${SUFFIX}"
    printf "%-6s " "$inst"
    for seed in $SEEDS; do
        # Baseline
        out=$("$BINARY" "$inst_file" \
            --pop $POP --elite-size $ELITE --gens $GENS_CAP \
            --max-evals $MAX_EVALS --pm $PM --seed $seed 2>&1)
        r=$(parse_result "$out")
        echo "$inst,$seed,$r" >> "$BASE_CSV"

        # Archive
        out=$("$BINARY" "$inst_file" \
            --pop $POP --elite-size $ELITE --gens $GENS_CAP \
            --max-evals $MAX_EVALS --pm $PM --seed $seed \
            --archive-size 5 --w-attract 1.5 2>&1)
        r=$(parse_result "$out")
        echo "$inst,$seed,$r" >> "$ARCH_CSV"
    done
    echo "OK"
done

echo ""
echo "=== Promedio por instancia ==="
printf "  %-6s  %-12s  %-12s  %-8s\n" "inst" "baseline" "archive" "delta%"
for inst in $INSTANCES; do
    b=$(awk -F',' -v i="$inst" 'NR>1 && $1==i {n++; s+=$3} END {printf "%.2f", s/n}' "$BASE_CSV")
    a=$(awk -F',' -v i="$inst" 'NR>1 && $1==i {n++; s+=$3} END {printf "%.2f", s/n}' "$ARCH_CSV")
    d=$(awk -v b="$b" -v a="$a" 'BEGIN {printf "%.4f", 100*(a-b)/b}')
    printf "  %-6s  %-12s  %-12s  %-8s\n" "$inst" "$b" "$a" "$d%"
done