## Process this file to produce a configure script:
##  autoreconf -i
##

## Preamble - used to set up meta paths, meta-information, etc.
# require a minimum version of AutoConf
AC_PREREQ([2.65])

# software name, version, contact address
AC_INIT([knot_lib_drivers],[1.0.0])

# if this file does not exist, `configure` was invoked in the wrong directory
AC_CONFIG_SRCDIR([src/abstract_driver.c])

# directories (relative to top-level) to look into for AutoConf/AutoMake files
#AC_CONFIG_AUX_DIR([build-aux])
#AC_CONFIG_MACRO_DIR([build-aux])
# enable AutoMake
AM_INIT_AUTOMAKE([1.10])
AM_MAINTAINER_MODE
# all defined C macros (HAVE_*) will be saved to this file
AC_CONFIG_HEADERS([config.h])

# Macros for the compilers. 
# This macro checks if you have a C compiler
AC_PROG_CC
AM_PROG_CC_C_O

# Check if you have a C++ compiler
AC_PROG_CXX
AC_PROG_CXX_C_O

# Check if the `install` program is present
AC_PROG_INSTALL

## Initialize GNU LibTool
LT_INIT([static])

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stddef.h string.h])
# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Substitute all conditionals in these files; this is normally used to
# create `Makefile`s but could also be used for scripts, include
# files, etc.
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
