/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright (C) 2015-2026 Leandro Nini
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "utpp/utpp.h"

#include "../src/sidmd5.h"

using namespace UnitTest;

SUITE(MD5)
{

TEST(TestMD5)
{
    constexpr int c = 7;

    static const char *const test[c] = {
        "",
        "a",
        "abc",
        "message digest",
        "abcdefghijklmnopqrstuvwxyz",
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
        "12345678901234567890123456789012345678901234567890123456789012345678901234567890"
    };

    /*
     * The test should produce the same values as given in section
     * A.5 of RFC 1321.
     */
    static const char *const res[c] = {
        "d41d8cd98f00b204e9800998ecf8427e",
        "0cc175b9c0f1b6a831c399e269772661",
        "900150983cd24fb0d6963f7d28e17f72",
        "f96b697d7cb7938d525a2f31aaf161d0",
        "c3fcd3d76192e4007dfb496cca67e13b",
        "d174ab98d277d9f5a5611c2c9f419d9f",
        "57edf4a22be3c955ac49da2e2107b67a"
    };

    for (int i = 0; i < c; ++i)
    {
        libsidplayfp::sidmd5 myMD5;
        myMD5.append((const uint8_t*)test[i], strlen(test[i]));

        auto digest = myMD5.getDigest();

        CHECK_EQUAL(res[i], digest);
    }
}

}
