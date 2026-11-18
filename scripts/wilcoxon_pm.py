#!/usr/bin/env python3
import csv
import sys
from scipy.stats import wilcoxon

if len(sys.argv) != 3:
    print(f"Uso: {sys.argv[0]} <base.csv> <tratamiento.csv>", file=sys.stderr)
    sys.exit(1)

BASE_CSV = sys.argv[1]
TREAT_CSV = sys.argv[2]

def load(path):
    data = {}
    with open(path) as f:
        for row in csv.DictReader(f):
            data[(row["instance"], row["seed"])] = float(row["profit"])
    return data

base = load(BASE_CSV)
treat = load(TREAT_CSV)
keys = sorted(set(base) & set(treat))
b = [base[k] for k in keys]
t = [treat[k] for k in keys]

wins = sum(1 for x, y in zip(t, b) if x > y)
ties = sum(1 for x, y in zip(t, b) if x == y)
losses = sum(1 for x, y in zip(t, b) if x < y)
print(f"Observaciones: {len(keys)} pares")
print(f"  tratamiento > baseline: {wins}")
print(f"  tratamiento = baseline: {ties}")
print(f"  tratamiento < baseline: {losses}")

stat, p = wilcoxon(t, b)
print(f"\nWilcoxon signed-rank test:")
print(f"  statistic = {stat:.4f}")
print(f"  p-value   = {p:.4f}")

if p < 0.05:
    print(f"  → Diferencia SIGNIFICATIVA")
else:
    print(f"  → NO hay diferencia significativa")