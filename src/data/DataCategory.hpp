#pragma once

#include <string>
#include <vector>

#include "DataItem.hpp"

class DataItem;

class DataCategory
{
public:
	std::string name;
	std::string presentationName;
	std::vector<DataItem *> items;
	std::vector<std::string> triggers;
	int order = 0;
};
