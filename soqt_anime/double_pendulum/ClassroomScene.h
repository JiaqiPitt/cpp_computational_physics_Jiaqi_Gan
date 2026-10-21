#ifndef CLASSROOMSCENE_H
#define CLASSROOMSCENE_H
#include "DoublePendulum.h"
#include "DogBoard.h"
#include "SpeechBubble.h"
#include "Sermon.h"
#include "SceneTools.h"
#include <Inventor/SbColor.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoEnvironment.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/annex/FXViz/nodes/SoShadowGroup.h>
#include <Inventor/annex/FXViz/nodes/SoShadowDirectionalLight.h>
#include <Inventor/annex/FXViz/nodes/SoShadowStyle.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <QPainterPath>
#include <random>
#include <QImage>
#include <QPainter>
#include <QString>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

class ClassroomScene {
public:
    // Placement (meters): the desk is centred at the origin
    static constexpr float DESK_TOP   = 0.94f;    // height of the desk top
    static constexpr float DESK_W     = 1.40f;    // desk top width (x)
    static constexpr float DESK_D     = 0.66f;    // desk top depth (z)
    static constexpr float MODEL_X    = -0.27f;   // teaching model on the left half of the desk
    static constexpr float MODEL_Z    = -0.02f;
    static constexpr float TEACHER_X  = 0.30f;    // dog teacher behind the right half
    static constexpr float TEACHER_Z  = -0.47f;   // front face of the board, behind the desk
    static constexpr float BOOK_X     = 0.43f;    // open book: under the middle of its spine
    static constexpr float BOOK_Z     = -0.16f;

    // What the dog jesus preaches is in Sermon.h, one speech bubble per line.

    // logoFile: University of Pittsburgh mark
    ClassroomScene(const DoublePendulum& pendulum, DogBoard* teacher, const QString& logoFile = "pitt_logo.png") {
        _logoFile = logoFile;
        const DoublePendulum::Parameters& p = pendulum.parameters();
        _k = float(p.l1 + p.l2);   // the stand is drawn in units of the total rod length

        _root = new SoSeparator;
        _root->ref();

        // camera setting, here we set the viewer starts from this view: a student's seat
        SoPerspectiveCamera* camera = new SoPerspectiveCamera;
        camera->position.setValue(0.12f, 1.38f, 3.05f);
        camera->pointAt(SbVec3f(0.12f, 1.14f, 0.0f));
        camera->heightAngle = 0.66f;
        camera->nearDistance = 0.1f;
        camera->farDistance = 60.0f;
        _root->addChild(camera);

        SoShadowGroup* shadows = new SoShadowGroup;
        shadows->quality = 1.0f;
        shadows->precision = 0.75f;
        shadows->smoothBorder = 0.3f;
        _root->addChild(shadows);

        // lights: daylight from the windows (casts shadows) and two soft lights
        SoEnvironment* environment = new SoEnvironment;
        environment->ambientIntensity = 0.70f;
        shadows->addChild(environment);
        SoShadowDirectionalLight* sun = new SoShadowDirectionalLight;
        sun->direction.setValue(-0.25f, -1.0f, -0.45f);
        sun->intensity = 0.80f;
        sun->bboxCenter.setValue(0.0f, 0.85f, -0.1f);
        sun->bboxSize.setValue(2.6f, 2.0f, 2.0f);
        shadows->addChild(sun);
        shadows->addChild(light(SbVec3f(0.6f, -0.4f, -0.7f), 0.40f));
        shadows->addChild(light(SbVec3f(0.0f, -0.3f, -1.0f), 0.30f));

        // untextured objects get a plain white texture, so every object in the
        // shadow shader samples a valid texture
        shadows->addChild(whiteTexture());

        shadows->addChild(floor());
        shadows->addChild(desk());
        // the dog jesus's open book
        shadows->addChild(openBook(SbVec3f(BOOK_X, DESK_TOP, BOOK_Z), 0.21f));

        SoSeparator* model = new SoSeparator;
        model->setName("teachingModel");
        SoTranslation* onDesk = new SoTranslation;
        onDesk->translation.setValue(MODEL_X, DESK_TOP, MODEL_Z);
        model->addChild(onDesk);
        model->addChild(stand());
        model->addChild(pendulumParts(p));
        shadows->addChild(model);

        if (teacher && teacher->valid()) {
            SoSeparator* dog = new SoSeparator;
            dog->setName("teacher");
            // the dog casts a shadow but does not receive one: the only shadow
            // that could fall on it (from the desk) is on the part hidden
            // behind the desk, and its large outline faces would smear it
            SoShadowStyle* casts = new SoShadowStyle;
            casts->style = SoShadowStyle::CASTS_SHADOW;
            dog->addChild(casts);
            SoTranslation* place = new SoTranslation;
            place->translation.setValue(TEACHER_X, 0.0f, TEACHER_Z);
            dog->addChild(place);
            dog->addChild(teacher->root());
            shadows->addChild(dog);
            // speech bubbles above and to the right of the head, tail pointing at the head:
            // one per line of the sermon, shown in turn by preach()
            float r = teacher->haloRadius();
            std::vector<QString> lines;
            for (const SermonLine& line : sermon()) {
                lines.push_back(line.html);
                _sermonSeconds.push_back(line.seconds);
            }
            teacher->head()->addChild(bubble::sequence(SbVec3f(0.38f, 0.47f, 0.05f), SbVec2f(0.55f * r, 0.75f * r), 1.05f, 0.42f,
                                                       lines, QFont("Chalkboard SE", 104), _sermonShow, _sermonPop));
        }
        _root->addChild(trail(pendulum));
        update(pendulum);
    }

