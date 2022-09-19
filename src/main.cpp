#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"


#define WIDTH 480
#define HEIGHT 480

float position[]={
	-0.5,-0.5,0.0,	 0.27,0.7,0.8,
	0.5f,-0.5,0.0,   0.0,0.7,0.8,
	0.0f,0.5,0.0,   0.5,0.3,0.2
};

const char* vertexSh="#version 330 core\n"
"attribute vec4 pos;\n"
"attribute vec4 col;\n"
"out vec4 color;\n"
"uniform float scale;\n"
"void main(){\n"
"gl_Position=vec4(scale * pos.x, scale * pos.y, scale * pos.z, 1.0);\n"
"color=col;\n"
"}\n";


const char* fragSh="#version 330 core\n"
"in vec4 color;\n"
"uniform vec4 u_color;\n"
"void main(){\n"
"gl_FragColor=u_color;\n"
"}\n";

int main()
{
    GLFWwindow* window;

    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK) std::cout << "Error" << std::endl;


    std::cout << glGetString(GL_VERSION) << std::endl;
	glViewport(0, 0, 800, 800);

	bool drawTriangle = true;
	float size = 0.8f;
	float color[4] = { 0.0f, 0.75f, 0.82f, 1.0f };

	// Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	if(!ImGui_ImplOpenGL2_Init()) std::cout << "Failed to initit OpenGL 2" << std::endl;


	unsigned int v_sh=glCreateShader(GL_VERTEX_SHADER);
	unsigned int f_sh=glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(v_sh,1,&vertexSh,NULL);
	glShaderSource(f_sh,1,&fragSh,NULL);
	glCompileShader(f_sh);
	glCompileShader(v_sh);


	unsigned int sh=glCreateProgram();
	glAttachShader(sh,v_sh);
	glAttachShader(sh,f_sh);
	glLinkProgram(sh);

	glDeleteShader(v_sh);
	glDeleteShader(f_sh);

	unsigned int buff,vert;
	glGenVertexArrays(1, &vert);
	glGenBuffers(1,&buff);

	glBindVertexArray(vert);
	glBindBuffer(GL_ARRAY_BUFFER,buff);
	glBufferData(GL_ARRAY_BUFFER,sizeof(position),position,GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(float)*6,0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(float)*6,(const void *)(sizeof(float)*3));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);




	glUseProgram(sh);
	glUniform1f(glGetUniformLocation(sh, "scale"), size);
	glUniform4f(glGetUniformLocation(sh, "u_color"), color[0], color[1], color[2], color[3]);

	glEnable(GL_DEPTH_TEST);

	while (!glfwWindowShouldClose(window))
	{
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL2_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		//Render Other Stuff


		//Render Imgui Stuff
		ImGui::Begin("Demo");
		ImGui::Text("Hello there adventurer!");
		ImGui::Checkbox("Draw Triangle", &drawTriangle);
		ImGui::SliderFloat("Size", &size, 0.1f, 2.0f);
		ImGui::ColorEdit4("Color", color);
		ImGui::End();

		glUseProgram(sh);
		glUniform1f(glGetUniformLocation(sh, "scale"), size);
		glUniform4f(glGetUniformLocation(sh, "u_color"), color[0], color[1], color[2], color[3]);
		glBindVertexArray(vert);
		if(drawTriangle) glDrawArrays(GL_TRIANGLES,0,3);
		glBindVertexArray(0);
		glUseProgram(0);

		//End of render
		ImGui::Render();
		ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}