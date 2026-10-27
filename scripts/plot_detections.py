#!/usr/bin/env python3
"""Plot the receiver's CSV as a radar-style polar scope plus a range-vs-id timeline.

Usage:
    python scripts/plot_detections.py [csv_path] [output_png] [--full-scope]

The CSV has one row per valid packet, in arrival order:
    id,range_m,azimuth_deg,timestamp_ms

Packet loss and reordering are recovered from the ids alone:
  * lost: ids missing between the smallest and largest id received
  * late: a packet whose id is lower than an id that already arrived
"""

import argparse
import csv
import math
import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")  # render to a file; no display needed
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D


def load(path):
    rows = []
    with open(path, newline="") as f:
        for r in csv.DictReader(f):
            rows.append({
                "id": int(r["id"]),
                "range_m": float(r["range_m"]),
                "azimuth_deg": float(r["azimuth_deg"]),
                "timestamp_ms": int(r["timestamp_ms"]),
            })
    return rows


def classify(rows):
    """Mark late packets in place (rows are in arrival order) and return lost ids."""
    max_seen = -1
    for r in rows:
        r["late"] = r["id"] < max_seen
        max_seen = max(max_seen, r["id"])
    seen = {r["id"] for r in rows}
    lost = sorted(set(range(min(seen), max(seen) + 1)) - seen)
    return lost


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("csv_path", nargs="?", default="data/detections.csv")
    parser.add_argument("output", nargs="?", default="plots/detections.png")
    parser.add_argument("--full-scope", action="store_true",
                        help="show the full 360-degree scope instead of zooming on the data")
    args = parser.parse_args()

    csv_path = Path(args.csv_path)
    if not csv_path.is_file():
        sys.exit("CSV not found: %s" % csv_path)

    rows = load(csv_path)
    if not rows:
        sys.exit("No detections in %s" % csv_path)

    lost = classify(rows)
    on_time = [r for r in rows if not r["late"]]
    late = [r for r in rows if r["late"]]
    by_id = sorted(rows, key=lambda r: r["id"])

    ranges = [r["range_m"] for r in rows]
    azimuths = [r["azimuth_deg"] for r in rows]
    r_min, r_max = min(ranges), max(ranges)
    az_min, az_max = min(azimuths), max(azimuths)

    fig = plt.figure(figsize=(14, 6.5))

    # ---- Left: polar "radar scope" (0 degrees = north, angles increase clockwise)
    ax = fig.add_subplot(1, 2, 1, projection="polar")
    ax.set_theta_zero_location("N")
    ax.set_theta_direction(-1)

    ax.plot([math.radians(r["azimuth_deg"]) for r in by_id],
            [r["range_m"] for r in by_id],
            color="0.75", linewidth=0.8, zorder=1)
    sc = ax.scatter([math.radians(r["azimuth_deg"]) for r in on_time],
                    [r["range_m"] for r in on_time],
                    c=[r["id"] for r in on_time], cmap="viridis", s=16, zorder=2)
    if late:
        ax.scatter([math.radians(r["azimuth_deg"]) for r in late],
                   [r["range_m"] for r in late],
                   marker="x", color="red", s=36, linewidths=1.5, zorder=3)

    if args.full_scope:
        ax.set_rlim(0, math.ceil(r_max / 500.0) * 500.0)
    else:
        pad_az = 3.0
        ax.set_thetalim(math.radians(max(0.0, az_min - pad_az)),
                        math.radians(min(360.0, az_max + pad_az)))
        ax.set_rlim(r_min - 30.0, r_max + 30.0)
    ax.set_title("Radar scope (range in m, azimuth in degrees)", pad=20)
    fig.colorbar(sc, ax=ax, pad=0.12, shrink=0.8, label="packet id (time)")

    # ---- Right: range vs packet id, with lost packets marked
    ax2 = fig.add_subplot(1, 2, 2)
    y_pad = 15.0
    y_lo, y_hi = r_min - y_pad, r_max + y_pad
    ax2.set_ylim(y_lo, y_hi)
    if lost:
        ax2.vlines(lost, y_lo, y_hi, colors="orange", alpha=0.35, linewidth=1.2)
    ax2.scatter([r["id"] for r in on_time], [r["range_m"] for r in on_time],
                s=14, color="tab:blue", zorder=2)
    if late:
        ax2.scatter([r["id"] for r in late], [r["range_m"] for r in late],
                    marker="x", color="red", s=36, linewidths=1.5, zorder=3)
    ax2.set_xlabel("packet id")
    ax2.set_ylabel("range (m)")
    ax2.set_title("Range over time")
    ax2.grid(alpha=0.3)
    ax2.legend(handles=[
        Line2D([0], [0], marker="o", color="w", markerfacecolor="tab:blue",
               markersize=7, label="received in order"),
        Line2D([0], [0], marker="x", color="red", linestyle="None",
               markersize=7, label="arrived late (out of order)"),
        Line2D([0], [0], color="orange", alpha=0.6, linewidth=2,
               label="lost (never arrived)"),
    ], loc="upper right")

    fig.suptitle("%s: %d received, %d lost, %d out of order"
                 % (csv_path.name, len(rows), len(lost), len(late)), fontsize=13)

    out = Path(args.output)
    out.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(out, dpi=150, bbox_inches="tight")
    print("Wrote %s (%d received, %d lost, %d out of order)"
          % (out, len(rows), len(lost), len(late)))


if __name__ == "__main__":
    main()