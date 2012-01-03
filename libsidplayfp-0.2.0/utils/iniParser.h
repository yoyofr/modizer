/*
 *  Copyright (C) 2010 Leandro Nini
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

#ifndef _INIPARSER_
#define _INIPARSER_

#include <string>
#include <iostream>
#include <fstream>
#include <map>

class iniParser {

public:
	static long parseLong(const char* str);

	static double parseDouble(const char* str);

private:
	typedef std::map<std::string, std::string> keys_t;

	std::map<std::string, keys_t> sections;

	class emptyPair {};

	std::string parseSection(const char* buffer);
	std::pair<std::string, std::string> parseKey(const char* buffer);

	std::map<std::string, keys_t>::const_iterator curSection;

public:
	class parseError : public std::exception {};

public:
	iniParser() {};
	~iniParser() {};

	bool open(const char* fName);
	void close();

	bool setSection(const char* section);
	const char* getValue(const char* key);
};

#endif

