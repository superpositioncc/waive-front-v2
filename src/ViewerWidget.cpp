#ifndef VIEWER_WIDGET_CPP
#define VIEWER_WIDGET_CPP

#include "DistrhoUI.hpp"
#include "Color.hpp"
#include "Geometry.hpp"
#include <iostream>

START_NAMESPACE_DISTRHO

using DGL_NAMESPACE::Color;
using DGL_NAMESPACE::GraphicsContext;
using DGL_NAMESPACE::Rectangle;

class ViewerWidget : public TopLevelWidget
{
public:
	ViewerWidget(Window &window, float (&p)[Parameters::NumParameters])
		: TopLevelWidget(window),
		  parameters(p)
	{
	}

protected:
	void onDisplay() override
	{
		const Window &win = getWindow();
		const GraphicsContext &context = getWindow().getGraphicsContext();

		const uint width = win.getWidth();
		const uint height = win.getHeight();

		float value = parameters[Parameters::Value];

		Rectangle<double> r;

		r.setPos(0, 0);
		r.setSize(width, height);
		Color(value, value, value).setFor(context);
		r.draw(context);
	}

private:
	float (&parameters)[Parameters::NumParameters];
};

END_NAMESPACE_DISTRHO

#endif
