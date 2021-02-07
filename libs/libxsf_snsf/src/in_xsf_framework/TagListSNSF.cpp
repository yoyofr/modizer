/*
 * xSF Tag List
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-25
 *
 * Storage of tags from PSF-style files, specifications found at
 * http://wiki.neillcorlett.com/PSFTagFormat
 */

#include <algorithm>
#include "TagListSNSF.h"

eq_strSNSF TagListSNSF::eqstr;

auto TagListSNSF::GetKeys() const -> const TagsList &
{
	return this->tagsOrder;
}

auto TagListSNSF::GetTags() const -> TagsList
{
	TagsList allTags;
	for (auto curr = this->tagsOrder.begin(), end = this->tagsOrder.end(); curr != end; ++curr)
		allTags.push_back(*curr + "=" + this->tags.find(*curr)->second);
	return allTags;
}

bool TagListSNSF::Exists(const std::string &name) const
{
	return std::find_if(this->tagsOrder.begin(), this->tagsOrder.end(), std::bind2nd(TagListSNSF::eqstr, name)) != this->tagsOrder.end();
}

std::string TagListSNSF::operator[](const std::string &name) const
{
	auto tag = this->tags.find(name);
	if (tag == this->tags.end())
		return "";
	return tag->second;
}

std::string &TagListSNSF::operator[](const std::string &name)
{
	auto tag = std::find_if(this->tagsOrder.begin(), this->tagsOrder.end(), std::bind2nd(TagListSNSF::eqstr, name));
	if (tag == this->tagsOrder.end())
	{
		this->tagsOrder.push_back(name);
		this->tags[name] = "";
	}
	return this->tags[name];
}

void TagListSNSF::Remove(const std::string &name)
{
	auto tagOrder = std::find_if(this->tagsOrder.begin(), this->tagsOrder.end(), std::bind2nd(TagListSNSF::eqstr, name));
	if (tagOrder != this->tagsOrder.end())
		this->tagsOrder.erase(tagOrder);
	if (this->tags.count(name))
		this->tags.erase(name);
}

void TagListSNSF::Clear()
{
	this->tagsOrder.clear();
	this->tags.clear();
}
