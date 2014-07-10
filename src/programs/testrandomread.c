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
#include <time.h>
#include <inttypes.h>

#include <sys/types.h>
#include <sys/stat.h>

static int64_t getFileSize(char const * fn)
{
	struct stat sb;
	
	int const rc = stat(fn,&sb);
	
	if ( rc != 0 )
		return -1;
	
	return sb.st_size;
}

static uint64_t randomNumber(unsigned int const b)
{
	unsigned int i = 0;
	uint64_t v = 0;
	
	for ( i = 0; i < b; ++i )
	{
		v <<= 8;
		v |= (uint64_t)(random() & 0xFF);
	}
	
	return v;
}

int main(int argc, char * argv[])
{
	char const * fn = NULL;
	int64_t fs = -1;
	static int64_t const maxrelfs = 512*1024*1024;
	static unsigned int numthreads = 32;
	static int const runs = 128;
	int t = 0;
	int allok = 1;

	if ( !(1<argc) )
	{
		fprintf(stderr,"usage: %s <filename>\n",argv[0]);
		return EXIT_FAILURE;
	}
	
	fn = argv[1];
	
	srandom(time(NULL));

	if ( (fs=getFileSize(fn)) < 0 )
	{
		int const error = errno;
		fprintf(stderr,"Failed to obtain file size: %s\n", strerror(error));
		return EXIT_FAILURE;
	}
	
	fprintf(stderr,"Size of file is %" PRIu64 "\n", (uint64_t)fs);
	
	if ( fs > maxrelfs )
		fs = maxrelfs;
		
	for ( t = 0; allok && (t < runs); ++t )
	{
		uint64_t low = randomNumber(8) % (fs+1);
		uint64_t high = randomNumber(8) % (fs+1);
		int rc = -1;
		char * mem = 0;
		uint64_t memsize = 0;
		char * tmem = 0;
		FILE * file = NULL;
		int ok = 1;
		uint64_t i;
		
		if ( low > high )
		{
			uint64_t tlow = low;
			low = high;
			high = tlow;
		}
		

		rc = liblustreparallelread_readfilepart(fn,low,high,numthreads,&mem,&memsize);
		
		if ( rc < 0 )
		{
			int const error = errno;
			fprintf(stderr, "liblustreparallelread_readfilepart failed: %s\n", strerror(error));
			return EXIT_FAILURE;
		}
		
		tmem = (char *)malloc(high-low);
		
		if ( ! tmem )
		{
			int const error = ENOMEM;
			free(mem);
			fprintf(stderr, "verification failed: %s\n", strerror(error));
			return EXIT_FAILURE;
		}
		
		if ( ! (file=fopen(fn,"rb")) )
		{
			int const error = errno;
			free(mem);
			free(tmem);
			fprintf(stderr, "verification failed: %s\n", strerror(error));
			return EXIT_FAILURE;		
		}
		
		if ( fseek(file,low,SEEK_SET) == -1 )
		{
			int const error = errno;
			fclose(file);
			free(mem);
			free(tmem);
			fprintf(stderr, "verification failed: %s\n", strerror(error));
			return EXIT_FAILURE;				
		}
		
		if ( fread(tmem,high-low,1,file) != 1 )
		{
			int const error = errno;
			fclose(file);
			free(mem);
			free(tmem);
			fprintf(stderr, "verification failed: %s\n", strerror(error));
			return EXIT_FAILURE;
		}
		
		ok = 1;
		
		for ( i = 0; ok && i < (high-low); ++i )
			ok = ok && (mem[i] == tmem[i]);

		fprintf(stderr,"low %" PRIu64 " high %" PRIu64 " dif %" PRIu64 " %s\n", low, high, (high-low)/(1024*1024), (ok?"ok":"failed"));
						
		allok = allok && ok;
		
		fclose(file);
		free(tmem);
		free(mem);
	}

	if ( allok )
	{
		fprintf(stderr,"ok\n");
		return EXIT_SUCCESS;
	}
	else
	{
		fprintf(stderr,"failed\n");
		return EXIT_FAILURE;
	}
}
