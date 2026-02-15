/*
 *  Copyright (C) 2010-2020 Leandro Nini
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

#ifndef INIHANDLER_H
#define INIHANDLER_H

#include <string>
#include <vector>

#include "types.h"

class iniHandler
{
private:
    using stringPair_t = std::pair<SID_STRING, SID_STRING>;
    using keys_t = std::vector<stringPair_t>;

    using keyPair_t = std::pair<SID_STRING, keys_t>;
    using sections_t = std::vector<keyPair_t>;

    class parseError {};

private:
    sections_t sections;

    sections_t::iterator curSection;

    SID_STRING fileName;

    bool changed;

private:
    SID_STRING parseSection(const SID_STRING &buffer);

    stringPair_t parseKey(const SID_STRING &buffer);

public:
    iniHandler();
    ~iniHandler();

    SID_STRING getFilename() const { return fileName; }

    bool tryOpen(const TCHAR *fName);
    bool open(const TCHAR *fName);
    bool write(const TCHAR *fName);
    void close();

    bool setSection(const TCHAR *section);
    const TCHAR *getValue(const TCHAR *key) const;

    void addSection(const TCHAR *section);
    void addValue(const TCHAR *key, const TCHAR *value);
    void removeValue(const TCHAR *key);
};

#endif // INIHANDLER_H
