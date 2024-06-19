#pragma once

#include "ShaderUniform.h"
#include "ShaderProgram.cpp"

namespace Shader
{
	struct ShaderUniforms
	{
		ShaderUniform<float> threshold = {"threshold", -1, 0.0f};
		ShaderUniform<float> width = {"width", -1, 0.0f};

		void init(ShaderProgram *shaderProgram)
		{
			threshold.find(shaderProgram->get());
			width.find(shaderProgram->get());
		}

		void use()
		{
			threshold.use();
			width.use();
		}
	};
};
