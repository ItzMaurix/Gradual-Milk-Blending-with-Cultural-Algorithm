#!/bin/bash
# ============================================================
# run_all.sh — Recolector de resultados (protocolo grupal GMB)
#
# Uso:
#   ./scripts/run_all.sh <SUBSET> [RUNS]
#
# SUBSET:
#   dist1     -> data/instances/dist1/*.mcgb       (47 instancias)
#   dist2     -> data/instances/dist2/*.txt        (47 instancias)
#   clusters  -> data/instances/clusters/*.mcgb    (25 instancias)
#   cALL      -> data/instances/cALL/*.mcgb        (5 instancias)
#
# Defaults (protocolo grupal):
#   RUNS=20, seeds 1..20, max-evals=100000
#
# Ejemplos:
#   ./scripts/run_all.sh dist2 20            # un subset
#   for s in dist1 dist2 clusters cALL; do   # todos
#       ./scripts/run_all.sh $s 20
#   done
#
# Genera por subset:
#   results/<subset>_raw.csv       — una fila por (instancia, seed)
#   results/<subset>_summary.csv   — best/avg/median/std/feasible_pct
#   results/<subset>_baselines.csv — by_grade y greedy por instancia
# ============================================================

set -e

# ===== Argumentos =====
SUBSET="${1:-}"
if [ -z "$SUBSET" ]; then
    echo "ERROR: falta SUBSET (dist1, dist2, clusters, cALL)" >&2
    echo "Uso: $0 <SUBSET> [RUNS]" >&2
    exit 1
fi
RUNS="${2:-20}"

# ===== Paths =====
INSTANCES_BASE="${INSTANCES_BASE:-data/instances}"
BINARY="${BINARY:-./gmb}"
OUTPUT_DIR="${OUTPUT_DIR:-results}"

# ===== Configuracion por subset (dir + extension) =====
case "$SUBSET" in
    dist1)     INSTANCES_DIR="$INSTANCES_BASE/dist1";    EXT="mcgb" ;;
    dist2)     INSTANCES_DIR="$INSTANCES_BASE/dist2";    EXT="txt"  ;;
    clusters)  INSTANCES_DIR="$INSTANCES_BASE/clusters"; EXT="mcgb" ;;
    cALL)      INSTANCES_DIR="$INSTANCES_BASE/cALL";     EXT="mcgb" ;;
    *)
        echo "ERROR: SUBSET debe ser dist1, dist2, clusters o cALL" >&2
        echo "Recibido: '$SUBSET'" >&2
        exit 1
        ;;
esac

# ===== Parametros del CA (config 406 de irace) =====
CA_POP="${CA_POP:-31}"
CA_ELITE="${CA_ELITE:-7}"
CA_PC="${CA_PC:-0.8}"
CA_PM="${CA_PM:-0.054}"
CA_TOUR="${CA_TOUR:-5}"
CA_K_RCL="${CA_K_RCL:-2}"
CA_ARCHIVE="${CA_ARCHIVE:-3}"
CA_W_ATTRACT="${CA_W_ATTRACT:-2.10}"
CA_GENS_CAP="${CA_GENS_CAP:-100000}"
MAX_EVALS="${MAX_EVALS:-1000000}"

# ===== Seeds (protocolo: 1..20) =====
SEED_START="${SEED_START:-1}"
SEED_END="${SEED_END:-20}"

# ===== Validaciones =====
if [ ! -x "$BINARY" ]; then
    echo "ERROR: $BINARY no existe o no es ejecutable. Corre 'make' primero." >&2
    exit 1
fi

if [ ! -d "$INSTANCES_DIR" ]; then
    echo "ERROR: $INSTANCES_DIR no existe." >&2
    exit 1
fi

EXPECTED_RUNS=$((SEED_END - SEED_START + 1))
if [ "$RUNS" -ne "$EXPECTED_RUNS" ]; then
    echo "ADVERTENCIA: RUNS=$RUNS, pero seeds=$SEED_START..$SEED_END ($EXPECTED_RUNS valores)." >&2
    echo "             Ajustando RUNS=$EXPECTED_RUNS para coincidir con las seeds." >&2
    RUNS=$EXPECTED_RUNS
fi

# ===== Output =====
mkdir -p "$OUTPUT_DIR"
RAW_CSV="$OUTPUT_DIR/${SUBSET}_raw.csv"
SUMMARY_CSV="$OUTPUT_DIR/${SUBSET}_summary.csv"
BASELINES_CSV="$OUTPUT_DIR/${SUBSET}_baselines.csv"

# Formato CSV del protocolo: algoritmo, instancia, seed, f_best, evals_best, evals_total, time
echo "algorithm,instance,seed,profit,revenue,transport,feasible,evals_total,evals_best,time_s" > "$RAW_CSV"
echo "instance,by_grade_profit,by_grade_feasible,greedy_profit,greedy_feasible" > "$BASELINES_CSV"

# ===== Loop principal =====
echo "============================================================"
echo "=== Ejecutando CA sobre subset '$SUBSET' ==="
echo "============================================================"
echo "  Instances dir : $INSTANCES_DIR"
echo "  Extension     : .$EXT"
echo "  Runs          : $RUNS (seeds $SEED_START..$SEED_END)"
echo "  Max evals     : $MAX_EVALS"
echo "  CA config     : pop=$CA_POP elite=$CA_ELITE pm=$CA_PM"
echo "                  tour=$CA_TOUR k_rcl=$CA_K_RCL"
echo "                  archive=$CA_ARCHIVE w_attract=$CA_W_ATTRACT"
echo "  Output        : $OUTPUT_DIR/${SUBSET}_*.csv"
echo ""

