#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <iostream>

// ---------- Шейдерлер ----------
const char* vertexSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
out vec3 vColor;

uniform vec2  uOffset;   // орбитадағы бағыт: (cos, sin)
uniform float uRadius;   // орбита радиусы
uniform float uScale;    // пульсация

void main() {
    gl_Position = vec4(aPos.xy * uScale + uOffset * uRadius, aPos.z, 1.0);
    vColor = aColor;
}
)";

const char* fragmentSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0);
}
)";

// ---------- Бастапқы мәндер ----------
const float START_SPEED  = 1.5f;   // радиан/секунд
const float START_RADIUS = 0.4f;
const float MIN_SPEED = 0.1f, MAX_SPEED = 10.0f;
const float MIN_RADIUS = 0.1f, MAX_RADIUS = 0.7f;  // 0.3 (төртбұрыш) + радиус ≤ 1.0

// ---------- Жаһандық күй ----------
float speed  = START_SPEED;
float radius = START_RADIUS;
float angle  = 0.0f;
bool  showPentagon = false;   // Space: төртбұрыш ↔ бесбұрыш

void keyCallback(GLFWwindow*, int key, int, int action, int) {
    // Ауыстыру бір басуға бір рет болуы керек, сондықтан processInput емес, callback
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
        showPentagon = !showPentagon;
}

void processInput(GLFWwindow* window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Жеделдетуге де dt керек: әйтпесе жылдамдықтың өзгеруі FPS-ке тәуелді болады
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) speed += 2.0f * dt;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) speed -= 2.0f * dt;
    speed = std::clamp(speed, MIN_SPEED, MAX_SPEED);

    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) radius += 0.3f * dt;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) radius -= 0.3f * dt;
    radius = std::clamp(radius, MIN_RADIUS, MAX_RADIUS);

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        speed  = START_SPEED;
        radius = START_RADIUS;
        angle  = 0.0f;
    }
}

unsigned int compileShader(GLenum type, const char* src) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Shader compile error:\n" << log << std::endl;
    }
    return shader;
}

unsigned int createProgram(const char* vsSrc, const char* fsSrc) {
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);

    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    int ok;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        std::cerr << "Program link error:\n" << log << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

// VAO + VBO + EBO. EBO VAO байланып тұрғанда жасалады және босатылмайды
void createMesh(const float* vertices, size_t vSize,
                const unsigned int* indices, size_t iSize,
                unsigned int& vao, unsigned int& vbo, unsigned int& ebo) {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);                       // VAO бірінші

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vSize, vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);   // ЖАҢА
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, iSize, indices, GL_STATIC_DRAW);

    const int STRIDE = 6 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0) — ЖАЗБАЙМЫЗ: EBO VAO күйінің бөлігі
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "Week 05: EBO", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1);
    glfwSetKeyCallback(window, keyCallback);

    // ---------- Төртбұрыш: 4 вершина, 6 индекс ----------
    float quadVertices[] = {
        // позиция          // түс
         0.3f,  0.3f, 0.0f,  1.0f, 0.0f, 0.0f,   // 0 — оң жоғарғы
         0.3f, -0.3f, 0.0f,  0.0f, 1.0f, 0.0f,   // 1 — оң төменгі
        -0.3f, -0.3f, 0.0f,  0.0f, 0.0f, 1.0f,   // 2 — сол төменгі
        -0.3f,  0.3f, 0.0f,  1.0f, 1.0f, 0.0f    // 3 — сол жоғарғы
    };
    unsigned int quadIndices[] = {
        0, 1, 3,    // бірінші үшбұрыш
        1, 2, 3     // екінші үшбұрыш
    };

    // ---------- 2-тапсырма. Бесбұрыш: 5 вершина, 3 үшбұрыш, 9 индекс ----------
    // Радиусы 0.3, вершиналар 90°-тан бастап 72° сайын, сағат тіліне қарсы
    float pentaVertices[] = {
        // позиция                // түс
         0.000f,  0.300f, 0.0f,   1.0f, 0.0f, 0.0f,   // 0 — жоғарғы (90°)
        -0.285f,  0.093f, 0.0f,   1.0f, 1.0f, 0.0f,   // 1 — сол жоғарғы (162°)
        -0.176f, -0.243f, 0.0f,   0.0f, 1.0f, 0.0f,   // 2 — сол төменгі (234°)
         0.176f, -0.243f, 0.0f,   0.0f, 1.0f, 1.0f,   // 3 — оң төменгі (306°)
         0.285f,  0.093f, 0.0f,   0.0f, 0.0f, 1.0f    // 4 — оң жоғарғы (18°)
    };
    unsigned int pentaIndices[] = {
        0, 1, 2,    // 0-вершинадан желпуіш (fan)
        0, 2, 3,
        0, 3, 4
    };

    unsigned int quadVAO, quadVBO, quadEBO;
    createMesh(quadVertices, sizeof(quadVertices), quadIndices, sizeof(quadIndices),
               quadVAO, quadVBO, quadEBO);

    unsigned int pentaVAO, pentaVBO, pentaEBO;
    createMesh(pentaVertices, sizeof(pentaVertices), pentaIndices, sizeof(pentaIndices),
               pentaVAO, pentaVBO, pentaEBO);

    unsigned int shader = createProgram(vertexSrc, fragmentSrc);

    // Uniform орындарын циклге дейін БІР рет табамыз
    int locOffset = glGetUniformLocation(shader, "uOffset");
    int locScale  = glGetUniformLocation(shader, "uScale");
    int locRadius = glGetUniformLocation(shader, "uRadius");

    float lastFrame = (float)glfwGetTime();
    double fpsTimer = glfwGetTime();
    int frames = 0;

    while (!glfwWindowShouldClose(window)) {
        // --- Уақыт ---
        float now = (float)glfwGetTime();
        float dt  = now - lastFrame;
        lastFrame = now;

        processInput(window, dt);
        angle += speed * dt;

        // Пульсация: [0.5, 1.0]
        float scale = 0.5f + 0.5f * (std::sin(now * 3.0f) + 1.0f) * 0.5f;

        glClearColor(0.08f, 0.09f, 0.13f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader);
        glUniform1f(locScale, scale);

        // Индекс саны — вершина саны емес: төртбұрыш 6, бесбұрыш 9
        unsigned int vao   = showPentagon ? pentaVAO : quadVAO;
        int          count = showPentagon ? 9 : 6;
        glBindVertexArray(vao);   // 3-тапсырма: бір bind, екі сызу

        // 1-объект: сағат тіліне қарсы
        glUniform1f(locRadius, radius);
        glUniform2f(locOffset, std::cos(angle), std::sin(angle));
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0);

        // 2-объект: сағат тілімен, кішірек орбитада
        glUniform1f(locRadius, radius * 0.5f);
        glUniform2f(locOffset, std::cos(-angle), std::sin(-angle));
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0);

        frames++;
        if (glfwGetTime() - fpsTimer >= 1.0) {
            std::cout << "FPS: " << frames
                      << "  speed: " << speed << " rad/s"
                      << "  radius: " << radius << std::endl;
            frames = 0;
            fpsTimer = glfwGetTime();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteBuffers(1, &quadEBO);      // ЖАҢА
    glDeleteVertexArrays(1, &pentaVAO);
    glDeleteBuffers(1, &pentaVBO);
    glDeleteBuffers(1, &pentaEBO);
    glDeleteProgram(shader);
    glfwTerminate();
}
