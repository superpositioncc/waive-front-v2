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

namespace Shader
{
	class ShaderRectangle
	{
	public:
		ShaderRectangle()
		{
		}

		void init()
		{
			if (initialized)
				return;

			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glGenBuffers(1, &EBO);

			glBindVertexArray(VAO);

			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
			glEnableVertexAttribArray(1);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

			glBindVertexArray(0);
		}

		void draw()
		{
			glBindVertexArray(VAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}

	private:
		unsigned int VAO, VBO, EBO;

		bool initialized = false;

		// clang-format off
	float vertices[16] = {
						/* positions */     /* texture coords */
	/*  bottom left */   -1.0f, -1.0f,        0.0f, 0.0f,
	/*     top left */   -1.0f,  1.0f,        0.0f, 1.0f,
	/*    top right */    1.0f,  1.0f,        1.0f, 1.0f,
	/* bottom right */    1.0f, -1.0f,        1.0f, 0.0f
	};

	unsigned int indices[6] = {
		0, 1, 2,
		2, 3, 0
	};
		// clang-format on
	};
};
