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

/**
 * @brief Simple functions related to GLSL shader management, compilation and usage
 */
namespace Shader
{
	/**
	 * @brief A class to manage a texture in a shader program
	 *
	 */
	class ShaderTexture
	{
	public:
		ShaderTexture()
		{
		}

		/**
		 * @brief Initialize the texture, by creating a texture object
		 *
		 */
		void init()
		{
			if (initialized)
				return;

			initialized = true;

			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			set(nullptr, 16, 16);
		}

		/**
		 * @brief Bind the texture to the current shader program
		 *
		 */
		void bind()
		{
			glBindTexture(GL_TEXTURE_2D, texture);
		}

		/**
		 * @brief Set the texture data
		 *
		 * @param data The texture data
		 * @param width The width of the texture
		 * @param height The height of the texture
		 */
		void set(const unsigned char *data, int width, int height)
		{
			bind();

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}

	private:
		bool initialized = false; /**< Whether the texture has been initialized */

		unsigned int texture; /**< The texture ID */
	};
};
