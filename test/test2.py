from scipy.special import sph_harm_y, genlaguerre, factorial
import numpy as np
import matplotlib.pyplot as plt

# bohr radius

a = 1

# wavefunction

def psi_squared(n, m, l, r, theta, phi):
    
    # radial part

    rho = (2 * r)/(n * a)
    normalization = np.sqrt((2/(n * a))**3 * factorial(n - l - 1)/(2 * n * factorial(n + l)))
    L = genlaguerre(n - l - 1, 2 * l + 1)(rho)
    R = normalization * np.exp(-rho/2) * rho**l * L

    # angular part

    Y = sph_harm_y(l, m, theta, phi)

    # psi

    psi = R * Y
    
    return np.abs(psi)**2

def spherical_to_cartesian(r, theta, phi):
    return r * np.sin(theta) * np.cos(phi), r * np.sin(theta) * np.sin(phi), r * np.cos(theta)

def sampler(n, l, m, n_samples=10000):

    r_max = 4 * n**2 * a # tolerance is approx. 3x value of expectation value of r
    samples = []

    while sum(len(s) for s in samples) < n_samples:

        # sample r via inverse CDF:
        # we use a distribution proportional to r^2 due to Jacobian of 
        # the transform from Cartesian to spherical coordinates

        r = r_max * np.cbrt(np.random.uniform(0, 1, n_samples))

        # sample theta via inverse CDF:
        # distribution is proportional to sin(theta) due to Jacobian,
        # multiply inside by 2 to stretch range of arccos from [0, 1] -> [-1, 1]

        theta = np.arccos(1 - 2 * np.random.uniform(0, 1, n_samples))

        # sample phi uniformly:
        # distribution is uniform, multiply by 2 pi to cover [0, 2pi] range

        phi = 2 * np.pi * np.random.uniform(0, 1, n_samples)

        # calculate probability

        prob = psi_squared(n, m, l, r, theta, phi)

        # rejection sampling

        prob_max = prob.max() # maximum (K)
        U = np.random.uniform(0, prob_max, n_samples) # uniform random value (U)

        # accept values above the threshold

        mask = U < prob # mask to compare arrays element wise

        r_accepted, theta_accepted, phi_accepted = r[mask], theta[mask], phi[mask]
        x, y, z = spherical_to_cartesian(r_accepted, theta_accepted, phi_accepted )
        samples.append(np.column_stack([x, y, z, prob[mask]]))

    return np.vstack(samples)[:n_samples]

# plotting

n, l, m = 4, 2, 1
points = sampler(n, l, m, n_samples=25000) # sample points
x, y, z = points[:, 0], points[:, 1], points[:, 2] # convert to Cartesian
density = points[:, 3] # density for visualization

fig = plt.figure(figsize=(8, 8), facecolor='black')
ax = fig.add_subplot(111, projection = '3d', facecolor='black')
ax.scatter(x, y, z, s=0.5, alpha=0.3, c=density, cmap='hot')
ax.set_axis_off()
plt.show()