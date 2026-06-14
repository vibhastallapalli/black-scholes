#pragma once
#include <string>

struct OptionParams {
    double S;      // spot price
    double K;      // strike price
    double T;      // time to expiry in years
    double r;      // risk-free rate (e.g. 0.05 for 5%)
    double sigma;  // volatility (e.g. 0.2 for 20%)
    std::string option_type; // "call" or "put"
};

struct OptionResult {
    double price;
    double delta;
    double gamma;
    double theta;
    double vega;
    double rho;
};

double normalCDF(double x);
double normalPDF(double x);
OptionResult price(const OptionParams& p);

// Returns implied volatility given a market price, or -1.0 on failure.
double impliedVol(double marketPrice, OptionParams p);
