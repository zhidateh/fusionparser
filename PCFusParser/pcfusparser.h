#ifndef PCFUSPARSER_H
#define PCFUSPARSER_H

#include <QFile>
#include <QMessageBox>
#include <QMap>
#include <QTextStream>
#include <QDebug>
#include "../mathfunction.h"

struct Actor{
    QVector<unsigned long long int> timestamp;
    //QVector<int> ego_index;
    QString objectClass;
    QVector<double> state_x;
    QVector<double> state_y;
    QVector<double> state_z;
    QVector<double> hdg;
    QVector<double> pitch;
    QVector<double> roll;
};


class PCFusParser
{
public:
    PCFusParser();
    QMap<QString,Actor> actors;
    double pcFPS = 50000.0;
    void setObjectClasses(QStringList objClasses);
    void readPCFus(QString path);
    void fillEgoTimestamp();
    void processAll();

    //the range of all trajectory
    double x_lower=9999999;
    double x_upper=0;
    double y_lower=9999999;
    double y_upper=0;

    //middle of all trajectory
    double x_mid;
    double y_mid;

private:
    MathFunction mathFunction;
    void filterWaypoint();
    void assignAVBus();
    void interpolateAll();
    void transformTimestamp();
    void transformWaypoint();

    //store classes inherited from fusion parser class
    QStringList objectClasses;

    //classification from Perception fusion algo
    QMap<QString, QString> map_of_objectclasses = { {"5"    ,"Car"},
                                                    {"6"    ,"Tru"},
                                                    {"17"   ,"Bic"},
                                                    {"3"    ,"Ped"}};

};

#endif // PCFUSPARSER_H
