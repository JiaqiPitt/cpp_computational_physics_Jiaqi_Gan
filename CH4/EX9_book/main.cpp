#include "Photoreceptor.h"
#include <QApplication>
#include <QFont>
#include <QGuiApplication>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QString>
#include <QStyleHints>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>

const double LAMBDA_MIN = 400.0;
const double LAMBDA_MAX = 700.0;
const int BAND_WIDTH = 1200;
const int BAND_HEIGHT = 120;
const int MARGIN = 60;

double clamp01(double x) {
    return std::min(1.0, std::max(0.0, x));
}

double wavelengthOfColumn(int col) {
    return LAMBDA_MIN + (LAMBDA_MAX - LAMBDA_MIN) * col / (BAND_WIDTH - 1);
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);

    const std::string dir = "../EX8_book/";
    Photoreceptor red(dir + "curve_red.csv");
    Photoreceptor green(dir + "curve_green.csv");
    Photoreceptor blue(dir + "curve_blue.csv");

    std::cout << std::fixed << std::setprecision(2);
    std::cout << " lambda     R     G     B\n";
    for (double lam = LAMBDA_MIN; lam <= LAMBDA_MAX; lam += 25.0) {
        std::cout << std::setw(7) << lam << std::setw(6) << clamp01(red(lam) / 100.0)
                  << std::setw(6) << clamp01(green(lam) / 100.0)
                  << std::setw(6) << clamp01(blue(lam) / 100.0) << "\n";
    }

    const int width = BAND_WIDTH + 2 * MARGIN;
    const int top1 = 70;
    const int top2 = top1 + BAND_HEIGHT + 70;
    const int height = top2 + BAND_HEIGHT + 70;

    QImage image(width, height, QImage::Format_RGB32);
    image.fill(Qt::white);

    for (int col = 0; col < BAND_WIDTH; ++col) {
        double lam = wavelengthOfColumn(col);
        double r = clamp01(red(lam) / 100.0);
        double g = clamp01(green(lam) / 100.0);
        double b = clamp01(blue(lam) / 100.0);
        double m = std::max({r, g, b});
        double rn = m > 0.0 ? r / m : 0.0;
        double gn = m > 0.0 ? g / m : 0.0;
        double bn = m > 0.0 ? b / m : 0.0;

        QRgb raw = qRgb(int(255 * r + 0.5), int(255 * g + 0.5), int(255 * b + 0.5));
        QRgb norm = qRgb(int(255 * rn + 0.5), int(255 * gn + 0.5), int(255 * bn + 0.5));
        for (int row = 0; row < BAND_HEIGHT; ++row) {
            image.setPixel(MARGIN + col, top1 + row, raw);
            image.setPixel(MARGIN + col, top2 + row, norm);
        }
    }

    QPainter painter(&image);
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 16));
    painter.drawText(QRect(0, 10, width, 40), Qt::AlignCenter,
                     "Cone response to monochromatic light, 400-700 nm");
    painter.setFont(QFont("Arial", 12));
    painter.drawText(QRect(MARGIN, top1 - 25, BAND_WIDTH, 20), Qt::AlignLeft,
                     "raw response:  R = red cones, G = green cones, B = blue cones");
    painter.drawText(QRect(MARGIN, top2 - 25, BAND_WIDTH, 20), Qt::AlignLeft,
                     "normalized to the strongest channel at each wavelength");
    painter.drawRect(MARGIN - 1, top1 - 1, BAND_WIDTH + 1, BAND_HEIGHT + 1);
    painter.drawRect(MARGIN - 1, top2 - 1, BAND_WIDTH + 1, BAND_HEIGHT + 1);

    for (int lam = 400; lam <= 700; lam += 10) {
        int x = MARGIN + int((lam - LAMBDA_MIN) / (LAMBDA_MAX - LAMBDA_MIN) * (BAND_WIDTH - 1) + 0.5);
        int tick = (lam % 50 == 0) ? 10 : 5;
        painter.drawLine(x, top1 + BAND_HEIGHT, x, top1 + BAND_HEIGHT + tick);
        painter.drawLine(x, top2 + BAND_HEIGHT, x, top2 + BAND_HEIGHT + tick);
        if (lam % 50 == 0) {
            painter.drawText(QRect(x - 30, top1 + BAND_HEIGHT + 12, 60, 20), Qt::AlignCenter, QString::number(lam));
            painter.drawText(QRect(x - 30, top2 + BAND_HEIGHT + 12, 60, 20), Qt::AlignCenter, QString::number(lam));
        }
    }
    painter.drawText(QRect(0, height - 28, width, 20), Qt::AlignCenter, "wavelength (nm)");
    painter.end();

    image.save("color_band.png");
    std::cout << "\nwrote color_band.png\n";

    if (argc > 1 && std::string(argv[1]) == "show") {
        QLabel label;
        label.setPixmap(QPixmap::fromImage(image));
        label.setWindowTitle("Color band 400-700 nm");
        label.show();
        return app.exec();
    }
    return 0;
}