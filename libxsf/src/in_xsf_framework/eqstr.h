/*
 * Case-insensitive string equality
 * Last modification on 2014-09-08
 *
 * Based on "How to do case-insensitive string comparison"
 * By Matt Austern
 * http://lafstern.org/matt/col2_new.pdf
 */

#pragma once

#include <functional>
#include <locale>
#include <string>
#include <algorithm>
#include <climits>

struct eq_str : std::binary_function<std::string, std::string, bool>
{
	struct eq_char : std::binary_function<char, char, bool>
	{
		const char *tab;
		eq_char(const char *t) : tab(t) { }
		bool operator()(char x, char y) const { return this->tab[x - CHAR_MIN] == this->tab[y - CHAR_MIN]; }
	};

	char tab[CHAR_MAX - CHAR_MIN + 1];

	eq_str(const std::locale &L = std::locale::classic())
	{
		for (int i = CHAR_MIN; i <= CHAR_MAX; ++i)
			this->tab[i - CHAR_MIN] = static_cast<char>(i);
		std::use_facet<std::ctype<char>>(L).toupper(this->tab, this->tab + (CHAR_MAX - CHAR_MIN + 1));
	}

	bool operator()(const std::string &x, const std::string &y) const
	{
		return x.length() == y.length() && std::equal(x.begin(), x.end(), y.begin(), eq_char(this->tab));
	}
};
