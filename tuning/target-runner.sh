#!/bin/bash
# target-runner.sh — paths sin espacios via symlink /tmp/gmb

GMB="/tmp/gmb/gmb"
LOG="/tmp/gmb-stderr-$$.log"

CONFIG_ID=$1
INSTANCE_ID=$2
SEED=$3
INSTANCE=$4
shift 4

"$GMB" "$INSTANCE" "$@" \
    --seed "$SEED" \
    --gens 10000 \
    --max-evals 100000 \
    > "$LOG" 2>&1
EXIT_CODE=$?

PROFIT=$(tr -d '\r' < "$LOG" | grep "^RESULT," | head -1 | cut -d',' -f2)

if [ -z "$PROFIT" ]; then
    echo "GMB_FAILED exit=$EXIT_CODE args=$@"
    echo "--- LOG ---"
    tail -50 "$LOG"
    rm -f "$LOG"
    exit 1
fi

echo "$PROFIT"
rm -f "$LOG"
