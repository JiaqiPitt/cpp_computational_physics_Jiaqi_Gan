// Exercise 11.5 as an animation: a 3D top that spins, precesses and nutates,
// solved with the same equations as ../main.cpp and shown in slow motion.
// The buttons choose how the top is let go.  Drag to turn the view, scroll
// to zoom.
#include "Top.h"
#include "TopScene.h"
#include <Inventor/Qt/SoQt.h>
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
#include <Inventor/sensors/SoTimerSensor.h>
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <cmath>

const double FRAME = 1.0 / 60.0;   // screen time per frame (s)
const double SLOW = 20.0;          // slow motion: 1 s of the top's time takes 20 s
const int STEPS = 10;              // RK4 steps per frame

// The three ways of letting go, as in ../main.cpp
struct Release {
    const char* name;
    double theta0, phidot0;
};
const Release RELEASES[3] = {
    {"Let go at rest (cusps)", 30 * M_PI / 180, 0.0},
    {"Push with the precession (waves)", 50 * M_PI / 180, 2.0},
    {"Push against it (loops)", 70 * M_PI / 180, -2.0},
};

struct Animation {
    Top top;
    TopScene* scene;
    QLabel* status;
    int release = 0;
    bool paused = false;

    void start(int k) {
        release = k;
        top.start(RELEASES[k].theta0, RELEASES[k].phidot0);
        scene->clearTrail(top);
        paused = false;
    }
};

// Called by the timer 60 times a second
void tick(void* data, SoSensor*) {
    Animation* a = static_cast<Animation*>(data);
    if (!a->paused) {
        for (int i = 0; i < STEPS; ++i) a->top.step(FRAME / SLOW / STEPS);
        a->scene->update(a->top);
    }
    a->status->setText(QString("%1     slow motion x1/%2     t = %3 s     theta = %4 deg")
                           .arg(RELEASES[a->release].name)
                           .arg(SLOW)
                           .arg(a->top.time(), 0, 'f', 3)
                           .arg(a->top.theta() * 180 / M_PI, 0, 'f', 1));
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QWidget window;
    window.setWindowTitle("Spinning top");
    window.resize(900, 800);
    SoQt::init(&window);

    TopScene scene;
    QVBoxLayout* layout = new QVBoxLayout(&window);
    SoQtExaminerViewer* viewer = new SoQtExaminerViewer(&window);
    viewer->setSceneGraph(scene.root());
    viewer->setBackgroundColor(SbColor(0.95f, 0.95f, 0.94f));
    viewer->setTransparencyType(SoGLRenderAction::SORTED_OBJECT_BLEND);   // fading trail
    layout->addWidget(viewer->getWidget(), 1);

    QLabel* status = new QLabel;
    Animation animation{Top(), &scene, status};
    QHBoxLayout* buttons = new QHBoxLayout;
    for (int k = 0; k < 3; ++k) {
        QPushButton* b = new QPushButton(RELEASES[k].name);
        QObject::connect(b, &QPushButton::clicked, [&animation, k] { animation.start(k); });
        buttons->addWidget(b);
    }
    QPushButton* pause = new QPushButton("Pause");
    QObject::connect(pause, &QPushButton::clicked, [&animation, pause] {
        animation.paused = !animation.paused;
        pause->setText(animation.paused ? "Resume" : "Pause");
    });
    buttons->addWidget(pause);
    layout->addLayout(buttons);
    layout->addWidget(status);

    animation.start(0);
    SoTimerSensor timer(tick, &animation);
    timer.setInterval(FRAME);
    timer.schedule();

    SoQt::show(&window);
    SoQt::mainLoop();

    timer.unschedule();
    delete viewer;
    return 0;
}