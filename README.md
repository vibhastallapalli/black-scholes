# Black-Scholes Options Pricing Model

A two-layer application combining a high-performance C++ pricing engine with a
Python visualization and interactive UI layer.

---

## What is Black-Scholes?

The Black-Scholes model (1973) is the cornerstone of modern quantitative finance.
It provides a closed-form solution for pricing European options — contracts that
give the holder the right (but not the obligation) to buy or sell an asset at a
fixed **strike price** on a fixed **expiry date**.

The model assumes:
- The underlying asset follows geometric Brownian motion
- Constant volatility and risk-free rate over the option's life
- No dividends, no transaction costs
- Continuous trading is possible

Despite its simplifying assumptions, it remains the standard pricing benchmark
used by options desks worldwide.

---

## Formula Derivation

### Key variables

| Symbol | Meaning |
|--------|---------|
| S      | Current spot price of the underlying |
| K      | Strike price |
| T      | Time to expiry (in years) |
| r      | Risk-free interest rate (e.g. 0.05 = 5%) |
| σ      | Volatility of the underlying (e.g. 0.2 = 20%) |
| N(·)   | Cumulative standard normal distribution |
| N'(·)  | Standard normal PDF |

### d1 and d2

```
d1 = [ ln(S/K) + (r + σ²/2)·T ] / [ σ·√T ]
d2 = d1 - σ·√T
```

`d1` encodes how far in-the-money the option is, adjusted for drift and
diffusion.  `d2` is the risk-neutral probability that the option expires
in-the-money.

### Option prices

```
Call price = S · N(d1)  −  K · e^(−rT) · N(d2)
Put  price = K · e^(−rT) · N(−d2)  −  S · N(−d1)
```

Put-call parity: `C − P = S − K·e^(−rT)`

### Greeks

The Greeks measure sensitivity of the option price to each input parameter.

**Delta (Δ)** — sensitivity to spot price movement

```
Δ_call = N(d1)
Δ_put  = N(d1) − 1
```

A call delta of 0.60 means the option gains ~$0.60 for every $1 rise in S.

**Gamma (Γ)** — rate of change of Delta (second-order spot sensitivity)

```
Γ = N'(d1) / [ S · σ · √T ]
```

High gamma near expiry means delta can change rapidly — critical for hedgers.

**Theta (Θ)** — daily time decay

```
Θ_call = [ −S·N'(d1)·σ / (2·√T) ] − [ r·K·e^(−rT)·N(d2)  ]   ÷ 365
Θ_put  = [ −S·N'(d1)·σ / (2·√T) ] + [ r·K·e^(−rT)·N(−d2) ]   ÷ 365
```

Theta is almost always negative — options lose value as expiry approaches.

**Vega (ν)** — sensitivity to a 1% change in volatility

```
ν = S · N'(d1) · √T / 100
```

Vega is identical for calls and puts with the same parameters.

---

## Directory Structure

```
black-scholes/
├── src/
│   ├── black_scholes.h/cpp   — pricing engine (Black-Scholes formulas + Greeks)
│   ├── database.h/cpp        — SQLite persistence + CSV export
│   └── main.cpp              — batch runner + --single UI mode
├── python/
│   ├── visualizer.py         — 4 plotly charts from results.csv
│   ├── ui.py                 — interactive terminal pricer
│   └── requirements.txt
├── data/
│   ├── options.db            — SQLite database (auto-created)
│   ├── results.csv           — exported batch results
│   └── charts/               — generated HTML charts
├── CMakeLists.txt
└── README.md
```

---

## Build Instructions

### Prerequisites

- CMake ≥ 3.15
- A C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+)
- SQLite3 development headers
  - Ubuntu/Debian: `sudo apt install libsqlite3-dev`
  - macOS (Homebrew): `brew install sqlite3`
  - Windows (vcpkg): `vcpkg install sqlite3`
- Python 3.9+

### C++ build

```bash
# from the black-scholes/ root
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

The compiled binary will be `build/black_scholes` (or `black_scholes.exe` on Windows).

### Python setup

```bash
cd python
pip install -r requirements.txt
# optional (for 3D surface chart):
pip install scipy
```

---

## How to Run

### Batch mode (generates 120 contracts)

Run the binary from the `black-scholes/` root so relative paths resolve:

```bash
./build/black_scholes        # Linux / macOS
build\Release\black_scholes.exe   # Windows
```

Output:
- Prints a summary table to stdout (asset, type, S, K, T, price, greeks)
- Writes `data/options.db` (SQLite)
- Writes `data/results.csv`

### Generate visualisation charts

```bash
python python/visualizer.py
```

Opens four interactive HTML charts in `data/charts/`:
1. `price_vs_spot.html`     — call/put price curves by spot
2. `greeks_vs_spot.html`    — 4-panel delta/gamma/theta/vega subplots
3. `price_vs_time.html`     — price vs time-to-expiry scatter
4. `surface_price.html`     — 3D surface: price vs spot × volatility

### Interactive single-contract UI

```bash
python python/ui.py
```

Prompts for asset, spot, strike, T, r, σ, and option type.  Internally writes
`data/ui_input.csv`, calls `black_scholes --single data/ui_input.csv`, reads
`data/ui_output.csv`, and renders a formatted result table.

### Single-contract flag (direct binary)

```bash
./build/black_scholes --single data/ui_input.csv
```

---

## Sample Output

```
Asset Type  S       K       T     Price     Delta     Gamma     Theta      Vega
-------------------------------------------------------------------------------
AAPL  call  175.32  168.41  0.93  22.1563   0.6234    0.009821  -0.0412   0.4821
TSLA  put   241.18  258.73  1.45  38.9201  -0.5812    0.006234  -0.0287   0.7103
SPY   call  447.65  440.00  0.50  28.4411   0.6891    0.007432  -0.0651   0.3912
...
Done. Stored 120 contracts.
Database : data/options.db
CSV      : data/results.csv
```

---

## What I Learned

### Numerical methods
Implementing `normalCDF` using `std::erfc` (the complementary error function)
rather than a polynomial approximation taught me that standard library numerical
functions are often more accurate and portable than hand-rolled series expansions.

### Derivatives pricing intuition
Working through the full Greeks formulas made the sensitivities concrete:
- **Delta** is not constant — it changes with every tick (hence gamma hedging)
- **Theta decay** accelerates as expiry approaches (the curve is convex)
- **Vega** is symmetric for calls and puts — a useful sanity check
- **Gamma and vega** peak at-the-money, exactly where pricing models are most
  sensitive to input assumptions

### C++/SQLite integration
Writing a thin SQLite wrapper from first principles (no ORM) reinforced how
prepared statements prevent SQL injection and how `BEGIN TRANSACTION` / `COMMIT`
can give 50–100× write throughput for bulk inserts.

### Two-layer architecture
The C++ → CSV → Python boundary kept the fast number-crunching in compiled code
while handing off visualisation to Python's rich ecosystem — a practical
pattern for quantitative tools.
