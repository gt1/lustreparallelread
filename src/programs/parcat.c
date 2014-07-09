/*
    liblustreparallelread
    Copyright (C) 2014 German Tischler
    Copyright (C) 2014 Genome Research Limited

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
#include <liblustreparallelread/liblustreparallelread_readfilepart.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char * argv[])
{
	unsigned int const numthreads = 32;
	int z = 0;

	for ( z = 1; z < argc; ++z )
	{	
		int rc = -1;
		char * mem = 0;
		char * smem = 0;
		uint64_t memsize = 0;
		char const * fn = argv[z];
		
		rc = liblustreparallelread_readfilepart(fn,0,UINT64_MAX,numthreads,&mem,&memsize);
		
		if ( rc < 0 )
		{
			int const error = errno;
			fprintf(stderr, "liblustreparallelread_readfilepart failed: %s\n", strerror(error));
			return EXIT_FAILURE;
		}
		
		smem = mem;
		
		while ( memsize )
		{
			ssize_t wrret = write ( STDOUT_FILENO, mem, memsize );
		
			if ( wrret < 0 )
				break;
			else
			{
				mem += wrret;
				memsize -= wrret;
			}
		}
		
		free(smem);
		smem = 0;
		mem = 0;
	}
		
	return EXIT_SUCCESS;
}
