#pragma once

#include <memory>
#include <vector>

#include "glm/glm.hpp"

#include "GLFW/glfw3.h"

// forward declaration
class ShaderProgram;

class VoxelGrid {
public:
    VoxelGrid(glm::vec4 color = glm::vec4(0.33f));

    ~VoxelGrid();

    void initGL(glm::vec3 origin, glm::vec3 dimensions);

    void render(glm::mat4 P, glm::mat4 MV, float kernel_size);

private:
    inline unsigned int get_line_count() {
        const glm::uvec3 &s = voxel_grid_size;
        return (s.x + 2) * (s.y + 2) +
               (s.x + 2) * (s.z + 2) +
               (s.y + 2) * (s.z + 2);
    }

    void render_voxel_grid_wireframe(glm::mat4 MVP);

    void generate_wireframe_vertices();

    glm::vec3 origin, dimensions;
    glm::uvec3 voxel_grid_size;

    std::vector<glm::vec3> vertices;
    glm::vec4 wireframe_color;
    float last_kernel_size;

    std::shared_ptr<ShaderProgram> shader;
    GLuint VAO, VBO;
    GLint loc_MVP, loc_Color;
};