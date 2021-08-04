/*
 * pv_modul.cc
 *
 *  Created on: Mar 24, 2021
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
class pv_modul : public cSimpleModule
{
  private: // only available form the class that defines them
    long numSent;
    long numReceived;

  protected: // accessible from the class that defines them und in other classes which inherit from that class
    struct triplet{
            int time;
            int capacity;
            float price; }
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
Define_Module(pv_modul);

void pv_modul::initialize(){
    // Initialize variables
    numSent = 0;
    WATCH(numSent); // allow variables to be inspectable in Tkenv/Qtenv
    //every Producer has a different csv-file to parse
    // Producer parses his csv-File
    std::string csv_file = par("csv_file").stdstringValue();
    read_csv(csv_file, &result);
    for (int j = 0; j < result.size(); j++){
        EV <<"Datum = "<< result[j].time <<" ;Capacity = "<<result[j].capacity<<" price = " << result[j].price<< " \n";
    }
    // producer sends an identification-request to the Controller
    char buffer [30];
    int a = sprintf(buffer,"1-Identification-Request");
    cMessage *explore_request = new cMessage(buffer);
    send(explore_request, "out");
    // Producer sends his first Data
    scheduleAt(simTime()+1, new cMessage("timer"));
}

void pv_modul::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage()==true){
           // if theres data to send in the csv_table
           if (csv_index < result.size()) {
               // send consumption-info (Format: 2 [Time]  [Consumption[kWh]]  [Price[Euro]])
               char msgname[20];
               int n = sprintf (msgname, "2 %d %d %f",result[csv_index].time,result[csv_index].capacity,result[csv_index].price);
               cMessage *consumption_info = new cMessage(msgname);
               // increment number of msg sent
               numSent++;
               send(consumption_info,"out");
               csv_index+=1;
               scheduleAt(simTime()+1, new cMessage("timer"));

           }
           else {
               EV << "End of the csv_table \n";}
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


void pv_modul::read_csv(std::string filename,std::vector<triplet>* csv_table){
    // Reads a CSV file into a vector of <zeit, erzeugungsdata > pairs where
        // each pair represents <column name, column values>

        // Create an input filestream
        std::ifstream myFile(filename);

        // Make sure the file is open
        if(!myFile.is_open()) throw std::runtime_error("Could not open file");

        // Helper vars
        std::string line,price,time;
        int val;
        float preis;


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
            std::getline(ss, time,',');
            int Time = std::stoi(time);
            //std::getline(ss, time, ',');
            // read Generationvolume
            ss >> val;
            // read the price per KwH
            std::getline(ss, price);
            price.erase(0,1);
            preis = std::stof(price);
            // Initialize and add <time, val> pairs to result
            csv_table->push_back({Time,val,preis});
        }

        myFile.close();
}

void pv_modul::erzeugungsinfo_senden(int erzeugung, int vorgang){
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






