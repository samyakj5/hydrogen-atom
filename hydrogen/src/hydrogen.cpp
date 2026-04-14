#include "hydrogen.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <random>
#include <vector>

const double a = 1.0; // bohr radius

namespace {

int magnetic_order(const OrbitalState& state) {
    return std::abs(state.m);
}

double radial_part(const OrbitalState& state, double r) {
    double rho = (2.0 * r) / (state.n * a);
    double normalization = std::sqrt(
        std::pow(2.0 / (state.n * a), 3) *
        std::tgamma(state.n - state.l) / (2.0 * state.n * std::tgamma(state.n + state.l + 1))
    );

    double laguerre = assoc_laguerre(state.n - state.l - 1, 2 * state.l + 1, rho);
    return normalization * std::exp(-rho / 2.0) * std::pow(rho, state.l) * laguerre;
}

std::complex<double> complex_harmonic(int l, int m, double theta, double phi) {
    int abs_m = std::abs(m);
    double angular = sph_legendre(l, abs_m, theta);
    std::complex<double> positive_m = angular * std::polar(1.0, static_cast<double>(abs_m) * phi);
    if (m < 0) {
        double phase = (abs_m % 2 == 0) ? 1.0 : -1.0;
        return phase * std::conj(positive_m);
    }
    return positive_m;
}

double real_harmonic(int l, int m, RealComponent component, double theta, double phi) {
    double angular = sph_legendre(l, m, theta);
    if (m == 0) {
        return angular;
    }

    double azimuthal = 0.0;
    if (component == RealComponent::Cosine) {
        azimuthal = std::cos(static_cast<double>(m) * phi);
    } else if (component == RealComponent::Sine) {
        azimuthal = std::sin(static_cast<double>(m) * phi);
    }
    return std::sqrt(2.0) * angular * azimuthal;
}

}

bool is_valid_orbital_state(const OrbitalState& state) {
    if (state.n < 1) {
        return false;
    }
    if (state.l < 0 || state.l >= state.n) {
        return false;
    }
    if (state.basis == OrbitalBasis::Real) {
        if (state.m < 0 || state.m > state.l) {
            return false;
        }
        if (state.m == 0) {
            return state.component == RealComponent::None;
        }
        return state.component == RealComponent::Cosine || state.component == RealComponent::Sine;
    }
    return magnetic_order(state) <= state.l;
}

double assoc_laguerre(int n, int m, double x) {
    if (n == 0) {
        return 1.0;
    }
    if (n == 1) {
        return 1.0 + m - x;
    }
    double L0 = 1.0;
    double L1 = 1.0 + m - x;
    double Lk = 0.0;
    for (int k = 2; k <= n; k++) {
        Lk = ((2 * k - 1 + m - x) * L1 - (k - 1 + m) * L0)/k;
        L0 = L1;
        L1 = Lk;
    }
    return Lk;
}

double assoc_legendre(int l, int m, double theta) {
    double x = std::cos(theta);
    double Pmm = 1.0;
    Pmm = Pmm * std::pow(-1, m) * std::pow(1.0 - std::pow(x, 2), m/2.0);
    for (double i = 1.0; i <= m; i++) {
        Pmm *= (2 * i - 1);
    }
    if (l == m) {
        return Pmm;
    }
    double Pmm1 = x * (2 * m + 1) * Pmm;
    if (l == m + 1) {
        return Pmm1;
    }
    double Pk = 0.0;
    for (int i = m + 2; i <= l; i++) {
        Pk = ((2 * i - 1) * x * Pmm1 - (i + m - 1) * Pmm)/(i - m);
        Pmm = Pmm1;
        Pmm1 = Pk;
    }
    return Pk;
}

double sph_legendre(int l, int m, double theta) {
    double norm = std::sqrt(
        (2.0 * l + 1) / (4.0 * M_PI) *
        std::tgamma(l - m + 1) /
        std::tgamma(l + m + 1)
    );
    return norm * assoc_legendre(l, m, theta);
}

double psi_squared(const OrbitalState& state, double r, double theta, double phi) {
    if (!is_valid_orbital_state(state)) {
        return 0.0;
    }

    double radial = radial_part(state, r);
    if (state.basis == OrbitalBasis::Complex) {
        std::complex<double> angular = complex_harmonic(state.l, state.m, theta, phi);
        return std::norm(radial * angular);
    }

    double angular = real_harmonic(state.l, state.m, state.component, theta, phi);
    double psi = radial * angular;
    return psi * psi;
}

void spherical_to_cartesian(double r, double theta, double phi, double& x, double& y, double& z) {
    x = r * std::sin(theta) * std::cos(phi);
    y = r * std::sin(theta) * std::sin(phi);
    z = r * std::cos(theta);
}

std::vector<Particle> sampler(const OrbitalState& state, int n_samples) {
    std::vector<Particle> samples;
    if (!is_valid_orbital_state(state) || n_samples <= 0) {
        return samples;
    }

    double r_max = 4 * state.n * state.n * a;
    samples.reserve(n_samples);

    // setup for random sampling

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> uniform(0.0, 1.0);

    // sample for max probability

    double prob_max = 0.0;
    for (int i = 0; i < 10000; i++) {
        double r     = r_max * std::cbrt(uniform(gen));
        double theta = std::acos(1.0 - 2.0 * uniform(gen));
        double phi   = 2.0 * M_PI * uniform(gen);
        prob_max = std::max(prob_max, psi_squared(state, r, theta, phi));
    }
    prob_max *= 1.1;

    while (static_cast<int>(samples.size()) < n_samples) {

        // sample r via inverse CDF

        double r = r_max * std::cbrt(uniform(gen));

        // sample theta via inverse CDF

        double theta = std::acos(1.0 - 2.0 * uniform(gen));

        // sample theta uniformly

        double phi = 2 * M_PI * uniform(gen);

        // calculate probability

        double prob = psi_squared(state, r, theta, phi);

        // rejection sampling

        double U = uniform(gen) * prob_max;
        if (U < prob) {
            double x, y, z;
            spherical_to_cartesian(r, theta, phi, x, y, z);
            samples.push_back({static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), static_cast<float>(prob / prob_max)});
        }
    }
    return samples;
}
