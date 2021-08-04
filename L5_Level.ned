//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//
simple household
{	
    parameters: 
        string csv_file; 
    gates:
        input in;
        output out;
}

simple cellmanager
{     
        gates:
        input in[];
        output out[];
}

simple pv_modul
{
    parameters: 
        string csv_file;        
     
    gates:
        input in;
        output out;

}
simple storageModul
{
    parameters: 
        string csv_file;         
    gates:
        input in;
        output out;

}
//
// Two instances (tic and toc) of Txc1 connected both ways.
// Tic and toc will pass messages to one another.
//
module L5_Level
{   
    parameters:
    	int num_storageModul;
    	int num_pv_modul;
    	int num_household;

    gates:
        input in[];
        output out[];
    
    submodules:
        Household[num_household]: household {
					
        }
        
        Cellmanager: cellmanager;

        PV_modul[num_pv_modul]: pv_modul {          
        }

        StorageModul[num_storageModul]: storageModul {  
        }
    connections:
        
        for i=0..num_household-1 {
        	Household[i].out --> {  delay = 30ms; } --> Cellmanager.in++;
        	Household[i].in <-- {  delay = 30ms; } <-- Cellmanager.out++;
        }
        for k=0..num_pv_modul-1 {
        	PV_modul[k].out --> {  delay = 30ms; } --> Cellmanager.in++;
        	PV_modul[k].in <-- {  delay = 30ms; } <-- Cellmanager.out++;
        }
        for j=0..num_storageModul-1 {
        	StorageModul[j].out --> {  delay = 30ms; } --> Cellmanager.in++;
        	StorageModul[j].in <-- {  delay = 30ms; } <-- Cellmanager.out++;
        }
        for m=0..sizeof(in)-1{
            out++ <-- Cellmanager.out++;
        }
        for n=0..sizeof(out)-1{
            in++ --> Cellmanager.in++;
        }

}
