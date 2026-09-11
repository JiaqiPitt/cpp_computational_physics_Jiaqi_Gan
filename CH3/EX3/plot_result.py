import csv
import matplotlib.pyplot as plt

ka, T = [], []

with open("resonance_scan.csv") as f:
    reader = csv.DictReader(f)
    for row in reader:
        ka.append(float(row["ka"]))
        T.append(float(row["T"]))

plt.plot(ka, T)
plt.xlabel("ka")
plt.ylabel("T")
plt.title("Transmission vs ka at V0/E = 0.3")
plt.grid(alpha=0.3)
plt.savefig("result.png", dpi=150)
print("saved result.png")
