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

/**
 * @brief Simple functions related to GLSL shader management, compilation and usage
 */
namespace Shader
{
	/**
	 * @brief A class to compile a GLSL shader program from a vertex and fragment shader
	 *
	 */
	class ShaderProgram
	{
	public:
		/**
		 * @brief Construct a new Shader Program object
		 *
		 * @param vertexSource The source code of the vertex shader
		 * @param fragmentSource The source code of the fragment shader
		 */
		ShaderProgram(char *vertexSource, char *fragmentSource)
		{
			this->vertexSource = vertexSource;
			this->fragmentSource = fragmentSource;
		}

		/**
		 * @brief Initialize the shader program, by compiling the shaders and linking them
		 *
		 */
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

		/**
		 * @brief Check if the shader program has been initialized
		 *
		 * @return true If the shader program has been initialized
		 * @return false If the shader program has not been initialized
		 */
		bool isInitialized()
		{
			return initialized;
		}

		/**
		 * @brief Print the OpenGL version
		 *
		 */
		void printVersion()
		{
			int major, minor;
			glGetIntegerv(GL_MAJOR_VERSION, &major);
			glGetIntegerv(GL_MINOR_VERSION, &minor);

			print("SHADER", "OpenGL version: " + std::to_string(major) + "." + std::to_string(minor));
		}

		/**
		 * @brief Get the shader program
		 *
		 * @return unsigned int The shader program
		 */
		unsigned int get()
		{
			return shaderProgram;
		}

		/**
		 * @brief Use the shader program
		 *
		 */
		void use()
		{
			glUseProgram(shaderProgram);
		}

	private:
		bool initialized = false; /**< Whether the shader program has been initialized */

		char *vertexSource;	  /**< The source code of the vertex shader */
		char *fragmentSource; /**< The source code of the fragment shader */

		unsigned int shaderProgram; /**< The shader program */
	};
};
