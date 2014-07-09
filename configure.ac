AC_INIT(lustreparallelread,0.0.0,[gt1@sanger.ac.uk],[lustreparallelread],[http://www.sanger.ac.uk])
LIBRARY_VERSION=0:0:0
AC_CANONICAL_SYSTEM
AC_PROG_LIBTOOL

AM_INIT_AUTOMAKE([std-options subdir-objects])
AM_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])

CFLAGS_SAVE="${CFLAGS}"
CFLAGS=""
AC_PROG_CC
CFLAGS="${CFLAGS_SAVE}"

AM_PROG_CC_C_O

AC_HEADER_STDC

CPPFLAGS="${CPPFLAGS} -W -Wall"

AC_ARG_ENABLE(optimization,
        AS_HELP_STRING([--enable-optimization],[use compiler optimization (default yes)]),
        [optimization=${enableval}],[optimization=yes])
AC_ARG_ENABLE(debug,
        AS_HELP_STRING([--enable-debug],[use compiler debug flags (default no)]),
        [debug=${enableval}],[debug=no])
AC_ARG_ENABLE(profile,
        AS_HELP_STRING([--enable-profile],[use compiler profiling flags (default no)]),
        [profile=${enableval}],[profile=no])
AC_ARG_ENABLE(fast,
        AS_HELP_STRING([--enable-fast],[disable evaluation of assertions (default no)]),
        [fast=${enableval}],[fast=no])

if test "${fast}" = "yes" ; then
	CPPFLAGS="${CPPFLAGS} -DNDEBUG"
fi

if test "${debug}" = "yes" ; then
        CFLAGS="${CFLAGS} -g -O0 -rdynamic"
	openmp="no"
else
	if test "${profile}" = "yes" ; then
	        CFLAGS="${CFLAGS} -g -pg -rdynamic"
	else
		if test "${optimization}" = "yes" ; then
			case ${CC} in
				gcc)
					CFLAGS="${CFLAGS} -O3 -rdynamic"
					;;
			esac
		fi

	fi
fi

AC_CHECK_HEADER(sys/vfs.h, [sys_vfs_h=yes], [sys_vfs_h=no])

if test ${sys_vfs_h} = "no" ; then
	AC_MSG_ERROR([Required include file sys/vfs.h is missing.])
fi

AC_CHECK_HEADER(lustre/liblustreapi.h, [lustre_liblustreapi_h=yes], [lustre_liblustreapi_h=no])

if test ${lustre_liblustreapi_h} = "no" ; then
	AC_MSG_ERROR([Required include file lustre/liblustreapi.h is missing.])
fi

AC_CHECK_HEADER(semaphore.h, [semaphore_h=yes], [semaphore_h=no])

if test ${semaphore_h} = "no" ; then
	AC_MSG_ERROR([Required include file semaphore.h is missing.])
fi

if test 0 -eq 1 ; then
AC_CHECK_HEADER(lustre/lustre_idl.h, [lustre_lustre_idl_h=yes], [lustre_lustre_idl_h=no])

if test ${lustre_lustre_idl_h} = "no" ; then
	AC_MSG_ERROR([Required include file lustre/lustre_idl.h is missing.])
fi
fi

AC_CHECK_LIB([lustreapi],[llapi_file_get_stripe])

if test "${ac_cv_lib_lustreapi_llapi_file_get_stripe}" != "yes" ; then
	AC_MSG_ERROR([Required function llapi_file_get_stripe not found in lustreapi library])
fi

dnl LIBS="${LIBS} -llustreapi"

CFLAGS_SAVE="${CFLAGS}"
CFLAGS="-pthread"
AC_CHECK_FUNC([sem_init],[sem_init=yes],[sem_init=no])
dnl AC_MSG_RESULT([${sem_init}])
CFLAGS="${CFLAGS_SAVE}"

if test "${sem_init}" = "no" ; then
	AC_MSG_ERROR([sem_init function not found])
fi

CFLAGS_SAVE="${CFLAGS}"
CFLAGS="-pthread"
AC_CHECK_FUNC([sem_post],[sem_post=yes],[sem_post=no])
dnl AC_MSG_RESULT([${sem_post}])
CFLAGS="${CFLAGS_SAVE}"

if test "${sem_post}" = "no" ; then
	AC_MSG_ERROR([sem_post function not found])
fi

CFLAGS_SAVE="${CFLAGS}"
CFLAGS="-pthread"
AC_CHECK_FUNC([sem_wait],[sem_wait=yes],[sem_wait=no])
dnl AC_MSG_RESULT([${sem_wait}])
CFLAGS="${CFLAGS_SAVE}"

if test "${sem_wait}" = "no" ; then
	AC_MSG_ERROR([sem_wait function not found])
fi

CFLAGS_SAVE="${CFLAGS}"
CFLAGS="-pthread"
AC_CHECK_FUNC([sem_destroy],[sem_destroy=yes],[sem_destroy=no])
dnl AC_MSG_RESULT([${sem_destroy}])
CFLAGS="${CFLAGS_SAVE}"

if test "${sem_destroy}" = "no" ; then
	AC_MSG_ERROR([sem_destroy function not found])
fi

CFLAGS_SAVE="${CFLAGS}"
CFLAGS="-pthread"
AC_CHECK_FUNC([sem_trywait],[sem_trywait=yes],[sem_trywait=no])
dnl AC_MSG_RESULT([${sem_trywait}])
CFLAGS="${CFLAGS_SAVE}"

if test "${sem_trywait}" = "no" ; then
	AC_MSG_ERROR([sem_trywait function not found])
fi
                     
AC_CHECK_HEADER(sys/types.h, [sys_types_h=yes], [sys_types_h=no])

if test ${sys_types_h} = "no" ; then
	AC_MSG_ERROR([Required include file sys/types.h is missing.])
fi

AC_CHECK_HEADER(sys/stat.h, [sys_stat_h=yes], [sys_stat_h=no])

if test ${sys_stat_h} = "no" ; then
	AC_MSG_ERROR([Required include file sys/stat.h is missing.])
fi

AC_CHECK_HEADER(unistd.h, [unistd_h=yes], [unistd_h=no])

if test ${unistd_h} = "no" ; then
	AC_MSG_ERROR([Required include file unistd.h is missing.])
fi

AC_CHECK_HEADER(fcntl.h, [fcntl_h=yes], [fcntl_h=no])

if test ${fcntl_h} = "no" ; then
	AC_MSG_ERROR([Required include file fcntl.h is missing.])
fi


PACKAGE_NAME=${PACKAGE}
PACKAGE_VERSION=${VERSION}

AC_SUBST([PACKAGE_NAME])
AC_SUBST([PACKAGE_VERSION])
AC_SUBST([LIBRARY_VERSION])
AC_SUBST([VERSION])
AC_OUTPUT(Makefile src/Makefile liblustreparallelread.pc)
