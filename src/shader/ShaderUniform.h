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

namespace Shader
{
	template <typename T>
	class ShaderUniform
	{
	public:
		char *name;
		int size = 1;

		T *value = nullptr;
		int location = -1;

		ShaderUniform(char *name, int size)
			: name(name), size(size)
		{
		}

		void find(unsigned int shaderProgram)
		{
			location = glGetUniformLocation(shaderProgram, name);

			if (location == -1)
				warn("SHADER", "Uniform " + std::string(name) + " not found");
		}

		void use()
		{
			if (std::is_same<T, int>::value)
			{
				if (size == 1)
					glUniform1i(location, *value);
				else
					glUniform3iv(location, size / 3, (int *)value);
			}
			else if (std::is_same<T, float>::value)
			{
				if (size == 1)
					glUniform1f(location, *value);
				else
					glUniform3fv(location, size / 3, (float *)value);
			}
			else
			{
				warn("SHADER", "Uniform type not supported");
			}
		}

		void set(T *value)
		{
			this->value = value;
		}
	};
};
