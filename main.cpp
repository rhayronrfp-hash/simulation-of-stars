#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <random>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <cmath>
#include <cstddef>
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class blackhole {
private:
    float mass;
    float r_s;
    float visualRadius;
    glm::vec3 position;

public:
    blackhole(float massValue, const glm::vec3& positionValue)
        : mass(massValue),
          r_s(2.0f * massValue),
          visualRadius(2.0f * massValue),
          position(positionValue)
    {}
    float getMass()                const { return mass; }
    float scharwazis() const { return r_s; }
    float getvisualradius()        const { return visualRadius; }
    const glm::vec3& getposition() const { return position; }
};

class Camera {
private:
    glm::vec3 position;
    glm::vec3 front   = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 right   = glm::vec3(1.0f, 0.0f,  0.0f);
    glm::vec3 up      = glm::vec3(0.0f, 1.0f,  0.0f);
    const glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    float yaw   = -90.0f;
    float pitch =   0.0f;
    float moveSpeed        = 4.0f;
    float mouseSensitivity = 0.1f;

    void updateVectors() {
        const float yawRad   = glm::radians(yaw);
        const float pitchRad = glm::radians(pitch);
        glm::vec3 f;
        f.x = std::cos(yawRad) * std::cos(pitchRad);
        f.y = std::sin(pitchRad);
        f.z = std::sin(yawRad) * std::cos(pitchRad);
        front = glm::normalize(f);
        right = glm::normalize(glm::cross(front, worldUp));
        up    = glm::normalize(glm::cross(right, front));
    }

public:
    explicit Camera(const glm::vec3& startPosition) : position(startPosition) {
        updateVectors();}

    const glm::vec3& getposition() const { return position; }

    glm::mat4 getViewMatrix() const {
        return glm::lookAt(position, position + front, up);
    }

    void processKeyboard(GLFWwindow* window, float deltaTime) {
        float speed = moveSpeed * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) speed *= 7.0f;
        if (glfwGetKey(window, GLFW_KEY_W)          == GLFW_PRESS) position += front   * speed;
        if (glfwGetKey(window, GLFW_KEY_S)          == GLFW_PRESS) position -= front   * speed;
        if (glfwGetKey(window, GLFW_KEY_A)          == GLFW_PRESS) position -= right   * speed;
        if (glfwGetKey(window, GLFW_KEY_D)          == GLFW_PRESS) position += right   * speed;
        if (glfwGetKey(window, GLFW_KEY_SPACE)      == GLFW_PRESS) position += worldUp * speed;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) position -= worldUp * speed;
    }
    void processMouse(double xoffset, double yoffset) {
        yaw   += static_cast<float>(xoffset) * mouseSensitivity;
        pitch += static_cast<float>(yoffset) * mouseSensitivity;
        if (pitch >  89.0f) pitch =  89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
        updateVectors();
    }

    void processScroll(double yoffset) {
        moveSpeed += static_cast<float>(yoffset) * 0.5f;
        if (moveSpeed <  0.5f) moveSpeed =  0.5f;
        if (moveSpeed > 30.0f) moveSpeed = 30.0f;
    }};

class particle {
public:
    float mass, r, phi, drdTau, L;
    glm::vec3 eR, ePhi;
    float tiltDegrees;

    particle(
        float massValue   = 1.0f,
        float r0          = 10.0f,
        float phi0        = 0.0f,
        float drdTau0     = 0.0f,
        float L0          = 0.0f,
        float tiltDeg     = 25.0f
    )
        : mass(massValue), r(r0), phi(phi0), drdTau(drdTau0), L(L0),
          tiltDegrees(tiltDeg),
          eR(1.0f, 0.0f, 0.0f),
          ePhi(0.0f, 0.0f, 1.0f)
    {
        const float tiltRad = glm::radians(tiltDeg);
        eR   = glm::vec3(1.0f, 0.0f, 0.0f);
        ePhi = glm::normalize(glm::vec3(0.0f, -std::sin(tiltRad), std::cos(tiltRad)));
    }

