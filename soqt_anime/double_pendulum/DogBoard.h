#ifndef DOGBOARD_H
#define DOGBOARD_H
#include "SceneTools.h"
#include <QImage>
#include <QPointF>
#include <QString>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/annex/FXViz/nodes/SoShadowStyle.h>
#include <cmath>
#include <cstdio>
#include <unordered_map>
#include <vector>

// The cute dog jesus teacher: a plywood cut-out in the shape of the dog jesus with the photo
// printed on the front, a wooden pole fixed to its back, and a round iron
// base on the floor.  The pole sits in a socket on the base, where a spring
// lets it rock in the plane of the board; setTilt() sets the angle, and
// inertia()/massHeight() give the numbers for the motion (SwayingBoard.h).
// The halo is a glowing ring of plywood mounted on the pole behind the head which makes the dog holly.
// Coordinates: origin on the floor under the pole, y up, the printed face of
// the board at z = 0 facing +z; the pivot is at the top of the base.
class DogBoard {
public:
    struct Halo { double cx, cy, radius, halfWidth; };   // in photo pixels

    struct Size {
        double boardHeight = 1.00;    // bottom of the board to the top of the halo (m)
        double poleLength  = 0.62;    // pole between the base and the board (m)
        double thickness   = 0.012;   // plywood (m)
    };
    static constexpr float BASE_HEIGHT = 0.035f;   // iron base; the pivot is on top of it

    DogBoard(const QString& photoFile, const QString& maskFile, const Halo& halo, const Size& size) {
        QImage photo(photoFile), mask(maskFile);
        if (photo.isNull() || mask.isNull() || photo.size() != mask.size()) {
            std::printf("could not read %s and %s (they must be images of the same size)\n",
                        qPrintable(photoFile), qPrintable(maskFile));
            return;
        }
        photo = photo.convertToFormat(QImage::Format_RGB888);
        mask = mask.convertToFormat(QImage::Format_Grayscale8);
        build(photo, mask, halo, size);
    }
    ~DogBoard() { if (_root) _root->unref(); }
    DogBoard(const DogBoard&) = delete;
    DogBoard& operator=(const DogBoard&) = delete;

    bool valid() const { return _root != nullptr; }
    SoSeparator* root() const { return _root; }
    void setTilt(double angle) { _tilt->rotation.setValue(SbVec3f(0, 0, 1), float(angle)); }

    // Let the halo's glow breathe slowly (t in seconds)
    void setGlow(double t) {
        float strength = float(0.75 + 0.25 * std::sin(2 * M_PI * t / 2.5));
        for (int i = 0; i < _glow->transparency.getNum(); ++i) {
            float base = _glowOpacity[size_t(i)];
            _glow->transparency.set1Value(i, 1.0f - base * strength);
        }
    }

    double inertia() const { return _inertia; }         // about the pivot (kg m^2)
    double massHeight() const { return _massHeight; }   // mass x height of centre of mass (kg m)
    double mass() const { return _mass; }
    int outlinePoints() const { return int(_outlineSize); }
    // Geometry, for checking the placement (heights above the pivot, z of the ring)
    double haloOffsetX() const { return _haloOffsetX; }
    // Group that moves with the dog, with its origin at the centre of the halo
    SoSeparator* head() const { return _head; }
    float haloRadius() const { return _haloRadius; }
    float boardBottom() const { return _boardBottom; }
    float boardTop() const { return _boardTop; }
    float ringFront() const { return _ringFront; }
    float ringBack() const { return _ringBack; }

private:
    static constexpr double PLYWOOD_DENSITY = 680.0;   // kg/m^3
    static constexpr double POLE_DENSITY    = 720.0;   // beech dowel, kg/m^3

