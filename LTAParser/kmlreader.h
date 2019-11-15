#ifndef KMLREADER_H
#define KMLREADER_H

#include <QVector>
#include <QString>
#include <QStringList>
#include "tinyxml2.h"

struct Coordinate
{
    QVector<double> LAT;
    QVector<double> LON;
    QVector<double> UTMX;
    QVector<double> UTMY;
};


class KmlReader
{
public:
    KmlReader();
    Coordinate coordinate;
    QVector<Coordinate> roadCoordinate;


    void readXml(const QString& fileName);
    QVector<double> roadCoordinate_UTMX;
    QVector<double> roadCoordinate_UTMY;

private:
    //essential
    void appendCoord();


    //low priority
    void printCoord(Coordinate &coord);
    void printRoadcoord();


    //XML Parser
    tinyxml2::XMLDocument* doc;
    tinyxml2::XMLElement* _placemark;
    tinyxml2::XMLElement* _name;
    tinyxml2::XMLElement* _multigeometry;
    tinyxml2::XMLElement* _linestring;
    tinyxml2::XMLElement* _coord;

};

#endif // KMLREADER_H
