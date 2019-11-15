#include "locfusparser.h"

LOCFusParser::LOCFusParser()
{
}

void LOCFusParser::readLOCFus(QString path)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) {
        QMessageBox::information(0, "Alert", "LOC Fusion data doesn't exist/invalid.");
        return;
    }

    QTextStream in(&file);

    int nRow =0;
    while(!in.atEnd()) {
        QString row = in.readLine();
        QStringList column = row.split(",");
        //column 2 is timestamp
        //column 4 is ego x
        //coulm 5 is ego y
        //column 6 is ego z
        //column 7 is ego yaw
        //column 8 is ego pitch
        //column 9 is ego roll

        if(nRow == 0){
            ++nRow;
            continue;
        }

        unsigned long long int timestamp = column.at(2).toLongLong();

        double ego_x = column.at(4).toDouble();
        double ego_y = column.at(5).toDouble();
        double ego_z = column.at(6).toDouble();
        double hdg = column.at(7).toDouble();
        double pitch = column.at(8).toDouble();
        double roll = column.at(9).toDouble();

        actors["loc_ego"].timestamp.push_back(timestamp);
        actors["loc_ego"].state_x.push_back(ego_x);
        actors["loc_ego"].state_y.push_back(ego_y);
        actors["loc_ego"].state_z.push_back(ego_z);
        actors["loc_ego"].hdg.push_back(mathFunction.normalizeAngle(hdg));
        actors["loc_ego"].pitch.push_back(mathFunction.normalizeAngle(pitch));
        actors["loc_ego"].roll.push_back(mathFunction.normalizeAngle(roll));

        nRow++;

    }
    file.close();

    actors["loc_ego"].objectClass = "AVBus";

    x_upper = *std::max_element(actors["loc_ego"].state_x.begin(),actors["loc_ego"].state_x.end());
    x_lower = *std::min_element(actors["loc_ego"].state_x.begin(),actors["loc_ego"].state_x.end());
    y_upper = *std::max_element(actors["loc_ego"].state_y.begin(),actors["loc_ego"].state_y.end());
    y_lower = *std::min_element(actors["loc_ego"].state_y.begin(),actors["loc_ego"].state_y.end());

}


void LOCFusParser::matchEgoTimestamp(Actor *actor, double fps)
{
    //fill in missing timestamp for AVBus vehicle first
    int nTimestamp = actor->timestamp.length();
    int j=0;
    while(j < nTimestamp -1){
        //difference in timestamp (normalise to 50ms)
        int dts = round((actor->timestamp.at(j+1) - actor->timestamp.at(j)) /fps);

        //qDebug() << actor->timestamp.at(j);
        //if timestamp gap is more than 1, interpolate it
        if(dts > 1){
            for(int _step = 1; _step < dts;_step++){
                //add timestamp of interval 1
                actor->timestamp.insert(j + _step, actor->timestamp.at(j)+ fps * _step );
            }
            nTimestamp = actor->timestamp.length();
            j--;
        }
        j++;
    }

    //remove avbus position from perception fusion
    actor->state_x.clear();
    actor->state_y.clear();
    actor->state_z.clear();
    actor->hdg.clear();
    actor->pitch.clear();
    actor->roll.clear();

    //match timestamp with localisation fusion timestamp
    int loc_ego_index=0;
    for(int j=0;j<actor->timestamp.length();++j){
        //look up for closest timestamp and  < perception fus ego timestamp
        for(int k= loc_ego_index; k < actors["loc_ego"].timestamp.length();++k){
            long long int dtimestamp = actor->timestamp.at(j) - actors["loc_ego"].timestamp.at(k);
            if(dtimestamp < 0 ){
                loc_ego_index = k-1;
                break;
            }
        }

        //interpolation
        double f = (actor->timestamp.at(j) - actors["loc_ego"].timestamp.at(loc_ego_index))/(actors["loc_ego"].timestamp.at(loc_ego_index+1) - actors["loc_ego"].timestamp.at(loc_ego_index));
        double ego_x = f*(actors["loc_ego"].state_x.at(loc_ego_index+1)-actors["loc_ego"].state_x.at(loc_ego_index)) + actors["loc_ego"].state_x.at(loc_ego_index);
        double ego_y = f*(actors["loc_ego"].state_y.at(loc_ego_index+1)-actors["loc_ego"].state_y.at(loc_ego_index)) + actors["loc_ego"].state_y.at(loc_ego_index);
        double ego_z = f*(actors["loc_ego"].state_z.at(loc_ego_index+1)-actors["loc_ego"].state_z.at(loc_ego_index)) + actors["loc_ego"].state_z.at(loc_ego_index);
        double hdg = f*(actors["loc_ego"].hdg.at(loc_ego_index+1)-actors["loc_ego"].hdg.at(loc_ego_index)) + actors["loc_ego"].hdg.at(loc_ego_index);
        double pitch = f*(actors["loc_ego"].pitch.at(loc_ego_index+1)-actors["loc_ego"].pitch.at(loc_ego_index)) + actors["loc_ego"].pitch.at(loc_ego_index);
        double roll = f*(actors["loc_ego"].roll.at(loc_ego_index+1)-actors["loc_ego"].roll.at(loc_ego_index)) + actors["loc_ego"].roll.at(loc_ego_index);

        //rewrite ego position with LOC fusion position x,y,z and heading
        actor->state_x.push_back(ego_x);
        actor->state_y.push_back(ego_y);
        actor->state_z.push_back(ego_z);
        actor->hdg.push_back(mathFunction.normalizeAngle(hdg));
        actor->pitch.push_back(mathFunction.normalizeAngle(pitch));
        actor->roll.push_back(mathFunction.normalizeAngle(roll));

        //loc_ego is 50 Hz and ego is 20Hz, so next match index should be ~2.5 away // enable to speed up process
        loc_ego_index += 2;
    }


}
