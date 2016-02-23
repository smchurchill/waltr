#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([waltr], [0.01], [schurchill@esica.com], [waltr])
AC_CONFIG_SRCDIR([src/waltr.cpp])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR([m4])


# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Call a special macro to ensure that we use C++11 standard ... in the year 2016.

AX_CXX_COMPILE_STDCXX_11()

# Checks for libraries.
# Boost:
AX_BOOST_BASE
AX_BOOST_ASIO
AX_BOOST_CHRONO
AX_BOOST_FILESYSTEM
AX_BOOST_PROGRAM_OPTIONS
AX_BOOST_SYSTEM

# Protobuf:
PKG_CHECK_MODULES([protobuf], [protobuf])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([memset])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
