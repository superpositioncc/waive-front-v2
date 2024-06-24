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
