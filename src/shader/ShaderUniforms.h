#pragma once

#include "ShaderUniform.h"
#include "ShaderProgram.cpp"

namespace Shader
{
	struct ShaderUniforms
	{
		ShaderUniform<float> threshold = ShaderUniform<float>("threshold", 1);
		ShaderUniform<float> width = ShaderUniform<float>("width", 1);
		ShaderUniform<float> colors = ShaderUniform<float>("colors", 3 * 5);
		ShaderUniform<int> colorIndex = ShaderUniform<int>("colorIndex", 1);
		ShaderUniform<float> size = ShaderUniform<float>("size", 1);

		void init(ShaderProgram *shaderProgram)
		{
			threshold.find(shaderProgram->get());
			width.find(shaderProgram->get());
			colors.find(shaderProgram->get());
			colorIndex.find(shaderProgram->get());
			size.find(shaderProgram->get());
		}

		void use()
		{
			threshold.use();
			width.use();
			colors.use();
			colorIndex.use();
			size.use();
		}
	};
};
