#include "black_scholes.h"
#include <cmath>
#include <numbers>
#include <stdexcept>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

double normalCDF(double x) {
    return 0.5 * std::erfc(-x / std::sqrt(2.0));
}

double normalPDF(double x) {
    return std::exp(-0.5 * x * x) / std::sqrt(2.0 * M_PI);
}

OptionResult price(const OptionParams& p) {
    if (p.T <= 0.0)
        throw std::invalid_argument("Time to expiry must be positive");
    if (p.sigma <= 0.0)
        throw std::invalid_argument("Volatility must be positive");
    if (p.S <= 0.0 || p.K <= 0.0)
        throw std::invalid_argument("Spot and strike must be positive");

    double sqrtT = std::sqrt(p.T);
    double d1 = (std::log(p.S / p.K) + (p.r + 0.5 * p.sigma * p.sigma) * p.T)
                / (p.sigma * sqrtT);
    double d2 = d1 - p.sigma * sqrtT;

    double Nd1  = normalCDF(d1);
    double Nd2  = normalCDF(d2);
    double Nnd1 = normalCDF(-d1);
    double Nnd2 = normalCDF(-d2);
    double npd1 = normalPDF(d1);
    double disc = std::exp(-p.r * p.T);

    OptionResult res{};

    bool isCall = (p.option_type == "call");

    res.price = isCall
        ? p.S * Nd1 - p.K * disc * Nd2
        : p.K * disc * Nnd2 - p.S * Nnd1;

    res.delta = isCall ? Nd1 : Nd1 - 1.0;

    res.gamma = npd1 / (p.S * p.sigma * sqrtT);

    double thetaCommon = -(p.S * npd1 * p.sigma) / (2.0 * sqrtT);
    res.theta = isCall
        ? (thetaCommon - p.r * p.K * disc * Nd2)  / 365.0
        : (thetaCommon + p.r * p.K * disc * Nnd2) / 365.0;

    // vega: per 1% move in vol
    res.vega = p.S * npd1 * sqrtT / 100.0;

    // rho: per 1% move in rate
    res.rho = isCall
        ?  p.K * p.T * disc * Nd2  / 100.0
        : -p.K * p.T * disc * Nnd2 / 100.0;

    return res;
}

double impliedVol(double marketPrice, OptionParams p) {
    const int    MAX_ITER = 100;
    const double TOL      = 1e-8;

    double sigma = 0.2;  // initial guess
    for (int i = 0; i < MAX_ITER; ++i) {
        p.sigma = sigma;
        OptionResult r = price(p);
        double diff  = r.price - marketPrice;
        if (std::abs(diff) < TOL)
            return sigma;
        // vega in the struct is per-1%-vol; convert back to per-unit for Newton step
        double vega = r.vega * 100.0;
        if (std::abs(vega) < 1e-12)
            return -1.0;
        sigma -= diff / vega;
        if (sigma <= 0.0)
            sigma = 1e-6;
    }
    return -1.0;
}
