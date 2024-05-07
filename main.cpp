#include "MediaPushWindow.h"
#include <QtWidgets/QApplication>
#include "AudioCapture.h"
#include <QDebug>



int main(int argc, char *argv[])
{




    QApplication a(argc, argv);
    MediaPushWindow w;
    w.resize(800, 600);
    w.show();

    return a.exec();
}
