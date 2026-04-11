#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/common.hpp>
#include <iostream>
#include "hydrogen.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in float aDensity;
    out float density;

    uniform mat4 projection;
    uniform mat4 view;

    void main() {
        gl_Position = projection * view * vec4(aPos * 0.03, 1.0);
        gl_PointSize = 3.0;
        density = aDensity;
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    in float density;

    vec3 inferno(float t) {
        t = clamp(t, 0.0, 1.0);

        return vec3(
            0.0002189403691192265 +
            t * (0.1065134194856116 +
            t * (0.3106657665093352 +
            t * (0.3920117279989011 +
            t * (0.1620609183311020 +
            t * 0.0202393004715273)))),

            0.0000059736830192100 +
            t * (0.0163491696240763 +
            t * (0.1381813238509890 +
            t * (0.3204900174451070 +
            t * (0.4022656962432290 +
            t * 0.1225207355614600)))),

            0.0000013433635390000 +
            t * (0.0029864928571429 +
            t * (0.0459973660714286 +
            t * (0.2140415178571430 +
            t * (0.4346892857142860 +
            t * 0.3022044642857140))))
        );
    }

    void main() {
        // distance from center of point
        vec2 coord = gl_PointCoord - vec2(0.5);
        float dist = length(coord);

        // discard pixels outside the circle
        if (dist > 0.5) discard;

        // fading at edges
        float alpha = 1.0 - smoothstep(0.3, 0.5, dist);
        vec3 color = inferno(density);

        FragColor = vec4(color, alpha * 1);
    }
)";

// orbit camera

float azimuth = 0.0f;
float elevation = 0.3f;
float radius = 3.0f;
double lastX = 400;
double lastY = 300;
bool mouseDown = false;

void mouseCallBack(GLFWwindow* window, int button, int action, int mods) {
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    if (ImGui::GetIO().WantCaptureMouse) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        mouseDown = (action == GLFW_PRESS);
        glfwGetCursorPos(window, &lastX, &lastY);
    }
}

void cursorCallBack(GLFWwindow* window, double x, double y) {
    ImGui_ImplGlfw_CursorPosCallback(window, x, y);
    if (ImGui::GetIO().WantCaptureMouse || !mouseDown) return;

    float dx = (x - lastX) * 0.005f;
    float dy = (y - lastY) * 0.005f;
    azimuth -= dx;
    elevation = glm::clamp(elevation + dy, -1.5f, 1.5f);
    lastX = x;
    lastY = y;
}

void scrollCallBack(GLFWwindow* window, double xoffset, double yoffset) {
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    if (ImGui::GetIO().WantCaptureMouse) return;

    radius = glm::clamp(radius - static_cast<float>(yoffset) * 0.2f, 0.5f, 10.0f);
}


int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // required on Mac

    GLFWwindow* window = glfwCreateWindow(800, 600, "hydrogen atom", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to init GLEW" << std::endl;
        return -1;
    }

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, false);
    ImGui_ImplOpenGL3_Init("#version 330");

    glfwSetMouseButtonCallback(window, mouseCallBack);
    glfwSetCursorPosCallback(window, cursorCallBack);
    glfwSetScrollCallback(window, scrollCallBack);

    int n = 1;
    int l = 0;
    int m = 0;

    std::vector<float> gpuData;
    std::vector<Particle> particles = sampler(n, l, m, 100000);
    gpuData.reserve(particles.size() * 4);
    for (const auto& p : particles) {
        gpuData.push_back(p.x);
        gpuData.push_back(p.y);
        gpuData.push_back(p.z);
        gpuData.push_back(p.density);
    }
    
    glEnable(GL_PROGRAM_POINT_SIZE);
    
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    unsigned int VBO;
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, gpuData.size() * sizeof(float), gpuData.data(), GL_STATIC_DRAW);

    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex shader error: " << infoLog << std::endl;
    }

    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment shader error: " << infoLog << std::endl;
    }

    unsigned int shaderProgram;
    shaderProgram = glCreateProgram();

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader); 

    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader link error: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader); 

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive blending

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f/600.0f, 0.1f, 1000.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glUseProgram(shaderProgram);

    int projLoc = glGetUniformLocation(shaderProgram, "projection");
    int viewLoc = glGetUniformLocation(shaderProgram, "view");

    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        glm::vec3 cameraPos = glm::vec3(radius * cos(elevation) * sin(azimuth), radius * sin(elevation), radius * cos(elevation) * cos(azimuth));
        glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glUseProgram(shaderProgram);
        
        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureMouse) {
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        }

        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, particles.size());
        
        ImGui::Begin("Quantum Numbers");

        bool changed = false;

        changed |= ImGui::SliderInt("n", &n, 1, 6);
        if (l > n - 1) l = n - 1;

        changed |= ImGui::SliderInt("l", &l, 0, n - 1);
        if (m < -l) m = -l;
        if (m > l) m = l;

        changed |= ImGui::SliderInt("m", &m, -l, l);

        if (changed) {
            particles = sampler(n, l, m, 100000);

            gpuData.clear();
            gpuData.reserve(particles.size() * 4);
            for (const auto& p : particles) {
                gpuData.push_back(p.x);
                gpuData.push_back(p.y);
                gpuData.push_back(p.z);
                gpuData.push_back(p.density);
            }

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, gpuData.size() * sizeof(float), gpuData.data(), GL_STATIC_DRAW);
        }
        
        ImGui::End();
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}