TOTAL=0
N_INST=0
for inst_file in "$INSTANCES_DIR"/*."$EXT"; do
    [ -e "$inst_file" ] || continue
    inst_name=$(basename "$inst_file" ".$EXT")
    printf "  %-40s " "$inst_name"

    for seed in $(seq "$SEED_START" "$SEED_END"); do
        output=$("$BINARY" "$inst_file" \
            --pop "$CA_POP" --elite-size "$CA_ELITE" \
            --pc "$CA_PC" --pm "$CA_PM" \
            --tour "$CA_TOUR" --k_rcl "$CA_K_RCL" \
            --archive-size "$CA_ARCHIVE" --w-attract "$CA_W_ATTRACT" \
            --gens "$CA_GENS_CAP" --max-evals "$MAX_EVALS" \
            --seed "$seed" 2>&1) || true

        result_line=$(echo "$output" | tr -d '\r' | grep "^RESULT," | head -1)

        if [ -z "$result_line" ]; then
            echo "" >&2
            echo "  ADVERTENCIA: sin RESULT en $inst_name seed=$seed" >&2
            continue
        fi

        # RESULT,profit,revenue,transport,feasible,evals_total,evals_best,time
        profit=$(echo "$result_line" | cut -d',' -f2)
        revenue=$(echo "$result_line" | cut -d',' -f3)
        transport=$(echo "$result_line" | cut -d',' -f4)
        feasible=$(echo "$result_line" | cut -d',' -f5)
        evals_total=$(echo "$result_line" | cut -d',' -f6)
        evals_best=$(echo "$result_line" | cut -d',' -f7)
        time_s=$(echo "$result_line" | cut -d',' -f8)

        echo "CA,$inst_name,$seed,$profit,$revenue,$transport,$feasible,$evals_total,$evals_best,$time_s" >> "$RAW_CSV"
        TOTAL=$((TOTAL + 1))
    done

    # Baseline: correr con pop=1, gens=1, sin archive, para no ensuciar
    bl_out=$("$BINARY" "$inst_file" \
        --pop 1 --elite-size 1 --gens 1 --pm 0.1 \
        --archive-size 0 2>&1) || true
    bl_line=$(echo "$bl_out" | tr -d '\r' | grep "^BASELINE," | head -1)
    if [ -n "$bl_line" ]; then
        echo "$inst_name,$(echo "$bl_line" | cut -d',' -f2-)" >> "$BASELINES_CSV"
    fi

    N_INST=$((N_INST + 1))
    printf "OK\n"
done

echo ""
echo "  Instancias procesadas : $N_INST"
echo "  Total de corridas CA  : $TOTAL"
echo "  Raw                   : $RAW_CSV"

# ===== Summary agregado =====
echo ""
echo "=== Generando summary agregado ==="
echo "instance,n_runs,best,avg,median,std,min_feasible_pct,avg_evals_best" > "$SUMMARY_CSV"

awk -F',' '
    NR > 1 && $1 == "CA" {
        inst = $2
        profit = $4
        feas = $7
        evals_best = $9
        if (!(inst in seen)) {
            seen[inst] = 1
            n[inst] = 0
            nfeas[inst] = 0
            sum[inst] = 0
            sumsq[inst] = 0
            sum_eb[inst] = 0
            best[inst] = -1e18
        }
        n[inst]++
        profits[inst, n[inst]] = profit
        sum[inst] += profit
        sumsq[inst] += profit * profit
        sum_eb[inst] += evals_best
        if (profit > best[inst]) best[inst] = profit
        if (feas == 1) nfeas[inst]++
    }
    END {
        for (inst in seen) {
            n_ = n[inst]
            avg = sum[inst] / n_
            var = (sumsq[inst] / n_) - (avg * avg)
            if (var < 0) var = 0
            std = sqrt(var)
            feas_pct = (n_ > 0) ? (100.0 * nfeas[inst] / n_) : 0
            avg_eb = sum_eb[inst] / n_

            # Mediana (insertion sort simple)
            for (i = 1; i <= n_; i++) tmp[i] = profits[inst, i]
            for (i = 2; i <= n_; i++) {
                v = tmp[i]; j = i - 1
                while (j >= 1 && tmp[j] > v) { tmp[j+1] = tmp[j]; j-- }
                tmp[j+1] = v
            }
            if (n_ % 2 == 1) med = tmp[(n_+1)/2]
            else med = (tmp[n_/2] + tmp[n_/2+1]) / 2

            printf "%s,%d,%.2f,%.2f,%.2f,%.2f,%.1f,%.0f\n",
                   inst, n_, best[inst], avg, med, std, feas_pct, avg_eb
        }
    }
' "$RAW_CSV" | sort >> "$SUMMARY_CSV"

echo "  Summary: $SUMMARY_CSV"
echo ""
echo "=== Listo ==="
echo ""
echo "Ver resumen:"
echo "  column -t -s',' $SUMMARY_CSV"

# Cuenta instancias con feasible_pct < 100
low_feas=$(awk -F',' 'NR>1 && $6 < 100 {n++} END {print n+0}' "$SUMMARY_CSV")
echo "  Instancias con feasible_pct < 100%: $low_feas"