    void build(const QImage& photo, const QImage& mask, const Halo& halo, const Size& size) {
        const double W = photo.width(), H = photo.height();
        std::vector<QPointF> outlinePx = outline(mask);
        _outlineSize = outlinePx.size();

        // Scale: the board runs from the bottom of the outline to the top of the halo
        double bottomPx = 0.0;
        for (const QPointF& p : outlinePx) bottomPx = std::max(bottomPx, p.y());
        double topPx = halo.cy - halo.radius - halo.halfWidth;
        double s = size.boardHeight / (bottomPx - topPx);   // metres per pixel
        double T = size.thickness, ringT = 0.008, gap = 0.003;

        // Board outline in metres (y up), x measured from photo column 0 for now
        std::vector<SbVec2f> poly;
        for (const QPointF& p : outlinePx) poly.emplace_back(float(p.x() * s), float((bottomPx - p.y()) * s));
        if (signedArea(poly) < 0) {   // make it counterclockwise seen from the front
            std::reverse(poly.begin(), poly.end());
            std::reverse(outlinePx.begin(), outlinePx.end());
        }

        // Halo ring in the same units
        double hx = halo.cx * s, hy = (bottomPx - halo.cy) * s;
        double r1 = (halo.radius - halo.halfWidth) * s, r2 = (halo.radius + halo.halfWidth) * s;

        // Put the pole straight under the centre of mass of board + halo
        double sigmaB = PLYWOOD_DENSITY * T, sigmaH = PLYWOOD_DENSITY * ringT;   // kg/m^2
        Moments mb = moments(poly);
        double areaH = M_PI * (r2 * r2 - r1 * r1);
        double xcm = (sigmaB * mb.x + sigmaH * areaH * hx) / (sigmaB * mb.area + sigmaH * areaH);
        for (SbVec2f& v : poly) v[0] -= float(xcm);
        hx -= xcm;
        mb = moments(poly);

        // The pole runs up the back of the board to the bottom of the halo,
        // and the halo ring is fixed behind the pole.
        double L = size.poleLength, poleR = 0.012;
        double ringMid = hy - 0.5 * (r1 + r2);               // lowest point of the ring band (board coords)
        double poleTop = L + ringMid;                        // measured from the pivot
        float poleZ = float(-T - poleR);
        float ringFront = float(-T - 2 * poleR - 0.004);     // 4 mm spacer between pole and ring
        float ringBack = float(ringFront - ringT);

        // Mass properties about the pivot, which sits L below the board
        double mBoard = sigmaB * mb.area, mHalo = sigmaH * areaH;
        double mPole = POLE_DENSITY * M_PI * poleR * poleR * poleTop;
        _mass = mBoard + mHalo + mPole;
        _massHeight = sigmaB * (mb.y + L * mb.area) + mHalo * (hy + L) + mPole * poleTop / 2;
        _inertia = sigmaB * (mb.xx + mb.yy + 2 * L * mb.y + L * L * mb.area)
                 + sigmaH * (M_PI / 2 * (std::pow(r2, 4) - std::pow(r1, 4)) + areaH * (hx * hx + (hy + L) * (hy + L)))
                 + mPole * poleTop * poleTop / 3;
        _haloOffsetX = hx;
        _haloBottom = float(L + hy - r2);
        _boardBottom = float(L);
        _boardTop = float(L + hy + r2);
        _ringFront = ringFront;
        _ringBack = ringBack;

        // ---------------- scene graph ----------------
        _root = new SoSeparator;
        _root->ref();

        // round iron base on the floor and the socket that holds the pole
        SoMaterial* iron = tools::material(SbColor(0.20f, 0.20f, 0.22f), SbColor(0.5f, 0.5f, 0.5f), 0.4f);
        SoSeparator* base = new SoSeparator;
        base->setName("teacherBase");
        SoTranslation* baseAt = new SoTranslation;
        baseAt->translation.setValue(0.0f, BASE_HEIGHT / 2, poleZ);
        base->addChild(baseAt);
        base->addChild(iron);
        SoCylinder* disc = new SoCylinder;
        disc->radius = 0.14f;
        disc->height = BASE_HEIGHT;
        base->addChild(disc);
        SoTranslation* socketAt = new SoTranslation;
        socketAt->translation.setValue(0.0f, BASE_HEIGHT / 2 + 0.015f, 0.0f);
        base->addChild(socketAt);
        SoCylinder* socket = new SoCylinder;   // the pole rocks a few degrees inside it
        socket->radius = float(poleR + 0.009);
        socket->height = 0.03f;
        base->addChild(socket);
        _root->addChild(base);

        // everything above the pivot rocks together
        SoSeparator* rocking = new SoSeparator;
        SoTranslation* pivotAt = new SoTranslation;
        pivotAt->translation.setValue(0.0f, BASE_HEIGHT, 0.0f);
        rocking->addChild(pivotAt);
        _tilt = new SoRotation;
        rocking->addChild(_tilt);

        // pole fixed to the back of the board, from the socket up to the halo
        SoMaterial* dowel = tools::material(SbColor(1.0f, 1.0f, 1.0f));
        SoSeparator* pole = new SoSeparator;
        SoTranslation* poleAt = new SoTranslation;
        poleAt->translation.setValue(0.0f, float(poleTop / 2), poleZ);
        pole->addChild(poleAt);
        pole->addChild(dowel);
        pole->addChild(tools::woodTexture(false, tools::BEECH, 3));
        SoCylinder* stick = new SoCylinder;
        stick->radius = float(poleR);
        stick->height = float(poleTop);
        pole->addChild(stick);
        rocking->addChild(pole);
        // spacer block joining the top of the pole to the halo ring
        rocking->addChild(tools::box(float(2 * poleR), 0.03f, 0.004f,
                                     SbVec3f(0.0f, float(poleTop - 0.015), float(-T - 2 * poleR - 0.002)), 0, iron));

        // the board itself, its bottom edge at the top of the pole's visible part
        SoSeparator* board = new SoSeparator;
        SoTranslation* boardAt = new SoTranslation;
        boardAt->translation.setValue(0.0f, float(L), 0.0f);
        board->addChild(boardAt);

        SoShapeHints* hints = new SoShapeHints;
        hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
        hints->shapeType = SoShapeHints::SOLID;
        hints->faceType = SoShapeHints::UNKNOWN_FACE_TYPE;   // concave outline: let Coin triangulate
        hints->creaseAngle = 0.9f;
        board->addChild(hints);

        SoTexture2* printed = photoTexture(photo);
        SoTexture2* plyFace = tools::woodTexture(true, tools::BIRCH, 5);
        SoTexture2* plyEdge = tools::plywoodEdgeTexture();
        // the print glows a little so the photo stays bright and its dark fur keeps its detail
        SoMaterial* white = tools::material(SbColor(1.0f, 1.0f, 1.0f), SbColor(0.10f, 0.10f, 0.10f), 0.2f);
        white->emissiveColor.setValue(0.4f, 0.4f, 0.4f);
        SoMaterial* ply = tools::material(SbColor(0.9f, 0.9f, 0.9f));

        // printed front face
        std::vector<SbVec3f> front;
        std::vector<SbVec2f> photoUV;
        for (size_t i = 0; i < poly.size(); ++i) {
            front.emplace_back(poly[i][0], poly[i][1], 0.0f);
            photoUV.emplace_back(float((outlinePx[i].x() + 0.5) / W), float(1.0 - (outlinePx[i].y() + 0.5) / H));
        }
        board->addChild(face(front, photoUV, false, white, printed));

        // plain plywood back face
        std::vector<SbVec3f> back;
        std::vector<SbVec2f> plyUV;
        for (const SbVec2f& v : poly) {
            back.emplace_back(v[0], v[1], float(-T));
            plyUV.emplace_back(v[0] / 0.3f, v[1] / 0.3f);
        }
        board->addChild(face(back, plyUV, true, ply, plyFace));

        // edge all the way round
        board->addChild(wall(poly, 0.0f, float(-T), true, ply, plyEdge));

        // halo: a ring of plywood behind the pole with the halo printed on it;
        // its material glows, and a soft golden band around it fades outwards
        const int N = 240;
        std::vector<SbVec2f> outer, inner;
        for (int i = 0; i < N; ++i) {
            double a = 2 * M_PI * i / N;
            outer.emplace_back(float(hx + r2 * std::cos(a)), float(hy + r2 * std::sin(a)));
            inner.emplace_back(float(hx + r1 * std::cos(a)), float(hy + r1 * std::sin(a)));
        }
        std::vector<SbVec3f> ringFrontPts;
        std::vector<SbVec2f> ringUV;
        std::vector<int> ringIdx;
        for (int i = 0; i < N; ++i) {
            ringFrontPts.emplace_back(outer[i][0], outer[i][1], ringFront);
            ringFrontPts.emplace_back(inner[i][0], inner[i][1], ringFront);
        }
        for (const SbVec3f& v : ringFrontPts) {
            double px = (v[0] + xcm) / s, py = bottomPx - v[1] / s;
            ringUV.emplace_back(float((px + 0.5) / W), float(1.0 - (py + 0.5) / H));
        }
        for (int i = 0; i < N; ++i) {   // quad between angle i and i+1, counterclockwise from the front
            int j = (i + 1) % N;
            ringIdx.insert(ringIdx.end(), {2 * i + 1, 2 * i, 2 * j, 2 * j + 1, -1});
        }
        SoMaterial* glowing = tools::material(SbColor(0.9f, 0.85f, 0.7f), SbColor(0.3f, 0.3f, 0.2f), 0.3f);
        glowing->emissiveColor.setValue(0.85f, 0.70f, 0.35f);
        board->addChild(mesh(ringFrontPts, ringUV, ringIdx, glowing, printed));
        std::vector<SbVec3f> ringBackPts;
        std::vector<SbVec2f> ringBackUV;
        std::vector<int> ringBackIdx;
        for (int i = 0; i < N; ++i) {
            ringBackPts.emplace_back(outer[i][0], outer[i][1], ringBack);
            ringBackPts.emplace_back(inner[i][0], inner[i][1], ringBack);
            ringBackUV.emplace_back(outer[i][0] / 0.3f, outer[i][1] / 0.3f);
            ringBackUV.emplace_back(inner[i][0] / 0.3f, inner[i][1] / 0.3f);
        }
        for (int i = 0; i < N; ++i) {
            int j = (i + 1) % N;
            ringBackIdx.insert(ringBackIdx.end(), {2 * i, 2 * i + 1, 2 * j + 1, 2 * j, -1});
        }
        board->addChild(mesh(ringBackPts, ringBackUV, ringBackIdx, ply, plyFace));
        board->addChild(wall(outer, ringFront, ringBack, true, glowing, plyEdge));
        board->addChild(wall(inner, ringFront, ringBack, false, glowing, plyEdge));
        board->addChild(glowBand(hx, hy, 0.5 * (r1 + r2), 2.6 * (r2 - r1), ringFront + 0.001f));

        rocking->addChild(board);

        // anchor at the centre of the halo (about the head), moving with the dog
        _head = new SoSeparator;
        SoTranslation* headAt = new SoTranslation;
        headAt->translation.setValue(float(hx), float(L + hy), 0.0f);
        _head->addChild(headAt);
        rocking->addChild(_head);
        _haloRadius = float(r2);
        _root->addChild(rocking);
    }

