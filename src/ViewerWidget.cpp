#ifndef VIEWER_WIDGET_CPP
#define VIEWER_WIDGET_CPP

#include "DistrhoUI.hpp"
#include "OpenGL.hpp"
#include <iostream>

START_NAMESPACE_DISTRHO

using DGL_NAMESPACE::GraphicsContext;

class ViewerWidget : public TopLevelWidget
{
public:
	ViewerWidget(Window &window, float (&p)[Parameters::NumParameters])
		: TopLevelWidget(window),
		  parameters(p)
	{
	}

	bool isInitialized()
	{
		return initialized;
	}

	void setFrame(uint8_t *frame, int width, int height)
	{
		newFrame = true;
		newFrameData = frame;
		newFrameWidth = width;
		newFrameHeight = height;
	}

protected:
	void onDisplay() override
	{
		if (!initialized)
		{
			init();
			initialized = true;
		}

		if (newFrame)
		{
			newFrame = false;

			glBindTexture(GL_TEXTURE_2D, texture);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, newFrameWidth, newFrameHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, newFrameData);
		}

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(shaderProgram);
		glUniform1f(thresholdUniformLocation, parameters[Parameters::Threshold]);
		glUniform1f(widthUniformLocation, parameters[Parameters::Width]);
		glBindVertexArray(VAO);
		glBindTexture(GL_TEXTURE_2D, texture);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		// glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}

	bool onMotion(const MotionEvent &event) override
	{
		setSize(getWindow().getWidth(), getWindow().getHeight());
		return true;
	}

private:
	float (&parameters)[Parameters::NumParameters];
	bool initialized = false;

	bool newFrame = false;
	uint8_t *newFrameData;
	int newFrameWidth;
	int newFrameHeight;

	// clang-format off
	float vertices[16] = {
                     /* positions */     /* texture coords */
/*  bottom left */   -1.0f, -1.0f,        0.0f, 0.0f,
/*     top left */   -1.0f,  1.0f,        0.0f, 1.0f,
/*    top right */    1.0f,  1.0f,        1.0f, 1.0f,
/* bottom right */    1.0f, -1.0f,        1.0f, 0.0f
	};
	unsigned int indices[6] = {
		0, 1, 2,
		2, 3, 0
	};
	// clang-format on

	unsigned int VAO, VBO, EBO;
	unsigned int vertexShader;
	unsigned int fragmentShader;
	unsigned int shaderProgram;
	unsigned int texture;

	int thresholdUniformLocation;
	int widthUniformLocation;

	const char *vertexShaderSource =
#include "shaders/main.vert"
		;
	const char *fragmentShaderSource =
#include "shaders/main.frag"
		;

	void init()
	{
		int major, minor;
		glGetIntegerv(GL_MAJOR_VERSION, &major);
		glGetIntegerv(GL_MINOR_VERSION, &minor);
		std::cout << "OpenGL version: " << major << "." << minor << std::endl;

		vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
		glCompileShader(vertexShader);

		int success;
		char infoLog[512];
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

		if (!success)
		{
			glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
					  << infoLog << std::endl;
		}
		else
		{
			std::cout << "Vertex shader compiled successfully" << std::endl;
		}

		fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
		glCompileShader(fragmentShader);

		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

		if (!success)
		{
			glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
					  << infoLog << std::endl;
		}
		else
		{
			std::cout << "Fragment shader compiled successfully" << std::endl;
		}

		shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vertexShader);
		glAttachShader(shaderProgram, fragmentShader);
		glLinkProgram(shaderProgram);

		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);

		if (!success)
		{
			glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
					  << infoLog << std::endl;
		}
		else
		{
			std::cout << "Shader program linked successfully" << std::endl;
		}

		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		glBindVertexArray(0);

		thresholdUniformLocation = glGetUniformLocation(shaderProgram, "threshold");
		widthUniformLocation = glGetUniformLocation(shaderProgram, "width");

		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1280, 720, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	}

	DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ViewerWidget)
};

END_NAMESPACE_DISTRHO

#endif
