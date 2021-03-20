/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2019 Leandro Nini <drfiemost@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <cstdlib>
#include <cstring>

#include "sidplayfp/sidplayfp.h"
#include "sidplayfp/SidTune.h"
#include "sidplayfp/sidbuilder.h"
#include "sidplayfp/builders/residfp.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

/*
 * Adjust these paths to point to existing ROM dumps
 */
#define KERNAL_PATH "/usr/share/vice/C64/kernal"
#define BASIC_PATH "/usr/share/vice/C64/basic"
#define CHARGEN_PATH "/usr/share/vice/C64/chargen"

void loadRom(const char* path, char* buffer)
{
    std::ifstream is(path, std::ios::binary);
    if (!is.is_open())
    {
        std::cout << "File " << path << " not found" << std::endl;
        exit(EXIT_FAILURE);
    }
    is.read(buffer, 8192);
    is.close();
}

int main(int argc, char* argv[])
{
    if (argc == 0)
    {
        std::cerr << "Missing test name" << std::endl;
        return -1;
    }

    sidplayfp m_engine;

    char kernal[8192];
    char basic[8192];
    char chargen[4096];

    loadRom(KERNAL_PATH, kernal);
    loadRom(BASIC_PATH, basic);
    loadRom(CHARGEN_PATH, chargen);

    m_engine.setRoms((const uint8_t*)kernal, (const uint8_t*)basic, (const uint8_t*)chargen);
    SidConfig config = m_engine.config();
    config.powerOnDelay = 0x1267;

    std::string name(VICE_TESTSUITE);

    int i = 1;
    while ((i < argc) && (argv[i] != nullptr))
    {
        if ((argv[i][0] == '-') && (argv[i][1] != '\0'))
        {
            if (!strcmp(&argv[i][1], "-sid"))
            {
                i++;
                if (!strcmp(&argv[i][0], "old"))
                {
                    config.sidEmulation = new ReSIDfpBuilder("test");
                    config.sidEmulation->create(1);
                    config.forceSidModel = true;
                    config.defaultSidModel = SidConfig::MOS6581;
                }
                else
                if (!strcmp(&argv[i][0], "new"))
                {
                    config.sidEmulation = new ReSIDfpBuilder("test");
                    config.sidEmulation->create(1);
                    config.forceSidModel = true;
                    config.defaultSidModel = SidConfig::MOS8580;
                }
            }
            if (!strcmp(&argv[i][1], "-cia"))
            {
                i++;
                if (!strcmp(&argv[i][0], "old"))
                {
                    config.ciaModel = SidConfig::MOS6526;
                }
                else
                if (!strcmp(&argv[i][0], "new"))
                {
                    config.ciaModel = SidConfig::MOS8521;
                }
            }
            if (!strcmp(&argv[i][1], "-vic"))
            {
                i++;
                if (!strcmp(&argv[i][0], "pal"))
                {
                    config.defaultC64Model = SidConfig::PAL;
                    config.forceC64Model = true;
                }
                else
                if (!strcmp(&argv[i][0], "ntsc"))
                {
                    config.defaultC64Model = SidConfig::NTSC;
                    config.forceC64Model = true;
                }
                else
                if (!strcmp(&argv[i][0], "oldntsc"))
                {
                    config.defaultC64Model = SidConfig::OLD_NTSC;
                    config.forceC64Model = true;
                }
            }
        }
        else
        {
            name.append(argv[i]).append(".prg");
        }

        i++;
    }

    m_engine.config(config);
    std::auto_ptr<SidTune> tune(new SidTune(name.c_str()));

    if (!tune->getStatus())
    {
        std::cerr << "Error: " << tune->statusString() << std::endl;
        return -1;
    }

    tune->selectSong(0);

    if (!m_engine.load(tune.get()))
    {
        std::cerr << m_engine.error() << std::endl;
        return -1;
    }

    //m_engine.debug(true, nullptr);

    for (;;)
    {
        m_engine.play(nullptr, 0);
        std::cerr << ".";
    }
}
