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

#include "nanogui/nanogui.h"

nanogui::Screen *screen;

using std::cout;
using std::endl;

ParticleSimulator *createSimulator(std::vector<glm::vec3> &positions, std::vector<glm::vec3> &velocities, Parameters &params);

void setWindowFPS(GLFWwindow *window, float fps);

void setNanoScreenCallbacksGLFW(GLFWwindow *window, nanogui::Screen *screen);

void cursorPosCallback(GLFWwindow *window, double x, double y);

void mouseButtonCallback(GLFWwindow *window, int button, int action, int modifiers);

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

void charCallback(GLFWwindow *window, unsigned int codepoint);

void dropCallback(GLFWwindow *window, int count, const char **filenames);

void scrollCallback(GLFWwindow *window, double x, double y);

void framebufferSizeCallback(GLFWwindow *window, int width, int height);

void createGUI(nanogui::Screen *screen, Parameters &params);

std::chrono::duration<double> second_accumulator;
unsigned int frames_last_second;

int main() {
    using namespace nanogui;

    GLFWwindow *window;

    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); //TessShader = 4 otherwise 3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); //TessShader = 0 otherwise 3
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //Open a window
    window = glfwCreateWindow(640, 480, "Totally fluids", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    //Generate rotator
    MouseRotator rotator;
    rotator.init(window);
    KeyTranslator trans;
    trans.init(window);

    //Set the GLFW-context the current window
    glfwMakeContextCurrent(window);
    cout << glGetString(GL_VERSION) << "\n";

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);

    // VSync: enable = 1, disable = 0
    glfwSwapInterval(0);

#ifdef _WIN32
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
        cout << "GLEW init error: " << glewGetErrorString(err) << "\n";
        return -1;
    }
#endif
    cout << "How many particles? ";
    int n_particles;
    std::cin >> n_particles;

    Parameters params(n_particles);
    Parameters::set_default_parameters(params);

    std::vector<glm::vec3> positions, velocities;
    ParticleSimulator *simulator = createSimulator(positions, velocities, params);

    screen = new Screen;
    screen->initialize(window, true);
    setNanoScreenCallbacksGLFW(window, screen);
    createGUI(screen, params);

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
    //ShaderProgram particlesShader("../shaders/particles.vert", "../shaders/particles.tessCont.glsl", "../shaders/particles.tessEval.glsl", "", "../shaders/particles.frag");
    ShaderProgram particlesShader("../shaders/particles.vert", "", "", "../shaders/particles.geom",
                                  "../shaders/particles.frag");
    particlesShader();

    //Parameters for tessellation shaders
    //glPatchParameteri(GL_PATCH_VERTICES, 1);  // tell OpenGL that every patch has 1 vertex


    //Declare uniform locations
    GLint MV_Loc, P_Loc, lDir_Loc, radius_Loc = -1;
    MV_Loc = glGetUniformLocation(particlesShader, "MV");
    P_Loc = glGetUniformLocation(particlesShader, "P");
    lDir_Loc = glGetUniformLocation(particlesShader, "lDir");
    radius_Loc = glGetUniformLocation(particlesShader, "radius");
    glm::mat4 MV, P;
    glm::vec3 lDir;
    glm::mat4 M = glm::mat4(1.0f);
    float radius = 0.1f;

    //Specify which pixels to draw to
    int width, height;

    // Calculate midpoint of scene
    const glm::vec3 scene_center(-(params.left_bound + params.right_bound) / 2,
                                 -(params.bottom_bound + params.top_bound) / 4,
                                 -(params.near_bound + params.far_bound) / 2);
    cout << glm::to_string(scene_center) << "\n";
    const float max_volume_side = params.get_max_volume_side();

    std::chrono::high_resolution_clock::time_point tp_last = std::chrono::high_resolution_clock::now();
    second_accumulator = std::chrono::duration<double>(0);
    frames_last_second = 0;

    // show the screen
    screen->setVisible(true);

    while (!glfwWindowShouldClose(window)) {
        std::chrono::high_resolution_clock::time_point tp_now = std::chrono::high_resolution_clock::now();
        std::chrono::high_resolution_clock::duration delta_time = tp_now - tp_last;
        tp_last = tp_now;

        std::chrono::milliseconds dt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(delta_time);

        float dt_s = 1e-3 * dt_ms.count();
#ifdef MY_DEBUG
        cout << "Seconds: " << dt_s << "\n";
#endif
        //dt_s = std::min(dt_s, 1.0f / 60);

        // Update window size
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        simulator->updateSimulation(params, dt_s);

        // Get mouse and key input
        rotator.poll(window);
        trans.poll(window);
        //printf("phi = %6.2f, theta = %6.2f\n", rotator.phi, rotator.theta);
        glm::mat4 VRotX = glm::rotate(M, rotator.phi, glm::vec3(0.0f, 1.0f, 0.0f)); //Rotation about y-axis
        glm::mat4 VRotY = glm::rotate(M, rotator.theta, glm::vec3(1.0f, 0.0f, 0.0f)); //Rotation about x-axis
        glm::mat4 VTrans = glm::translate(M, glm::vec3(trans.horizontal, 0.0f, trans.zoom));

        glm::vec4 eye_position = VRotX * VRotY * glm::vec4(0.0f, 0.0f, 3 * (max_volume_side + 0.5f), 1.0f);

        glm::mat4 V = VTrans * glm::lookAt(glm::vec3(eye_position), scene_center,
                                           glm::vec3(0.0f, 1.0f, 0.0f));
        P = glm::perspectiveFov(50.0f, static_cast<float>(width), static_cast<float>(height), 0.1f, 100.0f);
        MV = V * M;

        //Calculate light direction
        lDir = glm::vec3(1.0f);

        particlesShader();


        //Send uniform variables
        glUniformMatrix4fv(MV_Loc, 1, GL_FALSE, &MV[0][0]);
        glUniformMatrix4fv(P_Loc, 1, GL_FALSE, &P[0][0]);
        glUniform3fv(lDir_Loc, 1, &lDir[0]);
        glUniform1fv(radius_Loc, 1, &radius);


        // Clear the buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glCullFace(GL_BACK);
        glClearColor(params.bg_color.r,
                     params.bg_color.g,
                     params.bg_color.b,
                     1.0f);

        //Send VAO to the GPU
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, n_particles); //GeomShader
        //glDrawArrays(GL_PATCHES, 0, n_particles); //TessShader

        screen->drawWidgets();

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

