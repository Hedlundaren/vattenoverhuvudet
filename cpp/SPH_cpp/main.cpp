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

void setWindowFPS(GLFWwindow *window, float fps);

std::chrono::duration<double> second_accumulator;
unsigned int frames_last_second;

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

    // VSync: enable = 1, disable = 0
    glfwSwapInterval(0);

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
    int n = -1;
    std::cout << "How many particles? ";
    std::cin >> n;
    const int n_particles = n;

    Parameters params(n_particles);
    Parameters::set_default_parameters(params);

    std::vector<glm::vec3> positions = generate_uniform_vec3s(n_particles,
                                                              params.left_bound, params.right_bound,
                                                              params.bottom_bound, params.top_bound,
                                                              params.near_bound, params.far_bound);
    std::vector<glm::vec3> velocities = generate_uniform_vec3s(n_particles, 0, 0, 0, 0, 0, 0);
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

    simulator->setupSimulation(params, positions, velocities, pos_vbo, vel_vbo);

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
    const glm::vec3 scene_center(-(params.left_bound + params.right_bound) / 2,
                                 -(params.bottom_bound + params.top_bound) / 4,
                                 -(params.near_bound + params.far_bound) / 2);
    std::cout << glm::to_string(scene_center) << "\n";
    const float max_volume_side = params.get_max_volume_side();

    std::chrono::high_resolution_clock::time_point tp_last = std::chrono::high_resolution_clock::now();
    second_accumulator = std::chrono::duration<double>(0);
    frames_last_second = 0;

    while (!glfwWindowShouldClose(window)) {
        std::chrono::high_resolution_clock::time_point tp_now = std::chrono::high_resolution_clock::now();
        std::chrono::high_resolution_clock::duration delta_time = tp_now - tp_last;
        tp_last = tp_now;

        std::chrono::milliseconds dt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(delta_time);

        const float dt_s = 1e-3 * dt_ms.count();
#ifdef MY_DEBUG
        std::cout << "Seconds: " << dt_s << "\n";
#endif

        // Update window size
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        simulator->updateSimulation(params, dt_s);

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
        ++frames_last_second;
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, 1);
        }

        /* FPS DISPLAY HANDLING */
        second_accumulator += delta_time;
        if (second_accumulator.count() >= 1.0) {
            float newFPS = static_cast<float>( frames_last_second / second_accumulator.count());
            setWindowFPS(window, newFPS);
            frames_last_second = 0;
            second_accumulator = std::chrono::duration<double>(0);
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

void setWindowFPS(GLFWwindow *window, float fps) {
    std::stringstream ss;
    ss << "FPS: " << fps;

    glfwSetWindowTitle(window, ss.str().c_str());
}