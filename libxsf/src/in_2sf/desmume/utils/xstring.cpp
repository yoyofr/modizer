//taken from fceux on 10/27/08
//subsequently modified for desmume

/*
	Copyright (C) 2008-2009 DeSmuME team

	This file is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with the this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string>
#include <vector>

#include "xstring.h"

std::vector<std::string> tokenize_str(const std::string &str, const std::string &delims = ", \t")
{
	// Skip delims at beginning, find start of first token
	auto lastPos = str.find_first_not_of(delims, 0);
	// Find next delimiter @ end of token
	auto pos = str.find_first_of(delims, lastPos);

	// output vector
	std::vector<std::string> tokens;

	while (pos != std::string::npos || lastPos != std::string::npos)
	{
		// Found a token, add it to the vector.
		tokens.push_back(str.substr(lastPos, pos - lastPos));
		// Skip delims.  Note the "not_of". this is beginning of token
		lastPos = str.find_first_not_of(delims, pos);
		// Find next delimiter at end of token.
		pos = str.find_first_of(delims, lastPos);
	}

	return tokens;
}