void setNanoScreenCallbacksGLFW(GLFWwindow *window, nanogui::Screen *screen) {
    /* Propagate GLFW events to the appropriate Screen instance */
    glfwSetCursorPosCallback(window, &cursorPosCallback);
    glfwSetMouseButtonCallback(window, &mouseButtonCallback);
    glfwSetKeyCallback(window, &keyCallback);
    glfwSetCharCallback(window, &charCallback);
    glfwSetDropCallback(window, &dropCallback);
    glfwSetScrollCallback(window, &scrollCallback);

    /* React to framebuffer size events -- includes window
       size events and also catches things like dragging
       a window from a Retina-capable screen to a normal
       screen on Mac OS X */
    glfwSetFramebufferSizeCallback(window, &framebufferSizeCallback);
}

void cursorPosCallback(GLFWwindow *window, double x, double y) {
    screen->cursorPosCallbackEvent(x, y);
}

void mouseButtonCallback(GLFWwindow *window, int button, int action, int modifiers) {
    screen->mouseButtonCallbackEvent(button, action, modifiers);
}

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    screen->keyCallbackEvent(key, scancode, action, mods);
}

void charCallback(GLFWwindow *window, unsigned int codepoint) {
    screen->charCallbackEvent(codepoint);
}

void dropCallback(GLFWwindow *window, int count, const char **filenames) {
    screen->dropCallbackEvent(count, filenames);
}

void scrollCallback(GLFWwindow *window, double x, double y) {
    screen->scrollCallbackEvent(x, y);
}

void framebufferSizeCallback(GLFWwindow *window, int width, int height) {
    screen->resizeCallbackEvent(width, height);
}

void createGUI(nanogui::Screen *screen, Parameters &params) {
    using namespace nanogui;
    Parameters *p = &params;

    Window *window = new Window(screen, "Fluid parameters");
    window->setPosition(Vector2i(15, 15));
    window->setLayout(new GroupLayout());

    window = new Window(screen, "Background color");
    window->setPosition(Vector2i(425, 288));
    GridLayout *layout =
            new GridLayout(Orientation::Horizontal, 2,
                           Alignment::Middle, 15, 5);
    layout->setColAlignment(
            {Alignment::Maximum, Alignment::Fill});
    layout->setSpacing(0, 10);
    window->setLayout(layout);

    ColorWheel *colorwheel = new ColorWheel(window);

    colorwheel->setCallback([=](const Color &value) {
        p->bg_color.r = value.r();
        p->bg_color.g = value.g();
        p->bg_color.b = value.b();
    });

    screen->performLayout();
}

ParticleSimulator *createSimulator(std::vector<glm::vec3> &positions, std::vector<glm::vec3> &velocities,
                                   Parameters &params) {
    cout << "Use C++ [0] or OpenCL [1] for fluid simulation? ";
    int choice = -1;
    std::cin >> choice;

    if (choice == 0) {
        positions = generate_uniform_vec3s(params.n_particles, -1, -0.2, 0, 1, -1, 1);
        velocities = generate_uniform_vec3s(params.n_particles, 0, 0, 0, 0, 0, 0);

        return new CppParticleSimulator;
    } else if (choice == 1) {
        // Cylinder generation
        const float cylinder_radius = params.left_bound / 2;
        const glm::vec3 origin(-cylinder_radius * 0.75f, params.top_bound / 2, -cylinder_radius * 0.75f);
        const glm::vec3 size(cylinder_radius / 2, params.top_bound, cylinder_radius / 2);

        positions = generate_uniform_vec3s(params.n_particles,
                                           origin.x, origin.x + size.x,
                                           origin.y, origin.y + size.y,
                                           origin.z, origin.z + size.z);
        velocities = generate_uniform_vec3s(params.n_particles, 0, 0, 0, 0, 0, 0);

        return new OpenClParticleSimulator;
    }

    std::exit(EXIT_FAILURE);
}