    // ---------- outline of the mask ----------

    // Closed outline of the white part of the mask in photo pixel coordinates:
    // marching squares at half brightness on a reduced copy, then Douglas-Peucker.
    static std::vector<QPointF> outline(const QImage& mask) {
        const int w = 450;
        const int h = int(std::lround(double(mask.height()) * w / mask.width()));
        QImage small = mask.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                           .convertToFormat(QImage::Format_Grayscale8);
        const int gw = w + 2, gh = h + 2;   // one pixel of black padding all round
        std::vector<double> f(size_t(gw) * gh, 0.0);
        for (int y = 0; y < h; ++y) {
            const uchar* row = small.constScanLine(y);
            for (int x = 0; x < w; ++x) f[size_t(y + 1) * gw + (x + 1)] = row[x] / 255.0;
        }
        auto F = [&](int x, int y) { return f[size_t(y) * gw + x]; };
        const double iso = 0.5;

        // Each crossing sits on a grid edge: key 2*(y*gw+x) for the edge from
        // (x,y) to (x+1,y), and 2*(y*gw+x)+1 for the edge from (x,y) to (x,y+1).
        auto crossing = [&](long key) {
            long cell = key / 2;
            int x = int(cell % gw), y = int(cell / gw);
            double a = F(x, y);
            if (key % 2 == 0) {
                double t = (iso - a) / (F(x + 1, y) - a);
                return QPointF(x + t, y);
            }
            double t = (iso - a) / (F(x, y + 1) - a);
            return QPointF(x, y + t);
        };
        std::unordered_map<long, std::vector<long>> links;
        auto link = [&](long a, long b) { links[a].push_back(b); links[b].push_back(a); };
        for (int y = 0; y + 1 < gh; ++y) {
            for (int x = 0; x + 1 < gw; ++x) {
                double c0 = F(x, y), c1 = F(x + 1, y), c2 = F(x + 1, y + 1), c3 = F(x, y + 1);
                int code = (c0 > iso) | (c1 > iso) << 1 | (c2 > iso) << 2 | (c3 > iso) << 3;
                long e0 = 2L * (long(y) * gw + x);           // top
                long e1 = 2L * (long(y) * gw + x + 1) + 1;   // right
                long e2 = 2L * (long(y + 1) * gw + x);       // bottom
                long e3 = 2L * (long(y) * gw + x) + 1;       // left
                bool centre = (c0 + c1 + c2 + c3) / 4 > iso;
                switch (code) {
                    case 1: case 14: link(e3, e0); break;
                    case 2: case 13: link(e0, e1); break;
                    case 3: case 12: link(e3, e1); break;
                    case 4: case 11: link(e1, e2); break;
                    case 6: case 9:  link(e0, e2); break;
                    case 7: case 8:  link(e2, e3); break;
                    case 5:  if (centre) { link(e0, e1); link(e2, e3); } else { link(e3, e0); link(e1, e2); } break;
                    case 10: if (centre) { link(e3, e0); link(e1, e2); } else { link(e0, e1); link(e2, e3); } break;
                    default: break;
                }
            }
        }
        // Walk the loops and keep the one enclosing the largest area
        std::unordered_map<long, bool> used;
        std::vector<QPointF> best;
        double bestArea = 0.0;
        for (const auto& entry : links) {
            if (used[entry.first]) continue;
            std::vector<QPointF> loop;
            long start = entry.first, prev = -1, cur = start;
            do {
                used[cur] = true;
                loop.push_back(crossing(cur));
                const std::vector<long>& next = links[cur];
                long nxt = (next[0] != prev) ? next[0] : next[1];
                prev = cur;
                cur = nxt;
            } while (cur != start);
            double area = 0.0;
            for (size_t i = 0; i < loop.size(); ++i) {
                const QPointF& a = loop[i];
                const QPointF& b = loop[(i + 1) % loop.size()];
                area += a.x() * b.y() - b.x() * a.y();
            }
            if (std::fabs(area) > bestArea) { bestArea = std::fabs(area); best = loop; }
        }
        // back to full-size photo pixels (padding removed)
        double sx = double(mask.width()) / w, sy = double(mask.height()) / h;
        for (QPointF& p : best) p = QPointF((p.x() - 1 + 0.5) * sx - 0.5, (p.y() - 1 + 0.5) * sy - 0.5);
        return simplify(best, 1.2);
    }

