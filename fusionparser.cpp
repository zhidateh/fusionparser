#include "fusionparser.h"
#include "ui_fusionparser.h"

#define myqDebug() qDebug() << fixed << qSetRealNumberPrecision(5)

FusionParser::FusionParser(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::FusionParser)
{
    ui->setupUi(this);
    ui->graph->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    //make a square graph
    ui->graph->yAxis->setScaleRatio(ui->graph->xAxis,1.0);

    on_btn_clear_clicked();

    //set camera range from 10m to 300m
    ui->slider_camera->setRange(10,300);
    ui->slider_camera->setEnabled(true);

    //show the utm of clicked point on status bar
    connect(ui->graph, SIGNAL(mousePress(QMouseEvent*)), SLOT(clickedGraph(QMouseEvent*)));

    //show the "id" of plot in delete textbox
    connect(ui->graph, SIGNAL(plottableClick(QCPAbstractPlottable*,int,QMouseEvent*)), this, SLOT(graphClicked(QCPAbstractPlottable*,int)));

}




FusionParser::~FusionParser()
{
    delete ui;
}


void FusionParser::on_cb_loc_clicked(bool checked)
{
    if(!checked) {
        locPath.clear();
        ui->txt_loc->setText(locPath);
    }
}

void FusionParser::on_cb_pc_clicked(bool checked)
{
    if(!checked) {
        pcPath.clear();
        ui->txt_pc->setText(pcPath);
    }
}

void FusionParser::on_cb_lta_clicked(bool checked)
{
    if(!checked) {
        ltaPath.clear();
        ui->txt_lta->setText(ltaPath);
    }
}

void FusionParser::on_btn_browse_loc_clicked()
{
    locPath= QFileDialog::getOpenFileName(
                    this,
                    tr("Choose Localisation Fusion CSV File"),
                    "/home/kxr-sim-2/Documents/FusionParser/","All files (*.*);;Txt File (*.csv)"
                    );

    //check if path is empty
    if(locPath.length()){
        //check if file has correct extension
        QString ext = locPath.right(4);
        if (ext != ".csv"){
            QMessageBox::information(0, "Error", "LOC Fusion data has wrong file format.");
            locPath.clear();
            return;
        }
        else{
            ui->btn_process->setEnabled(true);
            ui->cb_loc->setChecked(true);
            ui->txt_loc->setText(locPath);
        }
    }
}

void FusionParser::on_btn_browse_pc_clicked()
{
    pcPath= QFileDialog::getOpenFileName(
                    this,
                    tr("Choose Perception Fusion Text File"),
                    "/home/kxr-sim-2/Documents/FusionParser/","All files (*.*);;Txt File (*.txt)"
                    );

    //check if path is empty
    if(pcPath.length()){
        //check if file has correct extension
        QString ext = pcPath.right(4);
        if (ext != ".txt"){
            QMessageBox::information(0, "Error", "PC Fusion data has wrong file format.");
            pcPath.clear();
            return;
        }
        else{
            ui->btn_process->setEnabled(true);
            ui->cb_pc->setChecked(true);
            ui->txt_pc->setText(pcPath);
        }
    }

}

void FusionParser::on_btn_browse_lta_clicked()
{
    ltaPath= QFileDialog::getOpenFileName(
                    this,
                    tr("Choose LTA Data KML File"),
                    "/home/kxr-sim-2/Documents/FusionParser/","All files (*.*);;KML File (*.kml)"
                    );

    //check if path is empty
    if(ltaPath.length()){
        //check if file has correct extension
        QString ext = ltaPath.right(4);
        if (ext != ".kml"){
            QMessageBox::information(0, "Error", "LTA data has wrong file format.");
            ltaPath.clear();
            return;
        }
        else{
            ui->btn_process->setEnabled(true);
            ui->cb_lta->setChecked(true);
            ui->txt_lta->setText(ltaPath);
        }
    }
}

