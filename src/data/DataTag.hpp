#pragma once

#include <string>
#include <vector>

#include "DataItem.hpp"

class DataTag
{
public:
	std::string name;
	std::vector<DataItem *> items;
};
