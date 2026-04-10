#include "hydrogen.h"
#include <cmath>
#include <vector>
#include <random>

const double a = 1.0; // bohr radius

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

double psi_squared(int n, int l, int m, double r, double theta, double phi) {
    
    // radial part

    double rho = (2.0 * r)/(n * a);
    double normalization = std::sqrt(
        std::pow(2.0/(n * a), 3) * 
        std::tgamma(n - l)/(2.0 * n * std::tgamma(n + l + 1))
    );

    double L = assoc_laguerre(n - l - 1, 2 * l + 1, rho);
    double R = normalization * std::exp(-rho/2) * std::pow(rho, l) * L;

    // angular part

    double Y = sph_legendre(l, std::abs(m), theta);

    // psi

    double psi = R * Y;

    return std::pow(std::abs(psi), 2);
}

void spherical_to_cartesian(double r, double theta, double phi, double& x, double& y, double& z) {
    x = r * std::sin(theta) * std::cos(phi);
    y = r * std::sin(theta) * std::sin(phi);
    z = r * std::cos(theta);
}

std::vector<Particle> sampler(int n, int l, int m, int n_samples) {
    double r_max = 4 * n * n * a;
    std::vector<Particle> samples;
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
        prob_max = std::max(prob_max, psi_squared(n, l, m, r, theta, phi));
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

        double prob = psi_squared(n, l, m, r, theta, phi);

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