    // Douglas-Peucker simplification of a closed polygon (tolerance in pixels)
    static std::vector<QPointF> simplify(const std::vector<QPointF>& p, double tol) {
        size_t n = p.size();
        size_t far = 0;
        double farDist = 0.0;
        for (size_t i = 1; i < n; ++i) {
            double d = std::hypot(p[i].x() - p[0].x(), p[i].y() - p[0].y());
            if (d > farDist) { farDist = d; far = i; }
        }
        std::vector<bool> keep(n + 1, false);
        keep[0] = keep[far] = keep[n] = true;
        std::vector<QPointF> q(p);
        q.push_back(p[0]);   // close the loop so the second half ends at the start
        dpRecurse(q, 0, far, tol, keep);
        dpRecurse(q, far, n, tol, keep);
        std::vector<QPointF> out;
        for (size_t i = 0; i < n; ++i) if (keep[i]) out.push_back(p[i]);
        return out;
    }
    static void dpRecurse(const std::vector<QPointF>& q, size_t i0, size_t i1, double tol, std::vector<bool>& keep) {
        if (i1 <= i0 + 1) return;
        double ax = q[i0].x(), ay = q[i0].y(), bx = q[i1].x(), by = q[i1].y();
        double len = std::hypot(bx - ax, by - ay);
        double maxDist = -1.0;
        size_t imax = i0;
        for (size_t i = i0 + 1; i < i1; ++i) {
            double d = len > 0 ? std::fabs((bx - ax) * (ay - q[i].y()) - (ax - q[i].x()) * (by - ay)) / len
                               : std::hypot(q[i].x() - ax, q[i].y() - ay);
            if (d > maxDist) { maxDist = d; imax = i; }
        }
        if (maxDist > tol) {
            keep[imax] = true;
            dpRecurse(q, i0, imax, tol, keep);
            dpRecurse(q, imax, i1, tol, keep);
        }
    }

