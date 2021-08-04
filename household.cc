/*
 * household.cc
 *
 *  Created on: Dec 9, 2020
 *      Author: cja0730
 */
#include <string.h>
#include <omnetpp.h>
#include <fstream>
#include <vector>
#include <utility> // std::pair
#include <stdexcept> // std::runtime_error
#include <sstream> // std::stringstream

using namespace omnetpp;

/**
 * Derive the Txc1 class from cSimpleModule. In the Tictoc1 network,
 * both the `tic' and `toc' modules are Txc1 objects, created by OMNeT++
 * at the beginning of the simulation.
 */
class household : public cSimpleModule
{
   private: // only available form the class that defines them
    long numSent;
    long numReceived;

  protected:
    struct triplet{
                int time;
                int consumption;
    }
        typedef Triplet;
    // The following redefined virtual function holds the algorithm.

    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    void read_csv(std::string filename,std::vector<triplet>* csv_table);
    void verbrauchsanfrage_senden(int verbrauch);
    std::vector<triplet> result;
    int csv_index = 0; //Anzahl der gesendeten Verbrauchsanfragen.
    std::string csv_file;
};

// The module class needs to be registered with OMNeT++
Define_Module(household);

void household::initialize()
{
       // Initialize variables
       numSent = 0;
       WATCH(numSent); // allow variables to be inspectable in Tkenv/Qtenv
       //every Consumer has a different csv-file to parse
       std::string csv_file = par("csv_file").stdstringValue();
       // Producer parses his csv-File
       read_csv(csv_file, &result);
       //for (int j = 0; j < result.size(); j++){
       //   EV <<"Datum = "<< result[j].time <<" ;Capacity = "<<result[j].consumption<<" \n";
       //}
       // Consumer sends an identification-request to the Controller
       char buffer [30];
       int a = sprintf(buffer,"1-Identification-Request");
       cMessage *explore_request = new cMessage(buffer);
       send(explore_request, "out");
       // Consumer sends his first Data
       scheduleAt(simTime()+1, new cMessage("timer"));

}

void household::read_csv(std::string filename,std::vector<triplet>* csv_table){
    // Reads a CSV file into a vector of <Timestamp,consumption > pairs where

        // Create an input filestream
        std::ifstream myFile(filename);

        // Make sure the file is open
        if(!myFile.is_open()) throw std::runtime_error("Could not open file");

        // Helper vars
        std::string line,time;
        int consumption;


        // 1 Zeile überspringen
        if(myFile.good())
        {
            std::getline(myFile,line);
        }
        // lese File zeile per zeile
        while(std::getline(myFile, line)){
            // Create a stringstream of the current line
            std::stringstream ss(line);
            // read the Timestamp
            std::getline(ss, time, ',');
            int Time = std::stoi(time);
            // read Consumptionvolume
            ss >> consumption;
            // Initialize and add <time, val> pairs to result
            csv_table->push_back({Time,consumption});
        }

        myFile.close();
}

/** Der Verbraucher bekommt ein ACK aus dem Controller, löscht den Msg.
 */
void household::handleMessage(cMessage *msg)
{
    // if its a scheduled message
    if(msg->isSelfMessage()==true){
        // if theres data to send in the csv_table
        if (csv_index < result.size()) {
            // send consumption-info (Format:2  [Time]  [Consumption[kWh])
            char msgname[20];
            int n = sprintf (msgname, "2 %d %d",result[csv_index].time,result[csv_index].consumption);
            cMessage *consumption_info = new cMessage(msgname);
            send(consumption_info,"out");
            numSent ++;
            csv_index+=1;
            scheduleAt(simTime()+1, new cMessage("timer"));

        }
        else {EV << "End of the csv_table \n";}
    }
}
/*
 * die Verbrauchsanfrage wird erstellt, in type cMessage umgewandelt,
 *  und zum Controller geschickt
 * */
void household::verbrauchsanfrage_senden(int verbrauch){
    /*

    // Verbrauchsanfrage erstellen (Format: 1 - [Auftrag-Nr] - [Vebrauch kWH])
    char msgname[20];
    //Auftragsnummer um 1 erhören
    int n = sprintf (msgname, "1 %d ",verbrauch);
    cMessage *Verbrauchsanfrage = new cMessage(msgname);
    // Verbrauchsanfrage mit Verbrauchsdata zum Controller senden
    EV << "Verbrauchsanfrage " << Verbrauchsanfrage << "  from Verbraucher sent.\n";
    sendDelayed(Verbrauchsanfrage,0.2,"out");
    csv_index +=1;
*/
}

