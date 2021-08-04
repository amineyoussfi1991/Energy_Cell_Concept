#include <string.h>
#include <omnetpp.h>
#include <utility> // std::pair
#include <sstream> // std::stringstream
#include <algorithm> // std::sort

using namespace omnetpp;

/**
 */
class cellmanager : public cSimpleModule
{
  protected:
    long balanced_num;
    long unbalanced_num;
    long balanced;
    long not_balanced;
    cLongHistogram balance_histogram;
    cOutVector balance_vector;
    struct merit_order{
            std::string producer_name;
            std::string consumer_name;
            int capacity;
    }
    typedef Merit_order;

    struct quadriplet{
        std::string producer_name;
        int capacity;
        float price;
        int priority;
    }
    typedef Quadriplet;

    struct less_than_key {
        inline bool operator() (const quadriplet& struct1, const quadriplet& struct2)
        {
            return (struct1.price < struct2.price);
        }
    };
    struct triplet{
        int Time; // TO DO : change numbers with real Datum
        std::vector<std::pair<std::string, int>> consumers;
        std::vector<std::pair<std::string, int>> storage;
        std::vector<quadriplet> producers;
        std::vector<std::pair<std::string, int>> cellmanager;
        std::vector<merit_order> Merit_order ;
        int Is_balanced;
    }
    typedef Triplet;
    // define Job_table
    std::vector<triplet> Job_table;
    // The following redefined virtual function holds the algorithm.
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    std::vector<std::pair<std::string , int>> verbraucher_list;
    std::vector<std::pair<std::string, int>> erzeuger_list;
    std::vector<std::pair<std::string, int>> storage_list;
    std::vector<std::pair<int, int>> cellmanager_list;
    void erzeugungsantwort(int index);
    virtual void finish() override;
    int Auftragsnummer;

};

// The module class needs to be registered with OMNeT++
Define_Module(cellmanager);

