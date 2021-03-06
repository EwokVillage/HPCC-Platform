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

import sq;
sq.DeclareCommon();

#option ('childQueries', true);
#option ('optimizeChildSource',true);

// Test skipped level iterating - including iterating grand children of processed children

// Different child operators, all inline.
house := sqHousePersonBookDs;
persons := sqHousePersonBookDs.persons;
books := persons.books;

output(house, { count(persons(dob < 19700101).books) });


deduped := dedup(persons, surname);

output(house, { count(deduped(dob < 19700101).books) });