    ~ClassroomScene() { _root->unref(); }
    ClassroomScene(const ClassroomScene&) = delete;
    ClassroomScene& operator=(const ClassroomScene&) = delete;

    SoSeparator* root() const { return _root; }
    static SbColor background() { return SbColor(0.95f, 0.95f, 0.94f); }   // matte white

    // Biscuit size
    float biscuitHalfLength() const { return 0.14f * _k; }
    float biscuitHalfWidth() const { return 0.065f * _k; }
    // Height of the pivot 
    float pivotHeight() const { return _pivotY; }
    float baseTop() const { return 0.07f * _k; }

    // Show the line of the sermon that belongs to time t (seconds); a new
    // bubble pops up from 70% of its size in about a tenth of a second
    void preach(double t) {
        if (!_sermonShow || _sermonSeconds.empty()) return;
        double total = 0.0;
        for (double s : _sermonSeconds) total += s;
        double inCycle = std::fmod(t, total), start = 0.0;
        int line = 0;
        while (line + 1 < int(_sermonSeconds.size()) && inCycle >= start + _sermonSeconds[line]) start += _sermonSeconds[line++];
        if (_sermonShow->whichChild.getValue() != line) _sermonShow->whichChild = line;
        float size = float(1.0 - 0.3 * std::exp(-(inCycle - start) / 0.06));
        _sermonPop->scaleFactor.setValue(size, size, size);
    }

    // Store the current position of the lower biscuit (call after every step)
    void addTrailPoint(const DoublePendulum& p) {
        _trail[_head] = SbVec3f(float(p.tipX()), float(p.tipY()), _rod2Z);
        _head = (_head + 1) % TRAIL_POINTS;
    }

    // Move the rods, biscuits and trail to the current state (call once per frame)
    void update(const DoublePendulum& p) {
        const DoublePendulum::State& s = p.state();
        float ex = float(p.elbowX()), ey = float(p.elbowY());
        float tx = float(p.tipX()), ty = float(p.tipY());
        // a cylinder lies along y; turning it by theta about z points it
        // along (sin theta, -cos theta), the direction of the rod
        _rod1->rotation.setValue(SbVec3f(0, 0, 1), float(s[0]));
        _rod1->translation.setValue(ex / 2, ey / 2, _rod1Z);
        _rod2->rotation.setValue(SbVec3f(0, 0, 1), float(s[1]));
        _rod2->translation.setValue((ex + tx) / 2, (ey + ty) / 2, _rod2Z);
        // each biscuit is fixed across the end of its rod and turns with it
        _bone1->rotation.setValue(SbVec3f(0, 0, 1), float(s[0]));
        _bone1->translation.setValue(ex, ey, _rod1Z);
        _bone2->rotation.setValue(SbVec3f(0, 0, 1), float(s[1]));
        _bone2->translation.setValue(tx, ty, _rod2Z);
        _elbow->translation.setValue(ex, ey, 0);

        std::vector<SbVec3f> ordered(TRAIL_POINTS);   // oldest first, to match the fading
        for (int i = 0; i < TRAIL_POINTS; ++i) ordered[i] = _trail[(_head + i) % TRAIL_POINTS];
        _trailCoords->point.setValues(0, TRAIL_POINTS, ordered.data());
    }

private:
    static constexpr int TRAIL_POINTS = 2400;   // 2 s of motion at 1200 steps per second

    static SoDirectionalLight* light(const SbVec3f& direction, float intensity) {
        SoDirectionalLight* l = new SoDirectionalLight;
        l->direction = direction;
        l->intensity = intensity;
        return l;
    }

    static SoTexture2* whiteTexture() {
        unsigned char white[3] = {255, 255, 255};
        SoTexture2* texture = new SoTexture2;
        texture->image.setValue(SbVec2s(1, 1), 3, white);
        texture->model = SoTexture2::MODULATE;
        return texture;
    }

    // ---------------- room ----------------
    SoSeparator* floor() {
        SoSeparator* sep = new SoSeparator;
        sep->setName("floor");
        SoShadowStyle* style = new SoShadowStyle;
        style->style = SoShadowStyle::SHADOWED;   // receives shadows, casts none
        sep->addChild(style);
        SoMaterial* matte = tools::material(SbColor(0.93f, 0.93f, 0.92f), SbColor(0, 0, 0), 0.0f);
        sep->addChild(tools::box(14.0f, 0.02f, 14.0f, SbVec3f(0, -0.01f, 0), 0, matte));   // top at y = 0
        return sep;
    }

