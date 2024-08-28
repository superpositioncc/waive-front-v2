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
