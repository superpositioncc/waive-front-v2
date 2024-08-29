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

#ifndef VIEWER_WIDGET_CPP
#define VIEWER_WIDGET_CPP

#include "DistrhoUI.hpp"
#include "OpenGL.hpp"
#include "util/Color.cpp"
#include "FrameData.h"
#include "../shader/ShaderRectangle.h"
#include "../shader/ShaderProgram.cpp"
#include "../shader/ShaderTexture.cpp"
#include "../shader/ShaderUniforms.h"
#include <iostream>
#include <vector>

START_NAMESPACE_DISTRHO

using Shader::ShaderProgram;
using Shader::ShaderRectangle;
using Shader::ShaderTexture;
using Shader::ShaderUniforms;

/**
 * @brief Clip a value between a lower and upper bound
 *
 * @param n
 * @param lower
 * @param upper
 * @return float
 */
float clip(float n, float lower, float upper)
{
	return std::max(lower, std::min(n, upper));
}

/**
 * @brief Viewer widget is the widget that displays the shader. It is the main widget of the viewer window and the presentation that the audience sees
 *
 */
class ViewerWidget : public TopLevelWidget
{
public:
	/**
	 * @brief Construct a new Viewer Widget object
	 *
	 * @param window Window
	 * @param p Parameters
	 * @param layersEnabled Vector of booleans representing which layers have been enabled
	 */
	ViewerWidget(Window &window, float (&p)[Parameters::NumParameters], std::vector<bool> *layersEnabled)
		: TopLevelWidget(window),
		  parameters(p),
		  layersEnabled(layersEnabled),
		  shaderProgram(
#include "../assets/shaders/main.vert"
			  ,
#include "../assets/shaders/main.frag"
		  )
	{
	}

	/**
	 * @brief Check if the widget is initialized
	 *
	 * @return true
	 * @return false
	 */
	bool isInitialized()
	{
		return initialized;
	}

	/**
	 * @brief Set the frame data
	 *
	 * @param i The index of the layer to set the frame data for
	 * @param frame The frame data
	 * @param width The width of the frame
	 * @param height The height of the frame
	 * @param colors The colors of the frame
	 */
	void setFrame(int i, uint8_t *frame, int width, int height, std::vector<float> colors)
	{
		if (!isInitialized())
			return;

		frameData[i]->data = frame;
		frameData[i]->width = width;
		frameData[i]->height = height;
		frameData[i]->waiting = true;

		for (int j = 0; j < 3 * 5; j++)
		{
			frameData[i]->colors[j] = colors[j];
		}
	}

protected:
	/**
	 * @brief Display the widget
	 *
	 */
	void onDisplay() override
	{
		if (!initialized)
		{
			init();
			initialized = true;
		}

		update();
		draw();
	}

	/**
	 * @brief Handle a window resize event
	 *
	 * @param event The window resize event
	 * @return true
	 * @return false
	 */
	bool onMotion(const MotionEvent &event) override
	{
		setSize(getWindow().getWidth(), getWindow().getHeight());
		return true;
	}

private:
	float (&parameters)[Parameters::NumParameters]; /**< The parameters of the shader */
	std::vector<bool> *layersEnabled;				/**< Vector of booleans representing which layers have been enabled */

	bool initialized = false; /**< Whether the widget has been initialized */

	std::vector<FrameData *> frameData;	   /**< The frame data for each layer */
	std::vector<ShaderTexture *> textures; /**< The textures for each layer */
	ShaderProgram shaderProgram;		   /**< The shader program */
	ShaderRectangle rectangle;			   /**< The shader rectangle */
	ShaderUniforms uniforms;			   /**< The shader uniforms */

	/**
	 * @brief Initialize the widget
	 *
	 */
	void init()
	{
		shaderProgram.init();
		rectangle.init();
		uniforms.init(&shaderProgram);

		for (int i = 0; i < 3; i++)
		{
			frameData.push_back(new FrameData());
			textures.push_back(new ShaderTexture());

			textures[i]->init();
		}

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
	}

	/**
	 * @brief Checks if the frame data has been updated and updates the textures
	 *
	 */
	void updateFrameData()
	{
		for (int i = 0; i < 3; i++)
		{
			FrameData *fd = frameData[i];

			if (fd->waiting)
			{
				fd->waiting = false;
				textures[i]->set(fd->data, fd->width, fd->height);
			}
		}
	}

	/**
	 * @brief Update frame data and set the uniforms
	 *
	 */
	void update()
	{
		updateFrameData();

		uniforms.blurSize.set(&parameters[Parameters::BlurSize]);

		auto currentTime = std::chrono::system_clock::now();
		float timeInSeconds = std::chrono::duration<float>(currentTime.time_since_epoch()).count();
		uniforms.time.set(&timeInSeconds);
	}

	/**
	 * @brief Draw the widget
	 *
	 */
	void draw()
	{
		float *background = Util::Color::HSVtoRGB(parameters[Parameters::BackgroundHue], parameters[Parameters::BackgroundSaturation], parameters[Parameters::BackgroundValue]);

		glClearColor(background[0], background[1], background[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		shaderProgram.use();

		for (int j = 4; j >= 0; j--)
		{
			float p = (float)j / 4.0f;
			float focus = 1.0 - clip(abs(parameters[Parameters::FocusDistance] - p) * 2.0f, 0.0, 1.0);

			uniforms.focusAmount.set(&focus);
			uniforms.colorIndex.set(&j);

			for (int i = 0; i < 3; i++)
			{
				if (!(*layersEnabled)[i])
					continue;

				uniforms.colors.set(frameData[i]->colors);

				float size = 1.0f - (parameters[Parameters::Space] * (1.0 + parameters[Parameters::Zoom] * 10.0)) * j + parameters[Parameters::Zoom] * 10.0f;
				uniforms.size.set(&size);

				textures[i]->bind();
				uniforms.use();
				rectangle.draw();
			}
		}
	}

	DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ViewerWidget)
};

END_NAMESPACE_DISTRHO

#endif
