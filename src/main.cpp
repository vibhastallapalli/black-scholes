#include "black_scholes.h"
#include "database.h"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

// ── helpers ──────────────────────────────────────────────────────────────────

static sqlite3* openDB(const std::string& path) {
    sqlite3* db = nullptr;
    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << "\n";
        sqlite3_close(db);
        std::exit(1);
    }
    return db;
}

// ── --single mode ─────────────────────────────────────────────────────────────
// Reads data/ui_input.csv, prices the contract, writes data/ui_output.csv

static void runSingle(const std::string& inputCSV) {
    std::ifstream fin(inputCSV);
    if (!fin) {
        std::cerr << "Cannot open " << inputCSV << "\n";
        std::exit(1);
    }

    std::string header, line;
    std::getline(fin, header);
    std::getline(fin, line);
    fin.close();

    // expected columns: asset,spot_price,strike_price,time_to_expiry,risk_free_rate,volatility,option_type
    std::istringstream ss(line);
    std::string asset, opt_type, tok;
    OptionParams p{};

    std::getline(ss, asset, ',');
    std::getline(ss, tok, ','); p.S     = std::stod(tok);
    std::getline(ss, tok, ','); p.K     = std::stod(tok);
    std::getline(ss, tok, ','); p.T     = std::stod(tok);
    std::getline(ss, tok, ','); p.r     = std::stod(tok);
    std::getline(ss, tok, ','); p.sigma = std::stod(tok);
    std::getline(ss, opt_type, ',');
    p.option_type = opt_type;

    OptionResult res = price(p);

    std::ofstream fout("data/ui_output.csv");
    fout << "option_price,delta,gamma,theta,vega\n";
    fout << std::fixed << std::setprecision(6)
         << res.price << "," << res.delta << "," << res.gamma
         << "," << res.theta << "," << res.vega << "\n";

    std::cout << "\n=== Black-Scholes Result ===\n"
              << "Asset:   " << asset     << "\n"
              << "Type:    " << p.option_type << "\n"
              << "S=" << p.S << "  K=" << p.K
              << "  T=" << p.T << "  r=" << p.r
              << "  σ=" << p.sigma << "\n\n"
              << std::fixed << std::setprecision(6)
              << "Price:  " << res.price << "\n"
              << "Delta:  " << res.delta << "\n"
              << "Gamma:  " << res.gamma << "\n"
              << "Theta:  " << res.theta << "\n"
              << "Vega:   " << res.vega  << "\n";
}

// ── batch mode ────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    if (argc >= 3 && std::string(argv[1]) == "--single") {
        runSingle(argv[2]);
        return 0;
    }

    // 1. Open / create DB
    sqlite3* db = openDB("data/options.db");
    initSchema(db);

    // 2. Seeded RNG for reproducibility
    std::mt19937 rng(42);

    std::vector<std::string> assets = {"AAPL", "TSLA", "SPY", "NVDA", "AMZN"};
    // approximate base prices per asset
    std::vector<double> bases       = {175.0,  250.0, 450.0, 500.0,  180.0};

    std::uniform_real_distribution<double> kFactor(0.85, 1.15);
    std::uniform_real_distribution<double> tDist(0.1, 2.0);
    std::uniform_real_distribution<double> rDist(0.03, 0.06);
    std::uniform_real_distribution<double> sigDist(0.15, 0.60);
    std::uniform_real_distribution<double> sFactor(0.8, 1.2);

    constexpr int N = 120;

    // print header
    std::cout << std::left
              << std::setw(6)  << "Asset"
              << std::setw(5)  << "Type"
              << std::setw(8)  << "S"
              << std::setw(8)  << "K"
              << std::setw(6)  << "T"
              << std::setw(9)  << "Price"
              << std::setw(9)  << "Delta"
              << std::setw(9)  << "Gamma"
              << std::setw(10) << "Theta"
              << std::setw(9)  << "Vega"
              << "\n";
    std::cout << std::string(79, '-') << "\n";

    // begin transaction for speed
    char* errMsg = nullptr;
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg);

    for (int i = 0; i < N; ++i) {
        int ai = i % 5;
        std::string asset = assets[ai];
        double baseS = bases[ai];

        OptionParams p{};
        p.S          = baseS * sFactor(rng);
        p.K          = p.S   * kFactor(rng);
        p.T          = tDist(rng);
        p.r          = rDist(rng);
        p.sigma      = sigDist(rng);
        p.option_type = (i % 2 == 0) ? "call" : "put";

        OptionResult res = price(p);

        int cid = insertContract(db, asset, p);
        insertResult(db, cid, res);

        std::cout << std::left  << std::fixed << std::setprecision(2)
                  << std::setw(6)  << asset
                  << std::setw(5)  << p.option_type
                  << std::setw(8)  << p.S
                  << std::setw(8)  << p.K
                  << std::setw(6)  << std::setprecision(2) << p.T
                  << std::setw(9)  << std::setprecision(4) << res.price
                  << std::setw(9)  << res.delta
                  << std::setw(9)  << std::setprecision(6) << res.gamma
                  << std::setw(10) << std::setprecision(4) << res.theta
                  << std::setw(9)  << res.vega
                  << "\n";
    }

    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errMsg);

    // 3. Export CSV
    exportToCSV(db, "data/results.csv");
    sqlite3_close(db);

    std::cout << "\nDone. Stored " << N << " contracts.\n"
              << "Database : data/options.db\n"
              << "CSV      : data/results.csv\n";
    return 0;
}
