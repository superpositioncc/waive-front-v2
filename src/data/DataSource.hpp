#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <fstream>

#include "DataTag.hpp"
#include "DataItem.hpp"
#include "DataCategory.hpp"
#include "DataSources.hpp"
#include "../util/Logger.cpp"
using namespace Util::Logger;

using json = nlohmann::json;

class DataSources;

class DataSource
{
public:
	std::string name;
	std::string path;

	std::vector<DataTag *> tags;
	std::vector<DataItem *> items;
	std::vector<DataCategory *> categories;

	DataSource(std::string path) : path(path) {}

	bool isValid() { return valid; }
	void load(DataSources *sources);

private:
	bool valid;

	void addCategoryIfNotAdded(DataCategory *category);
	void addTagIfNotAdded(DataTag *tag);
};