    // ---------- area moments of a polygon (Green's theorem) ----------
    struct Moments { double area, x, y, xx, yy; };   // integrals of 1, x, y, x^2, y^2 over the area
    static Moments moments(const std::vector<SbVec2f>& v) {
        Moments m{0, 0, 0, 0, 0};
        for (size_t i = 0; i < v.size(); ++i) {
            double x0 = v[i][0], y0 = v[i][1];
            double x1 = v[(i + 1) % v.size()][0], y1 = v[(i + 1) % v.size()][1];
            double c = x0 * y1 - x1 * y0;
            m.area += c / 2;
            m.x += (x0 + x1) * c / 6;
            m.y += (y0 + y1) * c / 6;
            m.xx += (x0 * x0 + x0 * x1 + x1 * x1) * c / 12;
            m.yy += (y0 * y0 + y0 * y1 + y1 * y1) * c / 12;
        }
        return m;
    }
    static double signedArea(const std::vector<SbVec2f>& v) { return moments(v).area; }

    // ---------- geometry helpers ----------

    static SoSeparator* mesh(const std::vector<SbVec3f>& pts, const std::vector<SbVec2f>& uv,
                             const std::vector<int>& idx, SoMaterial* material, SoTexture2* texture) {
        SoSeparator* sep = new SoSeparator;
        sep->addChild(material);
        sep->addChild(texture);
        SoCoordinate3* coords = new SoCoordinate3;
        coords->point.setValues(0, int(pts.size()), pts.data());
        sep->addChild(coords);
        SoTextureCoordinate2* tc = new SoTextureCoordinate2;
        tc->point.setValues(0, int(uv.size()), uv.data());
        sep->addChild(tc);
        SoIndexedFaceSet* faces = new SoIndexedFaceSet;
        faces->coordIndex.setValues(0, int(idx.size()), idx.data());
        faces->textureCoordIndex.setValues(0, int(idx.size()), idx.data());
        sep->addChild(faces);
        return sep;
    }

