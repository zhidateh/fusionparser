#include "pcfusparser.h"
#include "../fusionparser.h"

PCFusParser::PCFusParser()
{
}

void PCFusParser::setObjectClasses(QStringList objClasses)
{
    objectClasses = objClasses;
}

void PCFusParser::readPCFus(QString path)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) {
        QMessageBox::information(0, "Alert", "PC Fusion data doesn't exist/invalid.");
        return;
    }

    QTextStream in(&file);


    int nRow =0;
    while(!in.atEnd()) {
        QString row = in.readLine();
        QStringList column = row.split(" ");
        //column 1 is timestamp
        //column 2 is ego x
        //coulm 3 is ego y
        //column 5 is id
        //column 6 is pos x
        //column 7 is pos y
        //column 12 is heading
        //column 13 is classification

        unsigned long long int timestamp = column.at(1).toLongLong();

        //translate all timestamp to the first timestamp, e.g. (first is 10050000) -> 0 (second is 10100000) -> 1, ...
        //each timestamp has 50000 microseconds gap
        //unsigned long int _timestamp = (timestamp-first_timestamp)/50000;

        double ego_x = column.at(2).toDouble();
        double ego_y = column.at(3).toDouble();

        //if found in objectClasses string list
        if(objectClasses.indexOf( map_of_objectclasses[column.at(13)]) != -1 ){
            actors[column.at(5)].timestamp.push_back(timestamp);
            actors[column.at(5)].objectClass = map_of_objectclasses[column.at(13)];
            actors[column.at(5)].state_x.push_back(column.at(6).toDouble());
            actors[column.at(5)].state_y.push_back(column.at(7).toDouble());
            actors[column.at(5)].state_z.push_back(0);

            //limit range to [-pi,pi]
            double hdg = column.at(12).toDouble();
            actors[column.at(5)].hdg.push_back(mathFunction.normalizeAngle(hdg));

            actors[column.at(5)].pitch.push_back(0);
            actors[column.at(5)].roll.push_back(0);

        }


        //append ego data
        actors["ego"].timestamp.push_back(timestamp);
        actors["ego"].state_x.push_back(ego_x);
        actors["ego"].state_y.push_back(ego_y);
        actors["ego"].state_z.push_back(0);
        actors["ego"].hdg.push_back(0);
        actors["ego"].pitch.push_back(0);
        actors["ego"].roll.push_back(0);



        nRow++;

    }
    file.close();

    //get Ego heading from the position X, position Y
    actors["ego"].hdg = mathFunction.calculateHeading(actors["ego"].state_x,actors["ego"].state_y );

    //to change the 0heading to next non-zero value;
    int k=0;
    int count=0;
    while(k < actors["ego"].hdg.length()){
        double heading = actors["ego"].hdg.at(k);
        if(fabs(heading) < 0.001){
            count++;
        }
        else{
            while(count>0){
                actors["ego"].hdg[k-count] = actors["ego"].hdg[k];
                --count;
            }
        }
        ++k;
    }

    filterWaypoint();
    assignAVBus();
}

void PCFusParser::filterWaypoint()
{
    QMapIterator<QString,Actor> i(actors);

    while(i.hasNext()){
        i.next();
        if (i.key() == "ego" || i.key() == "loc_ego") continue;

        //remove path that doesnt have changes in x of 5m
        if(fabs(i.value().state_x.first()-i.value().state_x.last()) < 5){
            actors.remove(i.key());
            continue;
        }

        //remove path that doesnt have changes in y of 5m
        if(fabs(i.value().state_y.first()-i.value().state_y.last()) < 5){
            actors.remove(i.key());
            continue;
        }

        //remove path that have less than 10 waypoints
        if(i.value().state_x.length() < 10){
            actors.remove(i.key());
            continue;
        }


    }
}

void PCFusParser::assignAVBus()
{
    unsigned long long int _ts=-1;
    for(int k=0; k < actors["ego"].timestamp.length();++k){
        //add actor "AVBus" to inherit from ego, by filtering repeated attributes
        if(actors["ego"].timestamp[k] != _ts){
            actors["AVBus"].timestamp.push_back(actors["ego"].timestamp[k]);
            actors["AVBus"].state_x.push_back(actors["ego"].state_x[k]);
            actors["AVBus"].state_y.push_back(actors["ego"].state_y[k]);
            actors["AVBus"].state_z.push_back(0);
            actors["AVBus"].hdg.push_back(actors["ego"].hdg[k]);
            actors["AVBus"].pitch.push_back(0);
            actors["AVBus"].roll.push_back(0);

            _ts = actors["ego"].timestamp[k];
        }
    }
    actors["AVBus"].objectClass = "AVBus";
    actors.remove("ego");

}

