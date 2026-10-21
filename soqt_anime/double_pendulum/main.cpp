// A small double-pendulum teaching model on a teacher's desk, with the dog
// teacher standing next to it.  Drag with the mouse to turn the view, scroll
// to zoom; close the window to quit.  Run it from this directory so that
// dog_photo.png and dog_mask.png are found.
#include "ClassroomScene.h"
#include "DogBoard.h"
#include "DoublePendulum.h"
#include "SwayingBoard.h"
#include <Inventor/Qt/SoQt.h>
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
#include <Inventor/sensors/SoTimerSensor.h>
#include <QApplication>
#include <QWidget>
#include <cmath>
#include <cstdio>

const double FRAME_TIME      = 1.0 / 60.0;   // one animation frame (s)
const int    STEPS_PER_FRAME = 20;           // RK4 steps per frame, dt = 1/1200 s
const double SWAY_PERIOD     = 1.6;          // period of the dog teacher's rocking (s)
const double SWAY_ANGLE      = 0.07;         // starting tilt of the dog teacher (rad)

struct Animation {
    DoublePendulum* pendulum;
    SwayingBoard* sway;         // nullptr if the dog pictures were not found
    DogBoard* teacher;
    ClassroomScene* scene;
    double energy0;
    double nextReport;
};

// Called by the timer 60 times a second: advance both motions, then move the scene
void tick(void* data, SoSensor*) {
    Animation* a = static_cast<Animation*>(data);
    double dt = FRAME_TIME / STEPS_PER_FRAME;
    for (int i = 0; i < STEPS_PER_FRAME; ++i) {
        a->pendulum->step(dt);
        a->scene->addTrailPoint(*a->pendulum);
        if (a->sway) a->sway->step(dt);
    }
    a->scene->update(*a->pendulum);
    if (a->sway) {
        a->teacher->setTilt(a->sway->angle());
        a->teacher->setGlow(a->pendulum->time());
        a->scene->preach(a->pendulum->time());   // the next line of the sermon when it is due
    }
    if (a->pendulum->time() >= a->nextReport) {
        std::printf("t = %5.1f s   pendulum energy change = %+.2e J\n",
                    a->pendulum->time(), a->pendulum->energy() - a->energy0);
        a->nextReport += 10.0;
    }
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QWidget window;
    window.resize(1100, 800);
    window.setWindowTitle("Double pendulum class");
    SoQt::init(&window);

    DoublePendulum::Parameters params;   // lengths and masses: see DoublePendulum.h
    DoublePendulum pendulum(params, {2.0 * M_PI / 3, 5.0 * M_PI / 6, 0.0, 0.0});

    // Halo in the photo: centre and radius of the ring, and half its width (pixels)
    DogBoard::Halo halo{531.2, 281.3, 263.6, 14.0};
    DogBoard teacher("dog_photo.png", "dog_mask.png", halo, DogBoard::Size());
    ClassroomScene scene(pendulum, &teacher);

    SwayingBoard* sway = nullptr;
    if (teacher.valid()) {
        double k = SwayingBoard::stiffnessForPeriod(teacher.inertia(), teacher.massHeight(), SWAY_PERIOD);
        sway = new SwayingBoard({teacher.inertia(), teacher.massHeight(), k}, SWAY_ANGLE);
        std::printf("dog teacher: outline %d points, mass %.2f kg, I = %.3f kg m^2, M h = %.3f kg m, spring k = %.1f N m/rad\n",
                    teacher.outlinePoints(), teacher.mass(), teacher.inertia(), teacher.massHeight(), k);
    }
    std::printf("pendulum energy E0 = %.5f J\n", pendulum.energy());

    Animation animation{&pendulum, sway, &teacher, &scene, pendulum.energy(), 10.0};
    SoTimerSensor timer(tick, &animation);
    timer.setInterval(FRAME_TIME);
    timer.schedule();

    SoQtExaminerViewer* viewer = new SoQtExaminerViewer(&window);
    viewer->setSceneGraph(scene.root());
    viewer->setBackgroundColor(ClassroomScene::background());
    viewer->setHeadlight(FALSE);   // the room has its own lights
    viewer->setTransparencyType(SoGLRenderAction::SORTED_OBJECT_BLEND);   // fading trail
    viewer->show();

    SoQt::show(&window);
    SoQt::mainLoop();

    timer.unschedule();
    delete viewer;
    delete sway;
    return 0;
}