    // One polygon face; reversed = true flips it to face -z
    static SoSeparator* face(const std::vector<SbVec3f>& pts, const std::vector<SbVec2f>& uv, bool reversed,
                             SoMaterial* material, SoTexture2* texture) {
        std::vector<int> idx;
        for (size_t i = 0; i < pts.size(); ++i) idx.push_back(int(reversed ? pts.size() - 1 - i : i));
        idx.push_back(-1);
        return mesh(pts, uv, idx, material, texture);
    }

    // Side wall along a closed outline between z0 (front) and z1 (back).
    // outward = true for an outline whose outside is to the right of its
    // counterclockwise direction (outer edge), false for a hole.
    static SoSeparator* wall(const std::vector<SbVec2f>& loop, float z0, float z1, bool outward,
                             SoMaterial* material, SoTexture2* texture) {
        size_t n = loop.size();
        std::vector<SbVec3f> pts;
        std::vector<SbVec2f> uv;
        double run = 0.0;
        for (size_t i = 0; i <= n; ++i) {   // the first point is repeated so the texture wraps cleanly
            const SbVec2f& v = loop[i % n];
            if (i > 0) run += (v - loop[i - 1]).length();
            pts.emplace_back(v[0], v[1], z0);
            pts.emplace_back(v[0], v[1], z1);
            uv.emplace_back(float(run / 0.04), 0.0f);
            uv.emplace_back(float(run / 0.04), 1.0f);
        }
        std::vector<int> idx;
        for (size_t i = 0; i < n; ++i) {
            int f0 = int(2 * i), b0 = f0 + 1, f1 = f0 + 2, b1 = f0 + 3;
            if (outward) idx.insert(idx.end(), {f0, b0, b1, f1, -1});
            else idx.insert(idx.end(), {f0, f1, b1, b0, -1});
        }
        return mesh(pts, uv, idx, material, texture);
    }

