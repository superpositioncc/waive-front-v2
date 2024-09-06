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

#include <thread>
#include "tinyosc.h"
#include "../util/Logger.cpp"
#include "../data/DataCategory.hpp"
#include "OSCMessage.hpp"
#include <sstream>
#include <string>

#ifdef __APPLE__
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <cstring>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#endif



using namespace Util::Logger;

/**
 * @brief OSC server
 *
 */
class OSCServer
{
public:
	/**
	 * @brief Construct a new OSCServer object
	 *
	 * @param port The port to listen on
	 * @param dataSources The data sources
	 */
	OSCServer(int port, DataSources *dataSources) : dataSources(dataSources)
	{
		init(port);
		latestMessage.seen = true;
	}

	~OSCServer()
	{
		stop();
	}

	/**
	 * @brief Initialize the server
	 *
	 * @param port The port to listen on
	 */
	void init(int port)
	{
#ifdef __APPLE__
		fd = socket(AF_INET6, SOCK_DGRAM, 0);
		fcntl(fd, F_SETFL, O_NONBLOCK);
		struct sockaddr_in6 sin;
		sin.sin6_family = AF_INET6;
		sin.sin6_port = htons(port);
		sin.sin6_addr = in6addr_any;
		bind(fd, (struct sockaddr *)&sin, sizeof(struct sockaddr_in6));
		print("OSC", "Now listening on port " + std::to_string(port));
#else
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);

		fd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		struct sockaddr_in6 sin;
		sin.sin6_family = AF_INET6;
		sin.sin6_port = htons(port);
		sin.sin6_addr = in6addr_any;

		bind(fd, (struct sockaddr*)&sin, sizeof(struct sockaddr_in6));

		u_long mode = 1;
		ioctlsocket(fd, FIONBIO, &mode);

		print("OSC", "Now listening on port " + std::to_string(port));
#endif

		thread = std::thread(&OSCServer::run, this);
	}

	// /**
	//  * @brief Stop the server
	//  *
	//  */
	void stop()
	{
#ifdef __APPLE__
		thread.join();
		close(fd);
#else
		if (thread.joinable())
		{
			thread.join();
		}
		closesocket(fd);
		WSACleanup();
#endif
	}

	// /**
	//  * @brief Whether a message is available
	//  *
	//  * @return true A message is available
	//  * @return false A message is not available
	//  */
	bool available()
	{
		return !latestMessage.seen;
	}

	// /**
	//  * @brief Get the latest message
	//  *
	//  * @return OSCMessage The latest message
	//  */
	OSCMessage getMessage()
	{
		latestMessage.seen = true;
		return latestMessage;
	}

private:
	/**
	 * @brief Endlessly run the server (should be run in a separate thread)
	 *
	 */
	void run()
	{
		while (true)
		{
			fd_set readSet;
			FD_ZERO(&readSet);
			FD_SET(fd, &readSet);
			struct timeval timeout = {1, 0}; // select times out after 1 second
			if (select(fd + 1, &readSet, NULL, NULL, &timeout) > 0)
			{
				struct sockaddr sa; // can be safely cast to sockaddr_in
				socklen_t sa_len = sizeof(struct sockaddr_in);
				int len = 0;
				while ((len = (int)recvfrom(fd, buffer, sizeof(buffer), 0, &sa, &sa_len)) > 0)
				{
					if (!tosc_isBundle(buffer))
					{
						tosc_message osc;
						tosc_parseMessage(&osc, buffer, len);

						std::string address = tosc_getAddress(&osc);
						std::string format = tosc_getFormat(&osc);

						if (address == "/WAIVE_Sampler/Sample" && format == "ssi")
						{
							latestMessage.sampleName = tosc_getNextString(&osc);
							latestMessage.rawCategories = tosc_getNextString(&osc);

							std::istringstream iss(latestMessage.rawCategories);
							std::string tag;

							while (std::getline(iss, tag, '|'))
							{
								DataCategory *category = nullptr;

								for (DataCategory *c : dataSources->categories)
								{
									bool hasTrigger = false;

									for (std::string &trigger : c->triggers)
									{
										if (tag == trigger)
										{
											hasTrigger = true;
											break;
										}
									}

									if (hasTrigger)
									{
										category = c;
										break;
									}
								}

								if (category != nullptr)
								{
									latestMessage.categories.push_back(category);
								}
							}

							latestMessage.note = tosc_getNextInt32(&osc);

							if (latestMessage.categories.size() > 0)
							{
								latestMessage.seen = false;
							}
						}
					}
				}
			}
		}
	}

#ifdef __APPLE__
	int fd;				/**< The socket */
#else
	SOCKET fd;			/**< The socket */
#endif

	char buffer[2048];	/**< The buffer that will contain incoming messages */
	std::thread thread; /**< The thread to run the server in */

	OSCMessage latestMessage; /**< The latest message */
	DataSources *dataSources; /**< The data sources */
};
