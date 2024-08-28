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
#include "osc/OscReceivedElements.h"
#include "osc/OscPacketListener.h"
#include "ip/UdpSocket.h"
#include "../util/Logger.cpp"
#include "../data/DataCategory.hpp"
#include "OSCMessage.hpp"
#include <sstream>
#include <string>

using namespace Util::Logger;

class OSCServer : public osc::OscPacketListener
{
public:
	OSCServer(int port, DataSources *dataSources) : dataSources(dataSources)
	{
		init(port);
		latestMessage.seen = true;
	}

	~OSCServer()
	{
		stop();
	}

	void init(int port)
	{
		socket = new UdpListeningReceiveSocket(
			IpEndpointName(IpEndpointName::ANY_ADDRESS, port),
			this);

		// Create a new thread and run the socket in it
		thread = std::thread([this]()
							 { socket->RunUntilSigInt(); });
	}

	void stop()
	{
		socket->AsynchronousBreak();

		// Wait for the thread to finish before deleting the socket
		thread.join();

		delete socket;
	}

	bool available()
	{
		return !latestMessage.seen;
	}

	OSCMessage getMessage()
	{
		latestMessage.seen = true;
		return latestMessage;
	}

	void changePort(int port)
	{
		socket->AsynchronousBreak();
		thread.join();
		delete socket;

		socket = new UdpListeningReceiveSocket(
			IpEndpointName(IpEndpointName::ANY_ADDRESS, port),
			this);

		thread = std::thread([this]()
							 { socket->RunUntilSigInt(); });
	}

protected:
	virtual void ProcessMessage(const osc::ReceivedMessage &m,
								const IpEndpointName &remoteEndpoint)
	{
		(void)remoteEndpoint; // suppress unused parameter warning

		try
		{
			if (std::strcmp(m.AddressPattern(), "/WAIVE_Sampler/Sample") == 0)
			{
				osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();

				const char *a1 = (arg++)->AsString();
				const char *a2 = (arg++)->AsString();
				int a3 = (arg++)->AsInt32();

				if (arg != m.ArgumentsEnd())
					throw osc::ExcessArgumentException();

				latestMessage.seen = true;
				latestMessage.sampleName = std::string(a1);
				latestMessage.note = a3;

				latestMessage.categories.clear();

				std::string tags = std::string(a2);
				std::istringstream iss(tags);
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

				if (latestMessage.categories.size() > 0)
				{
					latestMessage.seen = false;
				}
			}
		}
		catch (osc::Exception &e)
		{
			error("OSC", "Error while parsing message: " + std::string(m.AddressPattern()) + ": " + e.what());
		}
	}

private:
	UdpListeningReceiveSocket *socket;
	std::thread thread;

	OSCMessage latestMessage;
	DataSources *dataSources;
};
