"""
plot_fig17_fct.py
-----------------
Reads ns-3 FlowMonitor XML files produced by queue-track and reproduces
Fig. 17 (average and 99th-percentile query flow completion time vs.
concurrent senders) for three AQM schemes: RED, CODEL, ECNSharp.

Expected XML naming convention (set by --id in the simulation):
    Queue_Track_{AQM}_{N}_Flow_Monitor.xml
where AQM in {RED, CODEL, ECNSharp} and N in SENDERS.

Usage:
    python3 plot_fig17_fct.py [--xml-dir /path/to/xmls] [--out fig17.png]
"""

import xml.etree.ElementTree as ET
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import argparse
import os

# ── configuration ────────────────────────────────────────────────────────────
SENDERS  = [25, 50, 75, 100, 125, 150, 175, 200]

SCHEMES  = {
    "DCTCP-RED-Tail": {"aqm": "RED",      "color": "black",   "marker": "s", "ls": "-"},
    r"ECN$^\sharp$":  {"aqm": "ECNSharp", "color": "red",     "marker": "o", "ls": "-"},
    "CoDel":          {"aqm": "CODEL",    "color": "magenta", "marker": "^", "ls": "-"},
}

# ── helpers ───────────────────────────────────────────────────────────────────

def parse_fct(xml_path):
    """
    Return a list of per-flow FCTs (seconds) from a FlowMonitor XML.
    FCT = timeLastRxPacket - timeFirstTxPacket for flows that completed
    (rxPackets > 0 and lostPackets == 0).
    Only incast / query flows are kept: those with small byte counts
    (between FLOW_SIZE_MIN=3000 and FLOW_SIZE_MAX=60000 bytes).
    If you want ALL flows, remove the size filter.
    """
    tree = ET.parse(xml_path)
    root = tree.getroot()

    fcts = []
    for f in root.findall('.//Flow'):
        rx  = int(f.get('rxPackets',  0))
        tx  = int(f.get('txPackets',  0))
        lost = int(f.get('lostPackets', 0))
        rx_bytes = int(f.get('rxBytes', 0))

        # Skip flows that never delivered anything
        if rx == 0 or tx == 0:
            continue

        # Keep only query/incast-sized flows (3 KB – 60 KB)
        # Remove or adjust this filter to include background flows too
        if not (3_000 <= rx_bytes <= 60_000):
            continue

        t_first_tx = float(f.get('timeFirstTxPacket', '0').replace('+', '').replace('ns', ''))
        t_last_rx  = float(f.get('timeLastRxPacket',  '0').replace('+', '').replace('ns', ''))

        fct_s = (t_last_rx - t_first_tx) / 1e9
        if fct_s > 0:
            fcts.append(fct_s)

    return fcts


def load_results(xml_dir):
    """
    Returns a dict:
        results[scheme_label][n_senders] = {'avg': float, 'p99': float, 'n': int}
    """
    results = {label: {} for label in SCHEMES}

    for label, cfg in SCHEMES.items():
        aqm = cfg['aqm']
        for n in SENDERS:
            xml_name = f"Queue_Track_{aqm}_{aqm}_{n}_Flow_Monitor.xml"
            xml_path = os.path.join(xml_dir, xml_name)

            if not os.path.exists(xml_path):
                print(f"  [WARN] Missing: {xml_path}")
                continue

            fcts = parse_fct(xml_path)
            if len(fcts) == 0:
                print(f"  [WARN] No qualifying flows in {xml_name}")
                continue

            arr = np.array(fcts)
            results[label][n] = {
                'avg': float(np.mean(arr)),
                'p99': float(np.percentile(arr, 99)),
                'n':   len(arr),
            }
            print(f"  {label:20s}  senders={n:3d}  flows={len(arr):4d}"
                  f"  avg={np.mean(arr)*1e3:.2f}ms  p99={np.percentile(arr,99)*1e3:.2f}ms")

    return results


# ── plotting ──────────────────────────────────────────────────────────────────

def plot_fig17(results, out_path):
    fig, axes = plt.subplots(1, 2, figsize=(10, 4))

    metrics = [
        ('avg', r'(a) Query flows: AVG'),
        ('p99', r'(b) Query flows: 99th percentile'),
    ]

    for ax, (metric, title) in zip(axes, metrics):
        for label, cfg in SCHEMES.items():
            xs, ys = [], []
            for n in SENDERS:
                if n in results[label]:
                    xs.append(n)
                    ys.append(results[label][n][metric])

            if not xs:
                continue

            ax.plot(
                xs, ys,
                color=cfg['color'],
                marker=cfg['marker'],
                linestyle=cfg['ls'],
                linewidth=1.5,
                markersize=6,
                markerfacecolor='white' if cfg['color'] != 'magenta' else 'magenta',
                markeredgecolor=cfg['color'],
                markeredgewidth=1.5,
                label=label,
            )

        ax.set_xlabel('Concurrent Sender(#)', fontsize=11)
        ax.set_ylabel('FCT(s)', fontsize=11)
        ax.set_title(title, fontsize=11, fontweight='bold')
        ax.set_xticks(SENDERS)
        ax.tick_params(axis='both', labelsize=9)
        ax.yaxis.set_major_formatter(ticker.ScalarFormatter(useMathText=True))
        ax.ticklabel_format(axis='y', style='sci', scilimits=(-3, -3))
        ax.spines['top'].set_visible(False)
        ax.spines['right'].set_visible(False)
        ax.grid(axis='y', linestyle='--', linewidth=0.5, alpha=0.5)

    # Single legend on the left subplot
    axes[0].legend(fontsize=9, loc='upper left', frameon=False)

    plt.tight_layout()
    plt.savefig(out_path, dpi=150, bbox_inches='tight')
    print(f"\nSaved: {out_path}")


# ── main ──────────────────────────────────────────────────────────────────────

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--xml-dir', default='.', help='Directory containing Flow Monitor XMLs')
    parser.add_argument('--out',     default='fig17_fct.png', help='Output image path')
    args = parser.parse_args()

    print("Loading results...")
    results = load_results(args.xml_dir)
    plot_fig17(results, args.out)