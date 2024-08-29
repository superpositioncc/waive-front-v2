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

#include "DistrhoPluginInfo.h"
#include <iostream>
#include <string>

#define RED "\x1B[31m"
#define GREEN "\x1B[32m"
#define YELLOW "\x1B[33m"
#define BLUE "\x1B[34m"
#define MAGENTA "\x1B[35m"
#define CYAN "\x1B[36m"
#define WHITE "\x1B[37m"
#define RESET "\x1b[0m"
#define DIM "\x1b[2m"

/**
 * @brief Various utility functions
 */
namespace Util
{
	/**
	 * @brief Functions to log messages to the console including a timestamp and namespace
	 */
	namespace Logger
	{
		/**
		 * @brief Print a message to the console
		 *
		 * @param ns Namespace of the message
		 * @param message Message to print
		 * @param color Color of the message
		 */
		static void print(const std::string &ns, const std::string &message, const std::string &color)
		{
			// Use strftime to get the time as HH:MM:SS
			time_t rawtime;
			char timeStr[9];
			time(&rawtime);
			strftime(timeStr, sizeof(timeStr), "%H:%M:%S", localtime(&rawtime));

			std::cout << DIM << timeStr << RESET << color << " [" << DISTRHO_PLUGIN_NAME << "][" << ns << "] " << RESET << message << std::endl;
		}

		/**
		 * @brief Print a message to the console in the default color
		 *
		 * @param ns Namespace of the message
		 * @param message Message to print
		 */
		static void print(const std::string &ns, const std::string &message)
		{
			print(ns, message, CYAN);
		}

		/**
		 * @brief Print an error message to the console in red
		 *
		 * @param ns Namespace of the message
		 * @param message Message to print
		 */
		static void error(const std::string &ns, const std::string &message)
		{
			print(ns, message, RED);
		}

		/**
		 * @brief Print a warning message to the console in yellow
		 *
		 * @param ns Namespace of the message
		 * @param message Message to print
		 */
		static void warn(const std::string &ns, const std::string &message)
		{
			print(ns, message, YELLOW);
		}
	};
};
