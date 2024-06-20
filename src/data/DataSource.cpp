#include "DataSource.hpp"
#include <string>

void DataSource::load(DataSources *sources)
{
	std::string dataPath = path + "/data.json";

	json data;

	try
	{
		std::ifstream file(dataPath);
		file >> data;

		name = data["name"];

		for (json item : data["items"])
		{

			DataItem *dataItem = new DataItem();

			dataItem->id = item["id"].get<std::string>();
			dataItem->title = item["title"].get<std::string>();
			dataItem->description = item["description"].get<std::string>();
			dataItem->nScenes = item["nScenes"].get<int>();
			dataItem->filename = item["filename"].get<std::string>();
			dataItem->source = this;
			dataItem->category = sources->findOrCreateCategory(item["category"].get<std::string>());

			for (std::string tag : item["tags"].get<std::vector<std::string>>())
			{
				DataTag *dataTag = sources->findOrCreateTag(tag);
				dataTag->items.push_back(dataItem);
				dataItem->tags.push_back(dataTag);

				addTagIfNotAdded(dataTag);
			}

			items.push_back(dataItem);
		}
	}
	catch (const std::exception &e)
	{
		warn("DATA", "Failed to load data file: " + dataPath);
		return;
	}

	valid = true;
}

void DataSource::addCategoryIfNotAdded(DataCategory *category)
{
	for (DataCategory *existingCategory : categories)
	{
		if (existingCategory == category)
		{
			return;
		}
	}

	categories.push_back(category);
}

void DataSource::addTagIfNotAdded(DataTag *tag)
{
	for (DataTag *existingTag : tags)
	{
		if (existingTag == tag)
		{
			return;
		}
	}

	tags.push_back(tag);
}
