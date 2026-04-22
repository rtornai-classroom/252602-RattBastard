#include <GL/glew.h>    //properties lib include
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static const int WIN_W = 600;
static const int WIN_H = 600;
static const float RADIUS = 2.5f;
static const int CURVE_SAMPLES = 300;

enum CurveMode {
    MODE_FREE = 0,
    MODE_HERMITE = 1,
    MODE_BEZIER_GMT = 2,
    MODE_BERNSTEIN = 3
};

static CurveMode mode = MODE_FREE;
static int bernCount = 0;

struct Point2f { float x, y; };
static std::vector<Point2f> pts;

static const Point2f PRESET_BEZIER[4] = {
    {150.f, 450.f},
    {150.f, 150.f},
    {450.f, 150.f},
    {450.f, 450.f},
};

static const Point2f PRESET_HERMITE[4] = {
    {240.f, 390.f},
    {390.f, 240.f},
    { 90.f, 240.f},
    {240.f,  90.f},
};

static int dragIdx = -1;
static double dragOffsetX = 0.0;
static double dragOffsetY = 0.0;
static GLuint program = 0;
static GLuint cpVAO = 0, cpVBO = 0;
static GLuint bezVAO = 0, bezVBO = 0;
static GLint uRes = -1;
static GLint uColor = -1;
static GLint uIsLine = -1;

static Point2f deCasteljau(const std::vector<Point2f>& pts, int n, float t) //ugyanaz mint 'stein csak jobb
{
    std::vector<Point2f> tmp(pts.begin(), pts.begin() + n);
    for (int r = 1; r < n; ++r)
        for (int i = 0; i < n - r; ++i) {
            tmp[i].x = (1.f - t) * tmp[i].x + t * tmp[i + 1].x;
            tmp[i].y = (1.f - t) * tmp[i].y + t * tmp[i + 1].y;
        }
    return tmp[0];
}

static Point2f hermiteGMT(const std::vector<Point2f>& pts, float t)
{
    const Point2f& P0 = pts[0];
    const Point2f& P1 = pts[1];
    float T0x = pts[2].x - P0.x, T0y = pts[2].y - P0.y;
    float T1x = pts[3].x - P1.x, T1y = pts[3].y - P1.y;

    float t2 = t * t, t3 = t2 * t;
    float h00 = 2 * t3 - 3 * t2 + 1;
    float h10 = t3 - 2 * t2 + t;
    float h01 = -2 * t3 + 3 * t2;
    float h11 = t3 - t2;

    return {
        h00 * P0.x + h10 * T0x + h01 * P1.x + h11 * T1x,
        h00 * P0.y + h10 * T0y + h01 * P1.y + h11 * T1y
    };
}

static int sampleCurve(std::vector<Point2f>& out)
{
    out.clear();
    int n = (int)pts.size();

    switch (mode) {

    case MODE_FREE: {
        if (n < 2) return n;
        out.reserve(CURVE_SAMPLES);
        for (int i = 0; i < CURVE_SAMPLES; ++i)
            out.push_back(deCasteljau(pts, n,
                (float)i / (float)(CURVE_SAMPLES - 1)));
        return n;
    }

    case MODE_HERMITE: {
        if (n < 4) return n;
        out.reserve(CURVE_SAMPLES);
        for (int i = 0; i < CURVE_SAMPLES; ++i)
            out.push_back(hermiteGMT(pts,
                (float)i / (float)(CURVE_SAMPLES - 1)));
        return 4;
    }

    case MODE_BEZIER_GMT: {
        int active = std::min(n, 4);
        if (active < 2) return active;
        out.reserve(CURVE_SAMPLES);
        for (int i = 0; i < CURVE_SAMPLES; ++i)
            out.push_back(deCasteljau(pts, active,
                (float)i / (float)(CURVE_SAMPLES - 1)));
        return active;
    }

    case MODE_BERNSTEIN: {
        int active = std::min(bernCount, n);
        if (active < 2) return active;
        out.reserve(CURVE_SAMPLES);
        for (int i = 0; i < CURVE_SAMPLES; ++i)
            out.push_back(deCasteljau(pts, active,
                (float)i / (float)(CURVE_SAMPLES - 1)));
        return active;
    }

    default: return n;
    }
}

