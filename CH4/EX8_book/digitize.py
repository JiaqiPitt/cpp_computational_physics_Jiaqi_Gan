import csv
import os
import sys

import matplotlib.pyplot as plt
from PIL import Image

RECTIFIED = "fig_4_6_rectified.png"
LAM0, DLAM = 365.0, 0.25
A1, DA = 120.0, 0.1

CURVES = {
    "blue":  ("blue cones, peak 420 nm, open circles", "tab:blue"),
    "rod":   ("rods, peak 498 nm, filled dots", "black"),
    "green": ("green cones, peak 534 nm, open circles", "tab:green"),
    "red":   ("red cones, peak 564 nm, open triangles", "tab:red"),
}


def to_data(col, row):
    return LAM0 + DLAM * col, A1 - DA * row


def to_pixel(lam, a):
    return (lam - LAM0) / DLAM, (A1 - a) / DA


def load_curve(name):
    path = f"curve_{name}.csv"
    if not os.path.exists(path):
        return []
    with open(path) as f:
        return [(float(r["lambda_nm"]), float(r["absorbance"])) for r in csv.DictReader(f)]


if len(sys.argv) != 2 or sys.argv[1] not in CURVES:
    print("usage: python3 digitize.py <" + "|".join(CURVES) + ">")
    sys.exit(1)

name = sys.argv[1]
label, color = CURVES[name]

img = Image.open(RECTIFIED)
fig, ax = plt.subplots(figsize=(11, 11))
ax.imshow(img, cmap="gray")
for other, (_, other_color) in CURVES.items():
    if other == name:
        continue
    pts = load_curve(other)
    if pts:
        cols, rows = zip(*(to_pixel(l, a) for l, a in pts))
        ax.plot(cols, rows, "x", color=other_color, ms=10, mew=2)
ax.set_title(f"{label}\nleft click = add point, right click = undo, Enter = finish")
ax.axis("off")
fig.tight_layout()

clicks = plt.ginput(n=-1, timeout=0, mouse_add=1, mouse_pop=3, mouse_stop=None)
plt.close(fig)

if not clicks:
    print("no points clicked, nothing saved")
    sys.exit(0)

points = sorted(to_data(c, r) for c, r in clicks)
with open(f"curve_{name}.csv", "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["lambda_nm", "absorbance"])
    for lam, a in points:
        w.writerow([f"{lam:.2f}", f"{a:.2f}"])

print(f"{len(points)} points saved to curve_{name}.csv")
print(f"{'lambda (nm)':>12} {'absorbance':>11} {'offset from nearest 10 nm':>27}")
for lam, a in points:
    print(f"{lam:12.2f} {a:11.2f} {lam - 10 * round(lam / 10):+27.2f}")
