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

float clip(float n, float lower, float upper)
{
	return std::max(lower, std::min(n, upper));
}

class ViewerWidget : public TopLevelWidget
{
public:
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

	bool isInitialized()
	{
		return initialized;
	}

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

	bool onMotion(const MotionEvent &event) override
	{
		setSize(getWindow().getWidth(), getWindow().getHeight());
		return true;
	}

private:
	float (&parameters)[Parameters::NumParameters];
	std::vector<bool> *layersEnabled;

	bool initialized = false;

	std::vector<FrameData *> frameData;
	std::vector<ShaderTexture *> textures;
	ShaderProgram shaderProgram;
	ShaderRectangle rectangle;
	ShaderUniforms uniforms;

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

	void update()
	{
		updateFrameData();

		uniforms.blurSize.set(&parameters[Parameters::BlurSize]);

		auto currentTime = std::chrono::system_clock::now();
		float timeInSeconds = std::chrono::duration<float>(currentTime.time_since_epoch()).count();
		uniforms.time.set(&timeInSeconds);
	}

	void draw()
	{
		float *background = Color::HSVtoRGB(parameters[Parameters::BackgroundHue], parameters[Parameters::BackgroundSaturation], parameters[Parameters::BackgroundValue]);

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
