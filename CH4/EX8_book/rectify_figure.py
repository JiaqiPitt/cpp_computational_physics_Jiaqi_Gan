import numpy as np
from PIL import Image

PHOTO = "fig_4_6_photo.jpg"
OUT = "fig_4_6_rectified.png"

LAM0, LAM1, DLAM = 365.0, 690.0, 0.25
A0, A1, DA = -10.0, 120.0, 0.1

img = np.asarray(Image.open(PHOTO).convert("L")).astype(float)
DARK = 100


def runs(idx):
    if len(idx) == 0:
        return []
    return np.split(idx, np.where(np.diff(idx) > 1)[0] + 1)


def robust_quadratic(x, y):
    ok = ~np.isnan(y)
    x, y = x[ok], y[ok]
    c = np.polyfit(x, y, 2)
    for _ in range(3):
        r = y - np.polyval(c, x)
        keep = np.abs(r) < max(3.0, 3 * np.std(r))
        c = np.polyfit(x[keep], y[keep], 2)
    return c


def frame_edge(positions, lo, hi, horizontal, pick):
    edge = []
    for p in positions:
        line = img[lo:hi, p] if horizontal else img[p, lo:hi]
        rs = runs(np.where(line < DARK)[0])
        edge.append(lo + pick(rs) if rs else np.nan)
    return np.array(edge, float)


def ticks_along(positions, edge_curve, offset, horizontal):
    if horizontal:
        profile = [img[int(round(np.polyval(edge_curve, p))) + offset, p] for p in positions]
    else:
        profile = [img[p, int(round(np.polyval(edge_curve, p))) + offset] for p in positions]
    rs = runs(np.where(np.array(profile) < 120)[0])
    return np.array([positions[r].mean() for r in rs if len(r) >= 3])


def merge_close(t, gap=15):
    out = [t[0]]
    for v in t[1:]:
        if v - out[-1] < gap:
            out[-1] = (out[-1] + v) / 2
        else:
            out.append(v)
    return np.array(out)


xs = np.arange(420, 3700)
ys = np.arange(460, 2180)

bottom_curve = robust_quadratic(xs, frame_edge(xs, 2100, 2300, True,
                                               lambda rs: min(rs, key=lambda r: abs(2100 + r.mean() - 2190))[0]))
top_curve = robust_quadratic(xs, frame_edge(xs, 380, 640, True, lambda rs: rs[0][-1]))
left_curve = robust_quadratic(ys, frame_edge(ys, 300, 560, False,
                                             lambda rs: min(rs, key=lambda r: abs(300 + r.mean() - 370))[-1]))
right_curve = robust_quadratic(ys, frame_edge(ys, 3560, 3820, False, lambda rs: rs[-1][0]))

bottom_ticks = ticks_along(np.arange(420, 3680), bottom_curve, -12, True)
top_ticks = merge_close(ticks_along(xs, top_curve, +12, True))
left_ticks = ticks_along(ys, left_curve, +12, False)
right_ticks = merge_close(ticks_along(ys, right_curve, -12, False))
right_ticks = right_ticks[right_ticks > 600]

lam_bottom = np.arange(370, 370 + 10 * len(bottom_ticks), 10)
lam_top = np.arange(380, 380 + 10 * len(top_ticks), 10)
A_left = np.arange(100, -1, -10)
A_right = np.arange(100, -1, -10)
assert lam_bottom[-1] == 680 and lam_top[-1] == 680
assert len(left_ticks) == 11 and len(right_ticks) == 11

pts_bottom = np.c_[bottom_ticks, np.polyval(bottom_curve, bottom_ticks)]
pts_top = np.c_[top_ticks, np.polyval(top_curve, top_ticks)]
pts_left = np.c_[np.polyval(left_curve, left_ticks), left_ticks]
pts_right = np.c_[np.polyval(right_curve, right_ticks), right_ticks]

