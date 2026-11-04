#ifndef TOPSCENE_H
#define TOPSCENE_H
#include "Top.h"
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTranslation.h>
#include <QColor>
#include <QImage>
#include <QPainter>
#include <cmath>
#include <vector>

class TopScene {
public:
    static constexpr float FLOOR = -0.10f;          // the post is 10 cm tall
    static constexpr float KNOB = 0.09f;            // height of the knob above the tip
    static constexpr int TRAIL = 1440;              // trail points, one per frame
    static constexpr double PHI0 = M_PI / 2;        // shift of phi on the screen

    TopScene() {
        _root = new SoSeparator;
        _root->ref();
        SoPerspectiveCamera* camera = new SoPerspectiveCamera;
        camera->position.setValue(0.0f, 0.24f, 0.40f);
        camera->pointAt(SbVec3f(0.0f, -0.01f, 0.0f));
        camera->heightAngle = 0.6f;
        camera->nearDistance = 0.05f;
        camera->farDistance = 3.0f;
        _root->addChild(camera);
        SoDirectionalLight* sun = new SoDirectionalLight;
        sun->direction.setValue(-0.3f, -1.0f, -0.4f);
        sun->intensity = 0.5f;
        _root->addChild(sun);

        // floor and post
        SoSeparator* stand = new SoSeparator;
        stand->addChild(material(0.50f, 0.48f, 0.45f, 0.0f));
        stand->addChild(cylinder(1.0f, 0.004f, FLOOR - 0.002f));
        stand->addChild(material(0.25f, 0.25f, 0.27f, 0.6f));
        stand->addChild(cylinder(0.025f, 0.006f, FLOOR + 0.003f));    // foot
        stand->addChild(cylinder(0.003f, -FLOOR, FLOOR / 2));          // post up to the tip
        _root->addChild(stand);

        // the top
        SoSeparator* top = new SoSeparator;
        _precess = new SoRotation;
        _tilt = new SoRotation;
        _spin = new SoRotation;
        top->addChild(_precess);
        top->addChild(_tilt);
        top->addChild(_spin);
        SoTexture2* stripes = stripeTexture();
        SoSeparator* body = new SoSeparator;                    // cone, tip down at the origin
        body->addChild(material(1, 1, 1, 0.3f));
        body->addChild(stripes);
        SoTranslation* up = new SoTranslation;
        up->translation.setValue(0.0f, 0.0175f, 0.0f);
        body->addChild(up);
        SoRotation* flip = new SoRotation;
        flip->rotation.setValue(SbVec3f(1, 0, 0), float(M_PI));
        body->addChild(flip);
        SoCone* cone = new SoCone;
        cone->bottomRadius = 0.03f;
        cone->height = 0.035f;
        body->addChild(cone);
        top->addChild(body);
        SoSeparator* disc = new SoSeparator;                    // the wide disc, striped all round
        disc->addChild(material(1, 1, 1, 0.3f));
        disc->addChild(sectorTexture());
        disc->addChild(cylinder(0.042f, 0.012f, 0.041f, SoCylinder::TOP | SoCylinder::BOTTOM));
        disc->addChild(stripes);
        disc->addChild(cylinder(0.042f, 0.012f, 0.041f, SoCylinder::SIDES));
        top->addChild(disc);
        top->addChild(material(0.20f, 0.20f, 0.22f, 0.6f));    // stem and knob
        top->addChild(cylinder(0.004f, 0.043f, 0.0685f));
        SoTranslation* knob = new SoTranslation;
        knob->translation.setValue(0.0f, KNOB, 0.0f);
        top->addChild(knob);
        SoSphere* ball = new SoSphere;
        ball->radius = 0.006f;
        top->addChild(ball);
        _root->addChild(top);

        // trail of the knob, fading out over TRAIL frames
        SoSeparator* trail = new SoSeparator;
        SoLightModel* unlit = new SoLightModel;
        unlit->model = SoLightModel::BASE_COLOR;
        trail->addChild(unlit);
        SoDrawStyle* style = new SoDrawStyle;
        style->lineWidth = 3.0f;
        trail->addChild(style);
        SoMaterial* fade = new SoMaterial;
        fade->diffuseColor.setNum(TRAIL);
        fade->transparency.setNum(TRAIL);
        for (int i = 0; i < TRAIL; ++i) {
            float age = float(i) / (TRAIL - 1);                 // 0 = oldest, 1 = newest
            fade->diffuseColor.set1Value(i, SbColor(0.10f, 0.35f, 0.80f));
            fade->transparency.set1Value(i, 1.0f - age);
        }
        trail->addChild(fade);
        SoMaterialBinding* binding = new SoMaterialBinding;
        binding->value = SoMaterialBinding::PER_VERTEX;
        trail->addChild(binding);
        _trailCoords = new SoCoordinate3;
        trail->addChild(_trailCoords);
        SoLineSet* line = new SoLineSet;
        line->numVertices.setValue(TRAIL);
        trail->addChild(line);
        _root->addChild(trail);
    }
    ~TopScene() { _root->unref(); }

