import sympy as sp
import numpy as np
import matplotlib.pyplot as plt

a = 1
rho = sp.symbols('rho', real=True, positive=True)

def laguerre(x, q):
    return sp.exp(x)/sp.factorial(q) * sp.diff(sp.exp(-x) * x**q, x, q)

def associated_laguerre(x, p, q):
    return (-1)**p * sp.diff(laguerre(x, p + q), x, p)

def legendre(x, l):
    return 1/(2**l * sp.factorial(l)) * sp.diff(((x**2 - 1)**l), x, l)

def associated_legendre(x, m, l):
    return (-1)**m * ((1 - x**2)**(m/2)) * sp.diff(legendre(x, l), x, m)

def spherical_harmonics(theta, phi, m, l):
    x = sp.symbols('x', real=True)
    P = associated_legendre(x, m, l).subs(x, sp.cos(theta))
    return sp.sqrt((2*l + 1)/(4*sp.pi) * sp.factorial(l - m)/sp.factorial(l + m)) * sp.exp(1j * m * phi) * P

def radial(n, l, r):
    L = associated_laguerre(rho, 2*l + 1, n - l - 1)
    L = L.subs(rho, 2*r/(n*a))   # now safe

    norm = sp.sqrt(((2/(n*a))**3) * sp.factorial(n - l - 1) / (2*n * sp.factorial(n + l)))
    return norm * sp.exp(-r/(n*a)) * (2*r/(n*a))**l * L

def wavefunction(n, l, r, m, theta, phi):
    return radial(n, l, r) * spherical_harmonics(theta, phi, m, l)

n, l, m = 3, 1, 1
r, theta, phi = sp.symbols('r theta phi', real=True)

psi = wavefunction(n, l, r, m, theta, phi)
psi_num = sp.lambdify((r, theta, phi), sp.Abs(psi)**2, 'numpy')

r_vals = np.linspace(0, 5, 100)
theta_vals = np.linspace(0, np.pi, 100)
phi_vals = np.linspace(0, 2 * np.pi, 100)

R, Theta, Phi = np.meshgrid(r_vals, theta_vals, phi_vals)

psi_sq = psi_num(R, Theta, Phi)

X = R * np.sin(Theta) * np.cos(Phi)
Y = R * np.sin(Theta) * np.sin(Phi)
Z = R * np.cos(Theta)

indices = (slice(None, None, 4), slice(None, None, 4), slice(None, None, 4))


fig = plt.figure(figsize=(8, 8))
ax = fig.add_subplot(111, projection='3d')

ax.scatter(X[indices], Y[indices], Z[indices], c=psi_sq[indices], cmap='viridis', s=1, alpha=0.6)
ax.set_xlabel('X')
ax.set_ylabel('Y')
ax.set_zlabel('Z')
plt.show()