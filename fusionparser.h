#ifndef FUSIONPARSER_H
#define FUSIONPARSER_H

#include <QMainWindow>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QDebug>
#include <QMap>
#include <QThread>
#include <QtConcurrent/QtConcurrent>
#include "mathfunction.h"
#include "qcustomplot.h"
#include "LTAParser/kmlreader.h"
#include "PCFusParser/pcfusparser.h"
#include "LOCFusParser/locfusparser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>

//struct Actor{
//    QVector<unsigned long long int> timestamp;
//    //QVector<int> ego_index;
//    QVector<double> state_x;
//    QVector<double> state_y;
//    QVector<double> state_z;
//    QVector<double> hdg;
//};


namespace Ui {
class FusionParser;
}

class FusionParser : public QMainWindow
{
    Q_OBJECT

public:
    explicit FusionParser(QWidget *parent = 0);
    ~FusionParser();

private slots:
    void on_btn_browse_pc_clicked();

    void on_btn_next_clicked();

    void on_btn_previous_clicked();

    void clickedGraph(QMouseEvent *event);

    void graphClicked(QCPAbstractPlottable*,int);

    void on_btn_delete_clicked();

    void on_btn_plotall_clicked();

    void on_slider_play_valueChanged(int value);

    void on_btn_save_clicked();

    void on_btn_process_clicked();

    void on_btn_browse_lta_clicked();

    void on_btn_browse_loc_clicked();

    void on_cb_loc_clicked(bool checked);

    void on_cb_pc_clicked(bool checked);

    void on_cb_lta_clicked(bool checked);

    void on_slider_right_play_valueChanged(int value);

    void on_slider_left_play_valueChanged(int value);

    void on_gb_viewer_toggled(bool arg1);

    void on_btn_clear_clicked();

    void on_slider_camera_valueChanged(int value);

    void on_btn_play_clicked();


public slots:

private slots:

    void on_btn_pause_clicked();

    void on_btn_stop_clicked();

    void on_btn_plotlta_clicked();

    void on_btn_plotpcfus_clicked();

    void on_btn_plotlocfus_clicked();


private:
    Ui::FusionParser *ui;
    MathFunction mathFunction;
    KmlReader *kmlReader = new KmlReader();
    PCFusParser *pcFusParser = new PCFusParser();
    LOCFusParser *locFusParser = new LOCFusParser();


    bool stopPlayingFlag_ = false;

    void initialise();
    void plotPCFus();
    void plotLOCFus();
    void plotLTA(double left, double right, double btm, double top,bool isDrawn);
    void exportXML(QString save_dir);
    void playView();
    void plotBoundingBox(QString id,double &center_x, double &center_y, double &hdg);
    void smoothHeading();

    int nGraphDrawn =0;

    QString locPath;
    QString pcPath;
    QString ltaPath;

    //MODV size: 7.44, 2.29
    QMap<QString,std::pair<float,float>> classSize = {  {"AVBus", std::make_pair(7.44,2.29)},
                                                        {"Car"  , std::make_pair(4.8,2.4)},
                                                        {"Tru"  ,std::make_pair(7.5,2.35)},
                                                        {"Bic"  ,std::make_pair(1.7, 0.7)},
                                                        {"Ped"  ,std::make_pair(0.25,0.55)} };
    QMap<QString,QPen> actorsColor;
    QMap<QString,Actor> actors;
    int actors_iter= -1;

    double pcFPS = 50000.0; //20Hz

    //the range of all trajectory
    double x_lower=9999999;
    double x_upper=0;
    double y_lower=9999999;
    double y_upper=0;

    //middle of all trajectory
    double x_mid;
    double y_mid;

};

#endif // FUSIONPARSER_H
