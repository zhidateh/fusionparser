#include "fusionparser.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    FusionParser w;
    w.show();

    return a.exec();
}
