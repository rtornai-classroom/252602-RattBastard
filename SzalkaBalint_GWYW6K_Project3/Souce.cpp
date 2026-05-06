#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float g_camAngle = 0.0f;
static float g_camHeight = 0.0f;

static const float CAM_RADIUS = 10.0f;
static const float ANGLE_STEP = 0.03f;
static const float HEIGHT_STEP = 0.30f;

static bool g_lightOn = true;

static const float LIGHT_ORBIT_RADIUS = 20.0f;
static const float LIGHT_ORBIT_SPEED = 0.6f;
static const float LIGHT_HEIGHT = 0.0f;
static const glm::vec3 LIGHT_COLOR = glm::vec3(1.0f, 1.0f, 0.55f);
static const float SPHERE_RADIUS = 0.45f;

static void keyCallback(GLFWwindow* window, int key, int /*scan*/, int action, int /*mods*/)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
        case GLFW_KEY_LEFT:   g_camAngle -= ANGLE_STEP;  break;
        case GLFW_KEY_RIGHT:  g_camAngle += ANGLE_STEP;  break;
        case GLFW_KEY_UP:     g_camHeight += HEIGHT_STEP; break;
        case GLFW_KEY_DOWN:   g_camHeight -= HEIGHT_STEP; break;
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
        default: break;
        }
    }

    if (action == GLFW_PRESS && key == GLFW_KEY_A)
        g_lightOn = !g_lightOn;
}

static std::string readFile(const char* path)
{
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "ERROR: Cannot open file: " << path << "\n";
        return "";
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static GLuint compileShader(GLenum type, const std::string& src)
{
    const char* cstr = src.c_str();
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &cstr, nullptr);
    glCompileShader(s);

    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        std::cerr << "Shader compile error (" << (type == GL_VERTEX_SHADER ? "VERT" : "FRAG") << "):\n" << log << "\n";
    }
    return s;
}

static GLuint linkProgram(const char* vsPath, const char* fsPath)
{
    GLuint vs = compileShader(GL_VERTEX_SHADER, readFile(vsPath));
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, readFile(fsPath));

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        std::cerr << "Program link error:\n" << log << "\n";
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

static const float H = 2.5f;

#define FACE(x0,y0,z0, x1,y1,z1, x2,y2,z2, x3,y3,z3, nx,ny,nz)  \
    x0,y0,z0, nx,ny,nz,   x1,y1,z1, nx,ny,nz,   x2,y2,z2, nx,ny,nz,  \
    x2,y2,z2, nx,ny,nz,   x3,y3,z3, nx,ny,nz,   x0,y0,z0, nx,ny,nz

static const float CUBE_VERTS[] = {
    FACE(-H,-H,-H,  H,-H,-H,  H, H,-H, -H, H,-H,   0, 0,-1),
    FACE(H,-H, H, -H,-H, H, -H, H, H,  H, H, H,   0, 0, 1),
    FACE(-H,-H, H, -H,-H,-H, -H, H,-H, -H, H, H,  -1, 0, 0),
    FACE(H,-H,-H,  H,-H, H,  H, H, H,  H, H,-H,   1, 0, 0),
    FACE(-H,-H,-H, -H,-H, H,  H,-H, H,  H,-H,-H,   0,-1, 0),
    FACE(-H, H, H, -H, H,-H,  H, H,-H,  H, H, H,   0, 1, 0),
};


struct SphereMesh {
    std::vector<float> verts;
    std::vector<unsigned int> indices;
};

static SphereMesh buildSphere(float r, int sectors, int stacks)
{
    SphereMesh m;

    for (int i = 0; i <= stacks; ++i) {

        float phi = float(M_PI) * (float(i) / stacks - 0.5f);
        float cosPhi = std::cos(phi);
        float sinPhi = std::sin(phi);

        for (int j = 0; j <= sectors; ++j) {
            float theta = 2.0f * float(M_PI) * float(j) / sectors;
            float cosT = std::cos(theta);
            float sinT = std::sin(theta);

            float nx = cosPhi * cosT;
            float ny = cosPhi * sinT;
            float nz = sinPhi;

            m.verts.insert(m.verts.end(), {r * nx, r * ny, r * nz, nx, ny, nz});
        }
    }

    for (int i = 0; i < stacks; ++i) {
        unsigned int row1 = static_cast<unsigned int>(i * (sectors + 1));
        unsigned int row2 = static_cast<unsigned int>((i + 1) * (sectors + 1));
        for (int j = 0; j < sectors; ++j) {
            unsigned int a = row1 + j, b = row1 + j + 1;
            unsigned int c = row2 + j, d = row2 + j + 1;

            if (i != 0)          m.indices.insert(m.indices.end(), { a, c, b });
            if (i != stacks - 1) m.indices.insert(m.indices.end(), { b, c, d });
        }
    }

    return m;
}