void FusionParser::on_btn_process_clicked()
{
    ui->txt_process->setText("\tLoading...");

    QStringList objectClasses;
    if(ui->cb_car_class->isChecked()) objectClasses         << "Car";
    if(ui->cb_bicyle_class->isChecked()) objectClasses      << "Bic";
    if(ui->cb_truck_class->isChecked()) objectClasses       << "Tru";
    if(ui->cb_ped_class->isChecked()) objectClasses         << "Ped";

    QStringList alert_msg;

    //read lta data if found path
    if(ltaPath.length()){
        kmlReader->readXml(ltaPath);
    }
    else alert_msg << "LTA Data not found.";

    //if both perception fusion and localisation fusion is available
    if(pcPath.length() && locPath.length()){
        //set perception fusion frequency, 50000.0 micro second (double)
        pcFusParser->pcFPS = 50000.0;
        pcFusParser->setObjectClasses(objectClasses);
        pcFusParser->readPCFus(pcPath);

        //match the AVBus timestamp from pc fusion with loc fusion
        locFusParser->readLOCFus(locPath);
        //matching also did interpolation of missing Ego timestamp
        locFusParser->matchEgoTimestamp(&pcFusParser->actors["AVBus"],pcFusParser->pcFPS);


        //do the rest of processing, including transforming to global coordinate, change to local timestep, ...
        pcFusParser->processAll();

        //set slider range to AVBus timestamp range
        ui->slider_play->setRange(0,pcFusParser->actors["AVBus"].timestamp.last());
        ui->txt_process->setText("");
    }

    //if only localisation fusion is available
    else if (pcPath.length() && !locPath.length()){
        //set perception fusion frequency, 50000.0 micro second (double)
        pcFusParser->pcFPS = 50000.0;
        pcFusParser->setObjectClasses(objectClasses);
        pcFusParser->readPCFus(pcPath);

        //interpolate Ego timestamp
        pcFusParser->fillEgoTimestamp();

        //do the rest of processing, including transforming to global coordinate, change to local timestep, ...
        pcFusParser->processAll();

        //set slider range to AVBus timestamp range
        ui->slider_play->setRange(0,pcFusParser->actors["AVBus"].timestamp.last());
        ui->txt_process->setText("");

        //set slider range to AVBus timestamp range
        ui->slider_play->setRange(0,pcFusParser->actors["AVBus"].timestamp.last());
        ui->txt_process->setText("");

        alert_msg << "Localisation Fusion Data not found.";
    }

    //if only localisation fusion is available
    else if (!pcPath.length() && locPath.length()){
        //match the AVBus timestamp from pc fusion with loc fusion
        locFusParser->readLOCFus(locPath);
        plotLOCFus();
        alert_msg << "Perception Fusion Data not found.";

    }


    if(alert_msg.length()){
        QMessageBox msgBox;
        msgBox.setWindowTitle("Alert");
        msgBox.setText(alert_msg.join('\n'));
        msgBox.exec();
    }

    //enable only after process is pressed

    if(locPath.length()) ui->btn_plotlocfus->setEnabled(true);
    if(pcPath.length()) ui->btn_plotpcfus->setEnabled(true);
    if(ltaPath.length()) ui->btn_plotlta->setEnabled(true);

    ui->btn_plotall     ->setEnabled(true);
    ui->btn_save        ->setEnabled(true);
    ui->btn_process     ->setEnabled(true);
    ui->btn_delete      ->setEnabled(true);
    ui->btn_clear       ->setEnabled(true);

    if(pcPath.length()){
        //setup double slider
        ui->slider_left_play->setRange(0,pcFusParser->actors["AVBus"].timestamp.last());
        ui->slider_right_play->setRange(0,pcFusParser->actors["AVBus"].timestamp.last());
        ui->slider_left_play->setValue(0);
        ui->slider_right_play->setValue(0);
        ui->txt_left_play->setText(QString::number(ui->slider_left_play->value()));
        ui->txt_right_play->setText(QString::number(pcFusParser->actors["AVBus"].timestamp.last()- ui->slider_right_play->value()));


    }
    smoothHeading();
}

void FusionParser::on_btn_clear_clicked()
{
    //enable only after browse is pressed
    ui->btn_plotall     ->setEnabled(false);
    ui->btn_plotlocfus  ->setEnabled(false);
    ui->btn_plotpcfus   ->setEnabled(false);
    ui->btn_plotlta     ->setEnabled(false);
    ui->btn_save        ->setEnabled(false);
    ui->btn_clear       ->setEnabled(false);
    ui->btn_process     ->setEnabled(false);
    ui->btn_delete      ->setEnabled(false);
    ui->cb_loc          ->setChecked(false);
    ui->cb_pc           ->setChecked(false);
    ui->cb_lta          ->setChecked(false);
    ui->gb_viewer       ->setChecked(false);

    //clear all variable
    ui->slider_play->setValue(0);
    ui->slider_camera->setValue(10);
    pcFusParser->actors.clear();
    ui->graph->clearPlottables();
    nGraphDrawn=0;

    ui->graph->replot();
    ui->graph->update();
    ui->txt_delete->clear();

    ui->txt_loc->clear();
    ui->txt_pc->clear();
    ui->txt_lta->clear();

    kmlReader = new KmlReader();
    pcFusParser = new PCFusParser();
    locFusParser = new LOCFusParser();

}