    void update(float dTau, float M, float rs) {
        if (r <= rs || !std::isfinite(r)) { r = 0.0f; return; }

        auto accel = [&](float radius) -> float {
            if (!std::isfinite(radius) || radius <= rs) return 0.0f;
            const float r2 = radius * radius;
            const float r3 = r2 * radius;
            const float r4 = r3 * radius;
            return -M / r2 + (L * L) / r3 - (3.0f * M * L * L) / r4;
        };

        const float k1_r = drdTau,                    k1_v = accel(r);
        const float k2_r = drdTau + 0.5f*dTau*k1_v,  k2_v = accel(r + 0.5f*dTau*k1_r);
        const float k3_r = drdTau + 0.5f*dTau*k2_v,  k3_v = accel(r + 0.5f*dTau*k2_r);
        const float k4_r = drdTau +      dTau*k3_v,  k4_v = accel(r +      dTau*k3_r);
        r      += (dTau / 6.0f) * (k1_r + 2.0f*k2_r + 2.0f*k3_r + k4_r);
        drdTau += (dTau / 6.0f) * (k1_v + 2.0f*k2_v + 2.0f*k3_v + k4_v);
        if (!std::isfinite(r) || r <= rs) { r = 0.0f; return; }
        phi += (L / (r * r)) * dTau;
    }
    glm::vec3 getposition(const glm::vec3& holePosition) const {
        return holePosition + r * (std::cos(phi) * eR + std::sin(phi) * ePhi);}

    glm::vec3 Orbital() const {
        float dphiDtau = (r > 0.001f) ? (L / (r * r)) : 0.0f;
        glm::vec3 tangent = -std::sin(phi) * eR + std::cos(phi) * ePhi;
        return tangent * (dphiDtau * r);
    }
    float getMass() const { return mass; }};

struct dustParticle {
    float radius;
    float phi;
    float angularSpeed;
    float height;
    float size;
    float intensity;
    float aspect;
};

struct DustVertex {
    float radius;
    float phi;
    float angularSpeed;
    float height;
    float size;
    float intensity;
    float aspect;
    glm::vec3 fixedPosition;
    float isFixed;
};

void scrollback(GLFWwindow* window, double xoffset, double yoffset) {
    (void)xoffset;
    Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    if (cam) cam->processScroll(yoffset);
}

static bool   firstMouse = true;
static double lastMouseX = 0.0;
static double lastMouseY = 0.0;

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) { lastMouseX = xpos; lastMouseY = ypos; firstMouse = false; }
    const double xoffset = xpos - lastMouseX;
    const double yoffset = lastMouseY - ypos;
    lastMouseX = xpos; lastMouseY = ypos;
    Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    if (cam) cam->processMouse(xoffset, yoffset);
}

std::string loadShaderFile(const std::string& relativePath, const std::filesystem::path& executableDirectory) {
    std::vector<std::filesystem::path> bases;
    bases.push_back(std::filesystem::current_path());
    bases.push_back(executableDirectory);
    for (const auto& base : bases) {
        std::filesystem::path candidateBase = base;
        for (int depth = 0; depth < 7; ++depth) {
            std::filesystem::path candidate = candidateBase / relativePath;
            std::ifstream file(candidate, std::ios::binary);
            if (file) {
                std::ostringstream contents;
                contents << file.rdbuf();
                return contents.str();
            }
            if (candidateBase.empty() || candidateBase == candidateBase.root_path()) break;
            candidateBase = candidateBase.parent_path();
        }
    }
    std::cerr << "Nao foi possivel carregar o shader: " << relativePath << '\n';
    return {};
}

