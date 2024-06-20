#pragma once

#include <string>
#include <vector>

#include "DataItem.hpp"

class DataItem;

class DataCategory
{
public:
	std::string name;
	std::vector<DataItem *> items;
};
