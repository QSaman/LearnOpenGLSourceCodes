#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <algorithm>

#include <shader_loader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define WIDTH 800
#define HEIGHT 600

float alpha = 0.2f;
const float speed = 0.5f;
double prevTime = 0.0;
double curTime;

struct MyImage
{
    const char* path;
    GLenum pixelFormat;
};

const bool enableWireframeMode = false;

GLFWwindow* initGLFW()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); //Fixing compilation on OS X
#endif

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Hello OpenGL", NULL, NULL);
    if (window != NULL)
        glfwMakeContextCurrent(window);
    return window;
}

bool loadOpenGL()
{
    return gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
}

//Callbacks:

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void setGLFWcallbacks(GLFWwindow* window)
{
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
}

void processInput(GLFWwindow* window)
{
    auto delta = curTime - prevTime;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    else if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
    {
        alpha += static_cast<float>(delta * speed);
		//windows.h has macro definition for both min and max so I got
		//compilation error in Windows. One solution is to use 
		//type parameter:
        alpha = std::min<float>(alpha, 1.0f);
    }
    else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
    {
        alpha -= static_cast<float>(delta * speed);
		//windows.h has macro definition for both min and max so I got
		//compilation error in Windows. One solution is to use 
		//type parameter:
        alpha = std::max<float>(alpha, 0.0f);
    }
}

void setupVAO(GLuint& vao, GLuint& vbo, GLuint& ebo)
{
    struct Vertex
    {
        GLfloat position[3];
        GLfloat color[3];
        GLfloat texture[2];
    };


    Vertex vertices[4] =
	{
        {{0.5f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},     //top right
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},    //bottom right
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},   //bottom left
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}     //top left
	};

    GLuint indices[2][3] =
    {
        {0, 1, 3},
        {1, 2, 3}
    };

	glGenVertexArrays(1, &vao);

	glGenBuffers(1, &vbo);

    glGenBuffers(1, &ebo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), (void*)NULL);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), (void*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

	// note that this is allowed, the call to glVertexAttribPointer registered 
	//VBO as the vertex attribute's bound vertex buffer object so afterwards 
	//we can safely unbind:
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally 
	//modify this VAO, but this rarely happens. Modifying other VAOs requires 
	//a call to glBindVertexArray anyways so we generally don't unbind VAOs 
	//(nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

bool setupTexture(GLuint* texture, int texNum, MyImage* image)
{
    for (int i = 0; i < texNum; ++i)
    {
        glBindTexture(GL_TEXTURE_2D, texture[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        int width, height, channelNumber;
        // tell stb_image.h to flip loaded texture's on the y-axis.
        stbi_set_flip_vertically_on_load(true);
        auto data = stbi_load(image[i].path, &width, &height, &channelNumber, 0);
        if (data == NULL)
        {
            std::cerr << "Cannot load " << image[i].path << std::endl;
            return false;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, image[i].pixelFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
    }
    return true;
}

void renderFrame(ShaderLoader& shader, GLuint vao, GLuint* texture, int texNum, GLint alphauniform)
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    shader.use();
    glUniform1f(alphauniform, alpha);
    for (int i = 0; i < texNum; ++i)
    {
        // bind textures on corresponding texture units
        glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(i));
        glBindTexture(GL_TEXTURE_2D, texture[i]);
    }
	glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
}

int main()
{
    auto window = initGLFW();
	if (window == NULL)
	{
        std::cerr << "Cannot create a GLFW window" << std::endl;
		glfwTerminate();
		return 1;
	}

    setGLFWcallbacks(window);

    if (!loadOpenGL())
    {
        std::cout << "Failed to load OpenGL" << std::endl;
        glfwTerminate();
        return 1;
    }

    ShaderLoader shader("shaders/shader.vs", "shaders/shader.fs");
    try
    {
        shader.linkShaders();
    }
    catch (std::string str)
    {
        std::cerr << str << std::endl;
        return 1;
    }

    GLuint texture[2];
    //Note that the png file has alpha channel!
    MyImage image[2] =
    {
      {"shaders/container.jpg", GL_RGB},
      {"shaders/awesomeface.png", GL_RGBA}
    };
    glGenTextures(2, texture);
    if (!setupTexture(texture, 2, image))
        return false;

    GLuint vao, vbo, ebo;
    setupVAO(vao, vbo, ebo);

	if (enableWireframeMode)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    shader.use();
    //tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    //We can use glUniform1i after shader.use
    glUniform1i(glGetUniformLocation(shader.getProgramId(), "ourTexture1"), 0);
    glUniform1i(glGetUniformLocation(shader.getProgramId(), "ourTexture2"), 1);

    auto alphaUnifrom = glGetUniformLocation(shader.getProgramId(), "alpha");
    while (!glfwWindowShouldClose(window))
    {
        curTime = glfwGetTime();
        processInput(window);
        renderFrame(shader, vao, texture, 2, alphaUnifrom);
        glfwSwapBuffers(window);
        glfwPollEvents();
        prevTime = curTime;
    }
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteTextures(2, texture);
    glfwTerminate();
}
