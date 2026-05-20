"""
ui.py — interactive terminal UI for pricing a single Black-Scholes contract.
Writes data/ui_input.csv, calls the compiled binary, reads data/ui_output.csv.
"""
import csv
import pathlib
import subprocess
import sys

ROOT     = pathlib.Path(__file__).parent.parent
DATA_DIR = ROOT / "data"

# Prefer the binary inside a build sub-directory, fall back to project root
BINARY_CANDIDATES = [
    ROOT / "build" / "black_scholes",
    ROOT / "build" / "black_scholes.exe",
    ROOT / "build" / "Release" / "black_scholes.exe",
    ROOT / "build" / "Debug"   / "black_scholes.exe",
    ROOT / "black_scholes",
    ROOT / "black_scholes.exe",
]


def find_binary() -> pathlib.Path:
    for p in BINARY_CANDIDATES:
        if p.exists():
            return p
    return None


def ask(prompt: str, cast=str, validate=None):
    while True:
        raw = input(prompt).strip()
        try:
            val = cast(raw)
        except (ValueError, TypeError):
            print(f"  Invalid input — expected {cast.__name__}")
            continue
        if validate and not validate(val):
            print("  Value out of valid range, try again.")
            continue
        return val


def get_inputs() -> dict:
    print("\n╔══════════════════════════════════════════╗")
    print("║   Black-Scholes Single Contract Pricer   ║")
    print("╚══════════════════════════════════════════╝\n")

    return {
        "asset":      ask("Asset name (e.g. AAPL): "),
        "spot_price": ask("Spot price S ($):        ", float, lambda v: v > 0),
        "strike_price": ask("Strike price K ($):     ", float, lambda v: v > 0),
        "time_to_expiry": ask("Time to expiry T (yrs): ", float, lambda v: v > 0),
        "risk_free_rate":  ask("Risk-free rate r (0–1): ", float, lambda v: 0 <= v < 1),
        "volatility":      ask("Volatility σ (0–1):    ", float, lambda v: 0 < v < 5),
        "option_type":     ask("Option type (call/put): ",
                               str, lambda v: v.lower() in ("call", "put")),
    }


def write_input_csv(params: dict) -> pathlib.Path:
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    path = DATA_DIR / "ui_input.csv"
    with open(path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(params.keys()))
        writer.writeheader()
        writer.writerow(params)
    return path


def call_engine(binary: pathlib.Path, input_csv: pathlib.Path) -> None:
    result = subprocess.run(
        [str(binary), "--single", str(input_csv)],
        capture_output=True,
        text=True
    )
    if result.returncode != 0:
        print("C++ engine error:\n", result.stderr)
        sys.exit(1)
    print(result.stdout)


def read_output_csv() -> dict | None:
    path = DATA_DIR / "ui_output.csv"
    if not path.exists():
        return None
    with open(path, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            return row
    return None


def display_result(params: dict, result: dict) -> None:
    try:
        from rich.table import Table
        from rich.console import Console

        console = Console()
        table = Table(title="Black-Scholes Pricing Result", show_header=True,
                      header_style="bold cyan")
        table.add_column("Field",  style="bold")
        table.add_column("Value",  justify="right")

        table.add_row("Asset",           params["asset"])
        table.add_row("Option Type",     params["option_type"].upper())
        table.add_row("Spot Price (S)",  f"${float(params['spot_price']):.2f}")
        table.add_row("Strike (K)",      f"${float(params['strike_price']):.2f}")
        table.add_row("Expiry (T)",      f"{float(params['time_to_expiry']):.3f} yrs")
        table.add_row("Rate (r)",        f"{float(params['risk_free_rate'])*100:.2f}%")
        table.add_row("Volatility (σ)",  f"{float(params['volatility'])*100:.1f}%")
        table.add_section()
        table.add_row("[bold green]Option Price",
                      f"[bold green]${float(result['option_price']):.4f}")
        table.add_row("Delta",   f"{float(result['delta']):.6f}")
        table.add_row("Gamma",   f"{float(result['gamma']):.6f}")
        table.add_row("Theta",   f"{float(result['theta']):.6f}  (per day)")
        table.add_row("Vega",    f"{float(result['vega']):.6f}  (per 1% vol)")
        console.print(table)

    except ImportError:
        # Fallback plain print
        print("\n--- Result ---")
        print(f"  Option Price : ${float(result['option_price']):.4f}")
        print(f"  Delta        : {float(result['delta']):.6f}")
        print(f"  Gamma        : {float(result['gamma']):.6f}")
        print(f"  Theta        : {float(result['theta']):.6f}  (per day)")
        print(f"  Vega         : {float(result['vega']):.6f}  (per 1% vol)")


def main():
    binary = find_binary()
    if binary is None:
        print("ERROR: Could not find compiled binary (black_scholes / black_scholes.exe).")
        print("Build the project first:\n  cd build && cmake .. && cmake --build .")
        sys.exit(1)

    params = get_inputs()
    input_csv = write_input_csv(params)
    call_engine(binary, input_csv)
    result = read_output_csv()
    if result:
        display_result(params, result)


if __name__ == "__main__":
    main()