static std::string loadTextFile(const char* path)
{
    std::ifstream f(path);
    if (!f.is_open()) { std::cerr << "ERROR: cannot open: " << path << "\n"; return ""; }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static GLuint compileShader(GLenum type, const char* src)
{
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(sh, sizeof(log), nullptr, log);
        std::cerr << "Shader compile error:\n" << log << "\n";
    }
    return sh;
}

static GLuint createProgram(const char* vertPath, const char* fragPath)
{
    std::string vs = loadTextFile(vertPath);
    std::string fs = loadTextFile(fragPath);
    GLuint vert = compileShader(GL_VERTEX_SHADER, vs.c_str());
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fs.c_str());
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        std::cerr << "Program link error:\n" << log << "\n";
    }
    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

static void makeStreamingVAO(GLuint& vao, GLuint& vbo)
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point2f), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

static void uploadPoints(GLuint vbo, const std::vector<Point2f>& pts)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
        (GLsizeiptr)(pts.size() * sizeof(Point2f)),
        pts.data(),
        GL_DYNAMIC_DRAW);
}

static int findPtAt(double mx, double my)
{
    for (int i = (int)pts.size() - 1; i >= 0; --i) {
        float dx = pts[i].x - (float)mx;
        float dy = pts[i].y - (float)my;
        if (std::sqrt(dx * dx + dy * dy) <= RADIUS + 4.0f)
            return i;
    }
    return -1;
}

static void loadPreset(const Point2f* arr, int count)
{
    pts.clear();
    for (int i = 0; i < count; ++i)
        pts.push_back(arr[i]);
    dragIdx = -1;
}

static void printHelp()
{
    std::cout << "\nDrag-and-Drop Bezier Hermitee\n"
        << "  ESC  kilép\n"
        << "  H    Hermite\n"
        << "  B    Bezier GMT\n"
        << "  A    Bezier Bernstein\n"
        << "  +    több aktiv\n"
        << "  -   kevesebb aktív\n\n";
}

static void keyCB(GLFWwindow* win, int key, int /*sc*/, int action, int /*mods*/)
{
    if (action != GLFW_PRESS) return;

    switch (key) {
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(win, GLFW_TRUE);
        break;

    case GLFW_KEY_H:
        mode = MODE_HERMITE;
        loadPreset(PRESET_HERMITE, 4);
        std::cout << "[H]\n";
        break;

    case GLFW_KEY_B:
        mode = MODE_BEZIER_GMT;
        loadPreset(PRESET_BEZIER, 4);
        std::cout << "[B]\n";
        break;

    case GLFW_KEY_A:
        mode = MODE_BERNSTEIN;
        loadPreset(PRESET_BEZIER, 4);
        bernCount = 4;
        std::cout << "[A]" << bernCount
            << " / " << pts.size() << "\n";
        break;

    case GLFW_KEY_KP_ADD:
        if (mode == MODE_BERNSTEIN) {
            int maxCount = (int)pts.size();
            if (bernCount < maxCount) ++bernCount;
            std::cout << "[+]"
                << bernCount << " / " << maxCount << "\n";
        }
        break;

    case GLFW_KEY_KP_SUBTRACT:
        if (mode == MODE_BERNSTEIN) {
            if (bernCount > 0) --bernCount;
            std::cout << "[-]"
                << bernCount << " / " << pts.size() << "\n";
        }
        break;

    default: break;
    }
}

