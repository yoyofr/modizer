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

#include"iniParser.h"

#include <stdlib.h>

long iniParser::parseLong(const char* str) {

	if (!str)
		throw parseError();

	return strtol(str, 0, 10);
}

double iniParser::parseDouble(const char* str) {

	if (!str)
		throw parseError();

	double result = 0.;

	while (*str == ' ') {
		str++;
	}

	bool negative = false;
	if (*str=='+') {
		str++;
	} else if (*str=='-') {
		negative = true;
		str++;
	}

	char *end;
	while (isdigit(*str)) {
		result = (result * 10.) + (double)((*str++) - '0');
	}

	if (*str=='.') {
		str++;
		double exponential = .1;
		while (isdigit(*str)) {
			result += ((double)((*str++) - '0') * exponential);
			exponential *= .1;
		}
	}

	long exponent = 0;

	if (*str=='e') {
		exponent = strtol(++str, &end, 10);
	}

	double exponential = 1.;

	if (exponent < 0) {
		while (exponent++)
			exponential *= .1;
	} else {
		while (exponent--)
			exponential *= 10.;
	}

	if (negative)
		result = -result;

	return result*exponential;
}

std::string iniParser::parseSection(const char* buffer) {

	std::string section(buffer);
	section.erase(0, 1);
	section.erase(section.find(']'));
	return section;
}

std::pair<std::string, std::string> iniParser::parseKey(const char* buffer) {

	std::string section(buffer);
	size_t pos = section.find('=');
	if (pos == std::string::npos)
		throw emptyPair();

	std::string key = section.substr(0, pos);
	std::string value = section.substr(pos+1);

	//Trim right spaces
	key.erase(key.find_last_not_of(' ')+1);

	return make_pair(key, value);
}

#define BUFSIZE 2048


bool iniParser::open(const char* fName) {

	std::ifstream iniFile(fName);
	if (iniFile.fail())
		return false;

	char buffer[BUFSIZE];
	std::map<std::string, keys_t>::iterator mIt;

	while (iniFile.good()) {
		iniFile.getline(buffer, BUFSIZE);
		switch (buffer[0]) {
		case ';':
		case '#':
			// skip comments
			break;
		case '[':
		{
			std::string section = parseSection(buffer);
			keys_t keys;
			std::pair<std::map<std::string, keys_t>::iterator, bool> it = sections.insert(make_pair(section, keys));
			mIt = it.first;
		}
			break;
		default:
			try {
				(*mIt).second.insert(parseKey(buffer));
			} catch (emptyPair& e) {};
			break;
		}
	}

	return true;
}

void iniParser::close() {

	sections.clear();
}

bool iniParser::setSection(const char* section) {

	curSection = sections.find(std::string(section));

	return (curSection!=sections.end());
}

const char* iniParser::getValue(const char* key) {

	std::map<std::string, std::string>::const_iterator keyIt = (*curSection).second.find(std::string(key));

	return (keyIt!=(*curSection).second.end())?keyIt->second.c_str():0;
}
