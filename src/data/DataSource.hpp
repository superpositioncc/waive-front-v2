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

/**
 * @brief Represents a data source
 *
 */
class DataSource
{
public:
	std::string name; /**< Name of the source */
	std::string path; /**< Path to the source */

	std::vector<DataTag *> tags;			/**< All tags in the source */
	std::vector<DataItem *> items;			/**< All items in the source */
	std::vector<DataCategory *> categories; /**< All categories in the source */

	/**
	 * @brief Construct a new DataSource object
	 *
	 * @param path Path to the source
	 */
	DataSource(std::string path) : path(path) {}

	/**
	 * @brief Whether the source is valid
	 *
	 * @return true The source is valid
	 * @return false The source is not valid
	 */
	bool isValid() { return valid; }

	/**
	 * @brief Load the source
	 *
	 * @param sources Data sources to load into
	 */
	void load(DataSources *sources);

private:
	bool valid; /**< Whether the source is valid */

	void addCategoryIfNotAdded(DataCategory *category); /**< Add a category if it is not added already */
	void addTagIfNotAdded(DataTag *tag);				/**< Add a tag if it is not added already */
};