    SoSeparator* root() const { return _root; }

    // Turn the top to the current Euler angles and add the knob to the trail
    void update(const Top& top) {
        _precess->rotation.setValue(SbVec3f(0, 1, 0), float(top.phi() + PHI0));
        _tilt->rotation.setValue(SbVec3f(1, 0, 0), float(top.theta()));
        _spin->rotation.setValue(SbVec3f(0, 1, 0), float(top.psi()));
        _trail[_head] = knob(top);
        _head = (_head + 1) % TRAIL;
        std::vector<SbVec3f> ordered(TRAIL);                   // oldest first, to match the fading
        for (int i = 0; i < TRAIL; ++i) ordered[i] = _trail[(_head + i) % TRAIL];
        _trailCoords->point.setValues(0, TRAIL, ordered.data());
    }

    // Start the trail again from where the knob is now
    void clearTrail(const Top& top) {
        _trail.assign(TRAIL, knob(top));
        _head = 0;
        update(top);
    }

private:
    // the knob is on the axis: tilt (0, KNOB, 0) by theta about x, then turn it by phi about y
    static SbVec3f knob(const Top& top) {
        double s = std::sin(top.theta()), c = std::cos(top.theta()), phi = top.phi() + PHI0;
        return SbVec3f(float(KNOB * s * std::sin(phi)), float(KNOB * c), float(KNOB * s * std::cos(phi)));
    }

    static SoMaterial* material(float r, float g, float b, float shine) {
        SoMaterial* m = new SoMaterial;
        m->diffuseColor.setValue(r, g, b);
        m->specularColor.setValue(shine, shine, shine);
        m->shininess = 0.4f;
        return m;
    }

    // a cylinder along y, centred at height y
    static SoSeparator* cylinder(float radius, float height, float y, int parts = SoCylinder::ALL) {
        SoSeparator* s = new SoSeparator;
        SoTranslation* at = new SoTranslation;
        at->translation.setValue(0.0f, y, 0.0f);
        s->addChild(at);
        SoCylinder* c = new SoCylinder;
        c->radius = radius;
        c->height = height;
        c->parts = parts;
        s->addChild(c);
        return s;
    }

    // Two textures painted with Qt: 8 red and yellow stripes going round the
    // sides, and the same 8 colours as slices on the flat faces of the disc.
    // Without them a spinning top would look like a still one.
    static SoTexture2* toTexture(const QImage& image) {
        int w = image.width(), h = image.height();
        std::vector<unsigned char> pixels(size_t(3) * w * h);   // texture rows run bottom to top
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x) {
                QRgb c = image.pixel(x, h - 1 - y);
                unsigned char* p = &pixels[3 * (size_t(y) * w + x)];
                p[0] = qRed(c), p[1] = qGreen(c), p[2] = qBlue(c);
            }
        SoTexture2* texture = new SoTexture2;
        texture->image.setValue(SbVec2s(short(w), short(h)), 3, pixels.data());
        texture->model = SoTexture2::MODULATE;
        return texture;
    }
    static QColor colour(int k) { return k % 2 ? QColor("#F9C80E") : QColor("#D7263D"); }
    static SoTexture2* stripeTexture() {
        QImage image(256, 4, QImage::Format_RGB32);
        for (int x = 0; x < 256; ++x)
            for (int y = 0; y < 4; ++y) image.setPixel(x, y, colour(x / 32).rgb());
        return toTexture(image);
    }
    static SoTexture2* sectorTexture() {
        QImage image(256, 256, QImage::Format_RGB32);
        image.fill(colour(0));
        QPainter p(&image);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        for (int k = 0; k < 8; ++k) {
            p.setBrush(colour(k));
            p.drawPie(QRectF(-64, -64, 384, 384), k * 45 * 16, 45 * 16);
        }
        p.end();
        return toTexture(image);
    }

    SoSeparator* _root;
    SoRotation *_precess, *_tilt, *_spin;
    SoCoordinate3* _trailCoords;
    std::vector<SbVec3f> _trail;
    int _head = 0;
};

#endif