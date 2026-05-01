#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

namespace {

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;
constexpr float kGravity = -7.5f;
constexpr std::size_t kMaxParticles = 12000;
constexpr float kSpawnInterval = 0.6f;
constexpr float kTrailLifeThreshold = 0.5f;
constexpr float kTrailSizeMin = 1.5f;
constexpr float kTrailSizeMax = 6.5f;
constexpr float kBurstSizeMin = 5.5f;
constexpr float kBurstSizeMax = 12.5f;

struct Particle {
    glm::vec3 position{};
    glm::vec3 velocity{};
    glm::vec4 color{};
    float life = 0.0f;
    float maxLife = 1.0f;
    float size = 1.0f;
    bool active = false;
};

struct Rocket {
    glm::vec3 position{};
    glm::vec3 velocity{};
    glm::vec3 burstColor{};
    float explodeY = 20.0f;
    bool active = false;
};

std::string ReadTextFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Tidak bisa membuka file: " + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint CreateShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::string log(static_cast<std::size_t>(len), '\0');
        glGetShaderInfoLog(shader, len, nullptr, log.data());
        throw std::runtime_error("Shader compile error:\n" + log);
    }

    return shader;
}

GLuint CreateProgram(const std::string& vsPath, const std::string& fsPath) {
    const std::string vsSource = ReadTextFile(vsPath);
    const std::string fsSource = ReadTextFile(fsPath);

    GLuint vs = CreateShader(GL_VERTEX_SHADER, vsSource);
    GLuint fs = CreateShader(GL_FRAGMENT_SHADER, fsSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
        std::string log(static_cast<std::size_t>(len), '\0');
        glGetProgramInfoLog(program, len, nullptr, log.data());
        throw std::runtime_error("Program link error:\n" + log);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

class FireworkSystem {
public:
    FireworkSystem()
        : rng_(std::random_device{}()),
          distX_(-18.0f, 18.0f),
          distVelX_(-0.6f, 0.6f),
          distVelY_(17.5f, 22.5f),
          distColor_(0.35f, 1.0f),
          distExplodeY_(18.0f, 26.0f),
          distParticleSpeed_(8.0f, 24.0f),
          distLife_(1.9f, 3.3f),
          distJitter_(-0.35f, 0.35f),
          distShape_(0.0f, 1.0f),
          distTrailLife_(0.18f, 0.38f) {
        particles_.resize(kMaxParticles);
    }

    void Update(float dt) {
        spawnTimer_ += dt;
        if (spawnTimer_ >= kSpawnInterval) {
            spawnTimer_ -= kSpawnInterval;
            SpawnRocket();
            if (RandomRange(0.0f, 1.0f) > 0.66f) {
                SpawnRocket();
            }
        }

        for (Rocket& rocket : rockets_) {
            if (!rocket.active) {
                continue;
            }

            rocket.velocity.y += kGravity * 0.35f * dt;
            rocket.position += rocket.velocity * dt;
            EmitTrail(rocket);

            if (rocket.position.y >= rocket.explodeY || rocket.velocity.y <= 0.2f) {
                ExplodeRocket(rocket);
                rocket.active = false;
            }
        }

        for (Particle& p : particles_) {
            if (!p.active) {
                continue;
            }

            p.life -= dt;
            if (p.life <= 0.0f) {
                p.active = false;
                continue;
            }

            p.velocity.y += kGravity * dt;
            p.velocity *= (1.0f - 0.1f * dt);
            p.position += p.velocity * dt;

            const float t = p.life / p.maxLife;
            p.color.a = std::max(0.0f, std::pow(t, 0.72f));

            if (p.maxLife <= kTrailLifeThreshold) {
                p.size = kTrailSizeMin + (kTrailSizeMax - kTrailSizeMin) * t;
            } else {
                p.size = kBurstSizeMin + (kBurstSizeMax - kBurstSizeMin) * t;
            }
        }
    }

    void BuildRenderBuffer(std::vector<float>& out) const {
        out.clear();
        out.reserve((kMaxParticles + rockets_.size()) * 8);

        for (const Particle& p : particles_) {
            if (!p.active) {
                continue;
            }

            out.push_back(p.position.x);
            out.push_back(p.position.y);
            out.push_back(p.position.z);
            out.push_back(p.color.r);
            out.push_back(p.color.g);
            out.push_back(p.color.b);
            out.push_back(p.color.a);
            out.push_back(p.size);
        }

        // Render glowing rocket heads during launch phase.
        for (const Rocket& rocket : rockets_) {
            if (!rocket.active) {
                continue;
            }

            out.push_back(rocket.position.x);
            out.push_back(rocket.position.y);
            out.push_back(rocket.position.z);
            out.push_back(1.0f);
            out.push_back(0.95f);
            out.push_back(0.8f);
            out.push_back(1.0f);
            out.push_back(9.5f);
        }
    }

private:
    void SpawnRocket() {
        Rocket rocket;
        rocket.active = true;
        rocket.position = glm::vec3(distX_(rng_), -13.0f, distX_(rng_) * 0.35f);
        rocket.velocity = glm::vec3(distVelX_(rng_), distVelY_(rng_), distVelX_(rng_));
        rocket.explodeY = distExplodeY_(rng_);
        rocket.burstColor = glm::vec3(distColor_(rng_), distColor_(rng_), distColor_(rng_));

        rockets_.push_back(rocket);
        if (rockets_.size() > 64) {
            rockets_.erase(rockets_.begin(), rockets_.begin() + static_cast<long>(rockets_.size() - 64));
        }
    }

    void ExplodeRocket(const Rocket& rocket) {
        constexpr int kParticlesPerExplosion = 900;

        for (int i = 0; i < kParticlesPerExplosion; ++i) {
            Particle* slot = FindInactiveParticle();
            if (slot == nullptr) {
                return;
            }

            const float shape = distShape_(rng_);
            const float theta = RandomRange(0.0f, glm::two_pi<float>());
            const float phi = std::acos(RandomRange(-1.0f, 1.0f));

            float speed = distParticleSpeed_(rng_);
            if (shape > 0.52f) {
                // Ring-biased explosion for variation.
                speed *= 0.65f;
            }

            glm::vec3 direction(
                std::sin(phi) * std::cos(theta),
                std::cos(phi),
                std::sin(phi) * std::sin(theta));

            if (shape > 0.52f) {
                direction.y *= 0.35f;
                direction = glm::normalize(direction);
            }

            slot->active = true;
            slot->position = rocket.position;
            slot->velocity = direction * speed + glm::vec3(distJitter_(rng_), distJitter_(rng_), distJitter_(rng_));
            slot->maxLife = distLife_(rng_);
            slot->life = slot->maxLife;

            const float sparkShift = RandomRange(0.85f, 1.35f);
            slot->color = glm::vec4(rocket.burstColor * sparkShift, 1.0f);
            slot->size = RandomRange(6.5f, 11.5f);
        }

#if defined(_WIN32)
        PlaySound(TEXT("SystemAsterisk"), nullptr, SND_ALIAS | SND_ASYNC | SND_NODEFAULT);
#endif
    }

    void EmitTrail(const Rocket& rocket) {
        constexpr int kTrailParticlesPerFrame = 4;
        for (int i = 0; i < kTrailParticlesPerFrame; ++i) {
            Particle* slot = FindInactiveParticle();
            if (slot == nullptr) {
                return;
            }

            slot->active = true;
            slot->position = rocket.position + glm::vec3(
                RandomRange(-0.12f, 0.12f),
                RandomRange(-0.18f, 0.18f),
                RandomRange(-0.12f, 0.12f));
            slot->velocity = glm::vec3(
                RandomRange(-0.45f, 0.45f),
                RandomRange(-2.2f, -0.9f),
                RandomRange(-0.45f, 0.45f));
            slot->maxLife = distTrailLife_(rng_);
            slot->life = slot->maxLife;
            slot->color = glm::vec4(1.0f, 0.68f, 0.28f, 0.95f);
            slot->size = RandomRange(3.0f, 5.8f);
        }
    }

    float RandomRange(float min, float max) {
        return std::uniform_real_distribution<float>(min, max)(rng_);
    }

    Particle* FindInactiveParticle() {
        for (Particle& p : particles_) {
            if (!p.active) {
                return &p;
            }
        }
        return nullptr;
    }

    std::vector<Particle> particles_;
    std::vector<Rocket> rockets_;

    std::mt19937 rng_;
    std::uniform_real_distribution<float> distX_;
    std::uniform_real_distribution<float> distVelX_;
    std::uniform_real_distribution<float> distVelY_;
    std::uniform_real_distribution<float> distColor_;
    std::uniform_real_distribution<float> distExplodeY_;
    std::uniform_real_distribution<float> distParticleSpeed_;
    std::uniform_real_distribution<float> distLife_;
    std::uniform_real_distribution<float> distJitter_;
    std::uniform_real_distribution<float> distShape_;
    std::uniform_real_distribution<float> distTrailLife_;

    float spawnTimer_ = 0.0f;
};

void FramebufferSizeCallback(GLFWwindow* /*window*/, int width, int height) {
    glViewport(0, 0, width, height);
}

}  // namespace

int main() {
    if (!glfwInit()) {
        std::cerr << "Gagal inisialisasi GLFW\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(kWindowWidth, kWindowHeight,
                                          "Implementasi Particle System pada Simulasi Kembang Api 3D Berbasis OpenGL", nullptr, nullptr);
    if (!window) {
        std::cerr << "Gagal membuat window\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Gagal inisialisasi GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);

    GLuint particleProgram = 0;
    GLuint skyProgram = 0;
    try {
        particleProgram = CreateProgram("shaders/particle.vert", "shaders/particle.frag");
        skyProgram = CreateProgram("shaders/sky.vert", "shaders/sky.frag");
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(kMaxParticles * 8 * sizeof(float)), nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(7 * sizeof(float)));
    glEnableVertexAttribArray(2);

    GLuint skyVao = 0;
    glGenVertexArrays(1, &skyVao);

    FireworkSystem system;
    std::vector<float> renderData;

    const auto start = std::chrono::high_resolution_clock::now();
    float prevTime = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window)) {
        const float now = static_cast<float>(glfwGetTime());
        float dt = now - prevTime;
        prevTime = now;

        dt = std::min(dt, 0.033f);

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        system.Update(dt);
        system.BuildRenderBuffer(renderData);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0,
                        static_cast<GLsizeiptr>(renderData.size() * sizeof(float)),
                        renderData.data());

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        const float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;

        const float t = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - start).count();
        glm::vec3 camPos(
            std::cos(t * 0.18f) * 38.0f,
            17.0f + std::sin(t * 0.27f) * 2.0f,
            std::sin(t * 0.18f) * 38.0f);

        glm::mat4 view = glm::lookAt(camPos, glm::vec3(0.0f, 13.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 200.0f);
        glm::mat4 viewProj = proj * view;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);
        glUseProgram(skyProgram);
        glUniform1f(glGetUniformLocation(skyProgram, "uTime"), t);
        glUniform2f(glGetUniformLocation(skyProgram, "uResolution"),
                    static_cast<float>(width), static_cast<float>(height));
        glBindVertexArray(skyVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glEnable(GL_DEPTH_TEST);
        glUseProgram(particleProgram);
        glUniformMatrix4fv(glGetUniformLocation(particleProgram, "uViewProj"), 1, GL_FALSE, glm::value_ptr(viewProj));

        glBindVertexArray(vao);
        const GLsizei count = static_cast<GLsizei>(renderData.size() / 8);
        glDrawArrays(GL_POINTS, 0, count);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &skyVao);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(skyProgram);
    glDeleteProgram(particleProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