GLuint compilashader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* sourceText = source.c_str();
    glShaderSource(shader, 1, &sourceText, nullptr);
    glCompileShader(shader);
    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE) {
        char infoLog[2048] = {};
        glGetShaderInfoLog(shader, 2048, nullptr, infoLog);
        std::cerr << "Erro no shader:\n" << infoLog << '\n';
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint linkProgram(GLuint vs, GLuint fs) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success != GL_TRUE) {
        char infoLog[2048] = {};
        glGetProgramInfoLog(program, 2048, nullptr, infoLog);
        std::cerr << "Erro ao linkar shaders:\n" << infoLog << '\n';
        glDeleteProgram(program);
        glDeleteShader(vs);
        glDeleteShader(fs);
        return 0;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

GLuint createQuadShaderProgram(const std::string& vertexSource, const std::string& fragmentSource) {
    GLuint vs = compilashader(GL_VERTEX_SHADER, vertexSource);
    GLuint fs = compilashader(GL_FRAGMENT_SHADER, fragmentSource);
    if (vs == 0 || fs == 0) { if (vs) glDeleteShader(vs); if (fs) glDeleteShader(fs); return 0; }
    return linkProgram(vs, fs);
}

GLuint createDustShaderProgram(const std::string& vertexSource, const std::string& fragmentSource) {
    GLuint vs = compilashader(GL_VERTEX_SHADER, vertexSource);
    GLuint fs = compilashader(GL_FRAGMENT_SHADER, fragmentSource);
    if (vs == 0 || fs == 0) { if (vs) glDeleteShader(vs); if (fs) glDeleteShader(fs); return 0; }
    return linkProgram(vs, fs);
}

bool resizeBloomBuffers(
    int width, int height, int bloomWidth, int bloomHeight,
    GLuint sceneFBO, GLuint sceneTexture, GLuint sceneRBO,
    GLuint pingpongFBO[2], GLuint pingpongColorbuffers[2]
) {
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTexture, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, sceneRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, sceneRBO);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, bloomWidth, bloomHeight, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return false;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

bool createDiskDustBuffer(int size, GLuint framebuffer, GLuint texture) {
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, size, size, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return complete;
}

GLuint loadEquirectangular(const char* path) {
    stbi_set_flip_vertically_on_load(true);
    int width = 0, height = 0, channels = 0;
    float* data = stbi_loadf(path, &width, &height, &channels, 3);
    if (!data) {
        std::cerr<<"Erro ao carregar HDR: "<< path << '\n';
        std::cerr<< "Motivo: "<< stbi_failure_reason() << '\n';
        return 0;
    }
    if (width <= 0 || height <= 0) {
        std::cerr <<"imagem tem dimensoes q nao funciona.\n";
        stbi_image_free(data);
        return 0;
    }
    const int originalWidth = width;
    const int originalHeight = height;
    const int maxDimension = 2048;
    if (std::max(width, height) > maxDimension) {
        const float scale = static_cast<float>(maxDimension) / static_cast<float>(std::max(width, height));
        const int reducedWidth = std::max(1, static_cast<int>(width * scale));
        const int reducedHeight = std::max(1, static_cast<int>(height * scale));
        for (int y = 0; y < reducedHeight; ++y) {
            const int sourceY = std::min(height - 1, static_cast<int>((static_cast<double>(y) / reducedHeight) * height));
            for (int x = 0; x < reducedWidth; ++x) {
                const int sourceX = std::min(width - 1, static_cast<int>((static_cast<double>(x) / reducedWidth) * width));
                const std::size_t sourceIndex = (static_cast<std::size_t>(sourceY) * width + sourceX) * 3;
                const std::size_t targetIndex = (static_cast<std::size_t>(y) * reducedWidth + x) * 3;
                data[targetIndex] = data[sourceIndex];
                data[targetIndex + 1] = data[sourceIndex + 1];
                data[targetIndex + 2] = data[sourceIndex + 2];
            }
        }
        width = reducedWidth;
        height = reducedHeight;
    }
    GLuint texID = 0;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    std::cout<<"HDR carregado: "<< path << " (" << originalWidth << "x" << originalHeight << " -> " << width << "x" << height << ")\n";
    return texID;
}
GLuint createfullsecreen() {
    float verts[] = { -1.f,-1.f,  1.f,-1.f,  1.f,1.f,  -1.f,1.f };
    unsigned int idx[] = { 0,1,2, 0,2,3 };
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo); glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    return vao;
}

float kerrISCO(float spin) {
    const float a = std::clamp(spin, -0.998f, 0.998f);
    const float z1 = 1.0f + std::cbrt(1.0f - a * a) * (std::cbrt(1.0f + a) + std::cbrt(1.0f - a));
    const float z2 = std::sqrt(3.0f * a * a + z1 * z1);
    const float direction = a >= 0.0f ? 1.0f : -1.0f;
    return 3.0f + z2 - direction * std::sqrt(std::max((3.0f - z1) * (3.0f + z1 + 2.0f * z2), 0.0f));
}

void kerrCircularOrbit(float radius, float spin, float& energy, float& angularMomentum, float& omega) {
    const float sqrtR = std::sqrt(radius);
    const float r32 = radius * sqrtR;
    const float denominator = std::pow(radius, 0.75f) * std::sqrt(std::max(r32 - 3.0f * sqrtR + 2.0f * spin, 0.000001f));
    energy = (r32 - 2.0f * sqrtR + spin) / denominator;
    angularMomentum = (radius * radius - 2.0f * spin * sqrtR + spin * spin) / denominator;
    omega = 1.0f / (r32 + spin);
}

