#include <iostream>
#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifdef _WIN32
#include "GL/glew.h"
#endif

#include "GLFW/glfw3.h"

#include "rendering/ShaderProgram.hpp"
#include "math/randomized.hpp"
#include "common/Rotator.hpp"
#include "constants.hpp"

#include "ParticleSimulator.hpp"
#include "OpenCL/OpenClParticleSimulator.hpp"
#include "CppParticleSimulator.hpp"

int main() {
    GLFWwindow *window;

    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    //Open a window
    window = glfwCreateWindow(640, 480, "Totally fluids", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_GREATER);

    //Generate rotator
    MouseRotator rotator;
    rotator.init(window);

    //Set the GLFW-context the current window
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

#ifdef _WIN32
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
        std::cout << "GLEW init error: " << glewGetErrorString(err) << "\n";
        return -1;
    }
#endif


    ParticleSimulator *simulator;

#ifdef USE_OPENCL_SIMULATION
    simulator = new OpenClParticleSimulator();
#else
    simulator = new CppParticleSimulator();
#endif


    //Generate particles
    const int n_particles = 192;
    std::vector<glm::vec3> positions = generate_uniform_vec3s(n_particles, -1, 1, -1, 1, -1, 1);
    std::vector<glm::vec3> velocities = generate_uniform_vec3s(n_particles, -1, 1, -1, 1, -1, 1);
    //std::vector<glm::vec3> positions = generate_linear_vec3s(n_particles, -1, 1, -1, 1, -1, 1);
    //std::vector<glm::vec3> velocities = generate_linear_vec3s(n_particles, -1, 1, -1, 1, -1, 1);

    //Generate VBOs
    GLuint pos_vbo = 0;
    glGenBuffers(1, &pos_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, pos_vbo);
    glBufferData(GL_ARRAY_BUFFER, n_particles * 3 * sizeof(float), positions.data(), GL_DYNAMIC_DRAW);

    GLuint vel_vbo = 0;
    glGenBuffers(1, &vel_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vel_vbo);
    glBufferData(GL_ARRAY_BUFFER, n_particles * 3 * sizeof(float), velocities.data(), GL_DYNAMIC_DRAW);

    simulator->setupSimulation(positions, velocities, pos_vbo, vel_vbo);

    // Generate VAO with all VBOs
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, pos_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL); //position
    glBindBuffer(GL_ARRAY_BUFFER, vel_vbo);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL); //velocity

    //How many attributes do we have? Enable them!
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);


    // Declare which shader to use and bind it
    ShaderProgram particlesShader("../shaders/particles.vert", "../shaders/particles.frag",
                                  "../shaders/particles.geom");
    particlesShader();


    //Declare uniform locations
    GLint MVP_Loc = -1;
    MVP_Loc = glGetUniformLocation(particlesShader, "MVP");
    glm::mat4 MVP;
    glm::mat4 M = glm::mat4(1.0f);

    //Specify which pixels to draw to
    int width, height;

    // Calculate midpoint of scene
    const glm::vec3 scene_center(-(Parameters::leftBound + Parameters::rightBound) / 2,
                                 (Parameters::bottomBound + Parameters::topBound) / 2,
                                 - (Parameters::nearBound + Parameters::farBound) / 2);
    std::cout << glm::to_string(scene_center) << "\n";
    const float max_volume_side = Parameters::get_max_volume_side();

    std::chrono::high_resolution_clock::time_point tp_last = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(window)) {
        std::chrono::high_resolution_clock::time_point tp_now = std::chrono::high_resolution_clock::now();
        std::chrono::high_resolution_clock::duration delta_time = tp_now - tp_last;
        tp_last = tp_now;

        std::chrono::milliseconds dt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(delta_time);

        const float dt_s = 1e-3 * dt_ms.count();
        std::cout << "Seconds: " << dt_s << "\n";

        // Update window size
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        simulator->updateSimulation(dt_s);

        // Get rotation input
        rotator.poll(window);
        //printf("phi = %6.2f, theta = %6.2f\n", rotator.phi, rotator.theta);
        glm::mat4 VRotX = glm::rotate(M, rotator.phi, glm::vec3(0.0f, 1.0f, 0.0f)); //Rotation about y-axis
        glm::mat4 VRotY = glm::rotate(M, rotator.theta, glm::vec3(1.0f, 0.0f, 0.0f)); //Rotation about x-axis

        glm::vec4 eye_position = VRotX * VRotY * glm::vec4(0.0f, 0.0f, 3 * (max_volume_side + 0.5f), 1.0f);

        glm::mat4 V = glm::lookAt(glm::vec3(eye_position), scene_center,
                                  glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 P = glm::perspectiveFov(50.0f, static_cast<float>(width), static_cast<float>(height), 0.1f, 100.0f);
        MVP = P * V * M;
        glUniformMatrix4fv(MVP_Loc, 1, GL_FALSE, &MVP[0][0]);

        // Clear the buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glCullFace(GL_BACK);


        //Send VAO to the GPU
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, n_particles);

        glfwSwapBuffers(window);
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, 1);
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
