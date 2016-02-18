#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([waltr], [0.01], [schurchill@esica.com], [waltr])
AC_CONFIG_SRCDIR([src/waltr.cpp])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.11 -Wall -Werror])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_MACRO_DIR([m4])


# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.

AX_BOOST_BASE([1.48],,)

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.

AC_OUTPUT
