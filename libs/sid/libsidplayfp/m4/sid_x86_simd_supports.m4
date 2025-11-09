# sid_x86_simd_supports.m4
#
# This file is part of libsidplayfp, a SID player engine.
#
#   Copyright (C) 2025 Leandro Nini
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

AC_DEFUN([SID_X86_SIMD_SUPPORTS],
  [AX_REQUIRE_DEFINED([AX_APPEND_FLAG])
    AC_MSG_CHECKING([for $1 support])
    sid_check_save_flags=$[]_AC_LANG_PREFIX[]FLAGS
    _AC_LANG_PREFIX[]FLAGS="$[]_AC_LANG_PREFIX[]FLAGS -m$1"

    AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM(
            [#include <immintrin.h>],
            [return 0;])],
        [
            AX_APPEND_FLAG([-m$1])
            AC_MSG_RESULT([yes])
        ],
        [
            AC_MSG_RESULT([no])
        ])

    _AC_LANG_PREFIX[]FLAGS=$sid_check_save_flags
])
