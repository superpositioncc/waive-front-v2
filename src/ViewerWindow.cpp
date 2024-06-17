#ifndef VIEWER_WINDOW_CPP
#define VIEWER_WINDOW_CPP

#include "DistrhoUI.hpp"
#include "ViewerWidget.cpp"

START_NAMESPACE_DISTRHO

class ViewerWindow : public Window
{
public:
	ViewerWindow(Application &app, float (&p)[Parameters::NumParameters])
		: Window(app),
		  viewerWidget(new ViewerWidget(*this, p))
	{
		setTitle("Viewer");
		setSize(1024, 768);
		setResizable(true);
		show();
	}

private:
	ViewerWidget *viewerWidget;
};

END_NAMESPACE_DISTRHO

#endif
