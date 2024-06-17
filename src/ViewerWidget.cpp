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
		// glViewport(0, 0, 1024, 768);
	}

protected:
	void onDisplay() override
	{
		if (!initialized)
		{
			init();
			initialized = true;
		}

		const Window &win = getWindow();
		const GraphicsContext &context = (getWindow().getGraphicsContext());

		const uint width = win.getWidth();
		const uint height = win.getHeight();

		float value = parameters[Parameters::Value];

		glClearColor(value, value, value, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(shaderProgram);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

private:
	float (&parameters)[Parameters::NumParameters];
	bool initialized = false;

	float vertices[9] = {
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.0f, 0.5f, 0.0f};

	unsigned int VAO, VBO;
	unsigned int vertexShader;
	unsigned int fragmentShader;
	unsigned int shaderProgram;

	const char *vertexShaderSource = "#version 330 core\n"
									 "layout (location = 0) in vec3 aPos;\n"
									 "void main()\n"
									 "{\n"
									 "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
									 "}\0";

	const char *fragmentShaderSource = "#version 330 core\n"
									   "out vec4 FragColor;\n"
									   "void main()\n"
									   "{\n"
									   "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
									   "}\n\0";

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

		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
		glEnableVertexAttribArray(0);
	}

	DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ViewerWidget)
};

END_NAMESPACE_DISTRHO

#endif
