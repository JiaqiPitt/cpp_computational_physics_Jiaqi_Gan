#ifndef RAINDROP_H
#define RAINDROP_H
#include <cmath>

// A photon through a spherical raindrop of radius 1 centred at the origin,
// traced in three dimensions.  The sunlight travels along -z.

struct Vec3 {
    double x, y, z;
};
inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator-(Vec3 a) { return {-a.x, -a.y, -a.z}; }
inline Vec3 operator*(double s, Vec3 a) { return {s * a.x, s * a.y, s * a.z}; }
inline double dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

// Snell's law in vector form.  d is the direction of the ray, m the unit
// normal of the surface on the side the ray comes from (dot(d, m) < 0), and
// eta = n_before / n_after.
inline Vec3 refract(Vec3 d, Vec3 m, double eta) {
    double cosI = -dot(d, m);
    double sin2T = eta * eta * (1 - cosI * cosI);   // never > 1 in a sphere
    double cosT = std::sqrt(1 - sin2T);
    return eta * d + (eta * cosI - cosT) * m;
}

// Mirror reflection at a surface with unit normal m
inline Vec3 reflect(Vec3 d, Vec3 m) { return d - 2 * dot(d, m) * m; }

// The photon hits the drop at (bx, by) in the plane across the beam
// (bx^2 + by^2 < 1), refracts in, reflects once from the back surface and
// refracts out.  n is the index of refraction of water, the air has n = 1.
// Returns the direction in which the photon leaves.
inline Vec3 trace(double bx, double by, double n) {
    const Vec3 d = {0, 0, -1};
    Vec3 p1 = {bx, by, std::sqrt(1 - bx * bx - by * by)};   // entry point, on the sunny side
    Vec3 t1 = refract(d, p1, 1 / n);                          // the outward normal is p1 itself
    Vec3 p2 = p1 + (-2 * dot(p1, t1)) * t1;                   // straight across to the back surface
    Vec3 t2 = reflect(t1, p2);
    Vec3 p3 = p2 + (-2 * dot(p2, t2)) * t2;                   // across again to where it leaves
    return refract(t2, -p3, n);                               // inside, the normal facing the ray is -p3
}

#endif