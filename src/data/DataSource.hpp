/*
WAIVE-FRONT
Copyright (C) 2024  Bram Bogaerts, Superposition

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

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
