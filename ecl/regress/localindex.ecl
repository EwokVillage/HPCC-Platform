/*##############################################################################

    Copyright (C) <2010>  <LexisNexis Risk Data Management Inc.>

    All rights reserved. This program is NOT PRESENTLY free software: you can NOT redistribute it and/or modify
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

namesRecord :=
            RECORD
string20        surname;
string10        forename;
integer2        age := 25;
            END;

namesTable := dataset([
        {'Hawthorn','Gavin',31},
        {'Hawthorn','Mia',30},
        {'Smithe','Pru',10},
        {'X','Z'}], namesRecord);

i := index(namesTable,,'temp::names');
build(i);
output(i(surname = 'Smithe'));
