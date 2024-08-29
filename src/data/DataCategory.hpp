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

#include "DataItem.hpp"

class DataItem;

/**
 * @brief Stores a category of data items
 *
 */
class DataCategory
{
public:
	std::string name;				   /**< Name of the category */
	std::string presentationName;	   /**< Name of the category as it should be presented */
	std::vector<DataItem *> items;	   /**< Items in the category */
	std::vector<std::string> triggers; /**< OSC triggers words for the category */
	int order = 0;					   /**< Order of the category in the list */
};
