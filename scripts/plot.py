#!/usr/bin/env python3
"""plot.py v5 — Gráficas GMB (boxplots en chunks + notación normal en pct)."""

import csv, os
from collections import defaultdict

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter
from scipy.stats import wilcoxon

RESULTS_DIR = "results"
PLOTS_DIR = os.path.join(RESULTS_DIR, "plots")
BOX_DIR = os.path.join(PLOTS_DIR, "boxplots")
SUBSETS = ["dist2", "dist1", "clusters", "cALL"]
LABEL = {"dist2": "Dist-2 (.txt, 47)", "dist1": "Dist-1 (.mcgb, 47)",
         "clusters": "Clusters (c1-c25)", "cALL": "cALL (5)"}
CONV_SELECTION = [
    ("dist2", "instanciaeil22-DB-2distribution"),
    ("dist1", "instanciac50-DB"),
    ("clusters", "instanciac1"),
    ("cALL", "instanciacALL"),
]


def _path(n):
    return os.path.join(RESULTS_DIR, n)


def _plot_path(n):
    return os.path.join(PLOTS_DIR, n)


def _short(i):
    return (i.replace("instancia", "").replace("-DB-2distribution", "")
             .replace("-DB", "").replace(".mcgb", "").replace(".txt", ""))


def _fmt_pct(v, _):
    if v == 0: return "0"
    av = abs(v)
    if av >= 1000: return f"{int(v):,}"
    if av >= 1: return f"{int(round(v))}"
    return f"{v:.2g}"


def load_summary(sub):
    p = _path(f"{sub}_summary.csv")
    if not os.path.exists(p): return {}
    d = {}
    with open(p) as f:
        for r in csv.DictReader(f):
            d[r["instance"]] = {
                "best": float(r["best"]), "avg": float(r["avg"]),
                "median": float(r["median"]), "std": float(r["std"]),
                "feasible_pct": float(r["min_feasible_pct"]),
            }
    return d


def load_baselines(sub):
    p = _path(f"{sub}_baselines.csv")
    if not os.path.exists(p): return {}
    d = {}
    with open(p) as f:
        for r in csv.DictReader(f):
            d[r["instance"]] = {
                "by_grade": float(r["by_grade_profit"]),
                "by_grade_feasible": int(r["by_grade_feasible"]) == 1,
                "greedy": float(r["greedy_profit"]),
                "greedy_feasible": int(r["greedy_feasible"]) == 1,
            }
    return d


def load_raw(sub):
    p = _path(f"{sub}_raw.csv")
    if not os.path.exists(p): return {}
    d = defaultdict(list)
    with open(p) as f:
        for r in csv.DictReader(f):
            if r["algorithm"] == "CA":
                d[r["instance"]].append(float(r["profit"]))
    return d


def load_conv_log(subset, inst_name):
    p = _path(f"conv_logs/{subset}_{inst_name}.log")
    if not os.path.exists(p): return None
    evals, profits, in_sec = [], [], False
    with open(p) as f:
        for line in f:
            line = line.strip()
            if "PROGRESO CADA 1000 EVALS" in line:
                in_sec = True; continue
            if in_sec and "|" in line and "eval" not in line.lower():
                try:
                    a, b = line.split("|")
                    evals.append(int(a.strip()))
                    profits.append(float(b.strip()))
                except ValueError:
                    continue
    return (evals, profits) if evals else None


# ========== Bar chart ==========

