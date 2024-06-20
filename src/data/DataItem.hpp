#pragma once

#include <string>
#include <vector>

#include "DataTag.hpp"
#include "DataSource.hpp"
#include "DataCategory.hpp"

class DataTag;
class DataSource;
class DataCategory;

class DataItem
{
public:
	std::vector<DataTag *> tags;
	DataCategory *category;
	DataSource *source;

	std::string id;
	std::string title;
	std::string description;
	std::string filename;
	int nScenes;
};
