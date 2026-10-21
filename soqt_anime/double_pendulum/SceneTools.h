#ifndef SCENETOOLS_H
#define SCENETOOLS_H
#include <Inventor/SbColor.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoSphere.h>
#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

// Small helpers used to build the scene: materials, procedural wood textures
// and simple solids.  All lengths are in metres.
namespace tools {

inline SoMaterial* material(const SbColor& diffuse, const SbColor& specular = SbColor(0.05f, 0.05f, 0.05f),
                            float shininess = 0.1f) {
    SoMaterial* m = new SoMaterial;
    m->diffuseColor = diffuse;
    m->ambientColor = diffuse * 0.5f;
    m->specularColor = specular;
    m->shininess = shininess;
    return m;
}

// Look of a wood species: base colour (0-255), number of growth rings across
// the texture, how dark the ring lines are, and how strong the fibre streaks are.
struct WoodStyle {
    double r, g, b;
    double rings;
    double ringDark;
    double streaks;
    double wave = 0.25;   // how far the rings wander (in ring spacings)
};
const WoodStyle WALNUT = {176, 120, 74, 6.0, 0.40, 0.12};
const WoodStyle BEECH  = {226, 190, 142, 4.0, 0.22, 0.08};
const WoodStyle BIRCH  = {232, 208, 168, 5.0, 0.18, 0.06};
const WoodStyle MAPLE  = {234, 202, 158, 4.0, 0.16, 0.06};
const WoodStyle CHERRY = {214, 150, 108, 5.0, 0.26, 0.08};

// Brightness (about 1) of the wood pattern at texture position (across, along),
// both in [0,1).  Rings run along the grain and wander a little; the pattern
// repeats seamlessly.
inline double woodShade(double across, double along, const WoodStyle& w, const std::vector<double>& streak) {
    // uneven ring spacing (whole periods across, so the pattern still repeats seamlessly)
    double uneven = (w.rings / 10.0) * (0.9 * std::sin(2 * M_PI * across + 0.4) + 0.35 * std::sin(2 * M_PI * 3 * across + 1.1));
    double phase = w.rings * across + uneven + w.wave * std::sin(2 * M_PI * along) + 0.32 * w.wave * std::sin(2 * M_PI * 3 * along + 1.0);
    double ring = std::pow(0.5 + 0.5 * std::cos(2 * M_PI * phase), 4.0);
    int n = int(streak.size());
    return 1.0 - w.ringDark * ring + w.streaks * streak[int(across * n) % n];
}

inline std::vector<double> fibreStreaks(int n, int seed) {
    std::mt19937 engine(seed);
    std::uniform_real_distribution<double> flat(-1.0, 1.0);
    std::vector<double> fibre(n), streak(n);
    for (double& f : fibre) f = flat(engine);
    for (int i = 0; i < n; ++i) {
        double sum = 0.0;
        for (int k = -3; k <= 3; ++k) sum += fibre[(i + k + n) % n];
        streak[i] = sum / 7.0;
    }
    return streak;
}

// Wood texture with the grain along the texture s direction (grainAlongS) or t.
inline SoTexture2* woodTexture(bool grainAlongS, const WoodStyle& w, int seed = 7) {
    const int N = 256;
    std::vector<double> streak = fibreStreaks(N, seed);
    std::vector<unsigned char> pixels(3 * N * N);
    for (int j = 0; j < N; ++j) {
        for (int i = 0; i < N; ++i) {
            double across = double(grainAlongS ? j : i) / N;
            double along = double(grainAlongS ? i : j) / N;
            double shade = woodShade(across, along, w, streak);
            unsigned char* px = &pixels[3 * (j * N + i)];
            px[0] = (unsigned char)std::clamp(w.r * shade, 0.0, 255.0);
            px[1] = (unsigned char)std::clamp(w.g * shade, 0.0, 255.0);
            px[2] = (unsigned char)std::clamp(w.b * shade, 0.0, 255.0);
        }
    }
    SoTexture2* texture = new SoTexture2;
    texture->image.setValue(SbVec2s(N, N), 3, pixels.data());
    texture->model = SoTexture2::MODULATE;
    return texture;
}

// Edge of a plywood sheet: thin light and dark layers stacked along t
inline SoTexture2* plywoodEdgeTexture() {
    const int N = 64;
    std::vector<unsigned char> pixels(3 * N * N);
    for (int j = 0; j < N; ++j) {
        bool dark = (j * 7 / N) % 2 == 1;   // 7 layers through the thickness
        for (int i = 0; i < N; ++i) {
            double shade = (dark ? 0.78 : 1.0) * (0.97 + 0.03 * std::sin(i * 0.9 + j));
            unsigned char* px = &pixels[3 * (j * N + i)];
            px[0] = (unsigned char)(226 * shade);
            px[1] = (unsigned char)(196 * shade);
            px[2] = (unsigned char)(150 * shade);
        }
    }
    SoTexture2* texture = new SoTexture2;
    texture->image.setValue(SbVec2s(N, N), 3, pixels.data());
    texture->model = SoTexture2::MODULATE;
    return texture;
}

// A box of size sx x sy x sz centred at 'centre', rotated by 'angle' about z.
// 'texture' may be nullptr for a plain coloured box.
inline SoSeparator* box(float sx, float sy, float sz, const SbVec3f& centre, float angle,
                        SoMaterial* material, SoTexture2* texture = nullptr) {
    SoSeparator* sep = new SoSeparator;
    SoTransform* xf = new SoTransform;
    xf->translation = centre;
    xf->rotation.setValue(SbVec3f(0, 0, 1), angle);
    sep->addChild(xf);
    sep->addChild(material);
    if (texture) sep->addChild(texture);
    SoCube* cube = new SoCube;
    cube->width = sx;
    cube->height = sy;
    cube->depth = sz;
    sep->addChild(cube);
    return sep;
}

// A cylinder along z from z0 to z1, centred on the z axis
inline SoSeparator* zCylinder(float radius, float z0, float z1, SoMaterial* material) {
    SoSeparator* sep = new SoSeparator;
    SoTransform* xf = new SoTransform;
    xf->translation.setValue(0, 0, (z0 + z1) / 2);
    xf->rotation.setValue(SbVec3f(1, 0, 0), float(M_PI) / 2);   // y axis -> z axis
    sep->addChild(xf);
    sep->addChild(material);
    SoCylinder* cyl = new SoCylinder;
    cyl->radius = radius;
    cyl->height = z1 - z0;
    sep->addChild(cyl);
    return sep;
}

inline SoSeparator* sphere(float radius, const SbVec3f& centre, SoMaterial* material) {
    SoSeparator* sep = new SoSeparator;
    SoTranslation* t = new SoTranslation;
    t->translation = centre;
    sep->addChild(t);
    sep->addChild(material);
    SoSphere* s = new SoSphere;
    s->radius = radius;
    sep->addChild(s);
    return sep;
}

}  // namespace tools
#endif
