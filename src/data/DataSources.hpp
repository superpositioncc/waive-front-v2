#pragma once

#include <string>
#include <vector>

#include "DataSource.hpp"
#include "DataCategory.hpp"
#include "DataItem.hpp"
#include "DataTag.hpp"

class DataSources
{
public:
	std::vector<DataSource *> sources;
	std::vector<DataTag *> tags;
	std::vector<DataItem *> items;
	std::vector<DataCategory *> categories;

	DataTag *findOrCreateTag(std::string name);
	DataCategory *findOrCreateCategory(std::string name);

	void collectItems();
	void sortCategories();
};
