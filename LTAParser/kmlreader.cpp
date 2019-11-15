#include "kmlreader.h"
#include <QXmlStreamReader>
#include <QFile>
#include <QtDebug>
#include <iostream>
#include "UTM.h"


#define myqDebug() qDebug() << fixed << qSetRealNumberPrecision(8)

KmlReader::KmlReader()
{

}


void KmlReader::readXml(const QString& fileName){
    QFile file(fileName);
    if(!file.open(QFile::ReadOnly | QFile::Text)){
        qDebug() << "Cannot read file" << file.errorString();
        return;
    }

    doc = new tinyxml2::XMLDocument;
    doc->LoadFile(fileName.toUtf8());
    _placemark = doc->FirstChildElement("kml") ->FirstChildElement("Document")->FirstChildElement("Folder")->FirstChildElement("Placemark");

    while ( _placemark != 0 ){
        _name = _placemark->FirstChildElement("name");
        _multigeometry = _placemark->FirstChildElement("MultiGeometry");
        if (_multigeometry == 0){
            _coord = _placemark->FirstChildElement("LineString");
            if(_coord != 0){
                _coord = _coord->FirstChildElement("coordinates");
                appendCoord();
            }
        }
        else{

            _linestring = _multigeometry->FirstChildElement("LineString");

            while(_linestring != 0){
                _coord = _linestring ->FirstChildElement("coordinates");
                appendCoord();
                _linestring = _linestring->NextSiblingElement("LineString");
            }
        }
        _placemark = _placemark->NextSiblingElement("Placemark");
    }

//    printCoord(coordinate);
//    printRoadcoord();
}

void KmlReader::appendCoord()
{
    Coordinate temp_coord;
    QString s = _coord->GetText();
    QStringList coord = s.split(" ");

    for(QString data:coord){
        double lat = data.split(",").at(1).toDouble();
        double lon = data.split(",").at(0).toDouble();

        coordinate.LAT.push_back(lat);
        coordinate.LON.push_back(lon);
        temp_coord.LAT.push_back(lat);
        temp_coord.LON.push_back(lon);


        double utmx;
        double utmy;
        int zone=0;
        LatLonToUTMXY(lat,lon, zone, utmx, utmy);

        coordinate.UTMX.push_back(utmx);
        coordinate.UTMY.push_back(utmy);
        temp_coord.UTMX.push_back(utmx);
        temp_coord.UTMY.push_back(utmy);

        roadCoordinate_UTMX.push_back(utmx);
        roadCoordinate_UTMY.push_back(utmy);

    }
    roadCoordinate.push_back(temp_coord);

}



void KmlReader::printCoord(Coordinate &coord)
{
    qDebug() << " ..........................";
    for(int i =0; i < coord.LAT.length(); i++){
        myqDebug() << "LAT: " << coord.LAT.at(i) << "\tLON: " << coord.LON.at(i)<< "\tUTMX: " << coord.UTMX.at(i)<< "\tUTMY: " << coord.UTMY.at(i);
    }
}

void KmlReader::printRoadcoord()
{
    int count =0;
    for(Coordinate coord:roadCoordinate){
        printCoord(coord);
        count += 1;
        if(count >20) break;
    }

}