int main()
{
    if (!glfwInit()) { std::cerr << "Failed to init GLFW\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(900, 700,
        "3 Kocka Kamera Feny Szalka Bálint", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate(); return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);
    glfwSwapInterval(1);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { std::cerr << "Failed to init GLEW\n"; return -1; }
    glEnable(GL_DEPTH_TEST);

    GLuint progCube = linkProgram("vertex_shader.glsl", "fragment_shader.glsl");
    GLuint progSphere = linkProgram("vertex_shader.glsl", "sphere_frag.glsl");

    GLint cubeModel = glGetUniformLocation(progCube, "model");
    GLint cubeView = glGetUniformLocation(progCube, "view");
    GLint cubeProj = glGetUniformLocation(progCube, "projection");
    GLint cubeLightPos = glGetUniformLocation(progCube, "lightPos");
    GLint cubeLightCol = glGetUniformLocation(progCube, "lightColor");
    GLint cubeViewPos = glGetUniformLocation(progCube, "viewPos");
    GLint cubeLightOn = glGetUniformLocation(progCube, "lightOn");

    GLint sphModel = glGetUniformLocation(progSphere, "model");
    GLint sphView = glGetUniformLocation(progSphere, "view");
    GLint sphProj = glGetUniformLocation(progSphere, "projection");
    GLint sphColor = glGetUniformLocation(progSphere, "emissiveColor");
    GLint sphLightOn = glGetUniformLocation(progSphere, "lightOn");

    GLuint cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTS), CUBE_VERTS, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    SphereMesh sphere = buildSphere(SPHERE_RADIUS, 32, 20);

    GLuint sphVAO, sphVBO, sphEBO;
    glGenVertexArrays(1, &sphVAO);
    glGenBuffers(1, &sphVBO);
    glGenBuffers(1, &sphEBO);

    glBindVertexArray(sphVAO);

    glBindBuffer(GL_ARRAY_BUFFER, sphVBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sphere.verts.size() * sizeof(float)), sphere.verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(sphere.indices.size() * sizeof(unsigned int)), sphere.indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    const GLsizei sphIndexCount = static_cast<GLsizei>(sphere.indices.size());

    const glm::vec3 CENTRES[] = {
        { 0.0f,  0.0f,   0.0f },
        { 0.0f,  0.0f, -10.0f },
        { 0.0f,  0.0f,  10.0f },
    };

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        float t = static_cast<float>(glfwGetTime());
        float lightAngle = t * LIGHT_ORBIT_SPEED;
        glm::vec3 lightPos(
            LIGHT_ORBIT_RADIUS * std::cos(lightAngle),
            LIGHT_ORBIT_RADIUS * std::sin(lightAngle),
            LIGHT_HEIGHT
        );

        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        glViewport(0, 0, fbW, fbH);

        glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec3 camPos(
            CAM_RADIUS * std::cos(g_camAngle),
            CAM_RADIUS * std::sin(g_camAngle),
            g_camHeight
        );
        glm::mat4 view = glm::lookAt(camPos,
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f));

        float aspect = (fbH > 0) ? static_cast<float>(fbW) / fbH : 1.0f;
        glm::mat4 proj = glm::perspective(glm::radians(55.0f), aspect, 0.1f, 500.0f);

        glUseProgram(progCube);
        glUniformMatrix4fv(cubeView, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(cubeProj, 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3fv(cubeLightPos, 1, glm::value_ptr(lightPos));
        glUniform3fv(cubeLightCol, 1, glm::value_ptr(LIGHT_COLOR));
        glUniform3fv(cubeViewPos, 1, glm::value_ptr(camPos));
        glUniform1i(cubeLightOn, g_lightOn ? 1 : 0);

        glBindVertexArray(cubeVAO);
        for (const auto& c : CENTRES) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), c);
            glUniformMatrix4fv(cubeModel, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        glBindVertexArray(0);

        glUseProgram(progSphere);
        glUniformMatrix4fv(sphView, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(sphProj, 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3fv(sphColor, 1, glm::value_ptr(LIGHT_COLOR));
        glUniform1i(sphLightOn, g_lightOn ? 1 : 0);

        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), lightPos);
            glUniformMatrix4fv(sphModel, 1, GL_FALSE, glm::value_ptr(model));
        }

        glBindVertexArray(sphVAO);
        glDrawElements(GL_TRIANGLES, sphIndexCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &cubeVAO);  glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &sphVAO);   glDeleteBuffers(1, &sphVBO);
    glDeleteBuffers(1, &sphEBO);
    glDeleteProgram(progCube);
    glDeleteProgram(progSphere);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}