ue = np.linspace(700, 3450, 40)
ve = np.linspace(800, 2000, 40)
edge_bottom = np.c_[ue, np.polyval(bottom_curve, ue)]
edge_top = np.c_[ue, np.polyval(top_curve, ue)]
edge_left = np.c_[np.polyval(left_curve, ve), ve]
edge_right = np.c_[np.polyval(right_curve, ve), ve]

U0, U1, V0, V1 = 2050.0, 1700.0, 1320.0, 900.0
DEG = 3
W_SCALE = 1e5


def basis(P):
    u = (P[:, 0] - U0) / U1
    v = (P[:, 1] - V0) / V1
    return np.stack([u**i * v**j for i in range(DEG + 1) for j in range(DEG + 1 - i)], 1)


def fit(known_pts, known_vals, edge1, edge2):
    Bk = basis(known_pts)
    nb = Bk.shape[1]
    rows = [np.c_[Bk, np.zeros((len(Bk), 2))]]
    rhs = [known_vals]
    for j, E in enumerate((edge1, edge2)):
        BE = basis(E)
        extra = np.zeros((len(BE), 2))
        extra[:, j] = -1
        rows.append(np.c_[BE, extra])
        rhs.append(np.zeros(len(BE)))
    sol, *_ = np.linalg.lstsq(np.vstack(rows), np.concatenate(rhs), rcond=None)
    return sol[:nb], sol[nb:]


coef_w, _ = fit(np.vstack([pts_bottom, pts_top]),
                W_SCALE / np.concatenate([lam_bottom, lam_top]), edge_left, edge_right)
coef_A, _ = fit(np.vstack([pts_left, pts_right]),
                np.concatenate([A_left, A_right]), edge_bottom, edge_top)


def to_data(P):
    B = basis(P)
    return W_SCALE / (B @ coef_w), B @ coef_A


lb, _ = to_data(pts_bottom)
lt, _ = to_data(pts_top)
_, al = to_data(pts_left)
_, ar = to_data(pts_right)
print(f"wavelength residual at ticks: rms {np.std(np.r_[lb - lam_bottom, lt - lam_top]):.2f} nm")
print(f"absorbance residual at ticks: rms {np.std(np.r_[al - A_left, ar - A_right]):.2f}")

lam = np.arange(LAM0, LAM1 + 1e-9, DLAM)
A = np.arange(A1, A0 - 1e-9, -DA)
LL, AA = np.meshgrid(lam, A)
w_target = (W_SCALE / LL).ravel()
A_target = AA.ravel()

u = 380 + 3320 * (W_SCALE / 365 - w_target) / (W_SCALE / 365 - W_SCALE / 690)
v = 2190 - 1750 * (A_target + 9.5) / 129.1
for _ in range(10):
    B = basis(np.c_[u, v])
    f1, f2 = B @ coef_w - w_target, B @ coef_A - A_target
    Bu = basis(np.c_[u + 0.5, v])
    Bv = basis(np.c_[u, v + 0.5])
    J11, J21 = (Bu @ coef_w - B @ coef_w) / 0.5, (Bu @ coef_A - B @ coef_A) / 0.5
    J12, J22 = (Bv @ coef_w - B @ coef_w) / 0.5, (Bv @ coef_A - B @ coef_A) / 0.5
    det = J11 * J22 - J12 * J21
    u -= (J22 * f1 - J12 * f2) / det
    v -= (-J21 * f1 + J11 * f2) / det

H, W = img.shape
u0 = np.clip(np.floor(u).astype(int), 0, W - 2)
v0 = np.clip(np.floor(v).astype(int), 0, H - 2)
fu, fv = u - u0, v - v0
val = (img[v0, u0] * (1 - fu) * (1 - fv) + img[v0, u0 + 1] * fu * (1 - fv)
       + img[v0 + 1, u0] * (1 - fu) * fv + img[v0 + 1, u0 + 1] * fu * fv)
Image.fromarray(val.reshape(LL.shape).clip(0, 255).astype(np.uint8)).save(OUT)
print(f"wrote {OUT}: column c -> wavelength {LAM0} + {DLAM}*c nm, row r -> absorbance {A1} - {DA}*r")