def plot_bars():
    for sub in SUBSETS:
        s, b = load_summary(sub), load_baselines(sub)
        if not s or not b:
            print(f"  SKIP bar_{sub}"); continue
        inst = sorted(set(s) & set(b))
        labels = [_short(i) for i in inst]
        ca = [s[i]["best"] for i in inst]
        bg = [b[i]["by_grade"] for i in inst]
        gr = [b[i]["greedy"] for i in inst]
        x = np.arange(len(inst)); w = 0.27
        fig, ax = plt.subplots(figsize=(max(12, len(inst)*0.35), 5.5))
        ax.bar(x-w, bg, w, label="by_grade", color="#e74c3c", alpha=0.85)
        ax.bar(x, gr, w, label="greedy", color="#f39c12", alpha=0.85)
        ax.bar(x+w, ca, w, label="CA (best)", color="#27ae60", alpha=0.95)
        ax.axhline(0, color="black", lw=0.6)
        ax.set_yscale("symlog", linthresh=100)
        ax.set_xticks(x); ax.set_xticklabels(labels, rotation=90, fontsize=7)
        ax.set_ylabel("Profit (symlog, linthresh=100)")
        ax.set_xlabel("Instancia")
        ax.set_title(f"CA (best) vs baselines — {LABEL[sub]}")
        ax.legend(fontsize=9); ax.grid(axis="y", ls="--", alpha=0.3)
        plt.tight_layout()
        plt.savefig(_plot_path(f"bar_{sub}.png"), dpi=150, bbox_inches="tight")
        plt.close()
        print(f"  OK bar_{sub}.png")


# ========== Boxplots en chunks de 10 ==========

def plot_boxplots():
    os.makedirs(BOX_DIR, exist_ok=True)
    CHUNK_SIZE = 10
    index_rows = []
    for sub in SUBSETS:
        raw = load_raw(sub)
        if not raw:
            print(f"  SKIP box_{sub}"); continue
        medians = {i: np.median(raw[i]) for i in raw}
        ordered = sorted(medians, key=lambda x: medians[x])
        n = len(ordered)
        n_chunks = (n + CHUNK_SIZE - 1) // CHUNK_SIZE
        for c in range(n_chunks):
            chunk = ordered[c*CHUNK_SIZE:(c+1)*CHUNK_SIZE]
            if not chunk: continue
            labels = [_short(i) for i in chunk]
            data = [raw[i] for i in chunk]
            fig, ax = plt.subplots(figsize=(max(7, len(chunk)*0.9), 5.5))
            bp = ax.boxplot(data, patch_artist=True, widths=0.6)
            for patch in bp["boxes"]:
                patch.set_facecolor("#27ae60"); patch.set_alpha(0.6)
            for m in bp["medians"]:
                m.set_color("#c0392b"); m.set_linewidth(2)
            ax.set_xticklabels(labels, rotation=45, ha="right", fontsize=9)
            ax.set_ylabel("Profit (20 seeds)")
            ax.set_title(
                f"Distribución de profit — {LABEL[sub]} (parte {c+1}/{n_chunks})")
            ax.grid(axis="y", ls="--", alpha=0.3)
            plt.tight_layout()
            fname = f"box_{sub}_{c+1:02d}.png"
            plt.savefig(os.path.join(BOX_DIR, fname),
                        dpi=150, bbox_inches="tight")
            plt.close()
            print(f"  OK boxplots/{fname}")
            index_rows.append([fname, sub, c+1, n_chunks, len(chunk),
                               "; ".join(labels)])
    with open(os.path.join(BOX_DIR, "index.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["file", "subset", "part", "total_parts",
                    "n_instances", "instances"])
        w.writerows(index_rows)
    print(f"  OK boxplots/index.csv")


# ========== Pct (avg) ==========

