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
	std::string filename;

	int sceneId;
};
