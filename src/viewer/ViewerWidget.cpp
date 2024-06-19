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

	void setFrame(uint8_t *frame, int width, int height)
	{
		frameData.data = frame;
		frameData.width = width;
		frameData.height = height;

		frameData.waiting = true;
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

	FrameData frameData;

	ShaderTexture texture;
	ShaderProgram shaderProgram;
	ShaderRectangle rectangle;
	ShaderUniforms uniforms;

	void init()
	{
		shaderProgram.init();
		rectangle.init();
		uniforms.init(&shaderProgram);
		texture.init();
	}

	void updateFrameData()
	{
		if (frameData.waiting)
		{
			frameData.waiting = false;
			texture.set(frameData.data, frameData.width, frameData.height);
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
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		shaderProgram.use();
		texture.bind();
		rectangle.draw();
	}

	DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ViewerWidget)
};

END_NAMESPACE_DISTRHO

#endif
