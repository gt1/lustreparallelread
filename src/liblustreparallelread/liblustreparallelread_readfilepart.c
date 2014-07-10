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
#include <liblustreparallelread/liblustreparallelread_getstripeinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>       
#include <unistd.h>

#include <pthread.h>
#include <semaphore.h>

typedef struct _StripeReadInfo
{
	char const * fn;
	uint64_t stripe_count;
	uint64_t stripe_size;
	uint64_t * stripesleft;
	uint64_t * stripesproc;
	uint64_t baselow;
	uint64_t from;
	uint64_t to;
	uint64_t numthreads;
	pthread_t * threads;
	int mutexinitok;
	pthread_mutex_t mutex;
	int spinlockinitok;
	pthread_spinlock_t spinlock;
	int seminitok;
	sem_t sema;
	uint64_t * stripereadspending;
	uint64_t stripereadspendingfill;
	int * fds;
	char * mem;
	int failed;
} StripeReadInfo;

int StripeReadInfo_join(StripeReadInfo * obj)
{
	if ( obj->threads )
	{
		uint64_t i = 0;
		for ( i = 0; i < obj->numthreads; ++i )
			if ( obj->threads[i] )
			{
				void * retval = 0;
				pthread_join(obj->threads[i],&retval);
			}
	
		free(obj->threads);
		obj->threads = 0;
	}

	return obj->failed;
}

StripeReadInfo * StripeReadInfo_delete(StripeReadInfo * obj)
{
	if ( obj )
	{
		if ( obj->threads )
		{
			uint64_t i = 0;
			for ( i = 0; i < obj->numthreads; ++i )
				if ( obj->threads[i] )
				{
					void * retval = 0;
					pthread_join(obj->threads[i],&retval);
				}
		
			free(obj->threads);
			obj->threads = 0;
		}
		if ( obj->stripesleft )
		{
			free(obj->stripesleft);
			obj->stripesleft = 0;
		}
		if ( obj->stripesproc )
		{
			free(obj->stripesproc);
			obj->stripesproc = 0;
		}
		if ( obj->seminitok )
		{
			sem_destroy(&(obj->sema));
			obj->seminitok = 0;
		}
		if ( obj->mutexinitok )
		{
			pthread_mutex_destroy(&(obj->mutex));
		}		
		if ( obj->spinlockinitok )
		{
			pthread_spin_destroy(&(obj->spinlock));
		}		
		if ( obj->stripereadspending )
		{
			free(obj->stripereadspending);
			obj->stripereadspending = 0;
		}
		if ( obj->fds )
		{
			uint64_t i = 0;
			for ( i = 0; i < obj->stripe_count; ++i )
				if ( obj->fds[i] != -1 )
				{
					close(obj->fds[i]);
					obj->fds[i] = -1;
				}
			free(obj->fds);
			obj->fds = 0;
		}
		
		free(obj);
		obj = 0;
	}
	
	return obj;
}

static uint64_t maxval(uint64_t a, uint64_t b)
{
	return a>b ? a:b;
}

static uint64_t minval(uint64_t a, uint64_t b)
{
	return a<b ? a:b;
}

static ssize_t doread(int fd, char * p, uint64_t s)
{
	// number of bytes read so far
	ssize_t r = 0;
	
	// while we want to read more
	while ( s )
	{
		ssize_t rr = read(fd,p,s);

		// failed?		
		if ( rr < 0 )
			return rr;
		// eof?
		else if ( rr == 0 )
			return r;
		// got some data
		else
		{
			r += rr;
			p += rr;
			s -= rr;
		}
	}
	
	return r;
}

