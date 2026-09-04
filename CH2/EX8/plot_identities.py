"""
Note: 

Plot sin(ix) vs i*sinh(x) and cos(ix) vs cosh(x) to show the identities hold.

Run after `make && ./identities`.
"""

import csv
import matplotlib.pyplot as plt

x, sinh_x, im_sin_ix, cosh_x, re_cos_ix = [], [], [], [], []

with open("identities_data.csv") as f:
    reader = csv.DictReader(f)
    for row in reader:
        x.append(float(row["x"]))
        sinh_x.append(float(row["sinh_x"]))
        im_sin_ix.append(float(row["im_sin_ix"]))
        cosh_x.append(float(row["cosh_x"]))
        re_cos_ix.append(float(row["re_cos_ix"]))

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

ax1.plot(x, sinh_x, label="sinh(x)", linewidth=3, color="lightgray")
ax1.plot(x, im_sin_ix, label="Im(sin(ix))", linewidth=1, linestyle="--", color="C0")
ax1.set_title("sin(ix) = i*sinh(x)")
ax1.set_xlabel("x")
ax1.legend()
ax1.grid(alpha=0.3)

ax2.plot(x, cosh_x, label="cosh(x)", linewidth=3, color="lightgray")
ax2.plot(x, re_cos_ix, label="Re(cos(ix))", linewidth=1, linestyle="--", color="C1")
ax2.set_title("cos(ix) = cosh(x)")
ax2.set_xlabel("x")
ax2.legend()
ax2.grid(alpha=0.3)

fig.suptitle("Trig / hyperbolic identities checked with std::complex")
fig.tight_layout()
fig.savefig("identities.png", dpi=150)
print("saved identities.png")
