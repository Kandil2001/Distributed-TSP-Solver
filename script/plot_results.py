#!/usr/bin/env python3
import argparse
import pathlib
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt


def read_solution(path: pathlib.Path) -> float | None:
    if not path.exists():
        return None
    text = path.read_text(encoding="utf-8").strip()
    if not text:
        return None
    try:
        return float(text.splitlines()[0].strip())
    except ValueError:
        return None


def read_route(path: pathlib.Path) -> list[int]:
    route: list[int] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if not stripped:
            continue
        route.append(int(stripped))
    return route


def read_coords(tsplib_path: pathlib.Path) -> list[tuple[float, float]]:
    coords: list[tuple[float, float]] = []
    in_section = False
    for line in tsplib_path.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if not stripped:
            continue
        if stripped.startswith("NODE_COORD_SECTION"):
            in_section = True
            continue
        if stripped.startswith("EOF"):
            break
        if in_section:
            parts = stripped.split()
            if len(parts) >= 3:
                coords.append((float(parts[1]), float(parts[2])))
    return coords


def close_route(route: list[int]) -> list[int]:
    if not route:
        return route
    if route[0] == route[-1]:
        return route
    return route + [route[0]]


def main() -> int:
    parser = argparse.ArgumentParser(description="Plot TSP route output using matplotlib.")
    parser.add_argument("--solution", default="solution.txt", help="Path to solution.txt")
    parser.add_argument("--route", default="my_route.txt", help="Path to my_route.txt")
    parser.add_argument("--tsp", default="data/berlin52.tsp", help="Optional TSPLIB file")
    parser.add_argument("--output", default="figures/route_comparison_python.png", help="Output image path")
    args = parser.parse_args()

    route_path = pathlib.Path(args.route)
    if not route_path.exists():
        print(f"Error: route file not found: {route_path}", file=sys.stderr)
        return 1

    tsp_path = pathlib.Path(args.tsp)
    if not tsp_path.exists():
        print(f"Error: TSPLIB file not found: {tsp_path}", file=sys.stderr)
        return 1

    route = read_route(route_path)
    coords = read_coords(tsp_path)

    if not route:
        print("Error: route file is empty.", file=sys.stderr)
        return 1
    if not coords:
        print("Error: could not parse coordinates from TSPLIB file.", file=sys.stderr)
        return 1

    if max(route) >= len(coords) or min(route) < 0:
        print("Error: route indices are out of bounds for TSPLIB coordinates.", file=sys.stderr)
        return 1

    route_closed = close_route(route)
    ref_route = list(range(len(coords)))
    ref_closed = close_route(ref_route)

    x_ref = [coords[i][0] for i in ref_closed]
    y_ref = [coords[i][1] for i in ref_closed]
    x_sol = [coords[i][0] for i in route_closed]
    y_sol = [coords[i][1] for i in route_closed]

    best_dist = read_solution(pathlib.Path(args.solution))

    fig, axes = plt.subplots(1, 2, figsize=(12, 5), constrained_layout=True)

    axes[0].plot(x_ref, y_ref, "o-", linewidth=1.0, markersize=3)
    axes[0].set_title("Reference order route")
    axes[0].set_aspect("equal", adjustable="box")

    axes[1].plot(x_sol, y_sol, "o-", linewidth=1.0, markersize=3, color="tab:orange")
    if best_dist is None:
        axes[1].set_title("Optimized route")
    else:
        axes[1].set_title(f"Optimized route (distance={best_dist:.2f})")
    axes[1].set_aspect("equal", adjustable="box")

    for ax in axes:
        ax.set_xlabel("x")
        ax.set_ylabel("y")

    out_path = pathlib.Path(args.output)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(out_path, dpi=160)
    print(f"Saved plot: {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
