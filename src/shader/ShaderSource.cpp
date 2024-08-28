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

#include "OpenGL.hpp"
#include "../util/Logger.cpp"
using namespace Util::Logger;
#include <iostream>
#include <string>

namespace Shader
{
	class ShaderSource
	{
	public:
		ShaderSource(const char *source, unsigned int type)
		{
			this->source = source;
			this->type = type;
		}

		void compile()
		{
			const std::string typeName = (type == GL_VERTEX_SHADER) ? "Vertex" : "Fragment";

			shader = glCreateShader(type);
			glShaderSource(shader, 1, &source, NULL);
			glCompileShader(shader);

			int success;
			char infoLog[512];
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

			if (!success)
			{
				glGetShaderInfoLog(shader, 512, NULL, infoLog);
				error("SHADER", typeName + std::string(" shader compilation failed. More information:\n\n") + infoLog);
			}
			else
			{
				print("SHADER", typeName + " shader compiled successfully");
			}
		}

		unsigned int get()
		{
			return shader;
		}

		void destroy()
		{
			glDeleteShader(shader);
		}

	private:
		const char *source;
		unsigned int shader;
		unsigned int type;
	};
};
