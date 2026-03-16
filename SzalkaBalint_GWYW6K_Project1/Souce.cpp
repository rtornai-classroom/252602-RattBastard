#include </Project0_OpenGL1/deps/include/GL/glew.h>
#include </Project0_OpenGL1/deps/include/GLFW/glfw3.h>

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static constexpr int WIN_W = 600;
static constexpr int WIN_H = 600;
static constexpr float RADIUS = 50.0f;
static constexpr float SPEED = 200.0f;
static constexpr float LINE_SPEED = 50.0f;

static std::string readFile(const char* path)
{
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "shaderfile not found: " << path << '\n';
        return "";
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static GLuint compileShader(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);

    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) 
    {
        char log[512];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        std::cerr << "shader compile: " << log << '\n';
    }
    return s;
}

static GLuint createProgram(const char* vertPath, const char* fragPath)
{
    std::string vsrc = readFile(vertPath);
    std::string fsrc = readFile(fragPath);

    GLuint vs = compileShader(GL_VERTEX_SHADER, vsrc.c_str());
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsrc.c_str());
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) 
    {
        char log[512];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        std::cerr << "program link: " << log << '\n';
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

static void ortho(float m[16], float l, float r, float b, float t)
{
    for (int i = 0; i < 16; ++i) m[i] = 0.0f;
    m[0] = 2.0f / (r - l);
    m[5] = 2.0f / (t - b);
    m[10] = -1.0f;
    m[12] = -(r + l) / (r - l);
    m[13] = -(t + b) / (t - b);
    m[15] = 1.0f;
}


int main()
{
    
    if (!glfwInit()) 
    {
        std::cerr << "glfwInit failed\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(WIN_W, WIN_H, "SzB Project", nullptr, nullptr);

    if (!window) 
    {
        std::cerr << "glfwCreateWindow failed\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) 
    {
        std::cerr << "glewInit failed\n";
        return 1;
    }
    glViewport(0, 0, WIN_W, WIN_H);

    float proj[16];
    ortho(proj, 0.0f, (float)WIN_W, 0.0f, (float)WIN_H);

    GLuint circleProgram = createProgram("circle_vert.glsl", "circle_frag.glsl");
    GLuint lineProgram = createProgram("default_vert.glsl", "line_frag.glsl");

    {
        const float r = RADIUS;
        const float verts[] = {
            -r, -r,
            r, -r,
            r, r,
            -r, r
        };
        const unsigned int idx[] = { 0, 1, 2,  0, 2, 3};

        GLuint VAO, VBO, EBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);

        const float cx = WIN_W / 2.0f;
        const float cy = WIN_H / 2.0f;
        const float hw = 100.0f;
        const float hh = 1.5f;
        const float lv[] = {
            cx - hw, cy - hh,
            cx + hw, cy - hh,
            cx + hw, cy + hh,
            cx - hw, cy + hh
        };
        const unsigned int li[] = { 0, 1, 2,  0, 2, 3};

        GLuint lVAO, lVBO, lEBO;
        glGenVertexArrays(1, &lVAO);
        glGenBuffers(1, &lVBO);
        glGenBuffers(1, &lEBO);

        glBindVertexArray(lVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(lv), lv, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(li), li, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);

        float lineY = WIN_H / 2.0f;
        float circleX = WIN_W / 2.0f;
        float circleY = WIN_H / 2.0f;
        float velX = SPEED;

        double lastTime = glfwGetTime();

        while (!glfwWindowShouldClose(window))
        {
            double now = glfwGetTime();
            float  dt = (float)(now - lastTime);
            lastTime = now;

            circleX += velX * dt;
            if (circleX + RADIUS >= (float)WIN_W) 
            {
                circleX = (float)WIN_W - RADIUS;
                velX = -SPEED;
            }
            if (circleX - RADIUS <= 0.0f) 
            {
                circleX = RADIUS;
                velX = SPEED;
            }

            glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(circleProgram);
            glUniformMatrix4fv(glGetUniformLocation(circleProgram, "uProjection"),1, GL_FALSE, proj);
            glUniform2f(glGetUniformLocation(circleProgram, "uCenter"),circleX, circleY);
            glUniform1f(glGetUniformLocation(circleProgram, "uRadius"),RADIUS);
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) 
                lineY += LINE_SPEED * dt;
            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) 
                lineY -= LINE_SPEED * dt;

            glUseProgram(lineProgram);
            glUniformMatrix4fv(glGetUniformLocation(lineProgram, "uProjection"),1, GL_FALSE, proj);
            glUniform2f(glGetUniformLocation(lineProgram, "uOffset"),0.0f, lineY - WIN_H / 2.0f);
            glBindVertexArray(lVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            glfwSwapBuffers(window);
            glfwPollEvents();

            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteVertexArrays(1, &lVAO);
        glDeleteBuffers(1, &lVBO);
        glDeleteBuffers(1, &lEBO);
    }

    glDeleteProgram(circleProgram);
    glDeleteProgram(lineProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
