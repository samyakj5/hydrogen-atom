#pragma once
#include <vector>

struct Particle {
    float x;
    float y;
    float z;
    float density;
};

enum class OrbitalBasis {
    Complex,
    Real,
};

enum class RealComponent {
    None,
    Cosine,
    Sine,
};

struct OrbitalState {
    int n;
    int l;
    int m;
    OrbitalBasis basis = OrbitalBasis::Complex;
    RealComponent component = RealComponent::None;
};

bool is_valid_orbital_state(const OrbitalState& state);
std::vector<Particle> sampler(const OrbitalState& state, int n_samples = 25000);
double psi_squared(const OrbitalState& state, double r, double theta, double phi);
void spherical_to_cartesian(double r, double theta, double phi, double& x, double& y, double& z);

double assoc_laguerre(int n, int m, double x);
double assoc_legendre(int l, int m, double theta);
double sph_legendre(int l, int m, double theta);
