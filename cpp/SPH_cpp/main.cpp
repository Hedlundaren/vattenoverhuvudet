#include <iostream>
#include <chrono>

#include <glm/glm.hpp>

#include <GLFW/glfw3.h>

#include "opencl_context_info.hpp"
#include "rendering/ShaderProgram.hpp"
#include "math/randomized.hpp"
#include "common/Rotator.hpp"
#include "common/MatrixStack.hpp"

#include "ParticleSimulator.hpp"
#include "CppParticleSimulator.hpp"
#include "OpenClParticleSimulator.hpp"

int main() {
    PrintOpenClContextInfo();

    GLFWwindow *window;

    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(640, 480, "Simple example", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    //Generate rotator!!!!! :D
    MouseRotator rotator;
    rotator.init(window);

    //Initialize
    MatrixStack MVstack;
    MVstack.init();

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    ParticleSimulator *simulator;

#ifdef USE_OPENCL_SIMULATION
    simulator = new OpenClParticleSimulator();
#else
    simulator = new CppParticleSimulator();
#endif

    //Generate particles
    const int n_particles = 5000;
    std::vector<glm::vec3> positions = generate_uniform_vec3s(n_particles, -1, 1, -1, 1, -1, 1);
    std::vector<glm::vec3> velocities = generate_uniform_vec3s(n_particles, -1, 1, -1, 1, -1, 1);

    //Generate VBOs
    GLuint pos_vbo = 0;
    glGenBuffers(1, &pos_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, pos_vbo);
    glBufferData(GL_ARRAY_BUFFER, n_particles * 3 * sizeof(float), positions.data(), GL_STATIC_DRAW);

    GLuint vel_vbo = 0;
    glGenBuffers(1, &vel_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vel_vbo);
    glBufferData(GL_ARRAY_BUFFER, n_particles * 3 * sizeof(float), velocities.data(), GL_STATIC_DRAW);

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


    ShaderProgram particlesShader("../shaders/particles.vert", "../shaders/particles.frag");
    particlesShader();

    std::chrono::high_resolution_clock::time_point tp_last = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(window)) {
        std::chrono::high_resolution_clock::time_point tp_now = std::chrono::high_resolution_clock::now();
        std::chrono::high_resolution_clock::duration delta_time = tp_now - tp_last;
        tp_last = tp_now;

        std::chrono::milliseconds dt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(delta_time);

        std::cout << "Seconds: " << 1e-3 * dt_ms.count() << "\n";
        const float dt_s = 1e-3 * dt_ms.count();

        simulator->updateSimulation(dt_s);

        float ratio;
        int width, height;

        glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float) height;
        glViewport(0, 0, width, height);

        // Get mouse handle (input)
        rotator.poll(window);
        //printf("phi = %6.2f, theta = %6.2f\n", rotator.phi, rotator.theta);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        MVstack.push();
        MVstack.rotX(rotator.theta);
        MVstack.rotY(rotator.phi);

        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, n_particles);

        MVstack.pop();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, 1);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