// send msg on all ports to all other modules (Consumer and producer)
void cellmanager::initialize(){

    // statitics
    balanced_num = 0;
    unbalanced_num = 0;
    balance_histogram.setName("balance_histogram");
    balance_vector.setName("balance_vector");

    // Cell-manager sends an identification-request on all his out-ports
    int n = gateSize("out");
        for (int i =0 ; i < n ; i++ ){
         char buffer [30];
         int a = sprintf(buffer,"1-Identification-Request");
         cMessage *explore_request = new cMessage(buffer);
         EV << "Forwarding message " << explore_request << " on port out[" << i << "]\n";
         send(explore_request, "out", i);
    }
}
void cellmanager::handleMessage(cMessage *msg)
{
    int cmd;
    // cast cMessage in string
    std::string erzeuger_name = msg->getSenderModule()->getFullName();
    std::string message = msg->getName();
    // extract Message-Type-number
    std::stringstream ss(message);
    ss >> cmd;
    int balance_result = -1;

    switch (cmd)  {
        // 1 : Identification-Request
        case 1: {

            // know if its is a Consumer or Producer
            cModule *Sending_module = msg->getSenderModule();
            cModuleType *ModuleType =  Sending_module->getModuleType();
            // get the Arrival gate of the sent message
            cGate *arrival_gate = msg->getArrivalGate();
            std::string str1(Sending_module->getFullName());
            // write the Household-name and its port in Household-Table
            if (strcmp(ModuleType->getName(),"household") == 0) {
               verbraucher_list.push_back(std::make_pair(str1,arrival_gate->getIndex()));
               EV <<"Verbraucher = "<< verbraucher_list[verbraucher_list.size()-1].first <<" ;port = "<<verbraucher_list[verbraucher_list.size()-1].second<<" \n";
            }
            // write the Cell-manager-name and its port in Cell-manager-Table
            else if (strcmp(ModuleType->getName(), "cellmanager") == 0) {
                // get index of Prant_module
                int cell_Index = Sending_module->getParentModule()->getIndex();
                // push in the Cell_manager_list
               cellmanager_list.push_back(std::make_pair(cell_Index,arrival_gate->getIndex()));
               //EV <<"Cellmanager = "<< cellmanager_list[cellmanager_list.size()-1].first <<" ;port = "<<cellmanager_list[cellmanager_list.size()-1].second<<" \n";
            }
            // write the pv_modul-name and its port in pv_modul-Table
            else if (strcmp(ModuleType->getName(), "pv_modul") == 0) {
               erzeuger_list.push_back(std::make_pair(str1,arrival_gate->getIndex()));
               EV <<"Erzeuger = "<< erzeuger_list[erzeuger_list.size()-1].first <<" ;port = "<<erzeuger_list[erzeuger_list.size()-1].second<<" \n";
            }
            // write the Storage-Modul-name and its port in Storage-Modul-Table
            else if (strcmp(ModuleType->getName(), "storageModul") == 0) {
               storage_list.push_back(std::make_pair(str1,arrival_gate->getIndex()));
               EV <<"Storage = "<< storage_list[storage_list.size()-1].first <<" ;port = "<<storage_list[storage_list.size()-1].second<<" \n";
                        }
            else {throw std::runtime_error("The Type of the Sending-Class couldn't be found");
            }
            break;
        }
        case 2:{
            // know if its is a Consumer or Producer
            cModule *Sending_module = msg->getSenderModule();
            std::string Sending_module_name = Sending_module->getFullName();
            cModuleType *ModuleType =  Sending_module->getModuleType();
            std::string Module_Type = ModuleType->getName();

            // parse the Timestamp
            int time;
            ss >> time;
            // search after the Timestamp in the Auftrag_table
            int found = -1;
            for (int i =0 ; i < Job_table.size() ; i++ ){
               if (Job_table[i].Time == time) {
                  found = i;
                  break;
                  }
             }
             // if not found then add Consumer/Producer und its data to the Auftrag Table
            if (found == -1){

                // if producer
                if (strcmp(ModuleType->getName(),"pv_modul") == 0){
                    //parse capacity and price
                    float price;
                    int capacity;
                    ss >> capacity;
                    ss >> price;
                    // write time and in producers add (producer_name, capacity, price,)
                    Job_table.push_back({time,std::vector<std::pair<std::string, int>>{},std::vector<std::pair<std::string, int>>{},std::vector<quadriplet>{},std::vector<std::pair<std::string, int>>{},std::vector<merit_order>{},1});
                    Job_table[Job_table.size()-1].producers.push_back({Sending_module_name,capacity,price,0});

                }
                // else if consumer parse Datum, capacity and price
                else if (strcmp(ModuleType->getName(),"household") == 0){
                    //parse capacity
                    int consumption;
                    ss >> consumption;
                    // write time and in consumers add (consumer_name,consumption)
                    Job_table.push_back({time,std::vector<std::pair<std::string, int>>{},std::vector<std::pair<std::string, int>>{},std::vector<quadriplet>{},std::vector<std::pair<std::string, int>>{},std::vector<merit_order>{},1});
                    Job_table[Job_table.size()-1].consumers.push_back({Sending_module_name,consumption});
                }
                // else if storageModul  parse Datum, capacity
                else if (strcmp(ModuleType->getName(),"storageModul") == 0){
                   //parse capacity
                   int consumption;
                   ss >> consumption;
                   // write time and in consumers add (consumer_name,consumption)
                   Job_table.push_back({time,std::vector<std::pair<std::string, int>>{},std::vector<std::pair<std::string, int>>{},std::vector<quadriplet>{},std::vector<std::pair<std::string, int>>{},std::vector<merit_order>{},1});
                   Job_table[Job_table.size()-1].storage.push_back({Sending_module_name,consumption});
                   }
                // else if Horizontal Communication  parse Datum, capacity
                else if (strcmp(ModuleType->getName(),"cellmanager") == 0){
                 //parse capacity
                 int consumption;
                 ss >> consumption;
                 // write time and in consumers add (consumer_name,consumption)
                 Job_table.push_back({time,std::vector<std::pair<std::string, int>>{},std::vector<std::pair<std::string, int>>{},std::vector<quadriplet>{},std::vector<std::pair<std::string, int>>{},std::vector<merit_order>{},1});
                 Job_table[Job_table.size()-1].cellmanager.push_back({Sending_module_name,consumption});
                 }

                // assign found to the last index in the array
                found = Job_table.size()-1;
            }

            // if found then add directly Consumer/Producer und its data to the Auftrag Table
            else if (found != -1){
                // if producer
                if (strcmp(ModuleType->getName(),"pv_modul") == 0){
                    //parse capacity and price
                    float price;
                    int capacity;
                    ss >> capacity;
                    ss >> price;
                    // write time and in producers add (producer_name, capacity, price,)
                    Job_table[found].producers.push_back({Sending_module_name,capacity,price,0});
                    }
                // else if consumer
                else if (strcmp(ModuleType->getName(),"household") == 0){
                    //parse capacity
                    int consumption;
                    ss >> consumption;
                    // add in consumers (consumer_name,consumption)
                    Job_table[found].consumers.push_back({Sending_module_name,consumption});
               }
                else if (strcmp(ModuleType->getName(),"storageModul") == 0){
                    //parse capacity
                    int consumption;
                    ss >> consumption;
                    // add in consumers (consumer_name,consumption)
                    Job_table[found].storage.push_back({Sending_module_name,consumption});
               }
                else if (strcmp(ModuleType->getName(),"cellmanager") == 0){
                    // parse capacity
                    int consumption;
                    ss >> consumption;
                   // if balance between consumption and Production wasnt done yet
                   if (Job_table[found].Is_balanced == 1){
                       //parse capacity
                       // add in cellmanager (consumer_name,consumption)
                       Job_table[found].cellmanager.push_back({Sending_module_name,consumption});
                   }

                   // if balance between consumption and Production was done
                   else if (Job_table[found].Is_balanced != 1){
                       bool cellmanager_producer_balance = false;
                       bool cellmanager_storage_balance = true;
                       // find the cellmanager-name the Cellmanager-Table and register the port
                       int cellmanager_port ;
                       int cellmanager_index = Sending_module->getParentModule()->getIndex();
                       for (int j = 0; j < cellmanager_list.size();j++){
                           if (cellmanager_index  == cellmanager_list[j].first){
                                  cellmanager_port = cellmanager_list[j].second;
                                  break;
                           }
                       }
                       //wenn  consumer nicht leer ist; dann gibts es nichts zu geben
                       if (Job_table[found].consumers.size() != 0){

                           EV <<" Cell-manager can't send anything because his own Consumer-producer-balance is not done\n";
                       }
                       //Wenn consumer leer ist (Übrige Erzeugung wird aufgeteilt)
                       else{
                           // as long as iterator not equal to the end of the pv_table
                           int producer_index = 0;
                           while (!cellmanager_producer_balance) {
                               // iterator on the pv_table (i)
                               int difference = consumption - (Job_table[found].producers[producer_index].capacity);
                               // if difference > 0
                               if (difference > 0){
                                   int b = Job_table[found].producers[producer_index].capacity ;
                                   //update consumption
                                   consumption = consumption - Job_table[found].producers[producer_index].capacity;
                                   //update capacity of producer to 0
                                   Job_table[found].producers[producer_index].capacity = 0;
                                   //push into Merit-Order-Table
                                   Job_table[found].Merit_order.push_back({Job_table[found].producers[producer_index].producer_name,Sending_module_name,b});
                                   //iterator increment
                                   producer_index+=1;
                                   //i
                                   }
                               else if (difference < 0){
                                   // consuming-menge
                                   int a = consumption ;
                                   // update producing
                                   Job_table[found].producers[producer_index].capacity = Job_table[found].producers[producer_index].capacity - consumption;
                                   // update consuming
                                   consumption = 0;
                                   //push into Merit-Order-Table
                                   Job_table[found].Merit_order.push_back({Job_table[found].producers[producer_index].producer_name,Sending_module_name,a});

                               }
                               // if they are equal
                               else {
                                   //push into Merit-Order-Table
                                   Job_table[found].Merit_order.push_back({Job_table[found].producers[producer_index].producer_name,Sending_module_name,consumption});
                                   // update capacity and consumption in each table
                                   // update consumption
                                   consumption = 0;
                                   //update producing
                                   Job_table[found].producers[producer_index].capacity = 0;
                                   // producer [producer_index] send his producing capacity to the consumer[consumer_index]
                                   // producer_index is incremented
                                   producer_index+=1;
                              }
                              if (consumption == 0){
                                   // Balancing is done
                                   cellmanager_producer_balance = true;
                                   EV <<" the Balancing of cellmanager-consumption and PV-production with Merit-Order-Princip is done  \n";
                                   //TO DO : Send all requests from Merit-order-Table
                                   for (int i = 0; i < Job_table[found].Merit_order.size(); i++){
                                       // iterate over the Merit-Order_table
                                       for (int j = 0; j < erzeuger_list.size();j++){
                                           // find the producer-name from Merit-Order-Table in the in the Erzeuger-Table and register the output
                                           if (Job_table[found].Merit_order[i].producer_name.compare(erzeuger_list[j].first) == 0){
                                               int producer_port = erzeuger_list[j].second;
                                               // write the msg
                                               char msgname[40];
                                               int n = sprintf (msgname, "3 %d %s %d",Job_table[found].Time,Job_table[found].Merit_order[i].producer_name.c_str(),Job_table[found].Merit_order[i].capacity);
                                               cMessage *production_request = new cMessage(msgname);
                                               // send the msg
                                               send(production_request,"out",cellmanager_port);
                                               EV << "Cellmanager to Cellmanager: producer --> Consumer  " << production_request << ".\n";
                                               break;
                                           }
                                        }
                                   }
                                   // delete all einträge in Merit-Order-Table
                                   int s = 0;
                                   while(s < Job_table[found].Merit_order.size()){
                                        Job_table[found].Merit_order.erase(Job_table[found].Merit_order.begin() + s);
                                   }
                                   // delete all Producers in Job_Table equal to 0
                                   int l = 0;
                                   while(l < Job_table[found].producers.size()){
                                        if (Job_table[found].producers[l].capacity == 0 )
                                           {Job_table[found].producers.erase(Job_table[found].producers.begin() + l);}
                                        else
                                           {l++;}
                                   }
                               }
                               // check if balancing of consumption and Production didnt successfully end
                               else if (producer_index > Job_table[found].producers.size() -1){
                                   // Production was smaller than total consumption
                                   cellmanager_producer_balance = true;
                                   EV <<" total PV-Production was smaller than Cellmanager-consumption \n";
                                   // go to the Balancing of Rest Consumption with Storage
                                   cellmanager_storage_balance = false;
                                   // Send  Producer-requests from Merit-order-Table
                                   for (int i = 0; i < Job_table[found].Merit_order.size(); i++){
                                       // iterate over the Merit-Order_table
                                       for (int j = 0; j < erzeuger_list.size();j++){
                                           // find the producer-name from Merit-Order-Table in the in the Erzeuger-Table and register the output
                                           if (Job_table[found].Merit_order[i].producer_name.compare(erzeuger_list[j].first) == 0){
                                               int producer_port = erzeuger_list[j].second;
                                               // write the msg
                                               char msgname[40];
                                               int n = sprintf (msgname, "3 %d %s %d",Job_table[found].Time,Job_table[found].Merit_order[i].producer_name.c_str(),Job_table[found].Merit_order[i].capacity);
                                               cMessage *production_request = new cMessage(msgname);
                                               // send the msg
                                               send(production_request,"out",cellmanager_port);
                                               EV << "Cellmanager to Cellmanager: PV --> Consumer  " << production_request << ".\n";
                                               break;
                                           }
                                        }
                                   }
                                   // Delete Producer-Einträge from Merit-Order-Table
                                   int f = 0;
                                   while(f < Job_table[found].Merit_order.size()){
                                      Job_table[found].Merit_order.erase(Job_table[found].Merit_order.begin() + f);
                                   }

                               }
                          }
                           int storage_index = 0;
                           while (!cellmanager_storage_balance) {
                               // iterator on the storage_table (i)
                               int difference = consumption - (Job_table[found].storage[storage_index].second);
                               // if difference > 0
                               if (difference > 0){
                                   int e = Job_table[found].storage[storage_index].second ;
                                   //update consumption
                                   consumption = consumption - Job_table[found].storage[storage_index].second;
                                   //update capacity of producer to 0
                                   Job_table[found].storage[storage_index].second = 0;
                                   //push into Merit-Order-Table
                                   Job_table[found].Merit_order.push_back({Job_table[found].storage[storage_index].first,Sending_module_name,e});
                                   //iterator increment
                                   storage_index+=1;

                                   }
                               else if (difference < 0){
                                   // consuming-menge
                                   int a = consumption ;
                                   // update producing
                                   Job_table[found].storage[storage_index].second = Job_table[found].storage[storage_index].second - consumption;
                                   // update consuming
                                   consumption = 0;
                                   //push into Merit-Order-Table
                                   Job_table[found].Merit_order.push_back({Job_table[found].storage[storage_index].first,Sending_module_name,a});

                               }
                               // if they are equal
                               else {
                                   //push into Merit-Order-Table
                                   Job_table[found].Merit_order.push_back({Job_table[found].storage[storage_index].first,Sending_module_name,consumption});
                                   // update capacity and consumption in each table
                                   // update consumption
                                   consumption = 0;
                                   //update producing
                                   Job_table[found].storage[storage_index].second = 0;
                                   // producer [producer_index] send his producing capacity to the consumer[consumer_index]
                                   // producer_index is incremented
                                   storage_index+=1;
                              }
                              if (consumption == 0){
                                   // Balancing betweenis done
                                   cellmanager_storage_balance = true;
                                   EV <<" the Balancing of cellmanager-consumption and storage-production with Merit-Order-Princip is done  \n";
                                   //TO DO : Send all requests from Merit-order-Table
                                   for (int i = 0; i < Job_table[found].Merit_order.size(); i++){
                                       // iterate over the Merit-Order_table
                                       for (int j = 0; j < storage_list.size();j++){
                                           // find the producer-name from Merit-Order-Table in the in the Erzeuger-Table and register the output
                                           if (Job_table[found].Merit_order[i].producer_name.compare(storage_list[j].first) == 0){
                                               int storage_port = storage_list[j].second;
                                               // write the msg
                                               char msgname[40];
                                               int n = sprintf (msgname, "3 %d %s %d",Job_table[found].Time,Job_table[found].Merit_order[i].producer_name.c_str(),Job_table[found].Merit_order[i].capacity);
                                               cMessage *storage_request = new cMessage(msgname);
                                               // send the msg
                                               send(storage_request,"out",cellmanager_port);
                                               EV << "Cellmanager to Cellmanager: Storage --> Consumer  " << storage_request << ".\n";
                                               break;
                                           }
                                        }
                                   }
                                   // delete storage-einträge in Merit-Order-Table
                                   int s = 0;
                                   while(s < Job_table[found].Merit_order.size()){
                                        Job_table[found].Merit_order.erase(Job_table[found].Merit_order.begin() + s);
                                   }
                                   // delete all Producers in Job_Table equal to 0
                                   int l = 0;
                                   while(l < Job_table[found].producers.size()){
                                        if (Job_table[found].producers[l].capacity == 0 )
                                           {Job_table[found].producers.erase(Job_table[found].producers.begin() + l);}
                                        else
                                           {l++;}
                                   }
                                   // delete all storage equal = 0
                                   int x = 0;
                                   while(x < Job_table[found].storage.size()){
                                        if (Job_table[found].storage[x].second == 0 )
                                           {Job_table[found].storage.erase(Job_table[found].storage.begin() + x);}
                                        else
                                           {x++;}
                                   }

                              }
                               // check if balancing of consumption and Production didnt successfully end
                               else if (storage_index > Job_table[found].storage.size()-1){
                                   EV <<" total PV-Production was smaller than Cellmanager-consumption \n";
                                   // go to the Balancing of Rest Consumption with Storage
                                   cellmanager_storage_balance = true;
                                   // Send all requests from Merit-order-Table
                                   for (int i = 0; i < Job_table[found].Merit_order.size(); i++){
                                       // iterate over the Merit-Order_table
                                       for (int j = 0; j < storage_list.size();j++){
                                           // find the producer-name from Merit-Order-Table in the in the Erzeuger-Table and register the output
                                           if (Job_table[found].Merit_order[i].producer_name.compare(storage_list[j].first) == 0){
                                               int storage_port = storage_list[j].second;
                                               // write the msg
                                               char msgname[40];
                                               int n = sprintf (msgname, "3 %d %s %d",Job_table[found].Time,Job_table[found].Merit_order[i].producer_name.c_str(),Job_table[found].Merit_order[i].capacity);
                                               cMessage *storage_request = new cMessage(msgname);
                                               // send the msg
                                               send(storage_request,"out",cellmanager_port);
                                               EV << "Cellmanager to Cellmanager: Storage --> Consumer  " << storage_request << ".\n";
                                               break;
                                           }
                                        }
                                   }
                                   // delete all einträge in Merit-Order-Table
                                   int s = 0;
                                   while(s < Job_table[found].Merit_order.size()){
                                        Job_table[found].Merit_order.erase(Job_table[found].Merit_order.begin() + s);
                                   }
                                   // delete all Producers in Job_Table equal to 0
                                   int l = 0;
                                   while(l < Job_table[found].producers.size()){
                                        if (Job_table[found].producers[l].capacity == 0 )
                                           {Job_table[found].producers.erase(Job_table[found].producers.begin() + l);}
                                        else
                                           {l++;}
                                   }
                                   // delete all storage equal = 0
                                   int x = 0;
                                   while(x < Job_table[found].storage.size()){
                                        if (Job_table[found].storage[x].second == 0 )
                                           {Job_table[found].storage.erase(Job_table[found].storage.begin() + x);}
                                        else
                                           {x++;}
                                   }

                               }
                          }

                       }
                   }
               }

            }
            // check if all Consumer and Producer in this Timestamp are registered
            if ((verbraucher_list.size() == Job_table[found].consumers.size())&&(erzeuger_list.size() == Job_table[found].producers.size())&&(storage_list.size() == Job_table[found].storage.size())){
            // if yes -> calculate the Balancing of consumption with generation with Merit-Order-Method
                EV <<" the Balancing of consumption and generation with Merit-Order-Princip will start  \n";
                // prioritize Producers according to the Price
                std::sort((Job_table[found].producers).begin(),(Job_table[found].producers).end(), less_than_key());
                // go through Consumer-Table und balance the consumption of each Consumer and write that MErit-Order Table
                int consumer_index = 0;
                int producer_index = 0;
                bool consumer_producer_balance = true;
                bool consumer_storage_balance = false;
                bool horizontal_communication = false;

                // 1. try to balance consumption with production
                while (consumer_producer_balance){
                    // difference between consumer[consumer_index] and producer[producer_index]
                    int difference = (Job_table[found].consumers[consumer_index].second) -  (Job_table[found].producers[producer_index].capacity);
                    if (difference > 0){
                        // update capacity and consumption in each table
                        int b = Job_table[found].producers[producer_index].capacity ;
                        Job_table[found].consumers[consumer_index].second = Job_table[found].consumers[consumer_index].second - Job_table[found].producers[producer_index].capacity;
                        //
                        Job_table[found].producers[producer_index].capacity = Job_table[found].producers[producer_index].capacity - Job_table[found].producers[producer_index].capacity;
                        // producer [producer_index] send his producing capacity to the consumer[consumer_index]
                        Job_table[found].Merit_order.push_back({Job_table[found].producers[producer_index].producer_name,Job_table[found].consumers[consumer_index].first,b});
                        // consumer_index is incremented
                        producer_index+=1;
                    }
                    else if (difference < 0){
                        // update capacity and consumption in each table
                        int a = Job_table[found].consumers[consumer_index].second ;
                        Job_table[found].producers[producer_index].capacity = Job_table[found].producers[producer_index].capacity - Job_table[found].consumers[consumer_index].second;
                        Job_table[found].consumers[consumer_index].second = Job_table[found].consumers[consumer_index].second - Job_table[found].consumers[consumer_index].second;

                        // producer [producer_index] send his producing capacity to the consumer[consumer_index]
                        Job_table[found].Merit_order.push_back({Job_table[found].producers[producer_index].producer_name,Job_table[found].consumers[consumer_index].first,a});
                        // consumer_index is incremented
                        consumer_index+=1;
                    }
                    // if they are equal
                    else {
                        // update capacity and consumption in each table
                        int c = Job_table[found].consumers[consumer_index].second;
                        Job_table[found].consumers[consumer_index].second = Job_table[found].consumers[consumer_index].second - Job_table[found].producers[producer_index].capacity;
                        Job_table[found].producers[producer_index].capacity = Job_table[found].producers[producer_index].capacity - Job_table[found].producers[producer_index].capacity;
                        // producer [producer_index] send his producing capacity to the consumer[consumer_index]


                        // consumer_index is incremented
                        consumer_index+=1;
                        producer_index+=1;
                    }
                    // check if balancing of consumption and Production successfully ended
                    if (consumer_index > Job_table[found].consumers.size() -1){
                        // Balancing is done
                        consumer_producer_balance = false;
                        EV <<" the Balancing of consumption and generation with Merit-Order-Princip is done  \n";
                        Job_table[found].Is_balanced = 2;
                        balance_result = 1;
                        balanced_num++;
                    }
                    // check if balancing of consumption and Production didnt successfully end
                    else if (producer_index > Job_table[found].producers.size() -1){
                        // total Production was smaller than total consumption
                        consumer_producer_balance = false;

                        // balancing of consumption and Production didnt successfully end
                        consumer_storage_balance = true;
                        EV <<" total Production was smaller than total consumption \n";
                        balance_result = 0;
                        unbalanced_num++;
                    }

                }
                // 2.try to balance the rest of consumption with storage
                int storage_index = 0;
                while (consumer_storage_balance){
                    // difference between consumer[consumer_index] and producer[producer_index]
                    int difference = (Job_table[found].consumers[consumer_index].second) -  (Job_table[found].storage[storage_index].second);
                    if (difference > 0){
                        // update capacity and consumption in each table
                        int b = Job_table[found].storage[storage_index].second ;
                        Job_table[found].consumers[consumer_index].second = Job_table[found].consumers[consumer_index].second - Job_table[found].storage[storage_index].second;
                        Job_table[found].storage[storage_index].second = Job_table[found].storage[storage_index].second - Job_table[found].storage[storage_index].second;
                        // producer [producer_index] send his producing capacity to the consumer[consumer_index]
                        Job_table[found].Merit_order.push_back({Job_table[found].storage[storage_index].first,Job_table[found].consumers[consumer_index].first,b});
                        // consumer_index is incremented
                        storage_index+=1;
                    }
                    else if (difference < 0){
                        // update capacity and consumption in each table
                        int a = Job_table[found].consumers[consumer_index].second ;
                        Job_table[found].storage[storage_index].second = Job_table[found].storage[storage_index].second - Job_table[found].consumers[consumer_index].second;
                        Job_table[found].consumers[consumer_index].second = Job_table[found].consumers[consumer_index].second - Job_table[found].consumers[consumer_index].second;

                        // producer [producer_index] send his producing capacity to the consumer[consumer_index]
                        Job_table[found].Merit_order.push_back({Job_table[found].storage[storage_index].first,Job_table[found].consumers[consumer_index].first,a});
                        // consumer_index is incremented
                        consumer_index+=1;
                    }
                    // if they are equal
                    else {
                        // update capacity and consumption in each table
                        int c = Job_table[found].consumers[consumer_index].second;
                        Job_table[found].consumers[consumer_index].second = Job_table[found].consumers[consumer_index].second - Job_table[found].storage[storage_index].second;
                        Job_table[found].storage[storage_index].second = Job_table[found].storage[storage_index].second - Job_table[found].storage[storage_index].second;
                        // producer [producer_index] send his producing capacity to the consumer[consumer_index]
                        Job_table[found].Merit_order.push_back({Job_table[found].storage[storage_index].first,Job_table[found].consumers[consumer_index].first,c});
                        // consumer_index is incremented
                        consumer_index+=1;
                        storage_index+=1;
                    }
                    // check if balancing of consumption and storage successfully ended
                    if (consumer_index > Job_table[found].consumers.size() -1){
                        // Balancing is done
                        consumer_storage_balance = false;
                        EV <<" the Balancing of consumption and storage is done  \n";
                        balance_result = 1;
                        balanced_num++;
                        Job_table[found].Is_balanced = 2;
                    }
                    // check if balancing of consumption and storage didnt successfully end
                    else if (storage_index > Job_table[found].storage.size() -1){
                        // total Production was smaller than total consumption
                        consumer_storage_balance = false;
                        // other neighbour-Cellmanager will be contacted
                        horizontal_communication = true;
                        EV <<" Storage-Capacity was  also smaller than total consumption \n";
                        balance_result = 0;
                        unbalanced_num++;
                    }

                }
                // delete all members in household,pv-modul,storage which capacity/consumption are equal to zero
                int k = 0;
                while(k < Job_table[found].consumers.size()){
                    if (Job_table[found].consumers[k].second == 0 )
                        {Job_table[found].consumers.erase(Job_table[found].consumers.begin() + k);}
                    else
                        {k++;}
                }
                int l = 0;
                while(l < Job_table[found].storage.size()){
                    if (Job_table[found].storage[l].second == 0 )
                        {Job_table[found].storage.erase(Job_table[found].storage.begin() + l);}
                    else
                        {l++;}
                }
                int m = 0;
                while(m < Job_table[found].producers.size()){
                    if (Job_table[found].producers[m].capacity == 0 )
                        {Job_table[found].producers.erase(Job_table[found].producers.begin() + m);}
                    else
                        {m++;}
                }
                // delete einträge in Merit-Table


                //3. contact all other neighbours (Cellmanager)
                if (horizontal_communication){
                    Job_table[found].Is_balanced = 3;
                    for (int j = 0; j < cellmanager_list.size();j++){
                        int cell_port = cellmanager_list[j].second;
                        //find the whole consumption_volume needed for the balance
                        int consumer_volume = 0;
                        for (int k = 0; k < Job_table[found].consumers.size();k++){consumer_volume += Job_table[found].consumers[k].second;}
                        // write the msg
                        char msgname[40];
                        // msg-Format: [2]-[Timestamp]-[consumption]
                        int n = sprintf (msgname, "2 %d %d",Job_table[found].Time,consumer_volume);
                        cMessage *horizontal_request = new cMessage(msgname);
                        // send the msg
                        send(horizontal_request,"out",cell_port);
                        EV << "horizontal request " << horizontal_request << "  from Cellmanager sent.\n";
                    }

                }


                // send Fahrplan to the Energy producers so that they can send it to the Consumer only if Household with PV/Storage/Horizontal Communication succeeded
                for (int i = 0; i < Job_table[found].Merit_order.size(); i++){
                // iterate over the Merit-Order_table
                    for (int j = 0; j < erzeuger_list.size();j++){
                        // find the producer-name from Merit-Order-Table in the in the Erzeuger-Table and register the output
                        if (Job_table[found].Merit_order[i].producer_name.compare(erzeuger_list[j].first) == 0){
                            int producer_port = erzeuger_list[j].second;
                            // write the msg
                            char msgname[40];
                            int n = sprintf (msgname, "3 %s %d",Job_table[found].Merit_order[i].consumer_name.c_str(),Job_table[found].Merit_order[i].capacity);
                            cMessage *production_request = new cMessage(msgname);
                            // send the msg
                            send(production_request,"out",producer_port);
                            break;
                        }
                    }
                }
                // delete all einträge in Merit-Order-Table
                int d = 0;
                while(d < Job_table[found].Merit_order.size()){
                       Job_table[found].Merit_order.erase(Job_table[found].Merit_order.begin() + d);

                }
            }
            break;

        }
        case 3: {
            std::string producer_name;
            int Time;
            int Producer_volume;
            int consumer_index = 0;
            // parse the Timestamp

            ss >> Time;

            // parse the producer name
            ss >> producer_name;

            // search after the Timestamp in the Auftrag_table
            int found = -1;
            for (int i =0 ; i < Job_table.size() ; i++ ){
              if (Job_table[i].Time == Time) {
                found = i;
                break;
              }
            }
            // know if its is a Consumer or Producer
            cModule *Sending_module = msg->getSenderModule();
            std::string Sending_module_name = Sending_module->getFullName();

            // find the cellmanager-name the Cellmanager-Table and register the port
            int cellmanager_port ;
            for (int j = 0; j < cellmanager_list.size();j++){
                if (Sending_module->getParentModule()->getIndex() == cellmanager_list[j].first){
                       cellmanager_port = cellmanager_list[j].second;
                       break;
                }
            }
            // find the producer_port in Production/Storage
            // parse the production volume
            ss >> Producer_volume;
            bool cellmanager_producer_balance = false;
            while (!cellmanager_producer_balance) {
                // iterator on the consumer_table (i)
                int difference = Producer_volume - (Job_table[found].consumers[consumer_index].second);
                // if difference > 0
                if (difference > 0){
                    int b = Job_table[found].consumers[consumer_index].second ;
                    //update Producer_volume
                    Producer_volume = Producer_volume - Job_table[found].consumers[consumer_index].second;
                    //update consuming-volume to 0
                    Job_table[found].consumers[consumer_index].second = 0;
                    //push into Merit-Order-Table
                    Job_table[found].Merit_order.push_back({producer_name,Job_table[found].consumers[consumer_index].first,b});
                    //iterator increment
                    consumer_index+=1;
                    //i
                    }
                else if (difference < 0){
                    // consuming-menge
                    int a = Producer_volume ;
                    // update producing
                    Job_table[found].consumers[consumer_index].second = Job_table[found].consumers[consumer_index].second-Producer_volume;
                    // update consuming
                    Producer_volume = 0;
                    //push into Merit-Order-Table
                    Job_table[found].Merit_order.push_back({producer_name,Job_table[found].consumers[consumer_index].first.c_str(),a});

                }
                // if they are equal
                else {
                    //push into Merit-Order-Table
                    Job_table[found].Merit_order.push_back({producer_name,Job_table[found].consumers[consumer_index].first,Producer_volume});
                    // update capacity and consumption in each table
                    // update consumption
                    Producer_volume = 0;
                    //update consuming
                    Job_table[found].consumers[consumer_index].second = 0;
                    // producer [producer_index] send his producing capacity to the consumer[consumer_index]
                    // producer_index is incremented
                    consumer_index+=1;
               }
                // check if balancing of Production-volume and consumers did successfully end
                if (consumer_index > Job_table[found].consumers.size() -1){
                    Job_table[found].Is_balanced = 4;
                    // Production was smaller than total consumption
                    cellmanager_producer_balance = true;
                    // Send  Producer-requests from Merit-order-Table
                    for (int i = 0; i < Job_table[found].Merit_order.size(); i++){
                        // iterate over the Merit-Order_table
                        // write the msg
                        char msgname[50];
                        int n = sprintf (msgname, "4 %d %s %s %d",Job_table[found].Time,Job_table[found].Merit_order[i].producer_name.c_str(),Job_table[found].Merit_order[i].consumer_name.c_str(),Job_table[found].Merit_order[i].capacity);
                        cMessage *production_request = new cMessage(msgname);
                        // send the msg
                        send(production_request,"out",cellmanager_port);
                        EV << "Cellmanager Y to Cellmanager X: --> Balancing after Ausgleich-Request was succesfull (No Consumer anymore) " << production_request << ".\n";
                    }
                    // delete all einträge in Merit-Order-Table
                    int s = 0;
                    while(s < Job_table[found].Merit_order.size()){
                         Job_table[found].Merit_order.erase(Job_table[found].Merit_order.begin() + s);
                    }
                    // store Production volume left in the storage
                    if (Producer_volume != 0){Job_table[found].storage.push_back({storage_list[0].first,Producer_volume});}
                    // delete all consumers in Job_Table equal to 0
                    int l = 0;
                    while(l < Job_table[found].consumers.size()){
                         if (Job_table[found].consumers[l].second == 0 )
                            {Job_table[found].consumers.erase(Job_table[found].consumers.begin() + l);}
                         else
                            {l++;}
                    }

                }
                else if (Producer_volume == 0){
                    Job_table[found].Is_balanced = 5;
                    // Balancing is done
                    cellmanager_producer_balance = true;
                    EV <<" the Balancing of Producer-Volume and Cell-manager-Consumption with Merit-Order-Princip wanst successful  \n";
                    // send 4 - [producer-name] [consumer-name] [productionvolume]
                    //Send all requests from Merit-order-Table
                    for (int i = 0; i < Job_table[found].Merit_order.size(); i++){
                        // iterate over the Merit-Order_table
                                // write the msg
                                char msgname[50];
                                int n = sprintf (msgname, "4 %d %s %s %d",Job_table[found].Time,Job_table[found].Merit_order[i].producer_name.c_str(),Job_table[found].Merit_order[i].consumer_name.c_str(),Job_table[found].Merit_order[i].capacity);
                                cMessage *production_request = new cMessage(msgname);
                                // send the msg
                                send(production_request,"out",cellmanager_port);
                                EV << "Cellmanager Y to Cellmanager X: --> Balancing after Ausgleich-Request wasnt succesfull (theres consumer yet)  " << production_request << ".\n";
                    }
                    // delete all einträge in Merit-Order-Table
                    int s = 0;
                    while(s < Job_table[found].Merit_order.size()){
                         Job_table[found].Merit_order.erase(Job_table[found].Merit_order.begin() + s);
                    }
                    // delete all consumers in Job_Table equal to 0
                    int l = 0;
                    while(l < Job_table[found].consumers.size()){
                         if (Job_table[found].consumers[l].second == 0 )
                            {Job_table[found].consumers.erase(Job_table[found].consumers.begin() + l);}
                         else
                            {l++;}
                    }
                }

            }





            break;
        }
        case 4:{
            // parse Time
            int Time;
            ss >> Time;
            // parse producer-name
            std::string producer_name;
            ss >> producer_name;
            char msgname[50];
            int n = sprintf (msgname, producer_name.c_str());
            // parse consumer-name
            std::string consumer_name;
            ss >> consumer_name;
            char msgname1[50];
            int n1 = sprintf (msgname1, consumer_name.c_str());
            // parse production volume
            int production_volume = 0;
            ss >> production_volume;
            // find producer-port
            // find the cellmanager-name the Cellmanager-Table and register the port
            int producer_port = -1 ;
            // find the type of producer (storage or pv ?)
            for (int j = 0; j < erzeuger_list.size();j++){
                if (producer_name.compare(erzeuger_list[j].first) == 0){
                      producer_port = erzeuger_list[j].second;
                      break;
                }
            }


            if (producer_port == -1)
            {
                for (int j = 0; j < storage_list.size();j++){
                    if (producer_name.compare(storage_list[j].first) == 0){
                           producer_port = storage_list[j].second;
                           break;
                    }
                }
            }
            if (producer_port == -1) {
                throw std::runtime_error("The producer-port wasnt found");
            }
            // send Message to producer
            char msgname2[40];
            int n2 = sprintf (msgname2, "3 %s %d",consumer_name.c_str(),production_volume);
            cMessage *production_request = new cMessage(msgname2);
            // send the msg
            send(production_request,"out",producer_port);
            break;
        }
        default:{
            //throw std::runtime_error("Te message couldn't be identificated");
            //delete msg;
            break;
        }

 }
}


