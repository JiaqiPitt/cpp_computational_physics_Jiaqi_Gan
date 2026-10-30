#ifndef RAINBOW_H
#define RAINBOW_H
#include "Raindrop.h"
#include "QatDataAnalysis/Hist1D.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

struct Tables {
    static constexpr double LMIN = 400.0, LMAX = 700.0, STEP = 0.1;   // nm
    std::vector<double> n, red, green, blue;

    // linear interpolation in table v at wavelength lambda
    static double at(const std::vector<double>& v, double lambda) {
        double x = (lambda - LMIN) / STEP;
        size_t i = std::min(size_t(x), v.size() - 2);
        double f = x - double(i);
        return (1 - f) * v[i] + f * v[i + 1];
    }
};

struct Sky {
    static constexpr int W = 1200, H = 650;            // pixels
    static constexpr double DEG_PER_PIXEL = 0.08;
    static constexpr int X0 = W / 2, Y0 = H - 50;      // pixel of the antisolar point
};

// Everything collected from a batch of photons.  Two tallies can be added,
// which is how the threads of part (c) will be combined.
struct Tally {
    static constexpr int BANDS = 6;      // 400-450, 450-500, ..., 650-700 nm
    std::vector<Hist1D> wide, narrow;    // scattering angle: 0-45 deg in 0.1 deg bins, 39.5-43 deg in 0.02 deg bins
    std::vector<float> rgb;              // summed cone response in each pixel of the sky
    // the colour across the bow, all azimuths together: phi from 38 to 44 deg in 0.01 deg bins
    static constexpr double PMIN = 38.0, PMAX = 44.0, PSTEP = 0.01;
    std::vector<double> profile;         // summed cone response in each bin
    long photons = 0;
    double worst = 0;                    // largest |traced angle - (4 theta_r - 2 theta_i)| (deg)

    Tally() : rgb(3 * size_t(Sky::W) * Sky::H, 0.0f), profile(3 * size_t(std::lround((PMAX - PMIN) / PSTEP)), 0.0) {
        for (int k = 0; k < BANDS; ++k) {
            wide.emplace_back(450, 0.0, 45.0);
            narrow.emplace_back(175, 39.5, 43.0);
        }
    }
    Tally& operator+=(const Tally& o) {
        for (int k = 0; k < BANDS; ++k) {
            wide[k] += o.wide[k];
            narrow[k] += o.narrow[k];
        }
        for (size_t i = 0; i < rgb.size(); ++i) rgb[i] += o.rgb[i];
        for (size_t i = 0; i < profile.size(); ++i) profile[i] += o.profile[i];
        photons += o.photons;
        worst = std::max(worst, o.worst);
        return *this;
    }
};

// Follow nPhotons photons using random numbers from the given seed.  Only
// reads the tables, so several of these can run at the same time.
inline Tally simulate(long nPhotons, unsigned seed, const Tables& t) {
    const double DEG = 180.0 / M_PI;
    std::mt19937_64 engine(seed);
    std::uniform_real_distribution<double> flat(0.0, 1.0);
    Tally tally;
    for (long i = 0; i < nPhotons; ++i) {
        double lambda = Tables::LMIN + (Tables::LMAX - Tables::LMIN) * flat(engine);
        double n = Tables::at(t.n, lambda);

        // the sunlight is uniform over the disk the drop shows to the sun
        double b = std::sqrt(flat(engine)), alpha = 2 * M_PI * flat(engine);
        Vec3 e = trace(b * std::cos(alpha), b * std::sin(alpha), n);

        // angle between the incident beam and the light coming back
        double phi = std::acos(std::clamp(e.z, -1.0, 1.0)) * DEG;
        double formula = (4 * std::asin(b / n) - 2 * std::asin(b)) * DEG;
        tally.worst = std::max(tally.worst, std::fabs(phi - formula));

        int band = std::min(int((lambda - Tables::LMIN) / 50.0), Tally::BANDS - 1);
        tally.wide[band].accumulate(phi);
        tally.narrow[band].accumulate(phi);

        // what the eye makes of it, and where it shows in the sky (we look back along -e)
        double cr = Tables::at(t.red, lambda), cg = Tables::at(t.green, lambda), cb = Tables::at(t.blue, lambda);
        double psi = std::atan2(-e.y, -e.x);
        int px = int(std::floor(Sky::X0 + phi * std::cos(psi) / Sky::DEG_PER_PIXEL));
        int py = int(std::floor(Sky::Y0 - phi * std::sin(psi) / Sky::DEG_PER_PIXEL));
        if (px >= 0 && px < Sky::W && py >= 0 && py < Sky::H) {
            size_t k = 3 * (size_t(py) * Sky::W + px);
            tally.rgb[k] += float(cr);
            tally.rgb[k + 1] += float(cg);
            tally.rgb[k + 2] += float(cb);
        }
        if (phi >= Tally::PMIN && phi < Tally::PMAX) {
            size_t k = 3 * size_t((phi - Tally::PMIN) / Tally::PSTEP);
            tally.profile[k] += cr;
            tally.profile[k + 1] += cg;
            tally.profile[k + 2] += cb;
        }
        ++tally.photons;
    }
    return tally;
}

#endif