#!/bin/bash
# ============================================================
# tune.sh — Sintonizacion hibrida de hiperparametros del CA
#
# Fase 1 (OFAT): variar k_rcl y pm manteniendo el resto en default.
# Fase 2 (Grid 2D): explorar todas las combinaciones k_rcl x pm.
#
# Uso:
#   ./scripts/tune.sh phase1     # solo OFAT (5 configs x 5 inst x 5 runs = 125)
#   ./scripts/tune.sh phase2     # solo grid (9 configs x 5 inst x 5 runs = 225)
#   ./scripts/tune.sh all        # ambas (350 corridas totales)
#
# Salidas:
#   results/tuning_phase1.csv            # corridas crudas de fase 1
#   results/tuning_phase1_summary.csv    # agregado por (k_rcl, pm)
#   results/tuning_phase2.csv            # corridas crudas de fase 2
#   results/tuning_phase2_summary.csv    # agregado por (k_rcl, pm)
# ============================================================

set -e

# ===== Configuracion =====
INSTANCES="eil22 eil30 a37 a46 c50"
INSTANCES_DIR="data/instances"
INSTANCES_SUFFIX="-DB-2distribution.txt"
BINARY="./gmb"
OUTPUT_DIR="results"
RUNS="${RUNS:-5}"
POP="${POP:-30}"
TOUR="${TOUR:-3}"
GENS="${GENS:-100}"
PC="${PC:-0.8}"
SEED_BASE="${SEED_BASE:-200}"

# ===== Validaciones =====
if [ ! -x "$BINARY" ]; then
    echo "ERROR: $BINARY no existe o no es ejecutable. Corre 'make' primero." >&2
    exit 1
fi

mkdir -p "$OUTPUT_DIR"

# ===== Ejecutar una corrida y devolver "profit,feasible" =====
run_one() {
    local k_rcl=$1
    local pm=$2
    local inst=$3
    local seed=$4
    local inst_file="$INSTANCES_DIR/instancia${inst}${INSTANCES_SUFFIX}"

    local out
    out=$("$BINARY" "$inst_file" \
        --pop "$POP" --gens "$GENS" --pc "$PC" --pm "$pm" \
        --tour "$TOUR" --k_rcl "$k_rcl" --seed "$seed" 2>&1)

    local line
    line=$(echo "$out" | grep "^RESULT," | head -1)

    if [ -z "$line" ]; then
        echo "ERR,0"
        return 1
    fi

    local profit=$(echo "$line" | cut -d',' -f2)
    local feas=$(echo "$line" | cut -d',' -f5)
    echo "$profit,$feas"
    return 0
}

# ===== Fase 1: OFAT =====
tune_phase1() {
    local csv="$OUTPUT_DIR/tuning_phase1.csv"
    echo "k_rcl,pm,instance,run,seed,profit,feasible" > "$csv"

    local total=0
    local expected=$(( (3 + 2) * 5 * RUNS ))  # 5 configs unicas

    echo "=== FASE 1: OFAT (k_rcl y pm) ==="
    echo "  Configs: (k_rcl in {2,3,5}, pm=0.2) + (k_rcl=3, pm in {0.1,0.3})"
    echo "  Total esperado: $expected corridas"
    echo ""

    # Bloque A: variar k_rcl con pm=0.2
    for k_rcl in 2 3 5; do
        for inst in $INSTANCES; do
            for run in $(seq 1 "$RUNS"); do
                seed=$((SEED_BASE + run))
                printf "  [A] k_rcl=%-2s pm=0.2  %-6s run=%-2d seed=%-4d " \
                       "$k_rcl" "$inst" "$run" "$seed"
                result=$(run_one "$k_rcl" 0.2 "$inst" "$seed")
                profit=$(echo "$result" | cut -d',' -f1)
                feas=$(echo "$result" | cut -d',' -f2)
                echo "$k_rcl,0.2,$inst,$run,$seed,$profit,$feas" >> "$csv"
                echo "profit=$profit feasible=$feas"
                total=$((total+1))
            done
        done
    done

    # Bloque B: variar pm con k_rcl=3 (excluyendo pm=0.2 ya cubierto arriba)
    for pm in 0.1 0.3; do
        for inst in $INSTANCES; do
            for run in $(seq 1 "$RUNS"); do
                seed=$((SEED_BASE + run))
                printf "  [B] k_rcl=3  pm=%-3s %-6s run=%-2d seed=%-4d " \
                       "$pm" "$inst" "$run" "$seed"
                result=$(run_one 3 "$pm" "$inst" "$seed")
                profit=$(echo "$result" | cut -d',' -f1)
                feas=$(echo "$result" | cut -d',' -f2)
                echo "3,$pm,$inst,$run,$seed,$profit,$feas" >> "$csv"
                echo "profit=$profit feasible=$feas"
                total=$((total+1))
            done
        done
    done

    echo ""
    echo "Fase 1 completa: $total corridas -> $csv"
}