def plot_pct():
    for sub in SUBSETS:
        s, b = load_summary(sub), load_baselines(sub)
        if not s or not b:
            print(f"  SKIP pct_{sub}"); continue
        inst = sorted(set(s) & set(b))
        labels, pct_bg, pct_gr = [], [], []
        excluded = 0
        for i in inst:
            if not b[i]["by_grade_feasible"] and not b[i]["greedy_feasible"]:
                excluded += 1; continue
            ca = s[i]["avg"]
            bg = b[i]["by_grade"] if b[i]["by_grade_feasible"] else None
            gr = b[i]["greedy"] if b[i]["greedy_feasible"] else None
            labels.append(_short(i))
            pct_bg.append(100*(ca-bg)/abs(bg) if bg is not None and abs(bg) > 1 else None)
            pct_gr.append(100*(ca-gr)/abs(gr) if gr is not None and abs(gr) > 1 else None)
        if not labels:
            print(f"  SKIP pct_{sub} (todos baselines infeasibles)"); continue
        fig, ax = plt.subplots(figsize=(max(12, len(labels)*0.35), 5.5))
        x = np.arange(len(labels)); w = 0.4
        ax.bar(x-w/2, [v if v is not None else 0 for v in pct_bg], w,
               label="vs by_grade (avg)", color="#e74c3c", alpha=0.85)
        ax.bar(x+w/2, [v if v is not None else 0 for v in pct_gr], w,
               label="vs greedy (avg)", color="#f39c12", alpha=0.85)
        for j, (pbg, pgr) in enumerate(zip(pct_bg, pct_gr)):
            if pbg is None:
                ax.text(j-w/2, 0, "N/A", ha="center", va="bottom",
                        fontsize=6, rotation=90, color="#7f8c8d")
            if pgr is None:
                ax.text(j+w/2, 0, "N/A", ha="center", va="bottom",
                        fontsize=6, rotation=90, color="#7f8c8d")
        ax.axhline(0, color="black", lw=0.8)
        ax.set_yscale("symlog", linthresh=10)
        ax.yaxis.set_major_formatter(FuncFormatter(_fmt_pct))
        ax.set_xticks(x); ax.set_xticklabels(labels, rotation=90, fontsize=7)
        ax.set_ylabel("% mejora (CA avg vs baseline)")
        title = f"% mejora (avg) — {LABEL[sub]}"
        if excluded: title += f"  [{excluded} N/A excluidos]"
        ax.set_title(title)
        ax.legend(fontsize=9); ax.grid(axis="y", ls="--", alpha=0.3)
        plt.tight_layout()
        plt.savefig(_plot_path(f"pct_{sub}.png"), dpi=150, bbox_inches="tight")
        plt.close()
        print(f"  OK pct_{sub}.png")


# ========== Convergence ==========

def plot_conv():
    for sub, inst_name in CONV_SELECTION:
        data = load_conv_log(sub, inst_name)
        if data is None:
            print(f"  SKIP conv_{sub}_{inst_name}"); continue
        evals, profits = data
        fig, ax = plt.subplots(figsize=(8, 4.5))
        ax.plot(evals, profits, "-", color="#27ae60", lw=2)
        ax.set_xlabel("Evaluaciones"); ax.set_ylabel("Mejor profit factible")
        ax.set_title(f"Convergencia — {_short(inst_name)} ({sub})")
        ax.grid(alpha=0.3)
        ax.xaxis.set_major_formatter(
            FuncFormatter(lambda x, _: f"{int(x/1000)}k" if x < 1e6 else f"{x/1e6:.1f}M"))
        plt.tight_layout()
        plt.savefig(_plot_path(f"conv_{sub}_{_short(inst_name)}.png"),
                    dpi=150, bbox_inches="tight")
        plt.close()
        print(f"  OK conv_{sub}_{_short(inst_name)}.png")


# ========== Wilcoxon (avg) ==========

def wilcoxon_stats(sub):
    s, b = load_summary(sub), load_baselines(sub)
    if not s or not b: return None
    inst = sorted(set(s) & set(b))
    if len(inst) < 5: return None
    ca = np.array([s[i]["avg"] for i in inst])
    bg = np.array([b[i]["by_grade"] for i in inst])
    gr = np.array([b[i]["greedy"] for i in inst])
    def st(ref):
        w = int(np.sum(ca > ref)); t = int(np.sum(ca == ref))
        l = int(np.sum(ca < ref))
        try: _, p = wilcoxon(ca, ref)
        except ValueError: p = float("nan")
        return {"w": w, "t": t, "l": l, "p": p, "n": len(ca)}
    return {"bg": st(bg), "gr": st(gr)}


