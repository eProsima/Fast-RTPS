# Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#
# Autoconf script FastRTPS
#

#
# Define variables for the FastRTPS version number.
#
m4_define([version_major],[2])
m4_define([version_minor],[0])
m4_define([version_micro],[3])

AC_INIT([fastrtps], [version_major.version_minor.version_micro], [support@eprosima.com], [eProsima FastRTPS], [http://eprosima.com/])
CONFIG_ARGS="$*"
AC_SUBST(CONFIG_ARGS)

#Minimum autoconf version we require.
AC_PREREQ([2.59])

AM_INIT_AUTOMAKE([1.10 no-define foreign subdir-objects])

# Makes version available in config.h
AC_DEFINE(FASTRTPS_VERSION_MAJOR, version_major, [fastrtps version major])
AC_DEFINE(FASTRTPS_VERSION_MINOR, version_minor, [fastrtps version minor])
AC_DEFINE(FASTRTPS_VERSION_MICRO, version_micro, [fastrtps version micro])

# Check stdcxx
AX_CXX_COMPILE_STDCXX_11(,optional)
AS_IF([test $HAVE_CXX11 = 1],
      [AC_DEFINE(HAVE_CXX0X, 1, ["Define if g++ supports C++0x features. "]),
      AC_DEFINE(HAVE_CXX_CONSTEXPR, 1, [""])],
      [AC_DEFINE(HAVE_CXX_CONSTEXPR, 0, [""]), AX_CXX_COMPILE_STDCXX_0X()])

# Check constexpr
AS_IF([test $HAVE_CXX11 = 1],
      [AC_DEFINE(HAVE_CXX_CONSTEXPR, 1, ["Define if g++ support constexpr feature. "])],
      [AC_DEFINE(HAVE_CXX_CONSTEXPR, 0, ["Define if g++ support constexpr feature. "])])

# Check endianess
AC_C_BIGENDIAN(
  AC_DEFINE(__BIG_ENDIAN__, 1, [machine is bigendian]),
  AC_DEFINE(__BIG_ENDIAN__, 0, [machine is littleendian]),
  AC_MSG_ERROR(unknown endianess),
  AC_MSG_ERROR(universial endianess not supported)
)

# OpenSSL
AX_CHECK_OPENSSL(ac_openssl_found=yes,
                 ac_openssl_found=no)

AS_IF([test "$ac_openssl_found" = yes],
     AC_DEFINE([TLS_FOUND], [1], [Defined if TLS support is enable]),
     AC_DEFINE([TLS_FOUND], [0], [Defined if TLS support is enable]))

# Security
AC_ARG_ENABLE([security],
    AS_HELP_STRING([--enable-security], [Enables security support]),
    ac_enable_security=$enableval,
    ac_enable_security=no
    )
AS_IF([test "$ac_enable_security" = yes],
    AS_IF([test "$ac_openssl_found" = yes],
        AC_DEFINE([HAVE_SECURITY], [1], [Defined if security support is enable]),
        AC_MSG_ERROR([To support security you need OpenSSL libraries])),
    AC_DEFINE([HAVE_SECURITY], [0], [Defined if security support is enable]))
AM_CONDITIONAL(SECURITY, test $ac_enable_security = yes)

# Check for libraries used in the main build process
AC_PROG_CXX
AC_PROG_CPP

# Init libtool
AC_CONFIG_MACRO_DIR([m4])
LT_INIT

AC_CONFIG_HEADERS(include/fastrtps/config.h:include/fastrtps/config.h.in)
AC_CONFIG_FILES([Makefile
                 include/Makefile
                 src/cpp/Makefile
                 .bin/fastrtpsgen])

AC_OUTPUT