void FusionParser::on_btn_plotall_clicked()
{
    ui->graph->clearPlottables();
    nGraphDrawn =0;
    if(pcPath.length()){
        plotPCFus();
        x_upper = pcFusParser->x_upper;
        x_lower = pcFusParser->x_lower;
        y_upper = pcFusParser->y_upper;
        y_lower = pcFusParser->y_lower;
    }

    if(locPath.length() && !pcPath.length()){
        plotLOCFus();
        x_upper = locFusParser->x_upper;
        x_lower = locFusParser->x_lower;
        y_upper = locFusParser->y_upper;
        y_lower = locFusParser->y_lower;
    }

    if(ltaPath.length()){
        if(!pcPath.length() && ! locPath.length()){
            plotLTA(0,
                    9999999,
                    0,
                    9999999,
                    false);
        }
        else{
            plotLTA(x_lower,
                    x_upper,
                    y_lower,
                    y_upper,
                    false);
        }
    }
}


void FusionParser::plotPCFus()
{
    actors_iter=0;
    while (actors_iter < pcFusParser->actors.keys().length()) {
        if(ui->graph->plottableCount()<= nGraphDrawn) ui->graph->addGraph();
        //ui->graph->addGraph();
        ui->graph->graph(nGraphDrawn)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 2));
        ui->graph->graph(nGraphDrawn)->setLineStyle(QCPGraph::lsNone);
        if (pcFusParser->actors.keys().at(nGraphDrawn) == "AVBus"){
            //red colour plot
            ui->graph->graph(nGraphDrawn)->setPen(QPen(QColor(255,0,0, 127)));
        }
        else{
            //random colour plot
            ui->graph->graph(nGraphDrawn)->setPen(QPen(QColor(qrand() % 256,qrand() % 256, qrand() % 256, 127)));
        }
        ui->graph->graph(nGraphDrawn)->setData(pcFusParser->actors[pcFusParser->actors.keys().at(actors_iter)].state_x , pcFusParser->actors[pcFusParser->actors.keys().at(actors_iter)].state_y);
        nGraphDrawn++;
        actors_iter++;
    }

    ui->graph->xAxis->setRange(pcFusParser->x_lower,pcFusParser->x_upper);
    ui->graph->yAxis->setRange(pcFusParser->y_lower,pcFusParser->y_upper);
    ui->graph->replot();
    ui->graph->update();
    actors_iter = -1;
}

void FusionParser::plotLOCFus()
{
    if(ui->graph->plottableCount()<= nGraphDrawn) ui->graph->addGraph();
    ui->graph->graph(nGraphDrawn)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 2));
    ui->graph->graph(nGraphDrawn)->setLineStyle(QCPGraph::lsNone);
    ui->graph->graph(nGraphDrawn)->setPen(QPen(QColor(255,0,0, 127)));
    ui->graph->graph(nGraphDrawn)->setData(locFusParser->actors["loc_ego"].state_x , locFusParser->actors["loc_ego"].state_y);
    ui->graph->xAxis->setRange(locFusParser->x_lower,locFusParser->x_upper);
    ui->graph->yAxis->setRange(locFusParser->y_lower,locFusParser->y_upper);
    ui->graph->replot();
    ui->graph->update();
    nGraphDrawn++;
}

void FusionParser::plotLTA(double left,double right, double btm, double top,bool isDrawn)
{
    //Road coordinates
    Coordinate roadCoordinate;

    for(Coordinate coord:kmlReader->roadCoordinate){
        //if each individual road segment is at Region of Interest
        if(((coord.UTMX.first() >= left && coord.UTMX.first() <= right) ||
            (coord.UTMX.last() >= left && coord.UTMX.last() <= right)) &&
           ((coord.UTMY.first() >= btm && coord.UTMY.first() <= top) ||
            (coord.UTMY.last() >= btm && coord.UTMY.last() <= top))   ){

            //interpolate the road
            Coordinate _coordinate = mathFunction.linearinterpolation(coord.UTMX,coord.UTMY,3);
            //store cerb line coordinate in roadCoordinate
            roadCoordinate.UTMX << _coordinate.UTMX;
            roadCoordinate.UTMY << _coordinate.UTMY;
        }
    }
    if(ui->graph->plottableCount()<= nGraphDrawn) ui->graph->addGraph();
    ui->graph->graph(nGraphDrawn)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCrossCircle, 3));
    ui->graph->graph(nGraphDrawn)->setLineStyle(QCPGraph::lsNone);
    ui->graph->graph(nGraphDrawn)->setPen(QPen(Qt::black));
    ui->graph->graph(nGraphDrawn)->setData(roadCoordinate.UTMX,roadCoordinate.UTMY);
    if(!isDrawn){
        ui->graph->xAxis->setRange(pcFusParser->x_lower,pcFusParser->x_upper);
        ui->graph->yAxis->setRange(pcFusParser->y_lower,pcFusParser->y_upper);

        ui->graph->replot();
        ui->graph->update();
    }
    nGraphDrawn++;
}

