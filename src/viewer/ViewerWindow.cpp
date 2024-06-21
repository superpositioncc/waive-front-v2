#ifndef VIEWER_WINDOW_CPP
#define VIEWER_WINDOW_CPP

#include "DistrhoUI.hpp"
#include "ViewerWidget.cpp"
#include <vector>

START_NAMESPACE_DISTRHO

class ViewerWindow : public Window
{
public:
	ViewerWindow(Application &app, float (&p)[Parameters::NumParameters], std::vector<bool> *layersEnabled)
		: Window(app),
		  viewerWidget(new ViewerWidget(*this, p, layersEnabled))
	{
		setTitle("Viewer");
		setSize(1280, 720);
		setResizable(true);
		show();

		setOffsetY(getOffsetY() - 720 / 2);
	}

	ViewerWidget *getViewerWidget()
	{
		return viewerWidget;
	}

private:
	ViewerWidget *viewerWidget;

	DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ViewerWindow)
};

END_NAMESPACE_DISTRHO

#endif