GLuint createNovikovThorneFluxTexture(int count, float spin, float outerRadius) {
    const float innerRadius = kerrISCO(spin);
    std::vector<float> flux(static_cast<std::size_t>(count), 0.0f);
    float maximumFlux = 0.0f;
    float accumulatedIntegral = 0.0f;
    float previousRadius = innerRadius;
    float previousIntegrand = 0.0f;

    for (int i = 1; i < count; ++i) {
        const float radius = innerRadius + (outerRadius - innerRadius) * static_cast<float>(i) / static_cast<float>(count - 1);
        float energy = 0.0f, angularMomentum = 0.0f, omega = 0.0f;
        kerrCircularOrbit(radius, spin, energy, angularMomentum, omega);
        const float epsilon = std::max(radius * 0.0001f, 0.00001f);
        float lowerEnergy = 0.0f, lowerL = 0.0f, lowerOmega = 0.0f;
        float upperEnergy = 0.0f, upperL = 0.0f, upperOmega = 0.0f;
        kerrCircularOrbit(std::max(innerRadius + 0.00001f, radius - epsilon), spin, lowerEnergy, lowerL, lowerOmega);
        kerrCircularOrbit(radius + epsilon, spin, upperEnergy, upperL, upperOmega);
        const float dLdr = (upperL - lowerL) / (radius + epsilon - std::max(innerRadius + 0.00001f, radius - epsilon));
        const float currentIntegrand = (energy - omega * angularMomentum) * dLdr;
        accumulatedIntegral += 0.5f * (previousIntegrand + currentIntegrand) * (radius - previousRadius);
        const float dOmegaDr = -1.5f * std::sqrt(radius) / std::pow(std::pow(radius, 1.5f) + spin, 2.0f);
        const float energyTerm = energy - omega * angularMomentum;
        const float value = -dOmegaDr * accumulatedIntegral / (4.0f * 3.14159265359f * radius * energyTerm * energyTerm);
        flux[static_cast<std::size_t>(i)] = std::max(value, 0.0f);
        maximumFlux = std::max(maximumFlux, flux[static_cast<std::size_t>(i)]);
        previousRadius = radius;
        previousIntegrand = currentIntegrand;
    }

    if (maximumFlux > 0.0f) {
        for (float& value : flux) value /= maximumFlux;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_1D, texture);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, count, 0, GL_RED, GL_FLOAT, flux.data());
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_1D, 0);
    return texture;
}

dustParticle spawnDustParticle(std::mt19937& rng, float innerRadius, float spin) {
    std::uniform_real_distribution<float> radial(0.0f, 1.0f);
    std::uniform_real_distribution<float> angle(0.0f, 6.2831853f);
    std::normal_distribution<float> height(0.0f, 0.14f);
    std::uniform_real_distribution<float> size(1.0f, 2.8f);
    std::uniform_real_distribution<float> intensity(0.22f, 0.72f);
    std::uniform_real_distribution<float> aspect(1.15f, 2.1f);
    const float radius = innerRadius + (20.0f - innerRadius) * std::pow(radial(rng), 1.3f);
    const float clampedHeight = glm::clamp(height(rng), -0.45f, 0.45f);
    return dustParticle{
        radius,
        angle(rng),
        1.0f / (std::pow(radius, 1.5f) + spin),
        clampedHeight,
        size(rng),
        intensity(rng),
        aspect(rng)
    };
}

particle spawndiskpaerticule(int index, int total, float tiltDeg, std::mt19937& rng) {
    std::uniform_real_distribution<float> radius(6.0f, 20.0f);
    std::uniform_real_distribution<float> angle(0.0f, 6.2831853f);
    const float r0   = radius(rng);
    const float phi0 = angle(rng);
    const float L0   = r0 * std::sqrt(1.0f / std::max(r0 - 3.0f, 0.01f));
    return particle(1.0f, r0, phi0, 0.0f, L0, tiltDeg);
}

void framecallback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}