void FusionParser::on_gb_viewer_toggled(bool arg1)
{
    if(arg1 && !pcFusParser->actors.contains("AVBus") ){
        QMessageBox::information(this,"Message","Not avaible if perception fusion data is missing");
        ui->gb_viewer->setChecked(false);
        return;
    }

    actorsColor["AVBus"] = QPen(Qt::red);

    ui->slider_play->setValue(ui->txt_left_play->text().toInt());
    ui->graph->clearPlottables();
    nGraphDrawn=0;

}

void FusionParser::on_slider_play_valueChanged(int value)
{
    ui->graph->clearPlottables();
    nGraphDrawn=0;

    if(value >= ui->txt_right_play->text().toInt()){
        value = ui->txt_right_play->text().toInt();
        ui->slider_play->setValue(value);

    }

    if(value <= ui->txt_left_play->text().toInt()){
        value = ui->txt_left_play->text().toInt();
        ui->slider_play->setValue(value);
    }

    ui->txt_process->setText(QString::number(value));

    QVector<double> _empty;
    _empty.push_back(0.0);
    QMapIterator<QString,Actor> i(pcFusParser->actors);
    int index;

    while(i.hasNext()){
        i.next();
        index = pcFusParser->actors[i.key()].timestamp.indexOf(value);
        if(index != -1){

            plotBoundingBox(i.key(),
                            pcFusParser->actors[i.key()].state_x[index],
                            pcFusParser->actors[i.key()].state_y[index],
                            pcFusParser->actors[i.key()].hdg[index]);

//            QVector<double> x,y;
//            x.push_back(pcFusParser->actors[i.key()].state_x[index]);
//            y.push_back(pcFusParser->actors[i.key()].state_y[index]);


//            if(ui->graph->plottableCount()<= nGraphDrawn) ui->graph->addGraph();
//            ui->graph->graph(nGraphDrawn)->setLineStyle(QCPGraph::lsNone);
//            if (i.key() == "AVBus"){
//                ui->graph->graph(nGraphDrawn)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssStar, 10));
//                ui->graph->graph(nGraphDrawn)->setPen(QPen(QColor(255,0,0, 127)));
//            }
//            else{
//                ui->graph->graph(nGraphDrawn)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 20));
//                ui->graph->graph(nGraphDrawn)->setPen(QPen(QColor(qrand() % 256,qrand() % 256, qrand() % 256, 127)));
//            }
//            ui->graph->graph(nGraphDrawn)->setData(x,y);
//            nGraphDrawn++;
        }
    }

    ui->graph->xAxis->setRange(pcFusParser->actors["AVBus"].state_x.at(value) - ui->slider_camera->value(), pcFusParser->actors["AVBus"].state_x.at(value) + ui->slider_camera->value());
    ui->graph->yAxis->setRange(pcFusParser->actors["AVBus"].state_y.at(value) - ui->slider_camera->value(), pcFusParser->actors["AVBus"].state_y.at(value) + ui->slider_camera->value());

    if(ltaPath.length()){
        plotLTA(ui->graph->xAxis->range().lower,
                ui->graph->xAxis->range().upper,
                ui->graph->yAxis->range().lower,
                ui->graph->yAxis->range().upper,
                true);
    }
    ui->graph->replot();
    ui->graph->update();

}

void FusionParser::on_btn_play_clicked()
{
    stopPlayingFlag_ = false;
    while ( true ) {
        playView();
        QApplication::processEvents();
        if ( stopPlayingFlag_ ) {
            break;
        }
    }
}

void FusionParser::playView()
{
    int nFrame = ui->slider_play->value();

    if(nFrame == ui->txt_right_play->text().toInt())
    {
        stopPlayingFlag_ = true;
        return;
    }

    ui->slider_play->setValue(nFrame+1);
}

void FusionParser::on_btn_pause_clicked()
{
    stopPlayingFlag_ = true;
}

void FusionParser::on_btn_stop_clicked()
{
    stopPlayingFlag_ = true;
    ui->slider_play->setValue( ui->txt_left_play->text().toInt()  );
}

void FusionParser::on_slider_camera_valueChanged(int value)
{
    ui->txt_camera->setText(QString::number(value) + " (m)");
    if(pcPath.length()) on_slider_play_valueChanged(ui->slider_play->value());
}

