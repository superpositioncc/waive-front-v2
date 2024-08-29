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

/**
 * @brief Simple functions related to GLSL shader management, compilation and usage
 */
namespace Shader
{
	/**
	 * @brief A class to manage a uniform in a shader program
	 *
	 * @tparam T The type of the uniform
	 */
	template <typename T>
	class ShaderUniform
	{
	public:
		char *name;	  /**< The name of the uniform */
		int size = 1; /**< The number of elements of the uniform */

		T *value = nullptr; /**< The value of the uniform */
		int location = -1;	/**< The location of the uniform in the shader program */

		/**
		 * @brief Construct a new Shader Uniform object
		 *
		 * @param name The name of the uniform
		 * @param size The number of elements of the uniform
		 */
		ShaderUniform(char *name, int size)
			: name(name), size(size)
		{
		}

		/**
		 * @brief Find the location of the uniform in the shader program
		 *
		 * @param shaderProgram The shader program
		 */
		void find(unsigned int shaderProgram)
		{
			location = glGetUniformLocation(shaderProgram, name);

			if (location == -1)
				warn("SHADER", "Uniform " + std::string(name) + " not found");
		}

		/**
		 * @brief Use the uniform in the shader program
		 *
		 */
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

		/**
		 * @brief Set the value of the uniform
		 *
		 * @param value The value of the uniform
		 */
		void set(T *value)
		{
			this->value = value;
		}
	};
};
