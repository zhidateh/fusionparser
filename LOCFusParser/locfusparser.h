#ifndef LOCFUSPARSER_H
#define LOCFUSPARSER_H

#include "../PCFusParser/pcfusparser.h"

class LOCFusParser
{
public:
    LOCFusParser();
    QMap<QString,Actor> actors;

    void readLOCFus(QString path);
    void matchEgoTimestamp(Actor *actor,double fps);

    //the range of all trajectory
    double x_lower=9999999;
    double x_upper=0;
    double y_lower=9999999;
    double y_upper=0;

private:
    MathFunction mathFunction;


};

#endif // LOCFUSPARSER_H