void FusionParser::on_btn_next_clicked()
{
    ui->graph->clearPlottables();
    nGraphDrawn =0;

    if (actors_iter < pcFusParser->actors.keys().length()-1) {
        actors_iter++;
    }
    else{
        actors_iter = pcFusParser->actors.keys().length()-1;
    }

    if(ui->graph->plottableCount()<= nGraphDrawn) ui->graph->addGraph();
    ui->graph->graph(nGraphDrawn)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 2));
    ui->graph->graph(nGraphDrawn)->setLineStyle(QCPGraph::lsNone);
    ui->graph->graph(nGraphDrawn)->setPen(QPen(Qt::black));
    ui->graph->graph(nGraphDrawn)->setData(   pcFusParser->actors[pcFusParser->actors.keys().at(actors_iter)].state_x ,
                                                                pcFusParser->actors[pcFusParser->actors.keys().at(actors_iter)].state_y);
    //ui->graph->xAxis->setRange(pcFusParser->x_lower,pcFusParser->x_upper);
    //ui->graph->yAxis->setRange(pcFusParser->y_lower,pcFusParser->y_upper);
    ui->graph->rescaleAxes();
    ui->graph->replot();
    ui->graph->update();
    nGraphDrawn++;

    ui->txt_delete->setText(pcFusParser->actors.keys().at(actors_iter));
    ui->statusBar->showMessage(pcFusParser->actors.keys().at(actors_iter) + ", " + QString::number(pcFusParser->actors[pcFusParser->actors.keys().at(actors_iter)].state_x.length()));

}

void FusionParser::on_btn_previous_clicked()
{
    ui->graph->clearPlottables();
    nGraphDrawn =0;
    if (actors_iter >0) {
        actors_iter--;
    }
    else{
        actors_iter = 0;
    }
    if(ui->graph->plottableCount()<= nGraphDrawn) ui->graph->addGraph();
    ui->graph->graph(nGraphDrawn)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 2));
    ui->graph->graph(nGraphDrawn)->setLineStyle(QCPGraph::lsNone);
    ui->graph->graph(nGraphDrawn)->setPen(QPen(Qt::black));
    ui->graph->graph(nGraphDrawn)->setData(   pcFusParser->actors[pcFusParser->actors.keys().at(actors_iter)].state_x ,
                                              pcFusParser->actors[pcFusParser->actors.keys().at(actors_iter)].state_y);
//    ui->graph->xAxis->setRange(pcFusParser->x_lower, pcFusParser->x_upper);
//    ui->graph->yAxis->setRange(pcFusParser->y_lower, pcFusParser->y_upper);
    ui->graph->rescaleAxes();
    ui->graph->replot();
    ui->graph->update();
    nGraphDrawn++;

    ui->txt_delete->setText(pcFusParser->actors.keys().at(actors_iter));
    ui->statusBar->showMessage(pcFusParser->actors.keys().at(actors_iter) + ", " + QString::number(pcFusParser->actors[pcFusParser->actors.keys().at(actors_iter)].state_x.length()));
}

void FusionParser::on_slider_left_play_valueChanged(int value)
{
    if(value > ui->txt_right_play->text().toInt()){
        value = ui->txt_right_play->text().toInt()-1;
        ui->slider_left_play->setValue(value);
    }

    ui->slider_play->setValue(value);
    ui->txt_left_play->setText(QString::number(value));
    ui->txt_save1->setText(QString::number(value));

}

void FusionParser::on_slider_right_play_valueChanged(int value)
{
    if(pcFusParser->actors["AVBus"].timestamp.last()- value < ui->txt_left_play->text().toULong()){
        value = pcFusParser->actors["AVBus"].timestamp.last()-ui->txt_left_play->text().toInt()-1;
        ui->slider_right_play->setValue(value);
    }
    ui->txt_right_play->setText(QString::number(pcFusParser->actors["AVBus"].timestamp.last()- value));
    ui->txt_save2->setText(QString::number(pcFusParser->actors["AVBus"].timestamp.last()- value));

}

void FusionParser::on_btn_delete_clicked()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Warning", "Are you sure you want to delete this path?",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        qDebug() << "Deleting...";
        //remove intended key
        pcFusParser->actors.remove(ui->txt_delete->text());
        on_btn_plotall_clicked();
    }

}

void FusionParser::on_btn_save_clicked()
{
    QString save_dir = QFileDialog::getSaveFileName(
                        this,
                        tr("Save trajectory"),
                        "/home/kxr-sim-2/","XML File (*.xml);;All files (*.*)"
                        );
    if(save_dir == ""){
        QMessageBox::critical(this,"Warning", "File is not saved");
        return;
    }


    QString ext = save_dir.right(4);
    if (ext != ".xml"){
        save_dir += ".xml";
    }

    exportXML(save_dir);
}

