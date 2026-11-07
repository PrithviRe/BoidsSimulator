#pragma once

#include <algorithm>
#include <vector>

class SpatialGrid {
  public:
    SpatialGrid(float minX, float maxX, float minY, float maxY, float cellSize, int maxParticles)
        : m_minX(minX), m_minY(minY), m_cellSize(cellSize),
          m_cols(static_cast<int>((maxX - minX) / cellSize) + 1),
          m_rows(static_cast<int>((maxY - minY) / cellSize) + 1),
          m_head(m_cols * m_rows, -1),
          m_next(maxParticles, -1) {}

    void clear() {
        std::fill(m_head.begin(), m_head.end(), -1);
    }

    void resize(int maxParticles) {
        m_next.resize(maxParticles, -1);
    }

    void insert(int id, float x, float y) {
        int cx = static_cast<int>((x - m_minX) / m_cellSize);
        int cy = static_cast<int>((y - m_minY) / m_cellSize);

        cx = std::clamp(cx, 0, m_cols - 1);
        cy = std::clamp(cy, 0, m_rows - 1);

        int cellIdx = cy * m_cols + cx;
        m_next[id] = m_head[cellIdx];
        m_head[cellIdx] = id;
    }

    template <typename Callback>
    void forEachNeighbor(float x, float y, Callback&& callback) const {
        int cx = static_cast<int>((x - m_minX) / m_cellSize);
        int cy = static_cast<int>((y - m_minY) / m_cellSize);

        int minCx = std::max(0, cx - 1);
        int maxCx = std::min(m_cols - 1, cx + 1);
        int minCy = std::max(0, cy - 1);
        int maxCy = std::min(m_rows - 1, cy + 1);

        for (int r = minCy; r <= maxCy; ++r) {
            for (int c = minCx; c <= maxCx; ++c) {
                int cellIdx = r * m_cols + c;
                int curr = m_head[cellIdx];
                while (curr != -1) {
                    callback(curr);
                    curr = m_next[curr];
                }
            }
        }
    }

    int getCols() const { return m_cols; }
    int getRows() const { return m_rows; }

  private:
    float m_minX;
    float m_minY;
    float m_cellSize;
    int m_cols;
    int m_rows;

    std::vector<int> m_head;
    std::vector<int> m_next;
};

