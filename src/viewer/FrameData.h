#pragma once

struct FrameData
{
	bool waiting = false;
	unsigned char *data;
	int width;
	int height;
};
