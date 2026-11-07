#pragma once

#include <cmath>

struct Boid {
    float x = 0.0f;
    float y = 0.0f;

    float vx = 0.0f;
    float vy = 0.0f;

    float ax = 0.0f;
    float ay = 0.0f;

    float r = 0.3f;
    float g = 0.7f;
    float b = 1.0f;

    void applyForce(float fx, float fy) {
        ax += fx;
        ay += fy;
    }

    void update(float dt, float minSpeed, float maxSpeed) {
        vx += ax * dt;
        vy += ay * dt;

        float speedSq = vx * vx + vy * vy;
        if (speedSq > maxSpeed * maxSpeed) {
            float scale = maxSpeed / std::sqrt(speedSq);
            vx *= scale;
            vy *= scale;
        } else if (speedSq < minSpeed * minSpeed && speedSq > 0.000001f) {
            float scale = minSpeed / std::sqrt(speedSq);
            vx *= scale;
            vy *= scale;
        }

        x += vx * dt;
        y += vy * dt;

        ax = 0.0f;
        ay = 0.0f;
    }

    float getAngle() const {
        return std::atan2(vy, vx);
    }
};