# ===== Fase 2: Grid 2D =====
tune_phase2() {
    local csv="$OUTPUT_DIR/tuning_phase2.csv"
    echo "k_rcl,pm,instance,run,seed,profit,feasible" > "$csv"

    local total=0
    local expected=$((3 * 3 * 5 * RUNS))  # 9 configs

    echo ""
    echo "=== FASE 2: Grid 2D (k_rcl x pm) ==="
    echo "  Configs: k_rcl in {2,3,5} x pm in {0.1,0.2,0.3}"
    echo "  Total esperado: $expected corridas"
    echo ""

    for k_rcl in 2 3 5; do
        for pm in 0.1 0.2 0.3; do
            for inst in $INSTANCES; do
                for run in $(seq 1 "$RUNS"); do
                    seed=$((SEED_BASE + run))
                    printf "  [2D] k_rcl=%-2s pm=%-3s %-6s run=%-2d seed=%-4d " \
                           "$k_rcl" "$pm" "$inst" "$run" "$seed"
                    result=$(run_one "$k_rcl" "$pm" "$inst" "$seed")
                    profit=$(echo "$result" | cut -d',' -f1)
                    feas=$(echo "$result" | cut -d',' -f2)
                    echo "$k_rcl,$pm,$inst,$run,$seed,$profit,$feas" >> "$csv"
                    echo "profit=$profit feasible=$feas"
                    total=$((total+1))
                done
            done
        done
    done

    echo ""
    echo "Fase 2 completa: $total corridas -> $csv"
}

# ===== Agregado por (k_rcl, pm) =====
summarize() {
    local input="$1"
    local output="$2"

    echo "k_rcl,pm,n_runs,avg_profit,min_profit,max_profit,std_profit,feasible_pct" > "$output"

    awk -F',' '
        NR>1 && $6 != "" && $6 != "ERR" {
            key = $1 "|" $2
            n[key]++
            sum[key] += $6
            sumsq[key] += $6 * $6
            if (!(key in min) || $6 < min[key]) min[key] = $6
            if (!(key in max) || $6 > max[key]) max[key] = $6
            if ($7 == 1) feas[key]++
        }
        END {
            for (key in n) {
                split(key, parts, "|")
                k_rcl = parts[1]
                pm = parts[2]
                avg = sum[key] / n[key]
                var = sumsq[key] / n[key] - avg * avg
                if (var < 0) var = 0
                std = sqrt(var)
                fpct = (n[key] > 0) ? (100.0 * feas[key] / n[key]) : 0
                printf "%s,%s,%d,%.2f,%.2f,%.2f,%.2f,%.1f\n",
                       k_rcl, pm, n[key], avg, min[key], max[key], std, fpct
            }
        }
    ' "$input" | sort -t',' -k1,1n -k2,2n >> "$output"
}

# ===== Main =====
MODE="${1:-all}"

case "$MODE" in
    phase1)
        tune_phase1
        summarize "$OUTPUT_DIR/tuning_phase1.csv" "$OUTPUT_DIR/tuning_phase1_summary.csv"
        ;;
    phase2)
        tune_phase2
        summarize "$OUTPUT_DIR/tuning_phase2.csv" "$OUTPUT_DIR/tuning_phase2_summary.csv"
        ;;
    all)
        tune_phase1
        summarize "$OUTPUT_DIR/tuning_phase1.csv" "$OUTPUT_DIR/tuning_phase1_summary.csv"
        tune_phase2
        summarize "$OUTPUT_DIR/tuning_phase2.csv" "$OUTPUT_DIR/tuning_phase2_summary.csv"
        ;;
    *)
        echo "Uso: $0 [phase1|phase2|all]" >&2
        exit 1
        ;;
esac

# ===== Reporte final =====
echo ""
echo "============================================"
echo "=== MEJOR CONFIGURACION POR FASE ==="
echo "============================================"

if [ -f "$OUTPUT_DIR/tuning_phase1_summary.csv" ]; then
    echo ""
    echo "--- Fase 1 (OFAT) ---"
    column -t -s',' "$OUTPUT_DIR/tuning_phase1_summary.csv"
    echo ""
    echo "  TOP 3 configs de Fase 1 (por avg_profit):"
    (head -1 "$OUTPUT_DIR/tuning_phase1_summary.csv"
     tail -n +2 "$OUTPUT_DIR/tuning_phase1_summary.csv" | sort -t',' -k4,4nr | head -3) \
        | column -t -s','
fi

if [ -f "$OUTPUT_DIR/tuning_phase2_summary.csv" ]; then
    echo ""
    echo "--- Fase 2 (Grid 2D) ---"
    column -t -s',' "$OUTPUT_DIR/tuning_phase2_summary.csv"
    echo ""
    echo "  TOP 3 configs de Fase 2 (por avg_profit):"
    (head -1 "$OUTPUT_DIR/tuning_phase2_summary.csv"
     tail -n +2 "$OUTPUT_DIR/tuning_phase2_summary.csv" | sort -t',' -k4,4nr | head -3) \
        | column -t -s','
fi

echo ""
echo "=== Fin ==="