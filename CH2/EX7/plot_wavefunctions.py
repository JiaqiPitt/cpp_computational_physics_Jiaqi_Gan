"""
Plot the wavefunction data produced by barrier.cpp.

Run this after `make && ./barrier` (that's what generates the two CSV files).
"""

import csv
import matplotlib.pyplot as plt


def load_csv(filename):
    x, re, im, psi2 = [], [], [], []
    with open(filename) as f:
        reader = csv.DictReader(f)
        for row in reader:
            x.append(float(row["x"]))
            re.append(float(row["re_psi"]))
            im.append(float(row["im_psi"]))
            psi2.append(float(row["psi2"]))
    return x, re, im, psi2


def plot_case(ax, filename, title, E, V):
    x, re, im, psi2 = load_csv(filename)
    ax.plot(x, re, label="Re(psi)")
    ax.plot(x, im, label="Im(psi)")
    ax.plot(x, psi2, label="|psi|^2", linewidth=2, color="black")

    x_step = [x[0], 0, 0, x[-1]]
    v_step = [0, 0, V, V]
    ax.plot(x_step, v_step, label="V(x)", color="0.4", linestyle=":", linewidth=2)
    ax.axhline(E, color="0.4", linestyle="--", linewidth=1, label="E")

    ax.set_title(title)
    ax.set_xlabel("x")
    ax.legend(loc="upper right")
    ax.grid(alpha=0.3)


fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

plot_case(ax1, "wavefunction_above.csv", "V/E = 0.5 (E > V, partial transmission)", 1.0, 0.5)
plot_case(ax2, "wavefunction_below.csv", "V/E = 2.0 (E < V, tunneling)", 1.0, 2.0)

fig.suptitle("Step potential scattering: wavefunction vs x")
fig.tight_layout()
fig.savefig("wavefunctions.png", dpi=150)
print("saved wavefunctions.png")
