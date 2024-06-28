#pragma once

#include "ShaderUniform.h"
#include "ShaderProgram.cpp"

namespace Shader
{
	struct ShaderUniforms
	{
		ShaderUniform<float> focusAmount = ShaderUniform<float>("focusAmount", 1);
		ShaderUniform<float> blurSize = ShaderUniform<float>("blurSize", 1);
		ShaderUniform<float> colors = ShaderUniform<float>("colors", 3 * 5);
		ShaderUniform<int> colorIndex = ShaderUniform<int>("colorIndex", 1);
		ShaderUniform<float> size = ShaderUniform<float>("size", 1);
		ShaderUniform<float> time = ShaderUniform<float>("time", 1);

		void init(ShaderProgram *shaderProgram)
		{
			focusAmount.find(shaderProgram->get());
			blurSize.find(shaderProgram->get());
			colors.find(shaderProgram->get());
			colorIndex.find(shaderProgram->get());
			size.find(shaderProgram->get());
			time.find(shaderProgram->get());
		}

		void use()
		{
			focusAmount.use();
			blurSize.use();
			colors.use();
			colorIndex.use();
			size.use();
			time.use();
		}
	};
};