    // ---------------- teacher's desk ----------------
    SoSeparator* desk() {
        const float top = 0.04f;
        const float bodyW = DESK_W - 0.08f, bodyD = DESK_D - 0.10f, plinth = 0.07f;
        const float bodyTop = DESK_TOP - top;            // underside of the top
        const float bodyH = bodyTop - plinth;
        const float yMid = plinth + bodyH / 2;
        const float panel = 0.025f;
        const float shelfY = plinth + 0.50f;             // underside of the shelf board

        SoTexture2* topGrain = tools::woodTexture(true, tools::BEECH, 21);
        SoTexture2* panelGrain = tools::woodTexture(false, tools::MAPLE, 22);
        SoTexture2* sideGrain = tools::woodTexture(false, tools::MAPLE, 23);
        tools::WoodStyle fineCherry = tools::CHERRY;
        fineCherry.rings = 14.0;   // a wide panel: many thin, calm rings
        fineCherry.wave = 0.06;
        SoTexture2* insetGrain = tools::woodTexture(true, fineCherry, 24);
        SoMaterial* wood = tools::material(SbColor(1.0f, 1.0f, 1.0f), SbColor(0.10f, 0.10f, 0.10f), 0.25f);
        SoMaterial* dark = tools::material(SbColor(0.30f, 0.28f, 0.27f));

        SoSeparator* sep = new SoSeparator;
        sep->setName("desk");
        SoSeparator* deskTop = tools::box(DESK_W, top, DESK_D, SbVec3f(0, DESK_TOP - top / 2, 0), 0, wood, topGrain);
        deskTop->setName("deskTop");
        sep->addChild(deskTop);
        // front panel (towards the class) and the two sides
        sep->addChild(tools::box(bodyW, bodyH, panel, SbVec3f(0, yMid, bodyD / 2 - panel / 2), 0, wood, panelGrain));
        for (float side : {-1.0f, 1.0f})
            sep->addChild(tools::box(panel, bodyH, bodyD, SbVec3f(side * (bodyW / 2 - panel / 2), yMid, 0), 0, wood, sideGrain));
        // teacher's side: back panel up to the shelf, shelf board between the side panels
        sep->addChild(tools::box(bodyW, shelfY - plinth, panel, SbVec3f(0, (plinth + shelfY) / 2, -bodyD / 2 + panel / 2), 0, wood, panelGrain));
        sep->addChild(tools::box(bodyW - 2 * panel, 0.02f, bodyD - panel, SbVec3f(0, shelfY + 0.01f, -panel / 2), 0, wood, sideGrain));
        // raised cherry panel on the front, with the Pitt plaque in its middle
        float insetFront = bodyD / 2 + 0.012f;
        sep->addChild(tools::box(bodyW - 0.26f, bodyH - 0.24f, 0.012f, SbVec3f(0, yMid + 0.02f, bodyD / 2 + 0.006f), 0, wood, insetGrain));
        if (SoSeparator* plaque = logoPlaque(0.46f, 0.32f, 0.010f, SbVec3f(0, yMid + 0.02f, insetFront))) sep->addChild(plaque);
        SoSeparator* base = tools::box(bodyW - 0.04f, plinth, bodyD - 0.04f, SbVec3f(0, plinth / 2, 0), 0, dark);
        base->setName("deskPlinth");
        sep->addChild(base);
        // two books lying on the shelf, one on the other
        SoMaterial* red = tools::material(SbColor(0.70f, 0.16f, 0.12f));
        SoMaterial* blue = tools::material(SbColor(0.16f, 0.30f, 0.62f));
        float shelfTop = shelfY + 0.02f;
        sep->addChild(tools::box(0.22f, 0.035f, 0.30f, SbVec3f(-0.35f, shelfTop + 0.0175f, -0.05f), 0, red));
        sep->addChild(tools::box(0.19f, 0.030f, 0.26f, SbVec3f(-0.34f, shelfTop + 0.035f + 0.015f, -0.06f), 0, blue));
        return sep;
    }