static void * liblustreparallelread_readfilepart_threadstart(void * voidobj)
{
	StripeReadInfo * obj = (StripeReadInfo *)voidobj;
	int running = 1;
	
	while ( running )
	{
		int rc = sem_wait(&(obj->sema));
		
		if ( rc != 0 )
		{
			switch ( errno )
			{
				case EINTR:
				case EAGAIN:
					break;
				default:
					running = 0;
			}
		}
		else
		{
			uint64_t pendid = 0;
			
			pthread_spin_lock(&(obj->spinlock));
		
			if ( ! obj->stripereadspendingfill )
			{
				running = 0;
			}
			else
			{
				pendid = obj->stripereadspending[--obj->stripereadspendingfill];
			}
			
			pthread_spin_unlock(&(obj->spinlock));
			
			if ( running )
			{
				if ( obj->stripesleft[pendid] )
				{
					uint64_t stripestart = 
						maxval(
							obj->baselow +
							obj->stripesproc[pendid] * obj->stripe_count * obj->stripe_size +
							pendid * obj->stripe_size,
							obj->from
						);
					uint64_t stripeend = 
						minval(
							stripestart + obj->stripe_size,
							obj->to
						);
						
					int seekrc = lseek(obj->fds[pendid],stripestart,SEEK_SET);
					ssize_t readrc = -1;
					
					if ( seekrc == (off_t) -1 )
					{
						pthread_mutex_lock(&(obj->mutex));
						fprintf(stderr,"Failed to seek file to position %" PRIu64 "\n", stripestart);
						pthread_mutex_unlock(&(obj->mutex));		
						
						obj->failed = 1;
					}
					
					readrc = doread(
						obj->fds[pendid],
						obj->mem + (stripestart - obj->from),
						stripeend-stripestart
					);

					if ( readrc != (int64_t)(stripeend-stripestart) )
					{
						pthread_mutex_lock(&(obj->mutex));
						fprintf(stderr,"Failed to read file from position %" PRIu64 "\n", stripestart);
						pthread_mutex_unlock(&(obj->mutex));
						
						obj->failed = 1;
					}

					obj->stripesproc[pendid]++;
					obj->stripesleft[pendid]--;
					
					pthread_spin_lock(&(obj->spinlock));
					obj->stripereadspending[obj->stripereadspendingfill++] = pendid;
					pthread_spin_unlock(&(obj->spinlock));

					sem_post(&(obj->sema));
				}
				else
				{
					uint64_t numpend;
					
					pthread_spin_lock(&(obj->spinlock));
					numpend = obj->stripereadspendingfill;					
					pthread_spin_unlock(&(obj->spinlock));
			
					/* if there are no more pending stripes */
					if ( ! numpend )
					{
						uint64_t i = 0;
						for ( i = 0; i < obj->numthreads; ++i )
							sem_post(&(obj->sema));
					}
				}
			}
			else
			{
			}
		}
	}
	
	return 0;
}

StripeReadInfo * StripeReadInfo_new(
	char const * rfn,
	unsigned int const stripe_count, 
	unsigned int const stripe_size,
	uint64_t rfrom, uint64_t rto, uint64_t rnumthreads,
	char * rmem
)
{
	StripeReadInfo * obj = (StripeReadInfo *)malloc(sizeof(StripeReadInfo));
	uint64_t const rbaselow = ( rfrom / stripe_size ) * stripe_size;
	uint64_t const numstripes = ( rto - rbaselow + stripe_size - 1 ) / stripe_size;
	uint64_t i = 0;
	
	if ( ! obj )
		return StripeReadInfo_delete(obj);
	
	memset(obj,0,sizeof(StripeReadInfo));
	
	obj->fn = rfn;
	obj->stripe_count = stripe_count;
	obj->stripe_size = stripe_size;
	obj->failed = 0;
	
	obj->stripesleft = (uint64_t *)malloc(stripe_count * sizeof(uint64_t));
	
	if ( ! obj->stripesleft )
		return StripeReadInfo_delete(obj);

	obj->stripesproc = (uint64_t *)malloc(stripe_count * sizeof(uint64_t));
	
	if ( ! obj->stripesproc )
		return StripeReadInfo_delete(obj);
		
	memset(obj->stripesproc,0,stripe_count * sizeof(uint64_t));
	
	obj->threads = (pthread_t *)malloc(rnumthreads * sizeof(pthread_t));
	
	if ( ! obj->threads )
		return StripeReadInfo_delete(obj);
	
	memset(obj->threads,0,rnumthreads * sizeof(pthread_t));
	
	if ( pthread_mutex_init(&(obj->mutex),0) != 0 )
	{
		obj->mutexinitok = 0;
		return StripeReadInfo_delete(obj);
	}
	else
	{
		obj->mutexinitok = 1;
	}

	if ( pthread_spin_init(&(obj->spinlock),0) != 0 )
	{
		obj->spinlockinitok = 0;
		return StripeReadInfo_delete(obj);
	}
	else
	{
		obj->spinlockinitok = 1;
	}

	if ( sem_init(&(obj->sema),0,0) )
	{
		obj->seminitok = 0;
		return StripeReadInfo_delete(obj);
	}	
	else
	{
		obj->seminitok = 1;
	}

	obj->stripereadspending = (uint64_t *)malloc(stripe_count * sizeof(uint64_t));
	
	if ( ! obj->stripereadspending )
		return StripeReadInfo_delete(obj);
		
	memset(obj->stripereadspending,0,stripe_count * sizeof(uint64_t));

	obj->fds = (int *)malloc(stripe_count * sizeof(int));
	
	if ( ! obj->fds )
		return StripeReadInfo_delete(obj);
		
	memset(obj->fds,0,stripe_count * sizeof(int));
	
	for ( i = 0; i < stripe_count; ++i )
		obj->fds[i] = -1;

	obj->stripereadspendingfill = 0;
		
	obj->baselow = rbaselow;
	obj->from = rfrom;
	obj->to = rto;
	obj->numthreads = rnumthreads;
	obj->mem = rmem;

	for ( i = 0; i < (uint64_t)stripe_count; ++i )
		obj->stripesleft[i] = numstripes / stripe_count;		
	for ( i = 0; i < (numstripes % stripe_count); ++i )
		obj->stripesleft[i]++;

	for ( i = 0; i < (uint64_t)stripe_count; ++i )
		if ( obj->stripesleft[i] )
		{
			int const fd = open(rfn,O_RDONLY);
			
			if ( fd < 0 )
				return StripeReadInfo_delete(obj);
				
			obj->fds[i] = fd;
			
			posix_fadvise(fd, 0, 0, POSIX_FADV_RANDOM);
			
			obj->stripereadspending[obj->stripereadspendingfill++] = i;
		}


	for ( i = 0; i < obj->numthreads; ++i )
	{
		if ( pthread_create(&(obj->threads[i]), 0, liblustreparallelread_readfilepart_threadstart, obj) != 0 )
		{			
			uint64_t j = 0;

			obj->threads[i] = 0;
			obj->stripereadspendingfill = 0;
			
			for ( j = 0; j < i; ++j )
				sem_post(&(obj->sema));

			return StripeReadInfo_delete(obj);
		}
	}

	for ( i = 0; i < (uint64_t)stripe_count; ++i )
		sem_post(&(obj->sema));
	
	return obj;
}

