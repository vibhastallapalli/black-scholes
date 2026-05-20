"""
visualizer.py — reads data/results.csv and generates plotly charts to data/charts/
"""
import pathlib
import pandas as pd
import numpy as np
import plotly.graph_objects as go
from plotly.subplots import make_subplots

CSV_PATH   = pathlib.Path(__file__).parent.parent / "data" / "results.csv"
CHARTS_DIR = pathlib.Path(__file__).parent.parent / "data" / "charts"
CHARTS_DIR.mkdir(parents=True, exist_ok=True)


def load_data() -> pd.DataFrame:
    df = pd.read_csv(CSV_PATH)
    return df


# ── Chart 1: Option Price vs Spot Price (AAPL, calls vs puts) ─────────────────

def chart_price_vs_spot(df: pd.DataFrame) -> None:
    asset = "AAPL"
    sub = df[df["asset"] == asset].sort_values("spot_price")

    calls = sub[sub["option_type"] == "call"]
    puts  = sub[sub["option_type"] == "put"]

    fig = go.Figure()
    fig.add_trace(go.Scatter(
        x=calls["spot_price"], y=calls["option_price"],
        mode="markers+lines", name="Call",
        marker=dict(color="royalblue", size=6),
        line=dict(color="royalblue")
    ))
    fig.add_trace(go.Scatter(
        x=puts["spot_price"], y=puts["option_price"],
        mode="markers+lines", name="Put",
        marker=dict(color="crimson", size=6),
        line=dict(color="crimson")
    ))
    fig.update_layout(
        title=f"{asset} — Option Price vs Spot Price",
        xaxis_title="Spot Price ($)",
        yaxis_title="Option Price ($)",
        template="plotly_white",
        legend=dict(title="Type")
    )
    out = CHARTS_DIR / "price_vs_spot.html"
    fig.write_html(str(out))
    print(f"Saved: {out}")


# ── Chart 2: Greeks vs Spot Price (4-panel subplot, AAPL calls) ───────────────

def chart_greeks_vs_spot(df: pd.DataFrame) -> None:
    asset = "AAPL"
    sub = df[(df["asset"] == asset) & (df["option_type"] == "call")].sort_values("spot_price")

    fig = make_subplots(
        rows=2, cols=2,
        subplot_titles=("Delta", "Gamma", "Theta", "Vega")
    )
    greek_map = [
        (1, 1, "delta",  "royalblue"),
        (1, 2, "gamma",  "darkorange"),
        (2, 1, "theta",  "crimson"),
        (2, 2, "vega",   "seagreen"),
    ]
    for row, col, greek, color in greek_map:
        fig.add_trace(
            go.Scatter(
                x=sub["spot_price"], y=sub[greek],
                mode="markers+lines",
                name=greek.capitalize(),
                marker=dict(color=color, size=5),
                line=dict(color=color)
            ),
            row=row, col=col
        )
        fig.update_xaxes(title_text="Spot Price ($)", row=row, col=col)
        fig.update_yaxes(title_text=greek.capitalize(),  row=row, col=col)

    fig.update_layout(
        title=f"{asset} Calls — Greeks vs Spot Price",
        template="plotly_white",
        showlegend=False,
        height=700
    )
    out = CHARTS_DIR / "greeks_vs_spot.html"
    fig.write_html(str(out))
    print(f"Saved: {out}")


# ── Chart 3: Option Price vs Time to Expiry ───────────────────────────────────

def chart_price_vs_time(df: pd.DataFrame) -> None:
    fig = go.Figure()
    for asset in df["asset"].unique():
        for otype in ["call", "put"]:
            sub = df[(df["asset"] == asset) & (df["option_type"] == otype)].sort_values("time_to_expiry")
            if sub.empty:
                continue
            fig.add_trace(go.Scatter(
                x=sub["time_to_expiry"], y=sub["option_price"],
                mode="markers",
                name=f"{asset} {otype}",
                marker=dict(size=5, opacity=0.7)
            ))
    fig.update_layout(
        title="Option Price vs Time to Expiry",
        xaxis_title="Time to Expiry (years)",
        yaxis_title="Option Price ($)",
        template="plotly_white",
        height=550
    )
    out = CHARTS_DIR / "price_vs_time.html"
    fig.write_html(str(out))
    print(f"Saved: {out}")


# ── Chart 4: 3D Surface — Price vs (Spot Price, Volatility) ──────────────────

def chart_surface_price(df: pd.DataFrame) -> None:
    calls = df[df["option_type"] == "call"]

    spot_vals = np.linspace(calls["spot_price"].min(), calls["spot_price"].max(), 40)
    vol_vals  = np.linspace(calls["volatility"].min(),  calls["volatility"].max(), 40)

    spot_grid, vol_grid = np.meshgrid(spot_vals, vol_vals)

    from scipy.interpolate import griddata
    points = calls[["spot_price", "volatility"]].values
    values = calls["option_price"].values
    price_grid = griddata(points, values, (spot_grid, vol_grid), method="linear")

    fig = go.Figure(data=[go.Surface(
        x=spot_vals,
        y=vol_vals,
        z=price_grid,
        colorscale="Viridis",
        colorbar=dict(title="Option Price ($)")
    )])
    fig.update_layout(
        title="3D Surface: Option Price vs Spot Price & Volatility (Calls)",
        scene=dict(
            xaxis_title="Spot Price ($)",
            yaxis_title="Volatility",
            zaxis_title="Option Price ($)"
        ),
        template="plotly_white",
        height=700
    )
    out = CHARTS_DIR / "surface_price.html"
    fig.write_html(str(out))
    print(f"Saved: {out}")


def main():
    print(f"Loading {CSV_PATH} ...")
    df = load_data()
    print(f"Loaded {len(df)} rows.\n")

    chart_price_vs_spot(df)
    chart_greeks_vs_spot(df)
    chart_price_vs_time(df)

    try:
        chart_surface_price(df)
    except ImportError:
        print("scipy not installed — skipping 3D surface chart (pip install scipy)")

    print("\nAll charts saved to data/charts/")


if __name__ == "__main__":
    main()
