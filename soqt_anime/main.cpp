#include <Inventor/Qt/SoQt.h>
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
// Coin includes
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoRotor.h>
#include <QApplication>
#include <QWidget>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    QWidget mainwindow;
    mainwindow.resize(500, 500);
    SoQt::init(&mainwindow);

    // Main node of the scene graph
    SoSeparator* root = new SoSeparator;
    root->ref();
    // Colour of the cone
    SoMaterial* material = new SoMaterial;
    material->diffuseColor.setValue(0.9f, 0.4f, 0.1f);    // orange
    material->specularColor.setValue(0.6f, 0.6f, 0.6f);
    material->shininess = 0.5f;
    root->addChild(material);

    SoRotor* rotor = new SoRotor;
    rotor->rotation.setValue(SbVec3f(0, 1, 0), 0);   
    rotor->speed = 0.2f;                             
    root->addChild(rotor);   

    // The cone: base radius 1, height 2, pointing along +y
    SoCone* cone = new SoCone;
    cone->bottomRadius = 1.0f;
    cone->height = 10.0f;
    root->addChild(cone);

    // Interactive viewer: drag to rotate, scroll to zoom
    SoQtExaminerViewer* ev = new SoQtExaminerViewer(&mainwindow);
    ev->setSceneGraph(root);
    ev->setTitle("Cone");
    ev->show();

    SoQt::show(&mainwindow);
    SoQt::mainLoop();

    delete ev;
    root->unref();
    return 0;
}