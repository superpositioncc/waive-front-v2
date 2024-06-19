#pragma once

#include "util/Logger.cpp"
#include <string>

void log(const std::string &ns, const std::string &message)
{
	Util::Logger::log(ns, message);
}

void error(const std::string &ns, const std::string &message)
{
	Util::Logger::error(ns, message);
}

void warn(const std::string &ns, const std::string &message)
{
	Util::Logger::warn(ns, message);
}
