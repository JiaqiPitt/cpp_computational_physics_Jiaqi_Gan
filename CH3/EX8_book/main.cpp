#include "TransmissionKa.h"
#include "QatPlotWidgets/PlotView.h"
#include "QatPlotting/PlotFunction1D.h"
#include <QApplication>
#include <iostream>

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    PRectF rect;
    rect.setXmin(0.0);
    rect.setXmax(5.0);
    rect.setYmin(0.0);
    rect.setYmax(1.05);

    PlotView view(rect);

    TransmissionKa f(1.0, 0.3);
    PlotFunction1D P(f);
    view.add(&P);
    view.save("transmission_vs_ka.png");

    std::cout << "wrote transmission_vs_ka.png\n";
    return 0;
}