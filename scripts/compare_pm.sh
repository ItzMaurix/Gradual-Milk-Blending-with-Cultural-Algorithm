#!/bin/bash
# ============================================================
# compare_pm.sh — Baseline (pm fijo) vs Adaptive pm
#
# 5 instancias × 5 seeds × 2 versiones = 50 corridas
# Presupuesto: 100k evaluaciones por corrida
# ============================================================

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
PM_BASE=0.3
PM_MIN=0.1
PM_MAX=0.5
PATIENCE=20

if [ ! -x "$BINARY" ]; then
    echo "ERROR: $BINARY no existe. Corre 'make' primero." >&2
    exit 1
fi

mkdir -p "$OUTPUT_DIR"
BASE_CSV="$OUTPUT_DIR/pm_base.csv"
ADA_CSV="$OUTPUT_DIR/pm_adaptive.csv"
echo "instance,seed,profit,evals_total,evals_best,time" > "$BASE_CSV"
echo "instance,seed,profit,evals_total,evals_best,time" > "$ADA_CSV"

parse_result() {
    # $1 = output completo
    # Extrae profit,evals_total,evals_best,time de la linea RESULT
    echo "$1" | grep "^RESULT," | head -1 | cut -d',' -f2,6,7,8
}

total=0
for inst in $INSTANCES; do
    inst_file="$INSTANCES_DIR/instancia${inst}${SUFFIX}"
    printf "%-6s " "$inst"

    for seed in $SEEDS; do
        # Baseline
        out=$("$BINARY" "$inst_file" \
            --pop $POP --elite-size $ELITE --gens $GENS_CAP \
            --max-evals $MAX_EVALS --pm $PM_BASE --seed $seed 2>&1)
        r=$(parse_result "$out")
        echo "$inst,$seed,$r" >> "$BASE_CSV"

        # Adaptive
        out=$("$BINARY" "$inst_file" \
            --pop $POP --elite-size $ELITE --gens $GENS_CAP \
            --max-evals $MAX_EVALS --pm $PM_BASE --seed $seed \
            --adaptive-pm --pm-min $PM_MIN --pm-max $PM_MAX \
            --stall-patience $PATIENCE 2>&1)
        r=$(parse_result "$out")
        echo "$inst,$seed,$r" >> "$ADA_CSV"

        total=$((total + 2))
    done
    echo "OK (5 seeds x 2 versiones)"
done

echo ""
echo "Total corridas: $total"
echo "  Baseline: $BASE_CSV"
echo "  Adaptive: $ADA_CSV"
echo ""

# --- Comparacion por instancia ---
echo "=== Promedio de profit por instancia ==="
printf "  %-6s  %-12s  %-12s  %-8s\n" "inst" "baseline" "adaptive" "delta%"
for inst in $INSTANCES; do
    b=$(awk -F',' -v i="$inst" 'NR>1 && $1==i {n++; s+=$3} END {if(n) printf "%.2f", s/n; else print "NA"}' "$BASE_CSV")
    a=$(awk -F',' -v i="$inst" 'NR>1 && $1==i {n++; s+=$3} END {if(n) printf "%.2f", s/n; else print "NA"}' "$ADA_CSV")
    d=$(awk -v b="$b" -v a="$a" 'BEGIN {if(b>0) printf "%.4f", 100*(a-b)/b; else print "NA"}')
    printf "  %-6s  %-12s  %-12s  %-8s\n" "$inst" "$b" "$a" "$d%"
done

echo ""
echo "=== Promedio global (sobre todas las instancias) ==="
b=$(awk -F',' 'NR>1 {n++; s+=$3} END {printf "%.2f", s/n}' "$BASE_CSV")
a=$(awk -F',' 'NR>1 {n++; s+=$3} END {printf "%.2f", s/n}' "$ADA_CSV")
echo "  baseline: $b"
echo "  adaptive: $a"
awk -v b="$b" -v a="$a" 'BEGIN {printf "  delta:    %.4f%%\n", 100*(a-b)/b}'

echo ""
echo "=== Tiempo total ==="
tb=$(awk -F',' 'NR>1 {s+=$6} END {printf "%.2f", s}' "$BASE_CSV")
ta=$(awk -F',' 'NR>1 {s+=$6} END {printf "%.2f", s}' "$ADA_CSV")
echo "  baseline: ${tb}s"
echo "  adaptive: ${ta}s"