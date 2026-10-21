#ifndef SPEECHBUBBLE_H
#define SPEECHBUBBLE_H
#include <QColor>
#include <QFont>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QString>
#include <QTextDocument>
#include <QTextOption>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/VRMLnodes/SoVRMLBillboard.h>
#include <Inventor/annex/FXViz/nodes/SoShadowStyle.h>
#include <algorithm>
#include <vector>

// A comic speech bubble.  Qt draws the bubble and its text (simple HTML, so
// words can have their own colours) into an image whose outside is
// transparent; the image is shown on a flat card that always turns to face
// the viewer (SoVRMLBillboard).
namespace bubble {

// Bubble centred at the origin of the card, just big enough for its text but
// at most maxWidth x maxHeight metres (the font shrinks only if the text does
// not fit even then); its tail points at 'tip' (metres, relative to the
// centre, in the plane of the card).
inline SoSeparator* card(const QString& html, const QFont& font, float maxWidth, float maxHeight, const SbVec2f& tip) {
    const float ppm = 2000.0f;   // pixels per metre on the image
    const float pad = 0.02f;     // room for the outline

    // lay out the text first
    QTextDocument doc;
    QTextOption noWrap;
    noWrap.setWrapMode(QTextOption::NoWrap);   // lines break only where the text says so
    doc.setDefaultTextOption(noWrap);
    QFont fitted(font);
    for (int tries = 0; tries < 40; ++tries) {
        doc.setDefaultFont(fitted);
        doc.setHtml(html);
        doc.setTextWidth(-1);
        if (doc.idealWidth() <= maxWidth * ppm * 0.88 && doc.size().height() <= maxHeight * ppm * 0.84) break;
        fitted.setPointSizeF(fitted.pointSizeF() * 0.95);
    }
    float width = std::clamp(float(doc.idealWidth() / ppm / 0.88), 0.45f, maxWidth);
    float height = std::clamp(float(doc.size().height() / ppm / 0.80), 0.20f, maxHeight);

    float x0 = std::min(-width / 2, tip[0]) - pad, x1 = std::max(width / 2, tip[0]) + pad;
    float y0 = std::min(-height / 2, tip[1]) - pad, y1 = std::max(height / 2, tip[1]) + pad;
    int w = int((x1 - x0) * ppm), h = int((y1 - y0) * ppm);
    auto px = [&](float x, float y) { return QPointF((x - x0) * ppm, (y1 - y) * ppm); };   // metres -> image

    QImage image(w, h, QImage::Format_RGBA8888);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // body with rounded corners, joined to a tail whose base sits on the bottom edge
    QPainterPath shape;
    shape.addRoundedRect(QRectF(px(-width / 2, height / 2), QSizeF(width * ppm, height * ppm)), 0.07 * ppm, 0.07 * ppm);
    float baseX = std::clamp(tip[0], -width / 2 + 0.12f, width / 2 - 0.12f);
    QPainterPath tail;
    tail.addPolygon(QPolygonF() << px(baseX - 0.07f, -height / 2 + 0.03f) << px(tip[0], tip[1]) << px(baseX + 0.05f, -height / 2 + 0.03f));
    tail.closeSubpath();
    painter.setPen(QPen(QColor("#2B2B2B"), 0.008 * ppm, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(QColor("#FFFDF6"));
    painter.drawPath(shape.united(tail).simplified());

    doc.setTextWidth(width * ppm * 0.92);
    QPointF centreLeft = px(-width / 2 * 0.92f, 0.0f);
    painter.translate(centreLeft.x(), centreLeft.y() - doc.size().height() / 2);
    doc.drawContents(&painter);
    painter.end();

    std::vector<unsigned char> pixels(size_t(4) * w * h);   // texture rows run bottom to top
    for (int y = 0; y < h; ++y)
        std::copy(image.constScanLine(h - 1 - y), image.constScanLine(h - 1 - y) + 4 * w, pixels.begin() + size_t(4) * w * y);

    SoSeparator* sep = new SoSeparator;
    SoShadowStyle* noShadow = new SoShadowStyle;
    noShadow->style = SoShadowStyle::NO_SHADOWING;
    sep->addChild(noShadow);
    SoLightModel* unlit = new SoLightModel;   // a drawing, not an object in the light
    unlit->model = SoLightModel::BASE_COLOR;
    sep->addChild(unlit);
    SoMaterial* white = new SoMaterial;
    white->diffuseColor.setValue(1, 1, 1);
    sep->addChild(white);
    SoTexture2* texture = new SoTexture2;
    texture->image.setValue(SbVec2s(short(w), short(h)), 4, pixels.data());
    texture->model = SoTexture2::MODULATE;
    sep->addChild(texture);
    SoShapeHints* hints = new SoShapeHints;
    hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    sep->addChild(hints);
    SoCoordinate3* corners = new SoCoordinate3;
    corners->point.set1Value(0, x0, y0, 0);
    corners->point.set1Value(1, x1, y0, 0);
    corners->point.set1Value(2, x1, y1, 0);
    corners->point.set1Value(3, x0, y1, 0);
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

// The bubble placed 'offset' from an anchor (metres), tail pointing back at
// 'tipFromAnchor', turning to face the viewer.
inline SoSeparator* at(const SbVec3f& offset, const SbVec2f& tipFromAnchor, float maxWidth, float maxHeight,
                       const QString& html, const QFont& font) {
    SoSeparator* sep = new SoSeparator;
    SoTranslation* where = new SoTranslation;
    where->translation = offset;
    sep->addChild(where);
    SoVRMLBillboard* facing = new SoVRMLBillboard;   // axis (0,0,0): always faces the viewer
    facing->addChild(card(html, font, maxWidth, maxHeight, tipFromAnchor - SbVec2f(offset[0], offset[1])));
    sep->addChild(facing);
    return sep;
}

// A line of text centred in the bubble, in dark ink
inline QString wrapCentred(const QString& html) {
    return "<div style=\"color:#262626\"><p align=\"center\" style=\"margin:0\">" + html + "</p></div>";
}

// Several bubbles at the same place, one shown at a time.  'show' selects the
// bubble (whichChild) and 'pop' scales it, so the caller can switch lines and
// let each new bubble pop up.
inline SoSeparator* sequence(const SbVec3f& offset, const SbVec2f& tipFromAnchor, float maxWidth, float maxHeight,
                             const std::vector<QString>& lines, const QFont& font, SoSwitch*& show, SoScale*& pop) {
    SoSeparator* sep = new SoSeparator;
    SoTranslation* where = new SoTranslation;
    where->translation = offset;
    sep->addChild(where);
    SoVRMLBillboard* facing = new SoVRMLBillboard;   // axis (0,0,0): always faces the viewer
    pop = new SoScale;
    facing->addChild(pop);
    show = new SoSwitch;
    for (const QString& line : lines)
        show->addChild(card(wrapCentred(line), font, maxWidth, maxHeight, tipFromAnchor - SbVec2f(offset[0], offset[1])));
    show->whichChild = 0;
    facing->addChild(show);
    sep->addChild(facing);
    return sep;
}

}  // namespace bubble
#endif
