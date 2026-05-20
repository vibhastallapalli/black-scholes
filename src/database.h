#pragma once
#include "black_scholes.h"
#include <sqlite3.h>
#include <string>

void initSchema(sqlite3* db);
int  insertContract(sqlite3* db, const std::string& asset, const OptionParams& p);
void insertResult(sqlite3* db, int contract_id, const OptionResult& res);
void exportToCSV(sqlite3* db, const std::string& filepath);