void FusionParser::exportXML(QString save_dir)
{
    int timestamp_start = ui->txt_save1->text().toInt();
    int timestamp_end   = ui->txt_save2->text().toInt();
    double translate_x  = ui->txt_translate1->text().toDouble();
    double translate_y  = ui->txt_translate2->text().toDouble();

    //if no actors found, file not saved
    if(pcFusParser->actors.keys().length() == 0){
        QMessageBox::information(this,"Message","Empty data, file not saved");
        return;
    }

    //default timestamp is full timestamp if both textbox is not filled
    if(timestamp_start == 0 && timestamp_end == 0 ) timestamp_end = pcFusParser->actors["AVBus"].timestamp.last();
    if(translate_x == 0 || translate_y == 0){
        translate_x = x_mid;
        translate_y = y_mid;
    }


    //Saved actors information in xml format
    tinyxml2::XMLDocument doc;

    tinyxml2::XMLDeclaration* declaration = doc.NewDeclaration();
    doc.InsertEndChild(declaration);

    tinyxml2::XMLElement* summary = doc.NewElement("Summary");
    doc.InsertEndChild(summary);

    tinyxml2::XMLElement* date = doc.NewElement("Date_Created");
    summary->LinkEndChild(date);
    date->SetText(QDate::currentDate().toString().toStdString().c_str());

    tinyxml2::XMLElement* time = doc.NewElement("Time_Created");
    summary->LinkEndChild(time);
    time->SetText(QTime::currentTime().toString().toStdString().c_str());

    tinyxml2::XMLElement* pcpath = doc.NewElement("Perception_Fusion_File");
    summary->LinkEndChild(pcpath);
    pcpath->SetText(pcPath.toStdString().c_str());

    tinyxml2::XMLElement* locpath = doc.NewElement("Localization_Fusion_File");
    summary->LinkEndChild(locpath);
    locpath->SetText(locPath.toStdString().c_str());

    tinyxml2::XMLElement* origin = doc.NewElement("Origin");
    summary->LinkEndChild(origin);
    origin->SetText((std::to_string(translate_x) + " , " + std::to_string(translate_y)).c_str());

    tinyxml2::XMLElement* utmx_left = doc.NewElement("UTMX_Left");
    summary->LinkEndChild(utmx_left);
    utmx_left->SetText(std::to_string(pcFusParser->x_lower).c_str());

    tinyxml2::XMLElement* utmx_right = doc.NewElement("UTMX_Right");
    summary->LinkEndChild(utmx_right);
    utmx_right->SetText(std::to_string(pcFusParser->x_upper).c_str());

    tinyxml2::XMLElement* utmy_btm = doc.NewElement("UTMY_Btm");
    summary->LinkEndChild(utmy_btm);
    utmy_btm->SetText(std::to_string(pcFusParser->y_lower).c_str());

    tinyxml2::XMLElement* utmy_top = doc.NewElement("UTMY_Top");
    summary->LinkEndChild(utmy_top);
    utmy_top->SetText(std::to_string(pcFusParser->y_upper).c_str());

    tinyxml2::XMLElement* trajectory = doc.NewElement("Trajectory");
    doc.InsertEndChild(trajectory);

    QVector<QMap<QString,int>> nClassAllFrames;
    QMap<QString,int> totalClass;
    for(int ts= 0; ts <= timestamp_end-timestamp_start;++ts ){
        tinyxml2::XMLElement* timestamp = doc.NewElement("Timestamp");
        trajectory->LinkEndChild(timestamp);
        timestamp->SetAttribute("localstep", std::to_string(ts).c_str());
        timestamp->SetAttribute("globalstep", std::to_string(timestamp_start+ts).c_str());

        QMap<QString,int> nClassPerFrame;
        QMapIterator<QString,Actor> i(pcFusParser->actors);
        while(i.hasNext()){
            i.next();
            int index = pcFusParser->actors[i.key()].timestamp.indexOf(timestamp_start + ts);
            if(index != -1){


                QString ClassAndId = pcFusParser->actors[i.key()].objectClass + '_' + i.key();
                tinyxml2::XMLElement* actor;
                if(i.key() == "AVBus"){
                    actor    = doc.NewElement("Ego");
                }
                else{
                    actor = doc.NewElement("Actor");
                }
                timestamp->LinkEndChild(actor);
                actor->SetAttribute("id"        ,   ClassAndId.toStdString().c_str());
                actor->SetAttribute("X"         ,   std::to_string(pcFusParser->actors[i.key()].state_x.at(index)-translate_x).c_str());
                actor->SetAttribute("Y"         ,   std::to_string(pcFusParser->actors[i.key()].state_y.at(index)-translate_y).c_str());
                actor->SetAttribute("Z"         ,   std::to_string(pcFusParser->actors[i.key()].state_z.at(index)).c_str());
                actor->SetAttribute("Heading"   ,   std::to_string(pcFusParser->actors[i.key()].hdg.at(index)).c_str());
                actor->SetAttribute("Pitch"     ,   std::to_string(pcFusParser->actors[i.key()].pitch.at(index)).c_str());
                actor->SetAttribute("Roll"      ,   std::to_string(pcFusParser->actors[i.key()].roll.at(index)).c_str());



                //for saving info
                nClassPerFrame[pcFusParser->actors[i.key()].objectClass]++;
                totalClass[pcFusParser->actors[i.key()].objectClass]++;

            }
        }
        nClassAllFrames.push_back(nClassPerFrame);
    }


    //find max number of class per frame
    QMap<QString,int> maxClassPerFrame;
    for(QMap<QString,int> nClassPerFrame: nClassAllFrames){
        for(int i=0;i < nClassPerFrame.keys().length();++i){
            if( nClassPerFrame.values().at(i) > maxClassPerFrame[nClassPerFrame.keys().at(i)]){
                maxClassPerFrame[nClassPerFrame.keys().at(i)] = nClassPerFrame.values().at(i);
            }
        }
    }

    //save max number per class
    for(int i=0;i<maxClassPerFrame.keys().length();++i){
        QString text = "Max_" + maxClassPerFrame.keys().at(i) + "_perFrame";
        tinyxml2::XMLElement* _text = doc.NewElement(text.toStdString().c_str());
        summary ->  LinkEndChild(_text);
        _text   ->  SetText(std::to_string( maxClassPerFrame.values().at(i)).c_str());
    }

    //save total number per class
    for(int i=0;i<totalClass.keys().length();++i){
        if(totalClass.keys().at(i) == "AVBus") continue;
        QString text = "Total_" + totalClass.keys().at(i);
        tinyxml2::XMLElement* _text = doc.NewElement(text.toStdString().c_str());
        summary ->  LinkEndChild(_text);
        _text   ->  SetText(std::to_string( totalClass.values().at(i)).c_str());
    }


    doc.SaveFile(save_dir.toStdString().c_str());

    QString message = "Successfully saved file in " + save_dir;
    QMessageBox::information(this,"Message",message);

}

