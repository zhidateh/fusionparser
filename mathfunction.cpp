#include "mathfunction.h"

MathFunction::MathFunction()
{



}


void MathFunction::rotationMatrix(double &x, double &y, double angle)
{
    double _x = x*cos(angle) - y*sin(angle);
    double _y = x*sin(angle) + y*cos(angle);
    x = _x;
    y = _y;
}

QVector<double> MathFunction::calculateHeading(QVector<double> &x, QVector<double> &y)
{
    QVector<double> _hdg;
    for(int i=0;i < x.length()-1; ++i){
        double _heading = atan2(y.at(i+1) - y.at(i), x.at(i+1) - x.at(i));
        _hdg.push_back(normalizeAngle(_heading));

    }

    //last heading is previous heading
    _hdg.push_back(_hdg.last());

    return _hdg;
}


Coordinate MathFunction::linearinterpolation(QVector<double> &xData, QVector<double> &yData, int nPoints)
{
    Coordinate _coordinate;

    // Interpolate
    for (int i =0; i < xData.length()-1;++i )
    {
        double xL = xData.at(i);
        double xR = xData.at(i+1);
        double yL = yData.at(i);
        double yR = yData.at(i+1);

        double dx = (xR-xL)/nPoints;
        double dy = (yR-yL)/nPoints;

        for(int i =0;i <= nPoints; ++i){
            _coordinate.UTMX.push_back(xL + i*dx);
            _coordinate.UTMY.push_back(yL + i*dy);

        }
    }
    return _coordinate;
}

double MathFunction::normalizeAngle(double angle)
{
    if (angle > M_PI){
        angle -= 2*M_PI;
    }
    else if(angle < -M_PI){
        angle += 2*M_PI;
    }
    return angle;
}

double MathFunction::reverseAngle(double angle)
{
    if (angle > 0){
        angle -= M_PI;
    }
    else if(angle < 0){
        angle += M_PI;
    }
    return angle;
}
