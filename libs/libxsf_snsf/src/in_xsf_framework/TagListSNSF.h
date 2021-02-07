/*
 * xSF Tag List
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-09-08
 *
 * Storage of tags from PSF-style files, specifications found at
 * http://wiki.neillcorlett.com/PSFTagFormat
 */

#pragma once

#include <map>
#include <vector>
#include "eqstr.h"
#include "ltstr.h"

class TagListSNSF
{
public:
	typedef std::map<std::string, std::string, lt_strSNSF> Tags;
	typedef std::vector<std::string> TagsList;
private:
	static eq_strSNSF eqstr;

	Tags tags;
	TagsList tagsOrder;
public:
	TagListSNSF() : tags(), tagsOrder() { }
	const TagsList &GetKeys() const;
	TagsList GetTags() const;
	bool Exists(const std::string &name) const;
	std::string operator[](const std::string &name) const;
	std::string &operator[](const std::string &name);
	void Remove(const std::string &name);
	void Clear();
};