void cellmanager::erzeugungsantwort(int index){
/*
    for (int j = 0; j < Auftrag_Table.size(); j++){
            EV <<"Auftragsnummer = "<< Auftrag_Table[j].vorgang <<" ;Verbraucher = "<<Auftrag_Table[j].verbraucher<<" ;Verbrauch = "<< Auftrag_Table[j].verbrauch<<" \n";
    }
   //TO DO
   // macht eine gewisse berechnung und liefert ob, der Verbrauch geliefert werden kann oder nicht
   // Die Antwort kann OK - Not OK sein
   // erzeugungsantowrt erstellen (Format: 4 - [OK or not OK])
    char msgname[30];
    int n = sprintf (msgname, "4 OK");
    cMessage *Erzeugungsantwort = new cMessage(msgname);
   // find the port of Auftrag_Table[found].Verbraucher
    for (int i = 0; i < verbraucher_list.size(); i++){
        if (verbraucher_list[i].first.compare(Auftrag_Table[index].verbraucher) == 0){
            send(Erzeugungsantwort,"out",verbraucher_list[i].second);
            break;
        }
    }
*/
}
void cellmanager::finish()
{
    // This function is called by OMNeT++ at the end of the simulation.
    //EV << "Sent:     " << numSent << endl;
    //EV << "Received: " << numReceived << endl;
    //EV << "Hop count, min:    " << hopCountStats.getMin() << endl;
    //EV << "Hop count, max:    " << hopCountStats.getMax() << endl;
    //EV << "Hop count, mean:   " << hopCountStats.getMean() << endl;
    //EV << "Hop count, stddev: " << hopCountStats.getStddev() << endl;

    recordScalar("#balanced", balanced_num);
    recordScalar("#unbalanced",unbalanced_num);

    balance_histogram.recordAs("balance_histogram");
}
