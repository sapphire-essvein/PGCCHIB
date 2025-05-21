#include <iostream>
#include <string>
#include <assert.h>
#include <vector>

using namespace std;

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

#include <cmath>

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

GLuint createTriangle(float x0, float y0, float x1, float y1, float x2, float y2);
int setupShader();
int setupGeometry();

const GLuint WIDTH = 800, HEIGHT = 600;

const GLchar *vertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;
uniform mat4 projection;
uniform mat4 model;
void main()	
{
	//...pode ter mais linhas de código aqui!
	gl_Position = projection * model * vec4(position.x, position.y, position.z, 1.0);
}
)";

const GLchar *fragmentShaderSource = R"(
#version 400
uniform vec4 inputColor;
out vec4 color;
void main()
{
	color = inputColor;
}
)";

struct Triangle {
    vec3 v1;
    vec3 v2;
    vec3 v3;
    vec3 color;
};

vector<Triangle> triangles;

vector <vec3> colors;
int iColor = 0;
vector<vec3> vertices;

int main()
{
	glfwInit();

	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Ola Triangulo! -- Rossana", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
	}

	const GLubyte *renderer = glGetString(GL_RENDERER);
	const GLubyte *version = glGetString(GL_VERSION);
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	colors.push_back(vec3(200, 191, 231));
	colors.push_back(vec3(174, 217, 224));
	colors.push_back(vec3(181, 234, 215));
	colors.push_back(vec3(255, 241, 182));
	colors.push_back(vec3(255, 188, 188));
	colors.push_back(vec3(246, 193, 199));
	colors.push_back(vec3(255, 216, 190));
	colors.push_back(vec3(220, 198, 224));
	colors.push_back(vec3(208, 230, 165));
	colors.push_back(vec3(183, 201, 226));

	for (int i=0; i<colors.size(); i++)
	{
		colors[i].r /= 255.0;
		colors[i].g /= 255.0;
		colors[i].b /= 255.0;
	}

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	GLuint shaderID = setupShader();
	
	GLuint VAO = createTriangle(-0.5,-0.5,0.5,-0.5,0.0,0.5);
	
	if (vertices.size() == 3)
	{
		Triangle tri;
		tri.v1 = vertices[0];
		tri.v2 = vertices[1];
		tri.v3 = vertices[2];
		tri.color = vec3(colors[iColor].r, colors[iColor].g, colors[iColor].b);
		iColor = (iColor + 1) % colors.size();
		triangles.push_back(tri);
		vertices.clear();
	}

	glUseProgram(shaderID);

	GLint colorLoc = glGetUniformLocation(shaderID, "inputColor");

	mat4 projection = ortho(0.0, 800.0, 600.0, 0.0, -1.0, 1.0);
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glLineWidth(10);
		glPointSize(20);

		glBindVertexArray(VAO);

		for (int i = 0; i < triangles.size(); i++)
		{
			vec3 vertices[3] = {
				triangles[i].v1,
				triangles[i].v2,
				triangles[i].v3
			};

			GLuint tempVBO;
			glGenBuffers(1, &tempVBO);
			glBindBuffer(GL_ARRAY_BUFFER, tempVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

			glUniform4f(colorLoc, triangles[i].color.r, triangles[i].color.g, triangles[i].color.b, 1.0f);

			mat4 model = mat4(1);
			glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

			glDrawArrays(GL_TRIANGLES, 0, 3);
			glDeleteBuffers(1, &tempVBO);
		}

		glBindVertexArray(0);

		glfwSwapBuffers(window);
	}
	glfwTerminate();
	return 0;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

int setupShader()
{
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}
	GLuint shaderProgram = glCreateProgram();
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
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

int setupGeometry()
{
	GLfloat vertices[] = {

		-0.5, -0.5, 0.0, 
		0.5, -0.5, 0.0,	 
		0.0, 0.5, 0.0,	
	};

	GLuint VBO, VAO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	return VAO;
}

GLuint createTriangle(float x0, float y0, float x1, float y1, float x2, float y2)
{
	GLuint VAO;

	GLfloat vertices[] = {
		x0, y0, 0.0,
		x1, y1, 0.0,
		x2, y2, 0.0,
	};

	GLuint VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	return VAO;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        cout << xpos << "  " << ypos << endl;

        // Transforma a posição para coordenadas do seu sistema (se necessário)
        vec3 clickPos = vec3(xpos, ypos, 0.0);

        // Adiciona à lista temporária
        vertices.push_back(clickPos);

        // Se já temos 3 cliques, criar o triângulo
        if (vertices.size() == 3)
        {
            Triangle tri;
            tri.v1 = vertices[0];
            tri.v2 = vertices[1];
            tri.v3 = vertices[2];
            tri.color = vec3(colors[iColor].r, colors[iColor].g, colors[iColor].b);
            iColor = (iColor + 1) % colors.size();

            triangles.push_back(tri);

            // Limpa para começar a capturar os próximos 3 cliques
            vertices.clear();
        }
    }
}