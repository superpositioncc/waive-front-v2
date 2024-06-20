#ifndef VIEWER_WIDGET_CPP
#define VIEWER_WIDGET_CPP

#include "DistrhoUI.hpp"
#include "OpenGL.hpp"
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

class ViewerWidget : public TopLevelWidget
{
public:
	ViewerWidget(Window &window, float (&p)[Parameters::NumParameters])
		: TopLevelWidget(window),
		  parameters(p),
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

	void setFrame(int i, uint8_t *frame, int width, int height)
	{
		if (!isInitialized())
			return;

		frameData[i]->data = frame;
		frameData[i]->width = width;
		frameData[i]->height = height;
		frameData[i]->waiting = true;
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

		uniforms.threshold.set(parameters[Parameters::Threshold]);
		uniforms.width.set(parameters[Parameters::Width]);
		uniforms.use();
	}

	void draw()
	{
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		shaderProgram.use();

		for (int i = 0; i < 3; i++)
		{
			textures[i]->bind();
			rectangle.draw();
		}
	}

	DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ViewerWidget)
};

END_NAMESPACE_DISTRHO

#endif