void PCFusParser::fillEgoTimestamp()
{
    //interpolate AVBus first
    int nTimestamp = actors["AVBus"].timestamp.length();
    int j=0;
    while(j < nTimestamp -1){
        //difference in timestamp (normalise to 50ms)
        int dts = round((actors["AVBus"].timestamp.at(j+1) - actors["AVBus"].timestamp.at(j)) /pcFPS);

        //if timestamp gap is more than 1, interpolate all attributes
        if(dts > 1){
            double dx   = actors["AVBus"].state_x.at(j+1) - actors["AVBus"].state_x.at(j);
            double dy   = actors["AVBus"].state_y.at(j+1) - actors["AVBus"].state_y.at(j);
            double dz       = actors["AVBus"].state_z.at(j+1) - actors["AVBus"].state_z.at(j);
            double dhdg     = actors["AVBus"].hdg.at(j+1) - actors["AVBus"].hdg.at(j);
            double dpitch   = actors["AVBus"].pitch.at(j+1) - actors["AVBus"].pitch.at(j);
            double droll    = actors["AVBus"].roll.at(j+1) - actors["AVBus"].roll.at(j);

            for(int _step = 1; _step < dts;_step++){

                //add timestamp of interval 50000
                actors["AVBus"].timestamp.insert(j + _step, actors["AVBus"].timestamp.at(j)+ pcFPS * _step );

                //add dx/dtimestamp to each state_x interval, state_x[j] + step * (dx/dtimestamp) //linear interpolation
                actors["AVBus"].state_x.insert(j+_step, actors["AVBus"].state_x.at(j) + _step * (dx/dts) );

                //add dy/dtimestamp to each state_y interval, state_y[j] + step * (dy/dtimestamp) //linear interpolation
                actors["AVBus"].state_y.insert(j+_step, actors["AVBus"].state_y.at(j) + _step * (dy/dts) );

                //add dz/dtimestamp to each state_z interval, state_z[j] + step * (dz/dtimestamp) //linear interpolation
                actors["AVBus"].state_z.insert(j+_step, actors["AVBus"].state_z.at(j) + _step * (dz/dts) );

                //add dhdg/dtimestamp to each heading interval, heading[j] + step * (dhdg/dtimestamp) //linear interpolation
                actors["AVBus"].hdg.insert(j+_step, actors["AVBus"].hdg.at(j) + _step * (dhdg/dts) );

                //add dpitch/dtimestamp to each pitch interval, pitch[j] + step * (dpitch/dtimestamp) //linear interpolation
                actors["AVBus"].pitch.insert(j+_step, actors["AVBus"].pitch.at(j) + _step * (dpitch/dts) );

                //add droll/dtimestamp to each roll interval, roll[j] + step * (droll/dtimestamp) //linear interpolation
                actors["AVBus"].roll.insert(j+_step, actors["AVBus"].roll.at(j) + _step * (droll/dts) );
            }
            nTimestamp = actors["AVBus"].timestamp.length();
            j--;
        }
        j++;
    }
}

void PCFusParser::processAll()
{
    interpolateAll();
    transformWaypoint();
    transformTimestamp();
}

void PCFusParser::interpolateAll()
{
    //interpolate the other actors
    QMapIterator<QString,Actor> i(actors);
    while(i.hasNext()){
        i.next();

        //ignore avbus as it has been interpolated
        if(i.key() == "AVBus" ) continue;

        int nTimestamp = actors[i.key()].timestamp.length();
        int j=0;
        while(j < nTimestamp -1){
            //difference in timestamp (normalise to 50ms)
            int dts = round((actors[i.key()].timestamp.at(j+1) - actors[i.key()].timestamp.at(j)) /pcFPS);

            //if timestamp gap is more than 1, interpolate all attributes
            if(dts > 1){
                double dx       = actors[i.key()].state_x.at(j+1) - actors[i.key()].state_x.at(j);
                double dy       = actors[i.key()].state_y.at(j+1) - actors[i.key()].state_y.at(j);
                double dz       = actors[i.key()].state_z.at(j+1) - actors[i.key()].state_z.at(j);
                double dhdg     = actors[i.key()].hdg.at(j+1) - actors[i.key()].hdg.at(j);
                double dpitch   = actors[i.key()].pitch.at(j+1) - actors[i.key()].pitch.at(j);
                double droll    = actors[i.key()].roll.at(j+1) - actors[i.key()].roll.at(j);

                for(int _step = 1; _step < dts;_step++){

                    //look up for matching AVBus timestamp
                    int AVBus_index = actors["AVBus"].timestamp.indexOf(actors[i.key()].timestamp.at(j));

                    //for interpolation of timestamp, get from AVBus
                    actors[i.key()].timestamp.insert(j + _step, actors["AVBus"].timestamp.at(AVBus_index+_step));

                    //add dx/dtimestamp to each state_x interval, state_x[j] + step * (dx/dtimestamp) //linear interpolation
                    actors[i.key()].state_x.insert(j+_step, actors[i.key()].state_x.at(j) + _step * (dx/dts) );

                    //add dy/dtimestamp to each state_y interval, state_y[j] + step * (dy/dtimestamp) //linear interpolation
                    actors[i.key()].state_y.insert(j+_step, actors[i.key()].state_y.at(j) + _step * (dy/dts) );

                    //add dz/dtimestamp to each state_z interval, state_z[j] + step * (dz/dtimestamp) //linear interpolation
                    actors[i.key()].state_z.insert(j+_step, actors[i.key()].state_z.at(j) + _step * (dz/dts) );

                    //add dhdg/dtimestamp to each heading interval, heading[j] + step * (dhdg/dtimestamp) //linear interpolation
                    actors[i.key()].hdg.insert(j+_step, actors[i.key()].hdg.at(j) + _step * (dhdg/dts) );

                    //add dpitch/dtimestamp to each pitch interval, pitch[j] + step * (dpitch/dtimestamp) //linear interpolation
                    actors[i.key()].pitch.insert(j+_step, actors[i.key()].pitch.at(j) + _step * (dpitch/dts) );

                    //add droll/dtimestamp to each roll interval, roll[j] + step * (droll/dtimestamp) //linear interpolation
                    actors[i.key()].roll.insert(j+_step, actors[i.key()].roll.at(j) + _step * (droll/dts) );


                }
                nTimestamp = actors[i.key()].timestamp.length();
                j--;
            }
            j++;
        }
    }
}