    // A light maple plaque of w x h x t metres, its back against the point
    // 'back' (centre of its back face), with the logo printed on the front.
    // The mark keeps its proportions and gets a clear margin all round.
    SoSeparator* logoPlaque(float w, float h, float t, const SbVec3f& back) {
        QImage logo(_logoFile);
        if (logo.isNull()) {
            std::printf("could not read %s, the desk has no plaque\n", qPrintable(_logoFile));
            return nullptr;
        }
        const tools::WoodStyle paleMaple = {244, 234, 216, 6.0, 0.05, 0.03, 0.10};
        const int W = 2048, H = int(W * h / w);
        QImage face(W, H, QImage::Format_RGB888);
        std::vector<double> streak = tools::fibreStreaks(256, 41);
        for (int y = 0; y < H; ++y) {
            uchar* row = face.scanLine(y);
            for (int x = 0; x < W; ++x) {
                double s = tools::woodShade(double(y) / H, double(x) / W, paleMaple, streak);
                row[3 * x + 0] = (uchar)std::clamp(paleMaple.r * s, 0.0, 255.0);
                row[3 * x + 1] = (uchar)std::clamp(paleMaple.g * s, 0.0, 255.0);
                row[3 * x + 2] = (uchar)std::clamp(paleMaple.b * s, 0.0, 255.0);
            }
        }
        QPainter painter(&face);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        double margin = 0.14 * H;   // clear space around the mark
        double scale = std::min((W - 2 * margin) / logo.width(), (H - 2 * margin) / logo.height());
        QSizeF size(logo.width() * scale, logo.height() * scale);
        painter.drawImage(QRectF(QPointF((W - size.width()) / 2, (H - size.height()) / 2), size), logo);
        painter.end();
        std::vector<unsigned char> pixels(size_t(3) * W * H);   // texture rows run bottom to top
        for (int y = 0; y < H; ++y)
            std::copy(face.constScanLine(H - 1 - y), face.constScanLine(H - 1 - y) + 3 * W, pixels.begin() + size_t(3) * W * y);

        SoSeparator* sep = new SoSeparator;
        sep->setName("pittPlaque");
        SoMaterial* plain = tools::material(SbColor(1.0f, 1.0f, 1.0f), SbColor(0.10f, 0.10f, 0.10f), 0.3f);
        plain->emissiveColor.setValue(0.40f, 0.40f, 0.40f);   // keeps the colours of the mark true
        sep->addChild(tools::box(w, h, t, back + SbVec3f(0, 0, t / 2), 0, plain, tools::woodTexture(true, paleMaple, 42)));
        // the printed face, half a millimetre in front of the plaque; it neither
        // casts nor receives shadows (it would shadow itself at that distance)
        SoShadowStyle* noShadow = new SoShadowStyle;
        noShadow->style = SoShadowStyle::NO_SHADOWING;
        sep->addChild(noShadow);
        sep->addChild(plain);
        SoTexture2* print = new SoTexture2;
        print->image.setValue(SbVec2s(short(W), short(H)), 3, pixels.data());
        print->model = SoTexture2::MODULATE;
        sep->addChild(print);
        SoCoordinate3* corners = new SoCoordinate3;
        float z = back[2] + t + 0.0005f;
        corners->point.set1Value(0, back[0] - w / 2, back[1] - h / 2, z);
        corners->point.set1Value(1, back[0] + w / 2, back[1] - h / 2, z);
        corners->point.set1Value(2, back[0] + w / 2, back[1] + h / 2, z);
        corners->point.set1Value(3, back[0] - w / 2, back[1] + h / 2, z);
        sep->addChild(corners);
        SoTextureCoordinate2* uv = new SoTextureCoordinate2;
        uv->point.set1Value(0, 0, 0);
        uv->point.set1Value(1, 1, 0);
        uv->point.set1Value(2, 1, 1);
        uv->point.set1Value(3, 0, 1);
        sep->addChild(uv);
        sep->addChild(new SoFaceSet);
        return sep;
    }

    // ---------------- the teacher's open book ----------------
    // Lying open on the desk with the head of the pages away from the teacher.
    // 'centre' is the point on the desk under the middle of the spine.
    SoSeparator* openBook(const SbVec3f& centre, float yaw) {
        const float coverW = 0.165f, coverD = 0.235f, coverT = 0.004f;   // each half of the cover
        const float pageW = 0.155f, pageD = 0.222f, pageT = 0.009f;
        SoSeparator* sep = new SoSeparator;
        sep->setName("openBook");
        // the thin pages would shadow themselves in the shadow map, so the book
        // takes no part in the shadows (nothing stands between it and the light,
        // and its own shadow would be only a centimetre wide)
        SoShadowStyle* noShadow = new SoShadowStyle;
        noShadow->style = SoShadowStyle::NO_SHADOWING;
        sep->addChild(noShadow);
        SoTransform* place = new SoTransform;
        place->translation = centre;
        place->rotation.setValue(SbVec3f(0, 1, 0), yaw);
        sep->addChild(place);
        SoShapeHints* hints = new SoShapeHints;   // light both sides of the hand-made faces
        hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
        hints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
        hints->faceType = SoShapeHints::UNKNOWN_FACE_TYPE;
        sep->addChild(hints);
        // cloth-bound cover, both halves flat on the desk
        SoMaterial* cloth = tools::material(SbColor(0.13f, 0.24f, 0.46f), SbColor(0.08f, 0.08f, 0.08f), 0.15f);
        for (float side : {-1.0f, 1.0f})
            sep->addChild(tools::box(coverW, coverT, coverD, SbVec3f(side * coverW / 2, coverT / 2, 0), 0, cloth));
        // the two stacks of pages, dipping into the fold at the spine
        SoMaterial* paper = tools::material(SbColor(1.0f, 1.0f, 1.0f), SbColor(0.05f, 0.05f, 0.05f), 0.1f);
        paper->emissiveColor.setValue(0.15f, 0.15f, 0.15f);
        SoTexture2* spread = bookSpread();
        SoTexture2* edges = pageEdgeTexture();
        for (int side : {-1, 1}) sep->addChild(pageStack(side, pageW, pageD, pageT, coverT, paper, spread, edges));
        return sep;
    }