    // Soft glow around the halo: a band of radius r and half width w whose
    // opacity is largest on the ring and fades to nothing at both edges.
    // Drawn unlit and transparent, without casting shadows.
    SoSeparator* glowBand(double cx, double cy, double r, double w, float z) {
        const int N = 240;
        const double radii[5] = {r - w, r - 0.4 * w, r, r + 0.4 * w, r + w};
        const float opacity[5] = {0.0f, 0.25f, 0.65f, 0.25f, 0.0f};
        SoSeparator* sep = new SoSeparator;
        SoShadowStyle* noShadow = new SoShadowStyle;
        noShadow->style = SoShadowStyle::NO_SHADOWING;
        sep->addChild(noShadow);
        SoLightModel* unlit = new SoLightModel;
        unlit->model = SoLightModel::BASE_COLOR;
        sep->addChild(unlit);
        _glow = new SoMaterial;
        _glow->diffuseColor.setNum(5 * N);
        _glow->transparency.setNum(5 * N);
        std::vector<SbVec3f> pts;
        for (int i = 0; i < N; ++i) {
            double a = 2 * M_PI * i / N;
            for (int k = 0; k < 5; ++k) {
                pts.emplace_back(float(cx + radii[k] * std::cos(a)), float(cy + radii[k] * std::sin(a)), z);
                _glow->diffuseColor.set1Value(5 * i + k, SbColor(1.0f, 0.80f, 0.30f));
                _glow->transparency.set1Value(5 * i + k, 1.0f - opacity[k]);
                _glowOpacity.push_back(opacity[k]);
            }
        }
        sep->addChild(_glow);
        SoMaterialBinding* binding = new SoMaterialBinding;
        binding->value = SoMaterialBinding::PER_VERTEX_INDEXED;
        sep->addChild(binding);
        SoCoordinate3* coords = new SoCoordinate3;
        coords->point.setValues(0, int(pts.size()), pts.data());
        sep->addChild(coords);
        std::vector<int> idx;
        for (int i = 0; i < N; ++i) {
            int j = (i + 1) % N;
            for (int k = 0; k < 4; ++k)   // quad between radius k and k+1, counterclockwise from the front
                idx.insert(idx.end(), {5 * i + k, 5 * i + k + 1, 5 * j + k + 1, 5 * j + k, -1});
        }
        SoIndexedFaceSet* faces = new SoIndexedFaceSet;
        faces->coordIndex.setValues(0, int(idx.size()), idx.data());
        faces->materialIndex.setValues(0, int(idx.size()), idx.data());
        sep->addChild(faces);
        return sep;
    }

    // The photo as a texture, very lightly mixed with wood grain so it looks printed on plywood
    static SoTexture2* photoTexture(const QImage& photo) {
        int w = photo.width(), h = photo.height();
        std::vector<double> streak = tools::fibreStreaks(256, 11);
        std::vector<unsigned char> pixels(size_t(3) * w * h);
        for (int y = 0; y < h; ++y) {
            const uchar* row = photo.constScanLine(h - 1 - y);   // texture rows run bottom to top
            for (int x = 0; x < w; ++x) {
                double grain = tools::woodShade(std::fmod(y / 300.0, 1.0), std::fmod(x / 900.0, 1.0), tools::BIRCH, streak);
                double k = 0.93 + 0.07 * grain;
                for (int c = 0; c < 3; ++c)
                    pixels[3 * (size_t(y) * w + x) + c] = (unsigned char)std::clamp(row[3 * x + c] * k, 0.0, 255.0);
            }
        }
        SoTexture2* texture = new SoTexture2;
        texture->image.setValue(SbVec2s(short(w), short(h)), 3, pixels.data());
        texture->model = SoTexture2::MODULATE;
        return texture;
    }

    SoSeparator* _root = nullptr;
    SoRotation* _tilt = nullptr;
    SoMaterial* _glow = nullptr;
    SoSeparator* _head = nullptr;
    float _haloRadius = 0;
    std::vector<float> _glowOpacity;
    double _inertia = 0, _massHeight = 0, _mass = 0, _haloOffsetX = 0;
    float _haloBottom = 0, _boardBottom = 0, _boardTop = 0, _ringFront = 0, _ringBack = 0;
    size_t _outlineSize = 0;
};

#endif