//show cursor utm x,y
void FusionParser::clickedGraph(QMouseEvent *event)
{
    double x = ui->graph->xAxis->pixelToCoord(event->pos().x());
    double y = ui->graph->yAxis->pixelToCoord(event->pos().y());


    QString text = "X: " + QString::number(x) + "\t\t\tY: " + QString::number(y);
    ui->statusBar->showMessage(text);
}

//show the "id" of selected plot
void FusionParser::graphClicked(QCPAbstractPlottable *plottable, int dataIndex)
{
    // since we know we only have QCPGraphs in the plot, we can immediately access interface1D()
    // usually it's better to first check whether interface1D() returns non-zero, and only then use it.
    //double dataValue = plottable->interface1D()->dataMainValue(dataIndex);
    //QString message = QString("Clicked on graph '%1' at data point #%2 with value %3.").arg(plottable->name()).arg(dataIndex).arg(dataValue);

    //all plots start from plotting graph
    //graph index (plottable name) 1 corresponding to graph[0]
    if(ui->graph->plottableCount() > 1){
        int index = plottable->name().mid(6).toInt()-1;
        if(index >= pcFusParser->actors.keys().length()){
            //index range: [0, pcFusParser->actors.keys.length)
            ui->txt_delete->setText("");
        }
        else{
            ui->txt_delete->setText(pcFusParser->actors.keys().at(index));
        }
    }
}