    // One stack of pages on the cover: x runs from the spine outwards on the
    // given side (-1 left, +1 right).  The top dips towards the spine.
    static SoSeparator* pageStack(int side, float W, float D, float T, float y0, SoMaterial* paper,
                                  SoTexture2* spread, SoTexture2* edges) {
        const int N = 24;
        std::vector<float> xs(N), top(N);
        for (int i = 0; i < N; ++i) {
            float f = float(i) / (N - 1);
            xs[i] = 0.002f + W * f * f;                                   // closer samples near the fold
            top[i] = y0 + T - 0.006f * std::exp(-(xs[i] - 0.002f) / 0.018f);
        }
        SoSeparator* sep = new SoSeparator;
        sep->addChild(paper);
        // printed top surface: u runs across the whole spread, v from the foot (0) to the head (1)
        {
            SoSeparator* s = new SoSeparator;
            s->addChild(spread);
            std::vector<SbVec3f> pts;
            std::vector<SbVec2f> uv;
            for (int i = 0; i < N; ++i)
                for (float z : {-D / 2, D / 2}) {
                    pts.emplace_back(side * xs[i], top[i], z);
                    // the teacher reads from -z, so the left page (left half of the image) lies at +x
                    uv.emplace_back(0.5f - side * (xs[i] / (W + 0.002f)) * 0.5f, z > 0 ? 1.0f : 0.0f);
                }
            std::vector<int> idx;
            for (int i = 0; i + 1 < N; ++i) {
                int a = 2 * i, b = 2 * i + 1, c = 2 * i + 2, d = 2 * i + 3;   // a,c at -z; b,d at +z
                if (side > 0) idx.insert(idx.end(), {b, d, c, a, -1});
                else idx.insert(idx.end(), {d, b, a, c, -1});
            }
            addMesh(s, pts, uv, idx);
            sep->addChild(s);
        }
        // fore-edge, head and foot: the edges of the paper
        {
            SoSeparator* s = new SoSeparator;
            s->addChild(edges);
            std::vector<SbVec3f> pts;
            std::vector<SbVec2f> uv;
            std::vector<int> idx;
            float xo = side * xs[N - 1];
            int base = 0;
            pts.insert(pts.end(), {SbVec3f(xo, y0, -D / 2), SbVec3f(xo, y0, D / 2), SbVec3f(xo, top[N - 1], D / 2), SbVec3f(xo, top[N - 1], -D / 2)});
            uv.insert(uv.end(), {SbVec2f(0, 0), SbVec2f(4, 0), SbVec2f(4, 1), SbVec2f(0, 1)});
            idx.insert(idx.end(), {0, 1, 2, 3, -1});
            for (float z : {-D / 2, D / 2}) {   // head and foot: outline of the stack's profile
                base = int(pts.size());
                pts.emplace_back(side * xs[0], y0, z);
                uv.emplace_back(0, 0);
                pts.emplace_back(xo, y0, z);
                uv.emplace_back(3, 0);
                for (int i = N - 1; i >= 0; --i) {
                    pts.emplace_back(side * xs[i], top[i], z);
                    uv.emplace_back(3.0f * xs[i] / xs[N - 1], (top[i] - y0) / T);
                }
                for (int k = 0; k < N + 2; ++k) idx.push_back(base + k);
                idx.push_back(-1);
            }
            addMesh(s, pts, uv, idx);
            sep->addChild(s);
        }
        return sep;
    }

    static void addMesh(SoSeparator* s, const std::vector<SbVec3f>& pts, const std::vector<SbVec2f>& uv, const std::vector<int>& idx) {
        SoCoordinate3* coords = new SoCoordinate3;
        coords->point.setValues(0, int(pts.size()), pts.data());
        s->addChild(coords);
        SoTextureCoordinate2* tc = new SoTextureCoordinate2;
        tc->point.setValues(0, int(uv.size()), uv.data());
        s->addChild(tc);
        SoIndexedFaceSet* faces = new SoIndexedFaceSet;
        faces->coordIndex.setValues(0, int(idx.size()), idx.data());
        faces->textureCoordIndex.setValues(0, int(idx.size()), idx.data());
        s->addChild(faces);
    }

    // Edges of many sheets of paper: fine lines along the edge
    static SoTexture2* pageEdgeTexture() {
        const int N = 64;
        std::vector<unsigned char> pixels(3 * N * N);
        std::mt19937 engine(9);
        std::uniform_real_distribution<double> flat(0.0, 1.0);
        for (int j = 0; j < N; ++j) {
            double shade = 0.90 + 0.08 * flat(engine);
            for (int i = 0; i < N; ++i)
                for (int c = 0; c < 3; ++c) pixels[3 * (j * N + i) + c] = (unsigned char)(245 * shade - (c == 2 ? 8 : 0));
        }
        SoTexture2* texture = new SoTexture2;
        texture->image.setValue(SbVec2s(N, N), 3, pixels.data());
        texture->model = SoTexture2::MODULATE;
        return texture;
    }

