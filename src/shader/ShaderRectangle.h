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
