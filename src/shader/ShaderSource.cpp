#pragma once

#include "OpenGL.hpp"
#include "Global.h"
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
				log("SHADER", typeName + " shader compiled successfully");
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
