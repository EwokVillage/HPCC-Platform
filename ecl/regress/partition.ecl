/*##############################################################################

    Copyright (C) 2011 HPCC Systems.

    All rights reserved. This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
############################################################################## */

myrec := record
string10 name;
unsigned4   value;
unsigned6   age;
string20    pad;
string20    key;
        end;


myfile1 := dataset('in1', myrec, thor);

f1 := distribute(myfile1, partition(age, pad, key));

output(f1,,'out1.d00');

f2 := partition(myfile1, key, value);

output(f2,,'out2.d00');
