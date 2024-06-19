#pragma once

#include "OpenGL.hpp"

namespace Shader
{
	template <typename T>
	struct ShaderUniform
	{
		char *name;
		int location;
		T value;

		void find(unsigned int shaderProgram)
		{
			location = glGetUniformLocation(shaderProgram, name);
		}

		void use()
		{
			glUniform1f(location, value);
		}

		void set(T value)
		{
			this->value = value;
		}
	};
};
