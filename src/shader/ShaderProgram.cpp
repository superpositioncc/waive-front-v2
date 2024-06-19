#pragma once

#include "ShaderSource.cpp"
#include "OpenGL.hpp"
#include "Global.h"

namespace Shader
{
	class ShaderProgram
	{
	public:
		ShaderProgram(char *vertexSource, char *fragmentSource)
		{
			this->vertexSource = vertexSource;
			this->fragmentSource = fragmentSource;
		}

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
				log("SHADER", "Shader program linked successfully.");
			}

			vertexShader.destroy();
			fragmentShader.destroy();
		}

		bool isInitialized()
		{
			return initialized;
		}

		void printVersion()
		{
			int major, minor;
			glGetIntegerv(GL_MAJOR_VERSION, &major);
			glGetIntegerv(GL_MINOR_VERSION, &minor);

			log("SHADER", "OpenGL version: " + std::to_string(major) + "." + std::to_string(minor));
		}

		unsigned int get()
		{
			return shaderProgram;
		}

		void use()
		{
			glUseProgram(shaderProgram);
		}

	private:
		bool initialized = false;

		char *vertexSource;
		char *fragmentSource;

		unsigned int shaderProgram;
	};
};
