#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "Boid.hpp"
#include "Shader.hpp"
#include "SpatialGrid.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

constexpr int WINDOW_WIDTH = 1200;
constexpr int WINDOW_HEIGHT = 900;

struct InstanceData {
    float posX, posY;
    float angle;
    float r, g, b;
};

static std::mt19937& getRNG() {
    static std::mt19937 rng(std::random_device{}());
    return rng;
}

static float randomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(getRNG());
}

void initBoid(Boid& b) {
    b.x = randomFloat(-0.85f, 0.85f);
    b.y = randomFloat(-0.85f, 0.85f);

    float angle = randomFloat(0.0f, 6.2831853f);
    float speed = randomFloat(0.3f, 0.6f);
    b.vx = std::cos(angle) * speed;
    b.vy = std::sin(angle) * speed;

    b.r = randomFloat(0.2f, 0.45f);
    b.g = randomFloat(0.6f, 1.0f);
    b.b = 1.0f;
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Boids Flocking Simulator", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int w, int h) {
        glViewport(0, 0, w, h);
    });

    Shader shader(SHADER_DIR "/boid.vs", SHADER_DIR "/boid.fs");

    // Base triangle mesh pointing along +X
    float meshVertices[] = {
        -0.007f, -0.0045f,
         0.012f,  0.0f,
        -0.007f,  0.0045f
    };

    GLuint VAO, meshVBO, instanceVBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &meshVBO);
    glGenBuffers(1, &instanceVBO);

    glBindVertexArray(VAO);

    // 1. Mesh vertices (per vertex)
    glBindBuffer(GL_ARRAY_BUFFER, meshVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(meshVertices), meshVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    // 2. Instance data (per boid instance)
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, 10000 * sizeof(InstanceData), nullptr, GL_DYNAMIC_DRAW);

    // Location 1: Position (vec2)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, posX));
    glVertexAttribDivisor(1, 1);

    // Location 2: Rotation Angle (float)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, angle));
    glVertexAttribDivisor(2, 1);

    // Location 3: Color Tint (vec3)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, r));
    glVertexAttribDivisor(3, 1);

    int boidCount = 3500;
    std::vector<Boid> boids(boidCount);
    for (auto& b : boids) initBoid(b);

    std::vector<InstanceData> instanceData(boidCount);

    constexpr float PERCEPTION_RADIUS = 0.065f;
    constexpr float SEPARATION_RADIUS = 0.028f;
    constexpr float MIN_SPEED = 0.15f;
    constexpr float MAX_SPEED = 0.85f;
    constexpr float MAX_STEER_FORCE = 3.5f;

    SpatialGrid spatialGrid(-1.1f, 1.1f, -1.1f, 1.1f, PERCEPTION_RADIUS, 10000);

    bool wrapBorders = false;
    double lastTime = glfwGetTime();

    std::cout << "========================================\n"
              << "       Boids Flocking Simulator\n"
              << "========================================\n"
              << " Left Mouse (Hold)  : Mouse Attractor (Food)\n"
              << " Right Mouse (Hold) : Mouse Predator (Scatter)\n"
              << " Space              : Repulsion impulse\n"
              << " Key '1'            : 1,500 boids\n"
              << " Key '2'            : 3,500 boids\n"
              << " Key '3'            : 7,000 boids\n"
              << " Key 'W'            : Toggle Screen Wrap vs Soft Boundaries\n"
              << " Key 'R'            : Reset flock\n"
              << " ESC                : Exit\n"
              << "========================================\n";

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float dt = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;
        dt = std::min(dt, 0.033f);

        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // Flocking count switches
        int maxVBOInstances = 10000;
        auto resizeFlock = [&](int newCount) {
            boidCount = newCount;
            boids.resize(boidCount);
            instanceData.resize(boidCount);
            spatialGrid.resize(boidCount);
            if (boidCount > maxVBOInstances) {
                maxVBOInstances = boidCount * 2;
                glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
                glBufferData(GL_ARRAY_BUFFER, maxVBOInstances * sizeof(InstanceData), nullptr, GL_DYNAMIC_DRAW);
            }
            for (auto& b : boids) initBoid(b);
        };

        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && boidCount != 1500) resizeFlock(1500);
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && boidCount != 3500) resizeFlock(3500);
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && boidCount != 7000) resizeFlock(7000);

        static bool rPressed = false;
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            if (!rPressed) {
                for (auto& b : boids) initBoid(b);
                rPressed = true;
            }
        } else { rPressed = false; }

        static bool wPressed = false;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            if (!wPressed) {
                wrapBorders = !wrapBorders;
                std::cout << "[Mode] Screen wrapping: " << (wrapBorders ? "ON" : "OFF (Soft Walls)") << '\n';
                wPressed = true;
            }
        } else { wPressed = false; }

        // Mouse coordinates mapped to [-1, 1]
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        int winW, winH;
        glfwGetWindowSize(window, &winW, &winH);

        float normMouseX = 0.0f, normMouseY = 0.0f;
        if (winW > 0 && winH > 0) {
            normMouseX = static_cast<float>(mouseX / winW) * 2.0f - 1.0f;
            normMouseY = 1.0f - static_cast<float>(mouseY / winH) * 2.0f;
        }

        bool leftMouse = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        bool rightMouse = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        bool spacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

        // 1. Clear spatial hash grid and insert all boids (O(N))
        spatialGrid.clear();
        for (int i = 0; i < boidCount; ++i) {
            spatialGrid.insert(i, boids[i].x, boids[i].y);
        }

        // 2. Compute flocking steering forces using spatial partitioning (O(N))
        for (int i = 0; i < boidCount; ++i) {
            Boid& b = boids[i];

            float sepX = 0.0f, sepY = 0.0f;
            float alignX = 0.0f, alignY = 0.0f;
            float cohX = 0.0f, cohY = 0.0f;
            int neighborCount = 0;
            int sepCount = 0;

            spatialGrid.forEachNeighbor(b.x, b.y, [&](int otherIdx) {
                if (i == otherIdx) return;
                const Boid& other = boids[otherIdx];

                float dx = b.x - other.x;
                float dy = b.y - other.y;
                float distSq = dx * dx + dy * dy;

                if (distSq < PERCEPTION_RADIUS * PERCEPTION_RADIUS && distSq > 0.000001f) {
                    float dist = std::sqrt(distSq);

                    // Separation: push away from nearby flockmates
                    if (dist < SEPARATION_RADIUS) {
                        sepX += (dx / dist) / dist;
                        sepY += (dy / dist) / dist;
                        sepCount++;
                    }

                    // Alignment: match heading
                    alignX += other.vx;
                    alignY += other.vy;

                    // Cohesion: group together
                    cohX += other.x;
                    cohY += other.y;

                    neighborCount++;
                }
            });

            // Flocking weights
            if (sepCount > 0) {
                float forceMag = std::sqrt(sepX * sepX + sepY * sepY);
                if (forceMag > 0.0001f) {
                    b.applyForce((sepX / forceMag) * 2.0f, (sepY / forceMag) * 2.0f);
                }
            }

            if (neighborCount > 0) {
                // Alignment steering
                alignX /= neighborCount;
                alignY /= neighborCount;
                float alignSpeed = std::sqrt(alignX * alignX + alignY * alignY);
                if (alignSpeed > 0.0001f) {
                    float desiredVx = (alignX / alignSpeed) * MAX_SPEED;
                    float desiredVy = (alignY / alignSpeed) * MAX_SPEED;
                    b.applyForce((desiredVx - b.vx) * 1.1f, (desiredVy - b.vy) * 1.1f);
                }

                // Cohesion steering
                cohX = (cohX / neighborCount) - b.x;
                cohY = (cohY / neighborCount) - b.y;
                float cohDist = std::sqrt(cohX * cohX + cohY * cohY);
                if (cohDist > 0.0001f) {
                    float desiredVx = (cohX / cohDist) * MAX_SPEED;
                    float desiredVy = (cohY / cohDist) * MAX_SPEED;
                    b.applyForce((desiredVx - b.vx) * 0.9f, (desiredVy - b.vy) * 0.9f);
                }
            }

            // Boundary handling
            if (wrapBorders) {
                if (b.x >  1.05f) b.x = -1.05f;
                if (b.x < -1.05f) b.x =  1.05f;
                if (b.y >  1.05f) b.y = -1.05f;
                if (b.y < -1.05f) b.y =  1.05f;
            } else {
                constexpr float margin = 0.85f;
                constexpr float turnFactor = 4.5f;
                if (b.x >  margin) b.applyForce(-turnFactor * (b.x - margin), 0.0f);
                if (b.x < -margin) b.applyForce(-turnFactor * (b.x + margin), 0.0f);
                if (b.y >  margin) b.applyForce(0.0f, -turnFactor * (b.y - margin));
                if (b.y < -margin) b.applyForce(0.0f, -turnFactor * (b.y + margin));
            }

            // Mouse interaction
            float mdx = normMouseX - b.x;
            float mdy = normMouseY - b.y;
            float mDistSq = mdx * mdx + mdy * mdy;

            if (leftMouse && mDistSq < 0.6f * 0.6f && mDistSq > 0.0001f) {
                // Attractor
                float mDist = std::sqrt(mDistSq);
                float attractForce = (1.0f - mDist / 0.6f) * 3.5f;
                b.applyForce((mdx / mDist) * attractForce, (mdy / mDist) * attractForce);
            }

            if ((rightMouse || spacePressed) && mDistSq < 0.5f * 0.5f && mDistSq > 0.0001f) {
                // Predator repulsion
                float mDist = std::sqrt(mDistSq);
                float repelForce = (1.0f - mDist / 0.5f) * 7.0f;
                b.applyForce((-mdx / mDist) * repelForce, (-mdy / mDist) * repelForce);
            }

            // Clamp max steering acceleration
            float accelSq = b.ax * b.ax + b.ay * b.ay;
            if (accelSq > MAX_STEER_FORCE * MAX_STEER_FORCE) {
                float scale = MAX_STEER_FORCE / std::sqrt(accelSq);
                b.ax *= scale;
                b.ay *= scale;
            }

            // 3. Update physics and construct GPU instance data
            b.update(dt, MIN_SPEED, MAX_SPEED);

            float speed = std::sqrt(b.vx * b.vx + b.vy * b.vy);
            float speedRatio = std::clamp((speed - MIN_SPEED) / (MAX_SPEED - MIN_SPEED), 0.0f, 1.0f);

            instanceData[i].posX = b.x;
            instanceData[i].posY = b.y;
            instanceData[i].angle = b.getAngle();
            // Color smoothly transitions from cyan to energetic warm orange at top speed
            instanceData[i].r = 0.2f + 0.8f * speedRatio;
            instanceData[i].g = 0.7f - 0.2f * speedRatio;
            instanceData[i].b = 1.0f - 0.7f * speedRatio;
        }

        // 4. Stream instance data to GPU
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, boidCount * sizeof(InstanceData), instanceData.data());

        // 5. Render boids
        glClearColor(0.04f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.use();
        glBindVertexArray(VAO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 3, boidCount);

        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &meshVBO);
    glDeleteBuffers(1, &instanceVBO);
    glDeleteVertexArrays(1, &VAO);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