    // The two open pages, drawn with Qt: a figure of the double pendulum (with
    // bone biscuits) on the left, text and the Lagrangian on the right.
    static SoTexture2* bookSpread() {
        const int W = 2048, H = 1468;   // two pages of 0.157 x 0.222 m side by side
        QImage image(W, H, QImage::Format_RGB888);
        image.fill(QColor("#FBF7EE"));
        QPainter p(&image);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::TextAntialiasing);
        // shade into the fold
        for (int x = 0; x < W; ++x) {
            double d = std::fabs(x - W / 2.0) / (W / 2.0);
            int a = int(70 * std::exp(-d / 0.06));
            if (a > 0) { p.setPen(QColor(120, 100, 70, a)); p.drawLine(x, 0, x, H); }
        }
        QColor ink("#2A2A2A"), grey("#9A968E");
        QFont title("STIX Two Text", 60, QFont::Bold), label("STIX Two Text", 46), body("STIX Two Text", 40);
        // left page: title and figure
        p.setPen(ink);
        p.setFont(title);
        p.drawText(QRectF(80, 70, W / 2 - 160, 100), Qt::AlignHCenter, "The Double Pendulum");
        QPointF pivot(W / 4.0, 330), elbow = pivot + QPointF(150, 260), tipPt = elbow + QPointF(240, 170);
        p.setPen(QPen(grey, 5, Qt::DashLine));
        p.drawLine(pivot, pivot + QPointF(0, 300));
        p.drawLine(elbow, elbow + QPointF(0, 230));
        p.setPen(QPen(ink, 9));
        p.drawLine(pivot - QPointF(110, 0), pivot + QPointF(110, 0));   // ceiling
        p.drawLine(pivot, elbow);
        p.drawLine(elbow, tipPt);
        p.setBrush(ink);
        p.drawEllipse(pivot, 12, 12);
        auto bone = [&](QPointF c, double angle) {   // a little bone at the end of a rod
            p.save();
            p.translate(c);
            p.rotate(angle);
            p.setPen(QPen(ink, 5));
            p.setBrush(QColor("#E8C38A"));
            QPainterPath path;
            path.addRoundedRect(QRectF(-55, -12, 110, 24), 10, 10);
            for (int sx : {-1, 1}) for (int sy : {-1, 1}) path.addEllipse(QPointF(sx * 55, sy * 13), 17, 17);
            p.drawPath(path.simplified());
            p.restore();
        };
        bone(elbow, 30);
        bone(tipPt, 55);
        p.setPen(QPen(ink, 4));
        p.setBrush(Qt::NoBrush);
        p.drawArc(QRectF(pivot.x() - 120, pivot.y() - 120, 240, 240), -90 * 16, 30 * 16);
        p.drawArc(QRectF(elbow.x() - 110, elbow.y() - 110, 220, 220), -90 * 16, 55 * 16);
        p.setFont(label);
        p.drawText(pivot + QPointF(30, 175), QString::fromUtf8("θ₁"));
        p.drawText(elbow + QPointF(45, 150), QString::fromUtf8("θ₂"));
        p.drawText(pivot + QPointF(105, 120), QString::fromUtf8("l₁"));
        p.drawText(elbow + QPointF(130, 60), QString::fromUtf8("l₂"));
        p.setFont(body);
        p.drawText(QRectF(80, 1010, W / 2 - 160, 60), Qt::AlignHCenter, "Fig. 7.1  Two rods, two biscuits.");
        // left page text lines under the figure
        std::mt19937 engine(17);
        std::uniform_real_distribution<double> flat(0.0, 1.0);
        auto textLines = [&](double x0, double y0, double width, int lines) {
            for (int k = 0; k < lines; ++k) {
                double len = (k % 6 == 5) ? width * (0.3 + 0.3 * flat(engine)) : width * (0.92 + 0.08 * flat(engine));
                p.fillRect(QRectF(x0, y0 + k * 50, len, 14), grey);
            }
        };
        textLines(110, 1110, W / 2 - 220, 6);
        // right page: a heading, text, the Lagrangian, more text
        p.setPen(ink);
        p.setFont(label);
        p.drawText(QRectF(W / 2 + 110, 80, W / 2 - 220, 70), Qt::AlignLeft, "7.2  The Lagrangian");
        textLines(W / 2 + 110, 190, W / 2 - 220, 11);
        p.setFont(QFont("STIX Two Text", 54));
        p.drawText(QRectF(W / 2 + 80, 760, W / 2 - 160, 90), Qt::AlignHCenter, QString::fromUtf8("L = T − V"));
        textLines(W / 2 + 110, 900, W / 2 - 220, 10);
        p.end();