def plot_wilcoxon():
    fig, axes = plt.subplots(1, 2, figsize=(14, 5))
    lbls = ["CA (avg) vs by_grade", "CA (avg) vs greedy"]
    keys = ["bg", "gr"]; colors = ["#e74c3c", "#f39c12"]
    rows = []
    for ax, lbl, key, col in zip(axes, lbls, keys, colors):
        names, w_, t_, l_, p_ = [], [], [], [], []
        for sub in SUBSETS:
            st = wilcoxon_stats(sub)
            if st is None: continue
            names.append(sub)
            w_.append(st[key]["w"]); t_.append(st[key]["t"])
            l_.append(st[key]["l"]); p_.append(st[key]["p"])
            rows.append([sub, key, st[key]["n"], st[key]["w"],
                         st[key]["t"], st[key]["l"], st[key]["p"]])
        x = np.arange(len(names))
        w = np.array(w_); t = np.array(t_); l = np.array(l_)
        ax.bar(x, w, color="#27ae60", label="Wins")
        ax.bar(x, t, bottom=w, color="#95a5a6", label="Ties")
        ax.bar(x, l, bottom=w+t, color=col, label="Losses")
        ax.set_xticks(x); ax.set_xticklabels(names)
        ax.set_ylabel("Instancias"); ax.set_title(lbl)
        top = max((w+t+l).max() if len(w) else 1, 1)
        for i, p in enumerate(p_):
            txt = f"p={p:.4f}" if p == p else "p=NA"
            ax.text(i, top*1.05, txt, ha="center", fontsize=9)
        ax.set_ylim(0, top*1.25)
        ax.legend(fontsize=9); ax.grid(axis="y", ls="--", alpha=0.3)
    plt.suptitle("Wilcoxon signed-rank: CA (avg) vs baselines",
                 fontsize=13, fontweight="bold")
    plt.tight_layout()
    plt.savefig(_plot_path("wilcoxon.png"), dpi=150, bbox_inches="tight")
    plt.close(); print("  OK wilcoxon.png")
    with open(_path("wilcoxon_results.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["subset", "baseline", "n", "wins", "ties", "losses", "p"])
        w.writerows(rows)


# ========== Summary table ==========

def plot_summary_table():
    rows = []
    for sub in SUBSETS:
        s, b = load_summary(sub), load_baselines(sub)
        st = wilcoxon_stats(sub)
        if not s or not b: continue
        n = len(s)
        avg_best = np.mean([s[i]["best"] for i in s])
        avg_avg = np.mean([s[i]["avg"] for i in s])
        avg_bg = np.mean([b[i]["by_grade"] for i in s])
        avg_gr = np.mean([b[i]["greedy"] for i in s])
        p_bg = st["bg"]["p"] if st else float("nan")
        p_gr = st["gr"]["p"] if st else float("nan")
        rows.append([sub, n, avg_best, avg_avg, avg_bg, avg_gr, p_bg, p_gr])
    fig, ax = plt.subplots(figsize=(13, 1 + 0.5*len(rows)))
    ax.axis("off")
    headers = ["Subset", "N", "CA best (μ)", "CA avg (μ)",
               "by_grade (μ)", "greedy (μ)", "p vs bg", "p vs gr"]
    cell = [[str(r[0]), str(r[1]), f"{r[2]:.1f}", f"{r[3]:.1f}",
             f"{r[4]:.1f}", f"{r[5]:.1f}",
             f"{r[6]:.4f}" if r[6] == r[6] else "NA",
             f"{r[7]:.4f}" if r[7] == r[7] else "NA"] for r in rows]
    t = ax.table(cellText=cell, colLabels=headers,
                 loc="center", cellLoc="center")
    t.auto_set_font_size(False); t.set_fontsize(10); t.scale(1, 1.8)
    for j in range(len(headers)):
        t[(0, j)].set_facecolor("#27ae60")
        t[(0, j)].set_text_props(color="white", fontweight="bold")
    plt.tight_layout()
    plt.savefig(_plot_path("summary_table.png"), dpi=150, bbox_inches="tight")
    plt.close(); print("  OK summary_table.png")


if __name__ == "__main__":
    os.makedirs(PLOTS_DIR, exist_ok=True)
    print(f"Generando gráficas en {PLOTS_DIR}/...")
    plot_bars()
    plot_boxplots()
    plot_pct()
    plot_conv()
    plot_wilcoxon()
    plot_summary_table()
    print("Listo.")