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

#include "ShaderSource.cpp"
#include "OpenGL.hpp"
#include "../util/Logger.cpp"
using namespace Util::Logger;

namespace Shader
{
	class ShaderProgram
	{
	public:
		ShaderProgram(char *vertexSource, char *fragmentSource)
		{
			this->vertexSource = vertexSource;
			this->fragmentSource = fragmentSource;
		}

		void init()
		{
			if (initialized)
				return;

			initialized = true;

			printVersion();

			ShaderSource vertexShader(vertexSource, GL_VERTEX_SHADER);
			vertexShader.compile();

			ShaderSource fragmentShader(fragmentSource, GL_FRAGMENT_SHADER);
			fragmentShader.compile();

			shaderProgram = glCreateProgram();
			glAttachShader(shaderProgram, vertexShader.get());
			glAttachShader(shaderProgram, fragmentShader.get());
			glLinkProgram(shaderProgram);

			int success;
			char infoLog[512];
			glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);

			if (!success)
			{
				glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
				error("SHADER", std::string("Shader program linking failed. More information:\n\n") + infoLog);
			}
			else
			{
				print("SHADER", "Shader program linked successfully.");
			}

			vertexShader.destroy();
			fragmentShader.destroy();
		}

		bool isInitialized()
		{
			return initialized;
		}

		void printVersion()
		{
			int major, minor;
			glGetIntegerv(GL_MAJOR_VERSION, &major);
			glGetIntegerv(GL_MINOR_VERSION, &minor);

			print("SHADER", "OpenGL version: " + std::to_string(major) + "." + std::to_string(minor));
		}

		unsigned int get()
		{
			return shaderProgram;
		}

		void use()
		{
			glUseProgram(shaderProgram);
		}

	private:
		bool initialized = false;

		char *vertexSource;
		char *fragmentSource;

		unsigned int shaderProgram;
	};
};
