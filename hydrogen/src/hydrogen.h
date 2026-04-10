#pragma once
#include <vector>

struct Particle {
    float x;
    float y;
    float z;
    float density;
};

std::vector<Particle> sampler(int n, int l, int m, int n_samples = 25000);
double psi_squared(int n, int l, int m, double r, double theta, double phi);
void spherical_to_cartesian(double r, double theta, double phi, double& x, double& y, double& z);

double assoc_laguerre(int n, int m, double x);
double assoc_legendre(int l, int m, double theta);
double sph_legendre(int l, int m, double theta);