/*
    liblustreparallelread
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#if ! defined(LUSTREPARALLELREAD_GETSTRIPE_H)
#define LUSTREPARALLELREAD_GETSTRIPE_H

#ifdef __cplusplus
extern "C" {
#endif

int liblustreparallelread_getstripeinfo(char const * filename, int * stripe_count, int * stripe_size);

#ifdef __cplusplus
}
#endif

#endif
