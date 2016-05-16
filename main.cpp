#include "device_tree_window.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Device_tree_window w;

    w.show();

    return a.exec();
}
