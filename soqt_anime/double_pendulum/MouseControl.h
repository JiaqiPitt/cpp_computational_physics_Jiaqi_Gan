#ifndef MOUSECONTROL_H
#define MOUSECONTROL_H
#include "ClassroomScene.h"
#include "DogBoard.h"
#include "DoublePendulum.h"
#include "SwayingBoard.h"
#include <Inventor/SbLine.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SbViewVolume.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/nodes/SoEventCallback.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <deque>

// MouseControl !!!
struct MouseControl {
    DoublePendulum* pendulum;
    ClassroomScene* scene;
    DogBoard* teacher = nullptr;   // the dog teacher and his rocking (nullptr if he is missing)
    SwayingBoard* sway = nullptr;

    enum Held { NOTHING = 0, BISCUIT_1 = 1, BISCUIT_2 = 2, TEACHER = 3 };
    int holding = NOTHING;
    double energy0 = 0.0;     // energy of the pendulum when it was last let go
    double clock = 0.0;       // animation time (s); keeps running while something is held
    double grabAngle = 0.0;   // direction from the dog's pivot to the mouse when he was picked up
    double grabTilt = 0.0;    // his tilt at that moment

    // How easy it is to catch the moving pendulum: a press counts if it is
    // within GRAB_RADIUS of a biscuit, or of where the biscuit was during the
    // last HISTORY frames (people click where they saw it a moment ago).
    // With 15 cm rods this also covers rod 2 and most of rod 1.
    static constexpr double GRAB_RADIUS = 0.10;   // m
    static constexpr size_t HISTORY = 15;         // frames, a quarter of a second
    static constexpr double MAX_TILT = 0.35;      // largest tilt of the dog teacher (rad), 20 degrees
    std::deque<SbVec2f> recentElbow, recentTip;  // recent biscuit positions, relative to the pivot

    // Call once per frame: remember where the biscuits are
    void remember() {
        recentElbow.push_back(SbVec2f(float(pendulum->elbowX()), float(pendulum->elbowY())));
        recentTip.push_back(SbVec2f(float(pendulum->tipX()), float(pendulum->tipY())));
        if (recentElbow.size() > HISTORY) recentElbow.pop_front();
        if (recentTip.size() > HISTORY) recentTip.pop_front();
    }

    // Adds the event handler to the scene
    void attach() {
        SoEventCallback* events = new SoEventCallback;
        events->addEventCallback(SoMouseButtonEvent::getClassTypeId(), handle, this);
        events->addEventCallback(SoLocation2Event::getClassTypeId(), handle, this);
        events->addEventCallback(SoKeyboardEvent::getClassTypeId(), handle, this);
        scene->root()->addChild(events);
    }

    static void handle(void* data, SoEventCallback* callback) {
        MouseControl* m = static_cast<MouseControl*>(data);
        const SoEvent* event = callback->getEvent();

        // next line of the sermon from the keyboard
        if (SO_KEY_PRESS_EVENT(event, N) || SO_KEY_PRESS_EVENT(event, RIGHT_ARROW)) {
            m->scene->nextLine(m->clock);
            callback->setHandled();
            return;
        }

        if (SO_MOUSE_PRESS_EVENT(event, BUTTON1)) m->press(callback);
        else if (SO_MOUSE_RELEASE_EVENT(event, BUTTON1) && m->holding) m->release(callback);
        else if (event->isOfType(SoLocation2Event::getClassTypeId()) && m->holding) m->drag(callback);
    }

    void press(SoEventCallback* callback) {
        const SoPickedPoint* picked = callback->getPickedPoint();
        // the speech bubble: next line
        if (picked && picked->getPath()->containsNode(scene->speechBubbles())) {
            scene->nextLine(clock);
            callback->setHandled();
            return;
        }
        // a biscuit: pick it up (it jumps to the mouse)
        double x, y;
        if (pointInPlane(callback, scene->swingPlaneZ(), scene->pivotWorld(), x, y)) {
            int which = biscuitAt(SbVec2f(float(x), float(y)));
            if (which != NOTHING) {
                holding = which;
                moveBiscuit(x, y);
                callback->setHandled();
                return;
            }
        }
        // the dog teacher: hold him where the mouse is
        if (teacher && sway && picked && picked->getPath()->containsNode(scene->teacherNode())) {
            SbVec3f pivot = scene->teacherPivotWorld();
            if (pointInPlane(callback, pivot[2], pivot, x, y)) {
                holding = TEACHER;
                grabAngle = std::atan2(y, x);   // direction from the pivot to the mouse
                grabTilt = sway->angle();
                sway->setState(grabTilt, 0.0);
                callback->setHandled();
            }
        }
    }

