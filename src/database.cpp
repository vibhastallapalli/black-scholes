#include "database.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

static void exec(sqlite3* db, const char* sql) {
    char* err = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
        std::string msg = err ? err : "unknown error";
        sqlite3_free(err);
        throw std::runtime_error("SQLite exec failed: " + msg);
    }
}

static std::string isoNow() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

void initSchema(sqlite3* db) {
    exec(db, R"sql(
        CREATE TABLE IF NOT EXISTS contracts (
            id            INTEGER PRIMARY KEY AUTOINCREMENT,
            asset         TEXT,
            spot_price    REAL,
            strike_price  REAL,
            time_to_expiry REAL,
            risk_free_rate REAL,
            volatility    REAL,
            option_type   TEXT
        );
    )sql");

    exec(db, R"sql(
        CREATE TABLE IF NOT EXISTS pricing_results (
            id            INTEGER PRIMARY KEY AUTOINCREMENT,
            contract_id   INTEGER,
            option_price  REAL,
            delta         REAL,
            gamma         REAL,
            theta         REAL,
            vega          REAL,
            calculated_at TEXT,
            FOREIGN KEY (contract_id) REFERENCES contracts(id)
        );
    )sql");
}

int insertContract(sqlite3* db, const std::string& asset, const OptionParams& p) {
    const char* sql =
        "INSERT INTO contracts (asset,spot_price,strike_price,time_to_expiry,"
        "risk_free_rate,volatility,option_type) VALUES (?,?,?,?,?,?,?);";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, asset.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 2, p.S);
    sqlite3_bind_double(stmt, 3, p.K);
    sqlite3_bind_double(stmt, 4, p.T);
    sqlite3_bind_double(stmt, 5, p.r);
    sqlite3_bind_double(stmt, 6, p.sigma);
    sqlite3_bind_text(stmt, 7, p.option_type.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return static_cast<int>(sqlite3_last_insert_rowid(db));
}

void insertResult(sqlite3* db, int contract_id, const OptionResult& res) {
    const char* sql =
        "INSERT INTO pricing_results (contract_id,option_price,delta,gamma,theta,vega,calculated_at)"
        " VALUES (?,?,?,?,?,?,?);";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, contract_id);
    sqlite3_bind_double(stmt, 2, res.price);
    sqlite3_bind_double(stmt, 3, res.delta);
    sqlite3_bind_double(stmt, 4, res.gamma);
    sqlite3_bind_double(stmt, 5, res.theta);
    sqlite3_bind_double(stmt, 6, res.vega);
    sqlite3_bind_text(stmt, 7, isoNow().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void exportToCSV(sqlite3* db, const std::string& filepath) {
    const char* sql = R"sql(
        SELECT c.id, c.asset, c.spot_price, c.strike_price, c.time_to_expiry,
               c.risk_free_rate, c.volatility, c.option_type,
               r.option_price, r.delta, r.gamma, r.theta, r.vega, r.calculated_at
        FROM contracts c
        JOIN pricing_results r ON r.contract_id = c.id
        ORDER BY c.id;
    )sql";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    std::ofstream f(filepath);
    f << "id,asset,spot_price,strike_price,time_to_expiry,risk_free_rate,"
         "volatility,option_type,option_price,delta,gamma,theta,vega,calculated_at\n";

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        f << sqlite3_column_int(stmt, 0)    << ","
          << sqlite3_column_text(stmt, 1)   << ","
          << sqlite3_column_double(stmt, 2) << ","
          << sqlite3_column_double(stmt, 3) << ","
          << sqlite3_column_double(stmt, 4) << ","
          << sqlite3_column_double(stmt, 5) << ","
          << sqlite3_column_double(stmt, 6) << ","
          << sqlite3_column_text(stmt, 7)   << ","
          << sqlite3_column_double(stmt, 8) << ","
          << sqlite3_column_double(stmt, 9) << ","
          << sqlite3_column_double(stmt, 10)<< ","
          << sqlite3_column_double(stmt, 11)<< ","
          << sqlite3_column_double(stmt, 12)<< ","
          << sqlite3_column_text(stmt, 13)  << "\n";
    }
    sqlite3_finalize(stmt);
}
