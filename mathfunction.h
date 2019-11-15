#ifndef MATHFUNCTION_H
#define MATHFUNCTION_H

#include <QVector>
#include <math.h>
#include "LTAParser/kmlreader.h"

class MathFunction
{
public:
    MathFunction();

    void rotationMatrix(double &x, double &y, double angle);

    QVector<double> calculateHeading(QVector<double> &x, QVector<double> &y);

    Coordinate linearinterpolation( QVector<double> &xData, QVector<double> &yData, int nPoints);

    double normalizeAngle(double angle);

    double reverseAngle(double angle);
};

#endif // MATHFUNCTION_H