int liblustreparallelread_readfilepart(
	char const * fn, 
	uint64_t from, uint64_t to,
	unsigned int const numthreads,
	char ** mem,
	uint64_t * memsize
)
{
	struct stat sb;
	int rc = -1;
	int stripe_count = -1;
	int stripe_size = -1;
	int error = -1;
	StripeReadInfo * stripeinfo = 0;
	int memgiven = (*mem) != NULL;

	if ( ! memgiven )
	{
		*mem = 0;
		*memsize = 0;
	}
	
	rc = stat(fn,&sb);
	
	/* cannot stat file */
	if ( rc < 0 )
	{
		return -1;
	}
	
	/* file not regular */
	if ( ! S_ISREG(sb.st_mode) )
	{
		errno = ENOTSUP;
		return -1;
	}
	
	/* reduce to to file size */
	if ( to > (uint64_t)sb.st_size )
		to = sb.st_size;
		
	/* check for invalid input range */
	if ( from > to )
	{
		errno = EINVAL;
		return -1;
	}

	/* if user does not provide a memory block then allocate one */
	if ( ! memgiven )
	{
		*mem = (char *)malloc(to-from);
		*memsize = to-from;
	}
	else
	{
		/* return error if given memory block is too small */
		if ( *memsize < (to-from) )
		{
			errno = EINVAL;
			return -1;
		}
	}
	
	if ( ! *mem )
	{
		errno = ENOMEM;
		return -1;
	}
		
	rc = liblustreparallelread_getstripeinfo(fn, &stripe_count, &stripe_size);
	
	if ( rc < 0 )
	{
		FILE * file = fopen(fn,"rb");
		
		if ( ! file )
			goto cleanupfail;
			
		if ( from != 0 )
		{
			if ( fseek(file, from, SEEK_SET) == -1 )
			{
				fclose(file);
				goto cleanupfail;
			}
		}

		if ( fread(*mem,to-from,1,file) != 1 )
		{
			fclose(file);
			goto cleanupfail;
		}
		
		fclose(file);
		
		return 0;
	}
	else
	{
		stripeinfo = StripeReadInfo_new(
			fn,stripe_count,stripe_size,from,to,numthreads,*mem
		);
		
		if ( ! stripeinfo )
		{
			errno = ENOMEM;
			goto cleanupfail;
		}
		
		int const failed = StripeReadInfo_join(stripeinfo);
		
		stripeinfo = StripeReadInfo_delete(stripeinfo);
		
		return failed ? -1 : 0;
	}
		
	cleanupfail:
	error = errno;
	stripeinfo = StripeReadInfo_delete(stripeinfo);
	if ( ! memgiven )
	{
		free(*mem);
		*mem = 0;
		*memsize = 0;
	}
	errno = error;
	return -1;
}
