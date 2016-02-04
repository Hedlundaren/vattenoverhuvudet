#include <iostream>
#include <chrono>

#include <glm/glm.hpp>

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "opencl_context_info.hpp"
#include "rendering/ShaderProgram.hpp"
#include "math/randomized.hpp"
#include "common/Rotator.hpp"

#include "ParticleSimulator.hpp"
#include "CppParticleSimulator.hpp"
#include "OpenClParticleSimulator.hpp"

#include "common/tic_toc.hpp"

int main() {
    PrintOpenClContextInfo();

    tic();
    GLFWwindow *window;

    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    //Open a window
    window = glfwCreateWindow(640, 480, "Looks like fluid right!?", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    //Generate rotator
    MouseRotator rotator;
    rotator.init(window);


    //Set the GLFW-context the current window
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
    std::vector <glm::vec3> positions = generate_uniform_vec3s(n_particles, -1, 1, -1, 1, -1, 1);
    std::vector <glm::vec3> velocities = generate_uniform_vec3s(n_particles, -1, 1, -1, 1, -1, 1);

    //Generate VBOs
    GLuint pos_vbo = 0;
    glGenBuffers (1, &pos_vbo);
    glBindBuffer (GL_ARRAY_BUFFER, pos_vbo);
    glBufferData (GL_ARRAY_BUFFER, n_particles * 3 * sizeof (float), positions.data(), GL_STATIC_DRAW);

    GLuint vel_vbo = 0;
    glGenBuffers (1, &vel_vbo);
    glBindBuffer (GL_ARRAY_BUFFER, vel_vbo);
    glBufferData (GL_ARRAY_BUFFER, n_particles * 3 * sizeof (float), velocities.data(), GL_STATIC_DRAW);
    
    simulator->setupSimulation(positions, velocities, pos_vbo, vel_vbo);

    // Generate VAO with all VBOs
    GLuint vao = 0;
    glGenVertexArrays (1, &vao);
    glBindVertexArray (vao);
    glBindBuffer (GL_ARRAY_BUFFER, pos_vbo);
    glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL); //position
    glBindBuffer (GL_ARRAY_BUFFER, vel_vbo);
    glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 0, NULL); //velocity

    //How many attributes do we have? Enable them!
    glEnableVertexAttribArray (0);
    glEnableVertexAttribArray (1);


    // Declare which shader to use and bind it
    ShaderProgram particlesShader("../shaders/particles.vert", "../shaders/particles.frag");
    particlesShader();


    //Declare uniform locations
    GLint MVP_Loc = -1;
    MVP_Loc = glGetUniformLocation(particlesShader, "MVP");
    glm::mat4 MVP;
    glm::mat4 M = glm::mat4(1.0f);

    //Specify which pixels to draw to
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);


    std::chrono::high_resolution_clock::time_point tp_last = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(window)) {
        std::chrono::high_resolution_clock::time_point tp_now = std::chrono::high_resolution_clock::now();
        std::chrono::high_resolution_clock::duration delta_time = tp_now - tp_last;
        tp_last = tp_now;

        std::chrono::milliseconds dt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(delta_time);

        //std::cout << "Seconds: " << 1e-3 * dt_ms.count() << "\n";
        const float dt_s = 1e-3 * dt_ms.count();


        simulator->updateSimulation(dt_s);


        // Get rotation input
        rotator.poll(window);
        //printf("phi = %6.2f, theta = %6.2f\n", rotator.phi, rotator.theta);
        glm::mat4 VRotX = glm::rotate(M, rotator.phi, glm::vec3(0.0f, 1.0f, 0.0f)); //Rotation around y-axis
        glm::mat4 VRotY = glm::rotate(M, rotator.theta, glm::vec3(1.0f, 0.0f, 0.0f)); //Rotation around x-axis
        glm::mat4 V = VRotX*VRotY*glm::lookAt(glm::vec3(0.0f,0.0f,1.0f),glm::vec3(0.0f,0.0f,0.0f),glm::vec3(0.0f,1.0f,0.0f));
        glm::mat4 P = glm::perspectiveFov(50.0f, 640.0f, 480.0f, 0.1f, 100.0f);
        MVP = P*V*M;
        glUniformMatrix4fv(MVP_Loc, 1, GL_FALSE, &MVP[0][0]);



        // Clear the buffers
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glCullFace(GL_BACK);


        //Send VAO to the GPU
        glBindVertexArray (vao);
        glDrawArrays (GL_POINTS, 0, n_particles);


        glfwSwapBuffers(window);
        glfwPollEvents();

        if (glfwGetKey (window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose (window, 1);
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    toc();
    exit(EXIT_SUCCESS);
}
