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
			auto category = item["category"][0];

			if (category.type() == json::value_t::null)
				continue;

			DataItem *dataItem = new DataItem();

			dataItem->id = item["id"].get<std::string>();
			dataItem->sceneId = item["sceneId"].get<int>();
			dataItem->title = item["title"].get<std::string>();
			dataItem->filename = dataItem->id + "-" + std::to_string(dataItem->sceneId);
			// dataItem->filename = item["filename"].get<std::string>();
			// dataItem->filename = dataItem->filename.substr(0, dataItem->filename.find_last_of("."));
			dataItem->source = this;
			dataItem->category = sources->findOrCreateCategory(item["category"].get<std::vector<std::string>>()[0]);

			for (std::string tag : item["tags"].get<std::vector<std::string>>())
			{
				DataTag *dataTag = sources->findOrCreateTag(tag);
				dataTag->items.push_back(dataItem);
				dataItem->tags.push_back(dataTag);

				addTagIfNotAdded(dataTag);
			}

			addCategoryIfNotAdded(dataItem->category);
			dataItem->category->items.push_back(dataItem);

			items.push_back(dataItem);
		}
	}
	catch (const std::exception &e)
	{
		warn("DATA", "Failed to load data file: " + dataPath);
		warn("DATA", e.what());
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