int main(int argc, char** argv) {
    (void)argc;
    const std::filesystem::path executableDirectory = std::filesystem::absolute(argv[0]).parent_path();
    if (!glfwInit()) { std::cerr<< "Erro ao inicializar GLFW\n"; return -1; }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "star simulator", nullptr, nullptr);
    if (!window) { std::cerr<< "Erro ao criar janela\n"; glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress))) {
        std::cerr<< "Erro ao inicializar GLAD\n";
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }

    glfwSetFramebufferSizeCallback(window, framecallback);
    int iw = 0, ih = 0;
    glfwGetFramebufferSize(window, &iw, &ih);
    glViewport(0, 0, iw, ih);
    glEnable(GL_DEPTH_TEST);
    glfwSwapInterval(1);

    const blackhole hole(1.0f, glm::vec3(0.0f, 0.0f, 0.0f));
    const float diskTiltDegrees = 25.0f;
    const float dimensionlessSpin = 0.85f;
    const float kerrSpin = dimensionlessSpin * hole.getMass();
    const float diskInnerRadius = kerrISCO(dimensionlessSpin) * hole.getMass();
    const float diskOuterRadius = 20.0f * hole.getMass();
    const int   N               = 900;
    const int   DUST_COUNT      = 60000;
    std::mt19937 rng(42);

