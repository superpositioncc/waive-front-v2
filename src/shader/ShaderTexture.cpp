#pragma once

#include "OpenGL.hpp"

namespace Shader
{
	class ShaderTexture
	{
	public:
		ShaderTexture()
		{
		}

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

		void bind()
		{
			glBindTexture(GL_TEXTURE_2D, texture);
		}

		void set(const unsigned char *data, int width, int height)
		{
			bind();
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}

	private:
		bool initialized = false;

		unsigned int texture;
	};
};
