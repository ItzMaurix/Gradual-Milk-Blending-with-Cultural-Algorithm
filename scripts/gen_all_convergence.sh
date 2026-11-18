#!/bin/bash
# Genera convergencia (seed=1) para todas las instancias de los 4 subsets.
# Tiempo estimado: ~5-8 min. Salida: results/conv_logs/

set -e

cd "/mnt/c/OneDrive/Escritorio/IAA/Proyecto GMB"

OUT_DIR="results/conv_logs"
mkdir -p "$OUT_DIR"

CA_POP=31
CA_ELITE=7
CA_PM=0.054
CA_TOUR=5
CA_K_RCL=2
CA_ARCHIVE=3
CA_W_ATTRACT=2.10
GENS_CAP=100000
MAX_EVALS=1000000
SEED=1

for SUBSET in dist2 dist1 clusters cALL; do
    case "$SUBSET" in
        dist2)    DIR="data/instances/dist2";    EXT="txt"  ;;
        dist1)    DIR="data/instances/dist1";    EXT="mcgb" ;;
        clusters) DIR="data/instances/clusters"; EXT="mcgb" ;;
        cALL)     DIR="data/instances/cALL";     EXT="mcgb" ;;
    esac

    echo "=== Subset: $SUBSET ==="
    for inst_file in "$DIR"/*."$EXT"; do
        [ -e "$inst_file" ] || continue
        inst=$(basename "$inst_file" ".$EXT")
        printf "  %-40s " "$inst"
        ./gmb "$inst_file" \
            --pop $CA_POP --elite-size $CA_ELITE \
            --pm $CA_PM --tour $CA_TOUR --k_rcl $CA_K_RCL \
            --archive-size $CA_ARCHIVE --w-attract $CA_W_ATTRACT \
            --gens $GENS_CAP --max-evals $MAX_EVALS \
            --seed $SEED \
            --log-every-1000-evals \
            > "$OUT_DIR/${SUBSET}_${inst}.log" 2>&1
        echo "OK"
    done
done

echo ""
echo "Total logs: $(ls "$OUT_DIR"/*.log | wc -l)"
echo "Espacio: $(du -sh "$OUT_DIR" | cut -f1)"
echo "Listo en $OUT_DIR/"