    void drag(SoEventCallback* callback) {
        double x, y;
        if (holding == TEACHER) {
            SbVec3f pivot = scene->teacherPivotWorld();
            if (!pointInPlane(callback, pivot[2], pivot, x, y)) return;
            // turn the board by as much as the mouse has turned about the pivot
            double turn = std::remainder(std::atan2(y, x) - grabAngle, 2 * M_PI);
            double tilt = std::clamp(grabTilt + turn, -MAX_TILT, MAX_TILT);
            sway->setState(tilt, 0.0);
            teacher->setTilt(tilt);
        } else {
            if (!pointInPlane(callback, scene->swingPlaneZ(), scene->pivotWorld(), x, y)) return;
            moveBiscuit(x, y);
        }
        callback->setHandled();
    }

    void release(SoEventCallback* callback) {
        if (holding == TEACHER) {
            std::printf("dog teacher let go at %.1f deg: he rocks between -%.1f and +%.1f deg\n",
                        sway->angle() * 180.0 / M_PI, std::fabs(sway->angle()) * 180.0 / M_PI,
                        std::fabs(sway->angle()) * 180.0 / M_PI);
        } else {
            energy0 = pendulum->energy();
            const DoublePendulum::State& s = pendulum->state();
            std::printf("let go at theta1 = %.1f deg, theta2 = %.1f deg, E0 = %.5f J\n",
                        s[0] * 180.0 / M_PI, s[1] * 180.0 / M_PI, energy0);
        }
        holding = NOTHING;
        callback->setHandled();
    }

    int biscuitAt(const SbVec2f& m) const {
        auto distance = [&](const std::deque<SbVec2f>& recent, double x, double y) {
            double d = (m - SbVec2f(float(x), float(y))).length();
            for (const SbVec2f& q : recent) d = std::min(d, double((m - q).length()));
            return d;
        };
        double d1 = distance(recentElbow, pendulum->elbowX(), pendulum->elbowY());
        double d2 = distance(recentTip, pendulum->tipX(), pendulum->tipY());
        if (std::min(d1, d2) >= GRAB_RADIUS) return NOTHING;
        return d2 < d1 ? BISCUIT_2 : BISCUIT_1;
    }

    // The held biscuit follows the mouse: biscuit 1 turns rod 1 about the
    // pivot, biscuit 2 turns rod 2 about the joint; the pendulum is at rest
    void moveBiscuit(double x, double y) {
        DoublePendulum& p = *pendulum;
        DoublePendulum::State s = p.state();
        if (holding == BISCUIT_1) s[0] = std::atan2(x, -y);
        else s[1] = std::atan2(x - p.elbowX(), -(y - p.elbowY()));
        s[2] = s[3] = 0.0;
        p.setState(s);
        recentElbow.clear();   // where it was before does not count any more
        recentTip.clear();
        scene->clearTrail(p);
        scene->update(p);
    }

    // The mouse ray (from the camera through the mouse) meets the plane z =
    // planeZ; returns the point relative to 'origin'
    bool pointInPlane(SoEventCallback* callback, float planeZ, const SbVec3f& origin, double& x, double& y) const {
        const SbViewportRegion& region = callback->getAction()->getViewportRegion();
        SbVec2f mouse = callback->getEvent()->getNormalizedPosition(region);
        SbViewVolume volume = scene->camera()->getViewVolume(region.getViewportAspectRatio());
        SbLine ray;
        volume.projectPointToLine(mouse, ray);
        SbVec3f hit;
        if (!SbPlane(SbVec3f(0, 0, 1), planeZ).intersect(ray, hit)) return false;
        x = hit[0] - origin[0];
        y = hit[1] - origin[1];
        return true;
    }
};

#endif
