import csv
import matplotlib.pyplot as plt

i_vals = []
modes = [[] for _ in range(5)]

with open("eigenmodes.csv") as f:
    reader = csv.DictReader(f)
    for row in reader:
        i_vals.append(int(row["i"]))
        for j in range(5):
            modes[j].append(float(row[f"mode{j+1}"]))

fig, axes = plt.subplots(5, 1, figsize=(8, 12), sharex=True)

for j in range(5):
    axes[j].plot(i_vals, modes[j])
    axes[j].set_ylabel(f"mode {j+1}")
    axes[j].grid(alpha=0.3)

axes[-1].set_xlabel("bead index i")
fig.suptitle("First five eigenmodes of the 100-bead fixed-end chain")
fig.tight_layout()
fig.savefig("eigenmodes.png", dpi=150)
print("saved eigenmodes.png")