void PCFusParser::transformWaypoint()
{
    QMapIterator<QString,Actor> i(actors);


    //start transforming the target coordinate systems
    while(i.hasNext()){
        i.next();


        //Load individual actor
        for(int j=0; j < actors[i.key()].state_x.length(); ++j){

            //ignore "AVBus" as they are already in global coordinates
            if (i.key() == "AVBus"){
                continue;
            }

            //rotate to AVBus heading
            mathFunction.rotationMatrix(actors[i.key()].state_x[j],
                                        actors[i.key()].state_y[j],
                                        actors["AVBus"].hdg.at(actors["AVBus"].timestamp.indexOf(actors[i.key()].timestamp[j])));

            //translate to AVBus position
            actors[i.key()].state_x[j]  += actors["AVBus"].state_x.at(actors["AVBus"].timestamp.indexOf(actors[i.key()].timestamp[j]));
            actors[i.key()].state_y[j]  += actors["AVBus"].state_y.at(actors["AVBus"].timestamp.indexOf(actors[i.key()].timestamp[j]));
            actors[i.key()].state_z[j]  += actors["AVBus"].state_z.at(actors["AVBus"].timestamp.indexOf(actors[i.key()].timestamp[j]));


            double hdg = actors[i.key()].hdg[j] + actors["AVBus"].hdg.at(actors["AVBus"].timestamp.indexOf(actors[i.key()].timestamp[j]));
            double pitch = actors[i.key()].pitch[j] + actors["AVBus"].pitch.at(actors["AVBus"].timestamp.indexOf(actors[i.key()].timestamp[j]));
            double roll = actors[i.key()].roll[j] + actors["AVBus"].roll.at(actors["AVBus"].timestamp.indexOf(actors[i.key()].timestamp[j]));

            //normalise angle to range [-pi,pi]
            actors[i.key()].hdg[j]  = mathFunction.normalizeAngle(hdg);
            actors[i.key()].pitch[j]  = mathFunction.normalizeAngle(pitch);
            actors[i.key()].roll[j]  = mathFunction.normalizeAngle(roll);


            //find range of trajectory for plotting, get max x,y and min x,y
            if(actors[i.key()].state_x[j] > x_upper) x_upper = actors[i.key()].state_x[j];
            if(actors[i.key()].state_x[j] < x_lower) x_lower = actors[i.key()].state_x[j];
            if(actors[i.key()].state_y[j] > y_upper) y_upper = actors[i.key()].state_y[j];
            if(actors[i.key()].state_y[j] < y_lower) y_lower = actors[i.key()].state_y[j];

        }
    }


    //make the range of data a square
    x_mid = 0.5*(x_upper+x_lower);
    y_mid = 0.5*(y_upper+y_lower);

    //check if y range or x range is larger, get the larger interval
    x_upper = x_mid + 0.5 * std::max(x_upper-x_lower,y_upper-y_lower);
    x_lower = x_mid - 0.5 * std::max(x_upper-x_lower,y_upper-y_lower);
    y_upper = y_mid + 0.5 * std::max(x_upper-x_lower,y_upper-y_lower);
    y_lower = y_mid - 0.5 * std::max(x_upper-x_lower,y_upper-y_lower);

}

void PCFusParser::transformTimestamp()
{
    QMapIterator<QString,Actor> i(actors);
    while(i.hasNext()){
        i.next();
        if(i.key() == "AVBus") continue;
        for(int j=0; j < actors[i.key()].timestamp.length(); ++j){
            actors[i.key()].timestamp[j] = actors["AVBus"].timestamp.indexOf(actors[i.key()].timestamp.at(j));
        }
    }

    for(int j=0;j<actors["AVBus"].timestamp.length();++j){
        actors["AVBus"].timestamp[j] = j;
    }

}