static void mouseButtonCB(GLFWwindow* /*win*/, int button, int action, int /*mods*/)
{
    double mx, my;
    glfwGetCursorPos(glfwGetCurrentContext(), &mx, &my);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            int idx = findPtAt(mx, my);
            if (idx >= 0) {
                dragIdx = idx;
                dragOffsetX = mx - pts[idx].x;
                dragOffsetY = my - pts[idx].y;
            }
            else {
                pts.push_back({ (float)mx, (float)my });
                dragIdx = (int)pts.size() - 1;
                dragOffsetX = 0.0;
                dragOffsetY = 0.0;
                if (mode == MODE_BERNSTEIN)
                    bernCount = (int)pts.size();
            }
        }
        else if (action == GLFW_RELEASE) {
            dragIdx = -1;
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        int idx = findPtAt(mx, my);
        if (idx >= 0) {
            pts.erase(pts.begin() + idx);
            if (dragIdx == idx) dragIdx = -1;
            else if (dragIdx > idx) --dragIdx;
            if (mode == MODE_BERNSTEIN)
                bernCount = std::min(bernCount, (int)pts.size());
        }
    }
}

static void cursorPosCB(GLFWwindow* /*win*/, double mx, double my)
{
    if (dragIdx >= 0 && dragIdx < (int)pts.size()) {
        pts[dragIdx].x = (float)(mx - dragOffsetX);
        pts[dragIdx].y = (float)(my - dragOffsetY);
    }
}

static void errorCB(int /*code*/, const char* desc)
{
    std::cerr << "GLFW error: " << desc << "\n";
}

int main()
{
    glfwSetErrorCallback(errorCB);
    if (!glfwInit()) { std::cerr << "glfwInit failed\n"; return 1; }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(WIN_W, WIN_H,
        "Hermite Bezier Drag-and-Drop",
        nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { std::cerr << "glewInit failed\n"; return 1; }

    printHelp();

    glfwSetKeyCallback(window, keyCB);
    glfwSetMouseButtonCallback(window, mouseButtonCB);
    glfwSetCursorPosCallback(window, cursorPosCB);

    program = createProgram("circle_vert.glsl", "circle_frag.glsl");
    uRes = glGetUniformLocation(program, "resolution");
    uColor = glGetUniformLocation(program, "color");
    uIsLine = glGetUniformLocation(program, "isLine");

    glEnable(GL_PROGRAM_POINT_SIZE);

    makeStreamingVAO(cpVAO, cpVBO); 
    makeStreamingVAO(bezVAO, bezVBO);

    std::vector<Point2f> curvePts;
    curvePts.reserve(CURVE_SAMPLES);

    while (!glfwWindowShouldClose(window)) {

        glClearColor(1.f, 1.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
        glUniform2f(uRes, (float)WIN_W, (float)WIN_H);

        int n = (int)pts.size();

        if (n > 0) uploadPoints(cpVBO, pts);

        int active = sampleCurve(curvePts);

        if (!curvePts.empty()) {
            uploadPoints(bezVBO, curvePts);
            glUniform1i(uIsLine, 1);
            glUniform4f(uColor, 0.4f, 0.85f, 1.0f, 1.0f);
            glBindVertexArray(bezVAO);
            glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)curvePts.size());
        }

        if (mode == MODE_HERMITE) {
            if (n >= 4) {
                std::vector<Point2f> tLines = {
                    pts[0], pts[2],
                    pts[1], pts[3] 
                };
                uploadPoints(bezVBO, tLines);
                glUniform1i(uIsLine, 1);
                glUniform4f(uColor, 1.0f, 0.55f, 0.0f, 1.0f);
                glBindVertexArray(bezVAO);
                glDrawArrays(GL_LINES, 0, 4);
            }
        }
        else if (active >= 2) {
            glUniform1i(uIsLine, 1);
            glUniform4f(uColor, 1.0f, 0.55f, 0.0f, 1.0f);
            glBindVertexArray(cpVAO);
            glDrawArrays(GL_LINE_STRIP, 0, active);
        }

        if (n > 0) {
            glUniform1i(uIsLine, 0);
            glUniform4f(uColor, 0.0f, 0.8f, 0.0f, 1.0f);
            glBindVertexArray(cpVAO);
            glDrawArrays(GL_POINTS, 0, n);
        }

        glBindVertexArray(0);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteBuffers(1, &cpVBO);
    glDeleteVertexArrays(1, &cpVAO);
    glDeleteBuffers(1, &bezVBO);
    glDeleteVertexArrays(1, &bezVAO);
    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}