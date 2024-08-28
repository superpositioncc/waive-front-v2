/*
WAIVE-FRONT
Copyright (C) 2024  Bram Bogaerts, Superposition

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "DataSources.hpp"

DataTag *DataSources::findOrCreateTag(std::string name)
{
	for (DataTag *tag : tags)
	{
		if (tag->name == name)
		{
			return tag;
		}
	}

	DataTag *tag = new DataTag();
	tag->name = name;

	tags.push_back(tag);

	return tag;
}

DataCategory *DataSources::findOrCreateCategory(std::string name)
{
	for (DataCategory *category : categories)
	{
		if (category->name == name)
		{
			return category;
		}
	}

	DataCategory *category = new DataCategory();
	category->name = name;

	categories.push_back(category);

	return category;
}

void DataSources::collectItems()
{
	for (DataSource *source : sources)
	{
		for (DataItem *item : source->items)
		{
			items.push_back(item);
		}
	}
}

void DataSources::sortCategories()
{
	std::sort(categories.begin(), categories.end(), [](DataCategory *a, DataCategory *b)
			  { return a->order < b->order; });
}
