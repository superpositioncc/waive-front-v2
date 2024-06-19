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

namespace Util
{
	namespace Logger
	{
		static void log(const std::string &ns, const std::string &message, const std::string &color)
		{
			// Use strftime to get the time as HH:MM:SS
			time_t rawtime;
			char timeStr[8];
			time(&rawtime);
			strftime(timeStr, sizeof(timeStr), "%H:%M:%S", localtime(&rawtime));

			std::cout << DIM << timeStr << RESET << color << " [" << DISTRHO_PLUGIN_NAME << "][" << ns << "] " << RESET << message << std::endl;
		}

		static void log(const std::string &ns, const std::string &message)
		{
			log(ns, message, CYAN);
		}

		static void error(const std::string &ns, const std::string &message)
		{
			log(ns, message, RED);
		}

		static void warn(const std::string &ns, const std::string &message)
		{
			log(ns, message, YELLOW);
		}
	};
};
