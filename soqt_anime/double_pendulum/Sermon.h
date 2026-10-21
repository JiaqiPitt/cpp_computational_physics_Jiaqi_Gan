#ifndef SERMON_H
#define SERMON_H
#include <QString>
#include <vector>

// The Sermon on the Pendulum: what the dog jesus teacher preaches, one
// speech bubble per line, shown in turn and then from the start again.
// Each line is simple HTML (<br> starts a new row in the bubble) with the
// number of seconds it stays up.  Edit this list to change the script.
struct SermonLine {
    QString html;
    double seconds;
};

inline std::vector<SermonLine> sermon() {
    auto gold = [](const char* s) { return "<span style=\"color:#B5651D\"><b>" + QString::fromUtf8(s) + "</b></span>"; };
    auto teal = [](const char* s) { return "<span style=\"color:#00897B\"><b>" + QString::fromUtf8(s) + "</b></span>"; };
    auto red = [](const char* s) { return "<span style=\"color:#C62828\"><b>" + QString::fromUtf8(s) + "</b></span>"; };
    return {
        {"In the beginning<br>was the " + gold("Lagrangian") + ".", 4.0},                                     // John 1:1
        {"And the Lagrangian was " + gold("T − V") + ",<br>and it was good.", 4.5},                            // Genesis 1
        {"Blessed are the " + teal("small angles") + ",<br>for they shall be linear.", 4.5},                   // Matthew 5; sin x ~ x
        {"Then came a second rod,<br>and there was " + red("chaos") + ".", 4.0},
        {"Let him who is without " + teal("sin θ") + "<br>cast the first " + gold("biscuit") + ".", 5.0},       // John 8:7
        {"I can walk on water.<br>I cannot predict this pendulum.", 4.5},
        {"Fear not! I bring you good tidings:<br>" + teal("energy is conserved") + ".", 5.0},                  // Luke 2:10
        {"Blessed are the " + gold("treats") + ",<br>for they are sensitive<br>to " + teal("initial conditions") + ".", 5.5},
        {"Go forth and " + gold("integrate") + ".<br>(RK4, dt = 1/1200 s)", 4.5},                             // Genesis 1:28
        {"Peace be with you.<br>" + red("Homework") + " is due Friday.", 4.5},
    };
}

#endif
