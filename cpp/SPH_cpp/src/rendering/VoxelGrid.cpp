#include "rendering/ShaderProgram.hpp"

#include "rendering/VoxelGrid.hpp"

#include <cmath>
#include "glm/gtx/string_cast.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "common/GLerror.hpp"

typedef unsigned int uint;

template<typename T>
T clamp(const T &value, const T &min, const T &max) {
    return std::min(std::max(value, min), max);
}

VoxelGrid::VoxelGrid(glm::vec4 color) : last_kernel_size(-1.0f), wireframe_color(color) {
}

VoxelGrid::~VoxelGrid() {
}

void VoxelGrid::initGL(glm::vec3 origin, glm::vec3 dimensions) {
    this->origin = origin;
    this->dimensions = dimensions;

    shader = std::shared_ptr<ShaderProgram>(new ShaderProgram(
            "../shaders/simple.vert",
            "", "", "",
            "../shaders/simple.frag"
    ));

    loc_MVP = glGetUniformLocation(*shader, "MVP");
    loc_Color = glGetUniformLocation(*shader, "Color");

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_LINE_WIDTH);
    glLineWidth(10);
}

void VoxelGrid::render(glm::mat4 P, glm::mat4 MV, float kernel_size) {
    if (last_kernel_size < 0 || last_kernel_size != kernel_size) {
        last_kernel_size = kernel_size;
        voxel_grid_size = glm::uvec3(
                static_cast<uint>(floorf(dimensions.x / last_kernel_size)),
                static_cast<uint>(floorf(dimensions.y / last_kernel_size)),
                static_cast<uint>(floorf(dimensions.z / last_kernel_size))
        );

        vertices.resize(2 * get_line_count());

        generate_wireframe_vertices();
    }

    render_voxel_grid_wireframe(P * MV);
}

void VoxelGrid::render_voxel_grid_wireframe(glm::mat4 MVP) {
    glBindVertexArray(VAO);
    CheckGLerror(__FILE__, __LINE__);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    CheckGLerror(__FILE__, __LINE__);

    glUseProgram(*shader);
    CheckGLerror(__FILE__, __LINE__);

    glUniform4fv(loc_Color, 1, glm::value_ptr(wireframe_color));
    CheckGLerror(__FILE__, __LINE__);
    glUniformMatrix4fv(loc_MVP, 1, GL_FALSE, glm::value_ptr(MVP));
    CheckGLerror(__FILE__, __LINE__);

    glDrawArrays(GL_LINES, 0, 2 * get_line_count());
    CheckGLerror(__FILE__, __LINE__);

    glUseProgram(0);
    CheckGLerror(__FILE__, __LINE__);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    CheckGLerror(__FILE__, __LINE__);

    glBindVertexArray(0);
    CheckGLerror(__FILE__, __LINE__);
}

void VoxelGrid::generate_wireframe_vertices() {
    typedef unsigned int uint;

    uint vertex_count = 0;

    // xy-planes
    {
        for (uint idx = 0; idx < voxel_grid_size.x + 2; ++idx) {
            for (uint idy = 0; idy < voxel_grid_size.y + 2; ++idy) {
                // line vertex located in near xy-plane
                vertices[vertex_count] = glm::vec3(
                        std::min(origin.x + idx * last_kernel_size, origin.x + dimensions.x),
                        - std::min(origin.y + idy * last_kernel_size, origin.y + dimensions.y),
                        origin.z
                );

                // line vertex located in far xy-plane
                vertices[vertex_count + 1] = glm::vec3(
                        std::min(origin.x + idx * last_kernel_size, origin.x + dimensions.x),
                        - std::min(origin.y + idy * last_kernel_size, origin.y + dimensions.y),
                        origin.z + dimensions.z
                );

                vertex_count += 2;
            }
        }

//        vertices[vertex_count] = glm::vec3(
//                origin.x + dimensions.x,
//                origin.y + dimensions.y,
//                origin.z
//        );
//
//        vertices[vertex_count + 1] = origin + dimensions;
//
//        vertex_count += 2;
    }

    // xz-planes
    {
        glm::vec3 vertex = origin;

        for (uint idx = 0; idx < voxel_grid_size.x + 2; ++idx) {
            for (uint idz = 0; idz < voxel_grid_size.z + 2; ++idz) {
                // line vertex located in near xy-plane
                vertices[vertex_count] = glm::vec3(
                        std::min(origin.x + idx * last_kernel_size, origin.x + dimensions.x),
                        - origin.y,
                        std::min(origin.z + idz * last_kernel_size, origin.z + dimensions.z)
                );

                // line vertex located in far xy-plane
                vertices[vertex_count + 1] = glm::vec3(
                        std::min(origin.x + idx * last_kernel_size, origin.x + dimensions.x),
                        - (origin.y + dimensions.y),
                        std::min(origin.z + idz * last_kernel_size, origin.z + dimensions.z)
                );

                vertex_count += 2;
            }
        }

//        vertices[vertex_count] = glm::vec3(
//                origin.x + dimensions.x,
//                origin.y,
//                origin.z + dimensions.z
//        );
//
//        vertices[vertex_count + 1] = origin + dimensions;
//
//        vertex_count += 2;
    }

    // yz-planes
    {
        glm::vec3 vertex = origin;

        for (uint idy = 0; idy < voxel_grid_size.y + 2; ++idy) {
            for (uint idz = 0; idz < voxel_grid_size.z + 2; ++idz) {
                // line vertex located in near xy-plane
                vertices[vertex_count] = glm::vec3(
                        origin.x,
                        - std::min(origin.y + idy * last_kernel_size, origin.y + dimensions.y),
                        std::min(origin.z + idz * last_kernel_size, origin.z + dimensions.z)
                );

                // line vertex located in far xy-plane
                vertices[vertex_count + 1] = glm::vec3(
                        origin.x + dimensions.x,
                        - std::min(origin.y + idy * last_kernel_size, origin.y + dimensions.y),
                        std::min(origin.z + idz * last_kernel_size, origin.z + dimensions.z)
                );

                vertex_count += 2;
            }
        }

//        vertices[vertex_count] = glm::vec3(
//                origin.x,
//                origin.y + dimensions.y,
//                origin.z + dimensions.z
//        );
//
//        vertices[vertex_count + 1] = origin + dimensions;
//
//        vertex_count += 2;
    }

    // push vertices to GPU
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(float) * vertices.size(), vertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
}