        std::vector<unsigned char> pixels(size_t(3) * W * H);   // texture rows run bottom to top
        for (int y = 0; y < H; ++y)
            std::copy(image.constScanLine(H - 1 - y), image.constScanLine(H - 1 - y) + 3 * W, pixels.begin() + size_t(3) * W * y);
        SoTexture2* texture = new SoTexture2;
        texture->image.setValue(SbVec2s(short(W), short(H)), 3, pixels.data());
        texture->model = SoTexture2::MODULATE;
        return texture;
    }

    // ---------------- walnut stand of the teaching model ----------------
    // All sizes are multiples of k = l1 + l2.  Its base board sits on the desk at y = 0.
    SoSeparator* stand() {
        const float k = _k;
        float baseH = 0.07f * k, baseD = 0.8f * k;
        float postW = 0.10f * k, postX = 1.15f * k, postH = 1.40f * k;
        float beamH = 0.12f * k, beamD = 0.12f * k;
        float baseW = 2 * postX + 0.4f * k;
        _pivotY = baseH + postH + beamH / 2;   // pivot in the middle of the beam's front face
        _beamFront = beamD / 2;

        SoTexture2* grainX = tools::woodTexture(true, tools::WALNUT, 7);
        SoTexture2* grainY = tools::woodTexture(false, tools::WALNUT, 8);
        SoMaterial* wood = tools::material(SbColor(1.0f, 1.0f, 1.0f), SbColor(0.08f, 0.08f, 0.08f), 0.15f);

        SoSeparator* sep = new SoSeparator;
        SoSeparator* base = tools::box(baseW, baseH, baseD, SbVec3f(0, baseH / 2, 0), 0, wood, grainX);
        base->setName("standBase");
        sep->addChild(base);
        for (float side : {-1.0f, 1.0f})   // posts stand on the base board
            sep->addChild(tools::box(postW, postH, postW, SbVec3f(side * postX, baseH + postH / 2, 0), 0, wood, grainY));
        // the beam rests on top of the posts
        sep->addChild(tools::box(baseW - 0.2f * k, beamH, beamD, SbVec3f(0, _pivotY, 0), 0, wood, grainX));
        float braceRun = 0.30f * k;
        for (float side : {-1.0f, 1.0f}) {   // 45 degree braces from the posts to the beam
            float x = side * (postX - postW / 2 - braceRun / 2);
            float y = _pivotY - beamH / 2 - braceRun / 2;
            sep->addChild(tools::box(0.50f * k, 0.07f * k, 0.07f * k, SbVec3f(x, y, 0), -side * float(M_PI) / 4, wood, grainX));
        }
        // small plate on the front of the beam carrying the pivot
        sep->addChild(tools::box(0.2f * k, 0.2f * k, 0.03f * k, SbVec3f(0, _pivotY, _beamFront + 0.015f * k), 0, wood, grainY));
        return sep;
    }

    // ---------------- rods, joints and biscuits ----------------
    SoSeparator* pendulumParts(const DoublePendulum::Parameters& p) {
        const float k = _k;
        _rod1Z = _beamFront + 0.06f * k;   // rod 1 swings in front of the beam,
        _rod2Z = _beamFront + 0.15f * k;   // rod 2 in front of rod 1, so they can pass
        float rodR = 0.013f * k, hubR = 0.03f * k, pinR = 0.011f * k;

        SoMaterial* aluminium = tools::material(SbColor(0.78f, 0.79f, 0.81f), SbColor(0.9f, 0.9f, 0.9f), 0.5f);
        SoMaterial* steel = tools::material(SbColor(0.35f, 0.35f, 0.37f), SbColor(0.7f, 0.7f, 0.7f), 0.5f);

        SoSeparator* sep = new SoSeparator;
        SoTranslation* pivot = new SoTranslation;
        pivot->translation.setValue(0, _pivotY, 0);
        sep->addChild(pivot);
        // axle from inside the plate through the hub of rod 1
        sep->addChild(tools::zCylinder(pinR, _beamFront, _rod1Z + 0.03f * k, steel));
        sep->addChild(tools::zCylinder(hubR, _rod1Z - 0.013f * k, _rod1Z + 0.013f * k, aluminium));

        _rod1 = new SoTransform;
        _rod2 = new SoTransform;
        sep->addChild(rod(_rod1, rodR, float(p.l1), aluminium));
        sep->addChild(rod(_rod2, rodR, float(p.l2), aluminium));

        // the joint between the rods: pin and the hub of rod 2
        SoSeparator* elbow = new SoSeparator;
        _elbow = new SoTranslation;
        elbow->addChild(_elbow);
        elbow->addChild(tools::zCylinder(pinR, _rod1Z, _rod2Z + 0.02f * k, steel));
        elbow->addChild(tools::zCylinder(hubR, _rod2Z - 0.013f * k, _rod2Z + 0.013f * k, aluminium));
        sep->addChild(elbow);

        SoTexture2* crumb = biscuitTexture();
        _bone1 = new SoTransform;
        _bone2 = new SoTransform;
        sep->addChild(biscuit(_bone1, crumb));
        sep->addChild(biscuit(_bone2, crumb));
        return sep;
    }

    SoSeparator* rod(SoTransform* xf, float radius, float length, SoMaterial* material) {
        SoSeparator* sep = new SoSeparator;
        sep->addChild(xf);
        sep->addChild(material);
        SoCylinder* cyl = new SoCylinder;
        cyl->radius = radius;
        cyl->height = length;
        sep->addChild(cyl);
        return sep;
    }

    // A flat dog-bone biscuit in the plane of the swing, long axis along local
    // x: a shaft with two round knobs at each end, the rod fixed at its middle.
    SoSeparator* biscuit(SoTransform* xf, SoTexture2* crumb) {
        const float k = _k;
        float knobR = 0.035f * k, knobY = 0.030f * k;          // knobs reach out to 0.065 k
        float knobX = biscuitHalfLength() - knobR;
        float shaftW = 0.05f * k, thick = 0.035f * k;
        SoMaterial* baked = tools::material(SbColor(1.0f, 1.0f, 1.0f), SbColor(0.15f, 0.12f, 0.08f), 0.2f);

        SoSeparator* sep = new SoSeparator;
        sep->addChild(xf);
        sep->addChild(baked);
        sep->addChild(crumb);
        SoCube* shaft = new SoCube;
        shaft->width = 2 * knobX;
        shaft->height = shaftW;
        shaft->depth = thick;
        sep->addChild(shaft);
        for (float sx : {-1.0f, 1.0f})
            for (float sy : {-1.0f, 1.0f}) {
                SoSeparator* knob = new SoSeparator;
                SoTransform* at = new SoTransform;
                at->translation.setValue(sx * knobX, sy * knobY, 0);
                at->rotation.setValue(SbVec3f(1, 0, 0), float(M_PI) / 2);   // round face towards the class
                knob->addChild(at);
                SoCylinder* disc = new SoCylinder;
                disc->radius = knobR;
                disc->height = thick;
                knob->addChild(disc);
                sep->addChild(knob);
            }
        return sep;
    }

    // Golden-brown baked surface with darker toasted specks
    static SoTexture2* biscuitTexture() {
        const int N = 128;
        std::mt19937 engine(4);
        std::uniform_real_distribution<double> flat(0.0, 1.0);
        std::vector<double> shade(N * N);
        for (double& v : shade) v = 0.92 + 0.08 * flat(engine);
        for (int n = 0; n < 160; ++n) {
            int cx = int(flat(engine) * N), cy = int(flat(engine) * N);
            double r = 1.0 + 1.5 * flat(engine), depth = 0.25 + 0.3 * flat(engine);
            for (int dy = -3; dy <= 3; ++dy)
                for (int dx = -3; dx <= 3; ++dx)
                    if (dx * dx + dy * dy <= r * r) shade[((cy + dy + N) % N) * N + (cx + dx + N) % N] *= 1.0 - depth;
        }
        std::vector<unsigned char> pixels(3 * N * N);
        for (int i = 0; i < N * N; ++i) {
            pixels[3 * i + 0] = (unsigned char)(220 * shade[i]);
            pixels[3 * i + 1] = (unsigned char)(158 * shade[i]);
            pixels[3 * i + 2] = (unsigned char)(84 * shade[i]);
        }
        SoTexture2* texture = new SoTexture2;
        texture->image.setValue(SbVec2s(N, N), 3, pixels.data());
        texture->model = SoTexture2::MODULATE;
        return texture;
    }

    // ---------------- trail ----------------
    SoSeparator* trail(const DoublePendulum& pendulum) {
        SoSeparator* sep = new SoSeparator;
        SoTranslation* at = new SoTranslation;   // same frame as the pendulum: the pivot
        at->translation.setValue(MODEL_X, DESK_TOP + _pivotY, MODEL_Z);
        sep->addChild(at);
        SoLightModel* unlit = new SoLightModel;
        unlit->model = SoLightModel::BASE_COLOR;
        sep->addChild(unlit);
        SoDrawStyle* style = new SoDrawStyle;
        style->lineWidth = 2.5f;
        sep->addChild(style);
        SoMaterial* fade = new SoMaterial;
        fade->diffuseColor.setNum(TRAIL_POINTS);
        fade->transparency.setNum(TRAIL_POINTS);
        for (int i = 0; i < TRAIL_POINTS; ++i) {
            float age = float(i) / (TRAIL_POINTS - 1);   // 0 = oldest, 1 = newest
            fade->diffuseColor.set1Value(i, SbColor(0.95f, 0.45f, 0.10f));
            fade->transparency.set1Value(i, 1.0f - std::pow(age, 1.5f));
        }
        sep->addChild(fade);
        SoMaterialBinding* binding = new SoMaterialBinding;
        binding->value = SoMaterialBinding::PER_VERTEX;
        sep->addChild(binding);
        _trail.assign(TRAIL_POINTS, SbVec3f(float(pendulum.tipX()), float(pendulum.tipY()), _rod2Z));
        _trailCoords = new SoCoordinate3;
        _trailCoords->point.setValues(0, TRAIL_POINTS, _trail.data());
        sep->addChild(_trailCoords);
        SoLineSet* line = new SoLineSet;
        line->numVertices.setValue(TRAIL_POINTS);
        sep->addChild(line);
        return sep;
    }

    SoSeparator* _root;
    SoTransform* _rod1 = nullptr;
    SoTransform* _rod2 = nullptr;
    SoTransform* _bone1 = nullptr;
    SoTransform* _bone2 = nullptr;
    SoTranslation* _elbow = nullptr;
    SoCoordinate3* _trailCoords = nullptr;
    std::vector<SbVec3f> _trail;
    QString _logoFile;
    SoSwitch* _sermonShow = nullptr;   // which speech bubble is shown
    SoScale* _sermonPop = nullptr;     // size of the bubble, for the pop when it changes
    std::vector<double> _sermonSeconds;
    int _head = 0;
    float _k = 0.3f, _pivotY = 0, _beamFront = 0, _rod1Z = 0, _rod2Z = 0;
};

#endif