void FusionParser::plotBoundingBox(QString id, double &center_x, double &center_y, double &hdg)
{
    QPen color;
    if(actorsColor[id].color().name() == "#000000"){
        color = QColor(qrand() % 256, 1 , 128 , 127);
        actorsColor[id] = color;
    }
    else color= actorsColor[id];

    double length = classSize[pcFusParser->actors[id].objectClass].first;
    double width = classSize[pcFusParser->actors[id].objectClass].second;

    QVector<double> _x = { 0.5*length, -0.5*length,-0.5*length,0.5*length};
    QVector<double> _y = { 0.5*width ,  0.5*width,-0.5*width,-0.5*width};

    for(int i=0; i<4;++i){
    //    mathFunction.rotationMatrix(_x[i],_y[i],-hdg);
        mathFunction.rotationMatrix(_x[i],_y[i],hdg);

    }

    //draw 4 lines
    for(int i=0; i < 4; ++i){
        //rotate to heading
        if(ui->graph->plottableCount()<= nGraphDrawn) ui->graph->addGraph();
        ui->graph->graph(nGraphDrawn)->setLineStyle(QCPGraph::lsLine);
        ui->graph->graph(nGraphDrawn)->setPen(color);
        // if iteration > range, set to 0
        QVector<double> line_x = { center_x + _x.at(i), center_x + _x.at( (i+1)>3? 0: i+1 )};
        QVector<double> line_y = { center_y + _y.at(i), center_y + _y.at( (i+1)>3? 0: i+1 )};
        ui->graph->graph(nGraphDrawn)->setData(line_x,line_y);
        nGraphDrawn++;
    }

    //draw front dot
    if(ui->graph->plottableCount()<= nGraphDrawn) ui->graph->addGraph();
    ui->graph->graph(nGraphDrawn)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 5));
    ui->graph->graph(nGraphDrawn)->setLineStyle(QCPGraph::lsNone);
    ui->graph->graph(nGraphDrawn)->setPen(QPen(color));
    //0 index is top left corner, 3 index is top right corner
    QVector<double> frontdot_x = { center_x + 0.5*(_x.at(0) + _x.at(3))};
    QVector<double> frontdot_y = { center_y + 0.5*(_y.at(0) + _y.at(3))};
    ui->graph->graph(nGraphDrawn)->setData(frontdot_x,frontdot_y);
    nGraphDrawn++;

//    ui->graph->replot();
//    ui->graph->update();
}


void FusionParser::on_btn_plotlta_clicked()
{
    ui->graph->clearPlottables();
    nGraphDrawn=0;
    plotLTA(0,999999,0,999999,false);
}

void FusionParser::on_btn_plotpcfus_clicked()
{
    ui->graph->clearPlottables();
    nGraphDrawn=0;
    plotPCFus();

}

void FusionParser::on_btn_plotlocfus_clicked()
{
    ui->graph->clearPlottables();
    nGraphDrawn=0;
    plotPCFus();
}

void FusionParser::smoothHeading()
{
    qDebug() << " hi ";
    double threshold = 15*M_PI/180.0;//30degree
    qDebug() << threshold;

    QMapIterator<QString,Actor> i(pcFusParser->actors);
    while(i.hasNext()){
        i.next();
        QVector<double> x = i.value().state_x;
        QVector<double> y = i.value().state_y;
        QVector<double> _hdg = mathFunction.calculateHeading(x,y);
        _hdg  = i.value().hdg;
        for(int j=0;j<_hdg.length()-1;++j){
            if(mathFunction.normalizeAngle(fabs(_hdg.at(j+1)-_hdg.at(j))) > threshold){
                qDebug() << i.key() << "\t" << mathFunction.normalizeAngle(fabs(_hdg.at(j+1)-_hdg.at(j))) << ", " << _hdg.at(j+1) << "-" << _hdg.at(j);

            }
        }



//        for(int j=0; j < i.value().hdg.length()-1;++j){
//            if (fabs(mathFunction.normalizeAngle(i.value().hdg.at(j+1) - i.value().hdg.at(j))) >threshold ){
//                pcFusParser->actors[i.key()].hdg[j+1] = pcFusParser->actors[i.key()].hdg[j];
//                //qDebug() << i.key() << "\t" << mathFunction.normalizeAngle(fabs(i.value().hdg.at(j+1) - i.value().hdg.at(j))) ;
//            }
//        }
//            else if (fabs(i.value().hdg.at(j+1) + i.value().hdg.at(j)) > threshold && i.value().hdg.at(j+1)*i.value().hdg.at(j) < 0 ){
//                qDebug() << i.key() << "\t" << fabs(i.value().hdg.at(j+1) + i.value().hdg.at(j)) ;
//            }

            //            double _heading = _hdg.at(j);
//            if(fabs(heading - _heading) < threshold){
//                continue;
//            }
//            else if(j!=0){
//                //check previous heading
//                double prev_heading = i.value().hdg.at(j-1);
//                double _prev_heading = _hdg.at(j-1);

//                int bugger = 0;
//                int _bugger = 0;
//                //see who's the bug
//                if(fabs(heading - prev_heading) > threshold){
//                    bugger = 1;
//                }
//                if(fabs(_heading - _prev_heading) > threshold){
//                    _bugger = 2;
//                }

//                if(bugger + _bugger == 1){
//                    //check


//                }



//        for(int j=0; j < i.value().hdg.length()-1;++j){
//            if (mathFunction.normalizeAngle(fabs(_hdg.at(j) - i.value().hdg.at(j))) >threshold ){
//                //pcFusParser->actors[i.key()].hdg[j+1] = pcFusParser->actors[i.key()].hdg[j];
//                qDebug() << i.key() << "\t" << _hdg.at(j) << "\t" << i.value().hdg.at(j) ;
//            }
//        }
    }

}
