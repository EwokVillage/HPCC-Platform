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

//Normalize a denormalised dataset...

householdRecord := RECORD
unsigned4 house_id;
string20  address1;
string20  zip;
    END;


childPersonRecord := RECORD
unsigned4 person_id;
string20  surname;
string20  forename;
    END;

personRecord := RECORD
unsigned4 house_id;
childPersonRecord x;
    END;

combinedRecord :=
                RECORD
householdRecord;
DATASET(childPersonRecord)   children;
                END;


personDataset := DATASET('person',personRecord,FLAT);
householdDataset := DATASET('household',householdRecord,FLAT);
combinedDataset := DATASET('combined',combinedRecord,FLAT);



combinedRecord householdToCombined(householdRecord l) :=
                TRANSFORM
                    SELF.children := [];
                    SELF := l;
                END;


combinedRecord doDenormalize(combinedRecord l, personRecord r) :=
                TRANSFORM
                    SELF.children := l.children + r.x;
                    SELF := l;
                END;


o1 := PROJECT(householdDataset, householdToCombined(LEFT));

o2 := denormalize(o1, personDataset, LEFT.house_id = RIGHT.house_id, doDenormalize(LEFT, RIGHT));

output(o2,,'out.d00');
