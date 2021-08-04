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

network L4_Level
{
    parameters:
        int num_L5_Cell;

        @display("bgb=234,237");
    submodules:
        L5_Cell[num_L5_Cell]: L5_Level;
    connections allowunconnected:
        for i=0..num_L5_Cell-2, for j=i+1..num_L5_Cell-1 {
            L5_Cell[i].out++ --> {  delay = 30ms; } --> L5_Cell[j].in++;
            L5_Cell[i].in++ <-- {  delay = 30ms; } <-- L5_Cell[j].out++;
        }
}
