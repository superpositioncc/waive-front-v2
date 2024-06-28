#pragma once

#include <string>
#include <vector>
#include "../data/DataCategory.hpp"

struct OSCMessage
{
	std::string sampleName;
	std::vector<DataCategory *> categories;
	int note;
	bool seen;
};