//disco
    std::vector<particle> particles;
    particles.reserve(N);
    for (int i = 0; i < N; ++i)
        particles.push_back(spawndiskpaerticule(i, N, diskTiltDegrees, rng));
    std::vector<dustParticle> dustParticles;
    dustParticles.reserve(DUST_COUNT);
    for (int i = 0; i < DUST_COUNT; ++i)
        dustParticles.push_back(spawnDustParticle(rng, diskInnerRadius, kerrSpin));
    std::vector<DustVertex> dustVertices(DUST_COUNT + N);
    for (int i = 0; i < DUST_COUNT; ++i) {
        const dustParticle& dust = dustParticles[static_cast<std::size_t>(i)];
        dustVertices[static_cast<std::size_t>(i)] = DustVertex{
            dust.radius, dust.phi, dust.angularSpeed, dust.height,
            dust.size, dust.intensity, dust.aspect, glm::vec3(0.0f), 0.0f
        };}
    for (int i = 0; i < N; ++i) {
        const particle& p = particles[static_cast<std::size_t>(i)];
        dustVertices[static_cast<std::size_t>(DUST_COUNT + i)] = DustVertex{
            0.0f, p.phi, 0.0f, 0.0f, 3.0f, 0.95f, 1.8f,
            p.getposition(hole.getposition()), 1.0f
        };}


    Camera camera(glm::vec3(0.0f, 5.0f, 25.0f));
    glfwSetWindowUserPointer(window, &camera);
    glfwSetScrollCallback(window, scrollback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    const std::string fullscreenVertexSource = loadShaderFile("Shaders/fullscreen.vert", executableDirectory);
    const std::string kerrFragmentSource = loadShaderFile("Shaders/kerr_disk.frag", executableDirectory);
    const std::string dustVertexSource = loadShaderFile("Shaders/dust.vert", executableDirectory);
    const std::string dustFragmentSource = loadShaderFile("Shaders/dust.frag", executableDirectory);
    const std::string bloomFragmentSource = loadShaderFile("Shaders/bloom.frag", executableDirectory);
    const std::string bloomCompositeFragmentSource = loadShaderFile("Shaders/bloom_composite.frag", executableDirectory);
    if (fullscreenVertexSource.empty() || kerrFragmentSource.empty() || dustVertexSource.empty() || dustFragmentSource.empty() || bloomFragmentSource.empty() || bloomCompositeFragmentSource.empty()) {
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }
    GLuint quadShaderProgram = createQuadShaderProgram(fullscreenVertexSource, kerrFragmentSource);
    GLuint dustShaderProgram = createDustShaderProgram(dustVertexSource, dustFragmentSource);
    GLuint bloomShaderProgram = createQuadShaderProgram(fullscreenVertexSource, bloomFragmentSource);
    GLuint bloomCompositeProgram = createQuadShaderProgram(fullscreenVertexSource, bloomCompositeFragmentSource);
    if (quadShaderProgram == 0 || dustShaderProgram == 0 || bloomShaderProgram == 0 || bloomCompositeProgram == 0) {
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }
    GLuint hdrTexture = loadEquirectangular("HDR_multi_nebulae_1.hdr");
    if (hdrTexture == 0)
        hdrTexture = loadEquirectangular("../HDR_multi_nebulae_1.hdr");
    if (hdrTexture == 0) {
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }
    GLuint diskFluxTexture = createNovikovThorneFluxTexture(512, dimensionlessSpin, diskOuterRadius / hole.getMass());
    GLuint quadVAO = createfullsecreen();
    GLuint dustVAO = 0, dustVBO = 0;
    glGenVertexArrays(1, &dustVAO);
    glGenBuffers(1, &dustVBO);
    glBindVertexArray(dustVAO);
    glBindBuffer(GL_ARRAY_BUFFER, dustVBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(dustVertices.size() * sizeof(DustVertex)), dustVertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(DustVertex), reinterpret_cast<void*>(offsetof(DustVertex, radius)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(DustVertex), reinterpret_cast<void*>(offsetof(DustVertex, phi)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(DustVertex), reinterpret_cast<void*>(offsetof(DustVertex, angularSpeed)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(DustVertex), reinterpret_cast<void*>(offsetof(DustVertex, height)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(DustVertex), reinterpret_cast<void*>(offsetof(DustVertex, size)));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(DustVertex), reinterpret_cast<void*>(offsetof(DustVertex, intensity)));
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(DustVertex), reinterpret_cast<void*>(offsetof(DustVertex, aspect)));
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, sizeof(DustVertex), reinterpret_cast<void*>(offsetof(DustVertex, fixedPosition)));
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, sizeof(DustVertex), reinterpret_cast<void*>(offsetof(DustVertex, isFixed)));
    glEnableVertexAttribArray(8);
    glBindVertexArray(0);
    GLuint sceneFBO = 0, sceneTexture = 0, sceneRBO = 0;
    GLuint pingpongFBO[2] = {};
    GLuint pingpongColorbuffers[2] = {};
    const int diskDustMapSize = 512;
    GLuint diskDustFBO = 0, diskDustTexture = 0;
    glGenFramebuffers(1, &diskDustFBO);
    glGenTextures(1, &diskDustTexture);
    if (!createDiskDustBuffer(diskDustMapSize, diskDustFBO, diskDustTexture)) {
        std::cerr << "Erro ao criar a textura de poeira do disco\n";
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }
    glGenFramebuffers(1, &sceneFBO);
    glGenTextures(1, &sceneTexture);
    glGenRenderbuffers(1, &sceneRBO);
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    int raytraceWidth = std::max(iw / 2, 1);
    int raytraceHeight = std::max(ih / 2, 1);
    int sceneWidth = raytraceWidth;
    int sceneHeight = raytraceHeight;
    int bloomWidth = std::max(raytraceWidth / 2, 1);
    int bloomHeight = std::max(raytraceHeight / 2, 1);
    if (!resizeBloomBuffers(raytraceWidth, raytraceHeight, bloomWidth, bloomHeight, sceneFBO, sceneTexture, sceneRBO, pingpongFBO, pingpongColorbuffers)) {
        std::cerr<< "Erro ao criar framebuffers do Bloom\n";
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }
    glEnable(GL_PROGRAM_POINT_SIZE);
    const float schwarzschildRadius = hole.scharwazis();
    const float kerrHorizonRadius = hole.getMass() + std::sqrt(hole.getMass() * hole.getMass() - kerrSpin * kerrSpin);
    std::cout<<"OpenGL: "<< reinterpret_cast<const char*>(glGetString(GL_VERSION)) << '\n';
    std::cout<<"Spin: "<<dimensionlessSpin<<" | Horizonte Kerr: "<<kerrHorizonRadius<<" | ISCO: "<<diskInnerRadius<<'\n';
    double lastFrame    = glfwGetTime();
    float dustSimulationTime = 0.0f;
    int    diskIndex     = 0;  
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        const double currentFrame = glfwGetTime();
        float deltaTime = static_cast<float>(currentFrame - lastFrame);
        lastFrame = currentFrame;
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        dustSimulationTime += deltaTime;
        camera.processKeyboard(window, deltaTime);
        for (auto& p : particles)
            if (p.r > schwarzschildRadius)
                p.update(deltaTime * 100.0f, hole.getMass(), schwarzschildRadius);
        for (auto& p : particles) {
            if (p.r <= schwarzschildRadius || !std::isfinite(p.r)) {
                p = spawndiskpaerticule(diskIndex % N, N, diskTiltDegrees, rng);
                diskIndex++;
            }
        }
        for (std::size_t i = 0; i < particles.size(); ++i) {
            DustVertex& vertex = dustVertices[DUST_COUNT + i];
            vertex.fixedPosition = particles[i].getposition(hole.getposition());
            vertex.phi = particles[i].phi;
        }
        glBindBuffer(GL_ARRAY_BUFFER, dustVBO);
        glBufferSubData(
            GL_ARRAY_BUFFER,
            static_cast<GLintptr>(DUST_COUNT * sizeof(DustVertex)),
            static_cast<GLsizeiptr>(N * sizeof(DustVertex)),
            &dustVertices[DUST_COUNT]
        );
        const glm::vec3 diskRadial(1.0f, 0.0f, 0.0f);
        const float diskTiltRadians = glm::radians(diskTiltDegrees);
        const glm::vec3 diskTangent = glm::normalize(glm::vec3(0.0f, -std::sin(diskTiltRadians), std::cos(diskTiltRadians)));
        const glm::vec3 diskNormal = glm::normalize(glm::cross(diskRadial, diskTangent));
        const glm::mat4 diskView = glm::lookAt(diskNormal * 30.0f, glm::vec3(0.0f), diskTangent);
        const glm::mat4 diskProjection = glm::ortho(-22.0f, 22.0f, -22.0f, 22.0f, 0.1f, 60.0f);
        glBindFramebuffer(GL_FRAMEBUFFER, diskDustFBO);
        glViewport(0, 0, diskDustMapSize, diskDustMapSize);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glBlendFunc(GL_ONE, GL_ONE);
        glUseProgram(dustShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(dustShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(diskView));
        glUniformMatrix4fv(glGetUniformLocation(dustShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(diskProjection));
        glUniform1f(glGetUniformLocation(dustShaderProgram, "simulationTime"), dustSimulationTime);
        glUniform3fv(glGetUniformLocation(dustShaderProgram, "diskCenter"), 1, glm::value_ptr(hole.getposition()));
        glUniform3fv(glGetUniformLocation(dustShaderProgram, "diskRadial"), 1, glm::value_ptr(diskRadial));
        glUniform3fv(glGetUniformLocation(dustShaderProgram, "diskTangent"), 1, glm::value_ptr(diskTangent));
        glUniform3fv(glGetUniformLocation(dustShaderProgram, "diskNormal"), 1, glm::value_ptr(diskNormal));
        glBindVertexArray(dustVAO);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(dustVertices.size()));
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

 //Renderizacao
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (width <= 0 || height <= 0) { glfwSwapBuffers(window); continue; }
        const int newRaytraceWidth = std::max(width / 2, 1);
        const int newRaytraceHeight = std::max(height / 2, 1);
        const int newBloomWidth = std::max(newRaytraceWidth / 2, 1);
        const int newBloomHeight = std::max(newRaytraceHeight / 2, 1);
        if (newRaytraceWidth != sceneWidth || newRaytraceHeight != sceneHeight) {
            if (!resizeBloomBuffers(newRaytraceWidth, newRaytraceHeight, newBloomWidth, newBloomHeight, sceneFBO, sceneTexture, sceneRBO, pingpongFBO, pingpongColorbuffers)) {
                std::cerr << "Erro ao redimensionar framebuffers do Bloom\n";
                break;
            }
            sceneWidth = newRaytraceWidth;
            sceneHeight = newRaytraceHeight;
            bloomWidth = newBloomWidth;
            bloomHeight = newBloomHeight;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
        glViewport(0, 0, sceneWidth, sceneHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const float     aspect     = static_cast<float>(width) / static_cast<float>(height);
        const glm::vec3 camPos     = camera.getposition();
        const glm::mat4 view       = camera.getViewMatrix();
        const glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        const glm::mat4 viewInv    = glm::inverse(view);
        const glm::mat4 projInv    = glm::inverse(projection);

        glDisable(GL_DEPTH_TEST);
        glUseProgram(quadShaderProgram);
        glUniform1f(glGetUniformLocation(quadShaderProgram, "blackHoleMass"), hole.getMass());
        glUniform1f(glGetUniformLocation(quadShaderProgram, "spinParameter"), kerrSpin);
        glUniform1f(glGetUniformLocation(quadShaderProgram, "iscoRadius"), diskInnerRadius);
        glUniform1f(glGetUniformLocation(quadShaderProgram, "diskOuterRadius"), diskOuterRadius);
        glUniform1f(glGetUniformLocation(quadShaderProgram, "diskMapExtent"), 22.0f * hole.getMass());
        glUniform1f(glGetUniformLocation(quadShaderProgram, "diskTemperatureScale"), 5600.0f);
        glUniform2f(glGetUniformLocation(quadShaderProgram, "screenSize"), static_cast<float>(sceneWidth), static_cast<float>(sceneHeight));
        glUniform3fv(glGetUniformLocation(quadShaderProgram, "cameraPosition"), 1, glm::value_ptr(camPos));
        glUniform3fv(glGetUniformLocation(quadShaderProgram, "blackHolePosition"), 1, glm::value_ptr(hole.getposition()));
        glUniformMatrix4fv(glGetUniformLocation(quadShaderProgram, "viewInverse"), 1, GL_FALSE, glm::value_ptr(viewInv));
        glUniformMatrix4fv(glGetUniformLocation(quadShaderProgram, "projInverse"), 1, GL_FALSE, glm::value_ptr(projInv));
        glUniform3fv(glGetUniformLocation(quadShaderProgram, "diskNormal"), 1, glm::value_ptr(diskNormal));
        glUniform3fv(glGetUniformLocation(quadShaderProgram, "diskRadial"), 1, glm::value_ptr(diskRadial));
        glUniform3fv(glGetUniformLocation(quadShaderProgram, "diskTangent"), 1, glm::value_ptr(diskTangent));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        glUniform1i(glGetUniformLocation(quadShaderProgram, "hdrMap"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, diskDustTexture);
        glUniform1i(glGetUniformLocation(quadShaderProgram, "diskDustMap"), 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_1D, diskFluxTexture);
        glUniform1i(glGetUniformLocation(quadShaderProgram, "diskFluxMap"), 2);
        glBindVertexArray(quadVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_DEPTH_TEST);

        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[0]);
        glViewport(0, 0, bloomWidth, bloomHeight);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glUseProgram(bloomShaderProgram);
        glUniform1i(glGetUniformLocation(bloomShaderProgram, "image"), 0);
        glUniform1i(glGetUniformLocation(bloomShaderProgram, "mode"), 0);
        glUniform1f(glGetUniformLocation(bloomShaderProgram, "threshold"), 0.85f);
        glUniform2f(glGetUniformLocation(bloomShaderProgram, "texelSize"), 1.0f / static_cast<float>(bloomWidth), 1.0f / static_cast<float>(bloomHeight));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneTexture);
        glBindVertexArray(quadVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        glUniform1i(glGetUniformLocation(bloomShaderProgram, "mode"), 1);
        bool horizontal = true;
        for (int i = 0; i < 10; ++i) {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal ? 1 : 0]);
            glUniform1i(glGetUniformLocation(bloomShaderProgram, "horizontal"), horizontal ? 1 : 0);
            glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[horizontal ? 0 : 1]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
            horizontal = !horizontal;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
        glUseProgram(bloomCompositeProgram);
        glUniform1i(glGetUniformLocation(bloomCompositeProgram, "scene"), 0);
        glUniform1i(glGetUniformLocation(bloomCompositeProgram, "bloom"), 1);
        glUniform1f(glGetUniformLocation(bloomCompositeProgram, "bloomStrength"), 1.25f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[0]);
        glBindVertexArray(quadVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glfwSwapBuffers(window);
    }
    glDeleteProgram(quadShaderProgram);
    glDeleteProgram(dustShaderProgram);
    glDeleteVertexArrays(1, &dustVAO);
    glDeleteBuffers(1, &dustVBO);
    glDeleteProgram(bloomShaderProgram);
    glDeleteProgram(bloomCompositeProgram);
    glDeleteFramebuffers(1, &sceneFBO);
    glDeleteFramebuffers(2, pingpongFBO);
    glDeleteTextures(1, &sceneTexture);
    glDeleteTextures(2, pingpongColorbuffers);
    glDeleteFramebuffers(1, &diskDustFBO);
    glDeleteTextures(1, &diskDustTexture);
    glDeleteTextures(1, &diskFluxTexture);
    glDeleteRenderbuffers(1, &sceneRBO);
    glDeleteTextures(1, &hdrTexture);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
