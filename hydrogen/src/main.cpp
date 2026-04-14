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
        float visibleDensity = pow(clamp(aDensity, 0.0, 1.0), 0.35);
        gl_PointSize = mix(4.5, 8.0, visibleDensity);
        density = aDensity;
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    in float density;

    vec3 inferno(float t) {
        t = clamp(t, 0.0, 1.0);
        const vec3 c0 = vec3(0.001462, 0.000466, 0.013866);
        const vec3 c1 = vec3(0.106041, 0.047399, 0.298364);
        const vec3 c2 = vec3(0.290763, 0.045644, 0.418637);
        const vec3 c3 = vec3(0.573632, 0.135480, 0.377779);
        const vec3 c4 = vec3(0.816144, 0.254082, 0.287140);
        const vec3 c5 = vec3(0.961293, 0.488716, 0.084289);
        const vec3 c6 = vec3(0.988362, 0.998364, 0.644924);

        if (t < 0.16) {
            return mix(c0, c1, smoothstep(0.0, 0.16, t));
        }
        if (t < 0.34) {
            return mix(c1, c2, smoothstep(0.16, 0.34, t));
        }
        if (t < 0.56) {
            return mix(c2, c3, smoothstep(0.34, 0.56, t));
        }
        if (t < 0.74) {
            return mix(c3, c4, smoothstep(0.56, 0.74, t));
        }
        if (t < 0.9) {
            return mix(c4, c5, smoothstep(0.74, 0.9, t));
        }
        return mix(c5, c6, smoothstep(0.9, 1.0, t));
    }

    void main() {
        // distance from center of point
        vec2 coord = gl_PointCoord - vec2(0.5);
        float dist = length(coord);

        // discard pixels outside the circle
        if (dist > 0.5) discard;

        // fading at edges
        float visibleDensity = 0.15 + 0.85 * pow(clamp(density, 0.0, 1.0), 0.35);
        float alpha = (1.0 - smoothstep(0.22, 0.5, dist)) * (0.45 + 0.95 * visibleDensity);
        vec3 color = inferno(visibleDensity) * (0.9 + 0.9 * visibleDensity);

        FragColor = vec4(color, alpha);
    }
)";

// orbit camera

float azimuth = 0.0f;
float elevation = 0.3f;
float radius = 2.2f;
double lastX = 400;
double lastY = 300;
bool mouseDown = false;

struct Viewport {
    int x;
    int y;
    int width;
    int height;
};

Viewport fixedAspectViewport(int framebufferWidth, int framebufferHeight, float targetAspect) {
    float framebufferAspect = static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight);

    if (framebufferAspect > targetAspect) {
        int height = framebufferHeight;
        int width = static_cast<int>(height * targetAspect);
        int x = (framebufferWidth - width) / 2;
        return {x, 0, width, height};
    }

    int width = framebufferWidth;
    int height = static_cast<int>(width / targetAspect);
    int y = (framebufferHeight - height) / 2;
    return {0, y, width, height};
}

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

    OrbitalState orbital{1, 0, 0};
    const int previewSampleCount = 8000;
    const int finalSampleCount = 50000;
    int currentSampleCount = finalSampleCount;
    const char* basisLabels[] = {"Complex", "Real"};
    const char* realComponentLabels[] = {"Cosine", "Sine"};

    auto normalizeOrbitalState = [&]() {
        if (orbital.l > orbital.n - 1) {
            orbital.l = orbital.n - 1;
        }

        if (orbital.basis == OrbitalBasis::Complex) {
            if (orbital.m < -orbital.l) orbital.m = -orbital.l;
            if (orbital.m > orbital.l) orbital.m = orbital.l;
            orbital.component = RealComponent::None;
            return;
        }

        if (orbital.m < 0) orbital.m = -orbital.m;
        if (orbital.m > orbital.l) orbital.m = orbital.l;
        if (orbital.m == 0) {
            orbital.component = RealComponent::None;
        } else if (orbital.component == RealComponent::None) {
            orbital.component = RealComponent::Cosine;
        }
    };

    std::vector<float> gpuData;
    std::vector<Particle> particles;
    
    glEnable(GL_PROGRAM_POINT_SIZE);
    
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    unsigned int VBO;
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    auto rebuildParticleBuffer = [&](int sampleCount) {
        particles = sampler(orbital, sampleCount);
        currentSampleCount = sampleCount;

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
    };

    rebuildParticleBuffer(finalSampleCount);

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
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const float simulationAspect = 4.0f / 3.0f;
    glm::vec3 initialCameraPos = glm::vec3(
        radius * cos(elevation) * sin(azimuth),
        radius * sin(elevation),
        radius * cos(elevation) * cos(azimuth)
    );
    glm::mat4 view = glm::lookAt(initialCameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glUseProgram(shaderProgram);

    int projLoc = glGetUniformLocation(shaderProgram, "projection");
    int viewLoc = glGetUniformLocation(shaderProgram, "view");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    while (!glfwWindowShouldClose(window)) {
        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

        Viewport simulationViewport = fixedAspectViewport(framebufferWidth, framebufferHeight, simulationAspect);
        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            simulationAspect,
            0.1f,
            1000.0f
        );

        glClear(GL_COLOR_BUFFER_BIT);
        glViewport(simulationViewport.x, simulationViewport.y, simulationViewport.width, simulationViewport.height);

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

        bool previewRequested = false;
        bool finalRequested = false;

        int basisIndex = orbital.basis == OrbitalBasis::Complex ? 0 : 1;
        if (ImGui::Combo("Basis", &basisIndex, basisLabels, IM_ARRAYSIZE(basisLabels))) {
            orbital.basis = basisIndex == 0 ? OrbitalBasis::Complex : OrbitalBasis::Real;
            normalizeOrbitalState();
            previewRequested = true;
            finalRequested = true;
        }

        previewRequested |= ImGui::SliderInt("n", &orbital.n, 1, 6);
        finalRequested |= ImGui::IsItemDeactivatedAfterEdit();
        normalizeOrbitalState();

        previewRequested |= ImGui::SliderInt("l", &orbital.l, 0, orbital.n - 1);
        finalRequested |= ImGui::IsItemDeactivatedAfterEdit();
        normalizeOrbitalState();

        if (orbital.basis == OrbitalBasis::Complex) {
            previewRequested |= ImGui::SliderInt("m", &orbital.m, -orbital.l, orbital.l);
            finalRequested |= ImGui::IsItemDeactivatedAfterEdit();
            normalizeOrbitalState();
        } else {
            previewRequested |= ImGui::SliderInt("|m|", &orbital.m, 0, orbital.l);
            finalRequested |= ImGui::IsItemDeactivatedAfterEdit();
            normalizeOrbitalState();

            if (orbital.m > 0) {
                int componentIndex = orbital.component == RealComponent::Sine ? 1 : 0;
                if (ImGui::Combo("Component", &componentIndex, realComponentLabels, IM_ARRAYSIZE(realComponentLabels))) {
                    orbital.component = componentIndex == 0 ? RealComponent::Cosine : RealComponent::Sine;
                    previewRequested = true;
                    finalRequested = true;
                }
            }
        }

        if (finalRequested) {
            rebuildParticleBuffer(finalSampleCount);
        } else if (previewRequested) {
            rebuildParticleBuffer(previewSampleCount);
        }

        ImGui::Text("Current samples: %d", currentSampleCount);
        
        ImGui::End();
        ImGui::Render();

        glViewport(0, 0, framebufferWidth, framebufferHeight);
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
