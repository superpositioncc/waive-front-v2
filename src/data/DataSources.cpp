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
