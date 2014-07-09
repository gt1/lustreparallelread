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
#include <stdlib.h>
#include <stdio.h>
#include <sys/vfs.h>
#include <errno.h>

#undef LASSERT
#undef LASSERTF
#undef LPX64
#define LPX64
#define LASSERT(cond) if (!(cond)) { printf("failed " #cond "\n"); }
#define LASSERTF(cond, fmt, arg) if (!(cond)) { printf("failed '" #cond "'" fmt, arg);}

#include <lustre/liblustreapi.h>
#include <lustre/lustre_user.h>
#include <lustre/lustre_idl.h>

static unsigned int maxu(unsigned int a, unsigned int b)
{
	return a>=b ? a : b;
}

/*
 * structure allocation as documented at 
 * http://wiki.lustre.org/manual/LustreManual20_HTML/SettingLustreProperties_HTML.html
 */
static void * alloc_lum()
{
	unsigned int const v1 = sizeof(struct lov_user_md_v1) + LOV_MAX_STRIPE_COUNT * sizeof(struct lov_user_ost_data_v1);
	unsigned int const v3 = sizeof(struct lov_user_md_v3) + LOV_MAX_STRIPE_COUNT * sizeof(struct lov_user_ost_data_v1);
	return malloc(maxu(v1, v3));
}

int liblustreparallelread_getstripeinfo(char const * filename, int * stripe_count, int * stripe_size)
{
	struct lov_user_md *lum_file = NULL;
	int rc = -1;
	struct statfs sfs;

	if ( ! stripe_count )
	{
		errno = EINVAL;
		return -1;
	}

	if ( ! stripe_size )
	{
		errno = EINVAL;
		return -1;
	}

	int const r = statfs(filename,&sfs);
	if ( r < 0 )
	{
		return -1;
	}

	/* return error if file system is not lustre */
	if ( sfs.f_type != 0xbd00bd0 )
	{
		errno = ENOTSUP;
		return -1;
	}
		
	lum_file = alloc_lum();
	
	if ( ! lum_file )
	{
		errno = ENOMEM;
		return -1;
	}
	
	rc = llapi_file_get_stripe(filename,lum_file);
	
	if ( rc )
	{
		free(lum_file);
		return -1;
	}
	
	*stripe_count = lum_file->lmm_stripe_count;
	*stripe_size = lum_file->lmm_stripe_size;
	
	free(lum_file);

	return 0;
}
