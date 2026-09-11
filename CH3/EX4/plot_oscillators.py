import csv
import matplotlib.pyplot as plt

t, x1, x2, x3, x4, x5, xcm = [], [], [], [], [], [], []

with open("positions.csv") as f:
    reader = csv.DictReader(f)
    for row in reader:
        t.append(float(row["t"]))
        x1.append(float(row["x1"]))
        x2.append(float(row["x2"]))
        x3.append(float(row["x3"]))
        x4.append(float(row["x4"]))
        x5.append(float(row["x5"]))
        xcm.append(float(row["xcm"]))

plt.plot(t, x1, label="ball 1 (m)")
plt.plot(t, x2, label="ball 2 (3m)")
plt.plot(t, x3, label="ball 3 (2m)")
plt.plot(t, x4, label="ball 4 (m)")
plt.plot(t, x5, label="ball 5 (2m)")
plt.plot(t, xcm, label="center of mass", color="black", linewidth=2, linestyle="--")

plt.xlabel("t (units of sqrt(m/k))")
plt.ylabel("displacement (cm)")
plt.title("Five coupled balls on a spring chain")
plt.legend()
plt.grid(alpha=0.3)
plt.savefig("oscillators.png", dpi=150)
print("saved oscillators.png")
