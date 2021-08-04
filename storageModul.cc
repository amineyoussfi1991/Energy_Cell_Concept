/*
 * storageModul.cc
 *
 *  Created on: Jun 8, 2021
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
 */
class storageModul : public cSimpleModule
{
  private: // only available form the class that defines them
    long numSent;
    long numReceived;

  protected: // accessible from the class that defines them und in other classes which inherit from that class
    struct triplet{
            int time;
            int capacity;
}
    typedef Triplet;
    // The following redefined virtual function holds the algorithm.
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    void read_csv(std::string filename,std::vector<triplet>* csv_table);
    std::vector<triplet> result;
    void erzeugungsinfo_senden(int erzeugung, int vorgang);
    std::string csv_file;
    int csv_index = 0;
};

// The module class needs to be registered with OMNeT++
Define_Module(storageModul);

void storageModul::initialize(){
    // Initialize variables
    numSent = 0;
    WATCH(numSent); // allow variables to be inspectable in Tkenv/Qtenv
    //every Producer has a different csv-file to parse
    std::string csv_file = par("csv_file").stdstringValue();
    // Producer parses his csv-File
    read_csv(csv_file, &result);
    for (int j = 0; j < result.size(); j++){
        EV <<"Datum = "<< result[j].time <<" ;StorageCapacity = "<<result[j].capacity<< " \n";
    }
    // producer sends an identification-request to the Controller
    char buffer [30];
    int a = sprintf(buffer,"1-Identification-Request");
    cMessage *explore_request = new cMessage(buffer);
    send(explore_request, "out");
    // Producer sends his first Data
    scheduleAt(simTime()+1, new cMessage("timer"));
}

void storageModul::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage()==true){
           // if theres data to send in the csv_table
           if (csv_index < result.size()) {
               // send consumption-info (Format: 2 [Time]  [StorageCapacity[kWh] )
               char msgname[20];
               int n = sprintf (msgname, "2 %d %d ",result[csv_index].time,result[csv_index].capacity);
               cMessage *consumption_info = new cMessage(msgname);
               // increment number of msg sent
               numSent++;
               send(consumption_info,"out");
               csv_index+=1;
               scheduleAt(simTime()+1, new cMessage("timer"));

           }
           else {EV << "End of the csv_table \n";}
    }
    else {
        // parse Fahrplan
        std::string erzeuger_name = msg->getSenderModule()->getName();
        std::string message = msg->getName();
        int cmd ;
        // extract Message-Type-number
        std::stringstream ss(message);
        ss >> cmd;
        //  extract consumer-name
        std::string consumer_name;
        ss >> consumer_name;
        // extract production-volume
        int production_volume;
        ss >> production_volume;
        // know the type of the sender
        cModule *Sending_module = msg->getSenderModule();
        cModuleType *ModuleType =  Sending_module->getModuleType();
        // show the whole message
        char msgname[40];
        int n = sprintf (msgname, "%d %s %d",cmd,consumer_name.c_str(),production_volume);
    }
}



void storageModul::read_csv(std::string filename,std::vector<triplet>* csv_table){
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

void storageModul::erzeugungsinfo_senden(int erzeugung, int vorgang){
/*
    // Verbrauchsanfrage erstellen (Format: 3 - [Erzeugungsinfo kWH])
    char msgname[20];
    int n = sprintf (msgname, "3 %d %d",vorgang,erzeugung);
    cMessage *Erzeugungsinfo = new cMessage(msgname);
    // Verbrauchsanfrage mit Verbrauchsdata zum Controller senden
    EV << "Erzeugungsinfo " << Erzeugungsinfo << "  zum Controller sent.\n";
    send(Erzeugungsinfo,"out");
    //scheduleAt(0.2, Verbrauchsanfrage);
*/
}









