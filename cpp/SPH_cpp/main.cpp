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

//#define USE_TESS_SHADER
//You still need to change comments in vert- and frag-shaders

void setWindowFPS(GLFWwindow *window, float fps);

GLuint makeTextureBuffer(int w, int h, GLenum format, GLint internalFormat);

void generateFBOs();

std::chrono::duration<double> second_accumulator;
unsigned int frames_last_second;

static int WIDTH = 640;
static int HEIGHT = 480;
#define RESOLUTION 1

int main() {
    GLFWwindow *window;

    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); //TessShader = 4 otherwise 3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); //TessShader = 0 otherwise 3
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //Open a window
    window = glfwCreateWindow(WIDTH, HEIGHT, "Totally fluids", NULL, NULL);
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
    std::cout << glGetString(GL_VERSION) << "\n";

    //Set up OpenGL functions
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.0,0.1,0.2,1);

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

    /*-------------------------------Declare variables-------------------------------------*/
    //Declare uniform variables
    glm::mat4 MV, P;
    glm::vec3 lDir;
    glm::mat4 M = glm::mat4(1.0f);
    float radius = 0.1f;

    // Calculate midpoint of scene
    const glm::vec3 scene_center(-(params.left_bound + params.right_bound) / 2,
                                 -(params.bottom_bound + params.top_bound) / 4,
                                 -(params.near_bound + params.far_bound) / 2);
    std::cout << glm::to_string(scene_center) << "\n";
    const float max_volume_side = params.get_max_volume_side();

    std::chrono::high_resolution_clock::time_point tp_last = std::chrono::high_resolution_clock::now();
    second_accumulator = std::chrono::duration<double>(0);
    frames_last_second = 0;

    /*------------------------------------------------------------------------------------------*/

#ifdef USE_OPENCL_SIMULATION
    // Cylinder generation
    const float cylinder_radius = params.left_bound / 2;
    const glm::vec3 origin(- cylinder_radius * 0.75f, params.top_bound / 2, - cylinder_radius * 0.75f);
    const glm::vec3 size(cylinder_radius / 2, params.top_bound, cylinder_radius / 2);

    std::vector<glm::vec3> positions = generate_uniform_vec3s(n_particles,
                                                              origin.x, origin.x + size.x,
                                                              origin.y, origin.y + size.y,
                                                              origin.z, origin.z + size.z);
    std::vector<glm::vec3> velocities = generate_uniform_vec3s(n_particles, 0, 0, 0, 0, 0, 0);
#else
    std::vector<glm::vec3> positions = generate_uniform_vec3s(n_particles, -1, -0.2, 0, 1, -1, 1);
    std::vector<glm::vec3> velocities = generate_uniform_vec3s(n_particles, 0, 0, 0, 0, 0, 0);
#endif

    /*---------------------------Generate VBA & VAO---------------------------------*/
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

    /*----------------------------------------------------------------------------------------*/

    /*------------------------------Generate FBOs----------------------------------------------*/

    generateFBOs();

    /*----------------------------------------------------------------------------------------*/

    /*-----------------------------DECLARE SHADERS--------------------------------------------*/

    // Declare which shader to use and bind it
    #ifdef USE_TESS_SHADER
        ShaderProgram particlesShader("../shaders/particles.vert", "../shaders/particles.tessCont.glsl", "../shaders/particles.tessEval.glsl", "", "../shaders/particles.frag");
        glPatchParameteri(GL_PATCH_VERTICES, 1);  // tell OpenGL that every patch has 1 vertex
        //PARTICLE_DEPTH SHADER
        particlesShader.MV_Loc = glGetUniformLocation(particlesShader, "MV");
        particlesShader.P_Loc = glGetUniformLocation(particlesShader, "P");
        particlesShader.lDir_Loc = glGetUniformLocation(particlesShader, "lDir");
        particlesShader.radius_Loc = glGetUniformLocation(particlesShader, "radius");
        particlesShader.camPos_Loc = glGetUniformLocation(particlesShader, "camPos");
    #else //Triagles
        //ShaderProgram particlesShader("../shaders/particles.vert", "", "","../shaders/particles.geom", "../shaders/particles.frag");
    #endif

    ShaderProgram backgroundShader("../shaders/simple.vert", "", "", "", "../shaders/simple.frag");
    ShaderProgram particleDepthShader("../shaders/particles.vert", "", "", "", "../shaders/particledepth.frag");
    ShaderProgram particleThicknessShader("../shaders/particles.vert", "", "", "", "../shaders/particlethickness.frag");
    ShaderProgram particleVelocityShader("../shaders/particles.vert", "", "", "", "../shaders/particlevelocity.frag");
    ShaderProgram curvatureFlowShader("../shaders/quad.vert", "", "", "", "../shaders/curvatureflow.frag");
    ShaderProgram liquidShadeShader("../shaders/quad.vert", "", "", "", "../shaders/liquidshade.frag");
    ShaderProgram compositionShader("../shaders/quad.vert", "", "", "", "../shaders/compose.frag");

    /*------------------------------------------------------------------------------------*/

    /*---------------------------Declare uniform locations-----------------------------*/

    //BACKGROUND SHADER
    backgroundShader.MVP_Loc = glGetUniformLocation(backgroundShader, "MVP");
    backgroundShader.lDir_Loc = glGetUniformLocation(backgroundShader, "lDir");
    // Bind output variables
    glBindFragDataLocation(backgroundShader, 0, "outColor");

    //PARTICLE_DEPTH SHADER
    particleDepthShader.MV_Loc = glGetUniformLocation(particleDepthShader, "MV");
    particleDepthShader.P_Loc = glGetUniformLocation(particleDepthShader, "P");
    particleDepthShader.lDir_Loc = glGetUniformLocation(particleDepthShader, "lDir");
    particleDepthShader.radius_Loc = glGetUniformLocation(particleDepthShader, "radius");
    particleDepthShader.camPos_Loc = glGetUniformLocation(particleDepthShader, "camPos");
    // Bind output variables
    glBindFragDataLocation(particleDepthShader, 0, "particleDepth");

    //PARTICLE_THICKNESS SHADER
    particleThicknessShader.MVP_Loc = glGetUniformLocation(particleThicknessShader, "MVP");
    particleThicknessShader.lDir_Loc = glGetUniformLocation(particleThicknessShader, "lDir");
    // Bind output variables
    glBindFragDataLocation(particleThicknessShader, 0, "particleThickness");

    //PARTICLE_VELOCITY SHADER
    particleVelocityShader.MVP_Loc = glGetUniformLocation(particleVelocityShader, "MVP");
    particleVelocityShader.lDir_Loc = glGetUniformLocation(particleVelocityShader, "lDir");
    // Bind output variables
    glBindFragDataLocation(particleVelocityShader, 0, "velocityMap");

    //CURVATURE_FLOW SHADER
    curvatureFlowShader.MVP_Loc = glGetUniformLocation(curvatureFlowShader, "MVP");
    curvatureFlowShader.lDir_Loc = glGetUniformLocation(curvatureFlowShader, "lDir");
    // Bind output variables
    glBindFragDataLocation(curvatureFlowShader, 0, "outDepth");


    //LIQUID_SHADE SHADER
    liquidShadeShader.MVP_Loc = glGetUniformLocation(liquidShadeShader, "MVP");
    liquidShadeShader.lDir_Loc = glGetUniformLocation(liquidShadeShader, "lDir");
    // Bind output variables
    glBindFragDataLocation(liquidShadeShader, 0, "outColor");

    //COMPOSITION SHADER
    compositionShader.MVP_Loc = glGetUniformLocation(compositionShader, "MVP");
    compositionShader.lDir_Loc = glGetUniformLocation(compositionShader, "lDir");
    // Bind output variables
    glBindFragDataLocation(compositionShader, 0, "outColor");

    /*---------------------------------------------------------------------------------------*/


    /*-----------------------------------RENDER LOOP-------------------------------------------*/

    while (!glfwWindowShouldClose(window)) {

        /*------------------Update clock and FPS---------------------------------------------*/
        std::chrono::high_resolution_clock::time_point tp_now = std::chrono::high_resolution_clock::now();
        std::chrono::high_resolution_clock::duration delta_time = tp_now - tp_last;
        tp_last = tp_now;

        std::chrono::milliseconds dt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(delta_time);

        float dt_s = 1e-3 * dt_ms.count();
        #ifdef MY_DEBUG
                std::cout << "Seconds: " << dt_s << "\n";
        #endif
        //dt_s = std::min(dt_s, 1.0f / 60);
        /*----------------------------------------------------------------------------------------*/

        // Update window size
        glfwGetFramebufferSize(window, &WIDTH, &HEIGHT);

        //Set viewport to whole window
        glViewport(0, 0, WIDTH, HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //Use particle shader
        particleDepthShader();

        //Update positions
        simulator->updateSimulation(params, dt_s);

        /*----------------------------PARTICLE SHADER---------------------------------------------*/
        // Get mouse and key input
        rotator.poll(window);
        trans.poll(window);
        //printf("phi = %6.2f, theta = %6.2f\n", rotator.phi, rotator.theta);
        glm::mat4 VRotX = glm::rotate(M, rotator.phi, glm::vec3(0.0f, 1.0f, 0.0f)); //Rotation about y-axis
        glm::mat4 VRotY = glm::rotate(M, rotator.theta, glm::vec3(1.0f, 0.0f, 0.0f)); //Rotation about x-axis
        glm::mat4 VTrans = glm::translate(M, glm::vec3(trans.horizontal, 0.0f, trans.zoom));

        glm::vec4 eye_position = VRotX * VRotY * glm::vec4(0.0f, 0.0f, 3 * (max_volume_side + 0.5f), 1.0f);
        glm::vec4 camPos = eye_position;
        glm::mat4 V = VTrans * glm::lookAt(glm::vec3(eye_position), scene_center,
                                  glm::vec3(0.0f, 1.0f, 0.0f));
        P = glm::perspectiveFov(50.0f, static_cast<float>(WIDTH), static_cast<float>(HEIGHT), 0.1f, 100.0f);
        MV = V * M;

        //Calculate light direction
        lDir = glm::vec3(1.0f);

        //Send uniform variables
        glUniformMatrix4fv(particleDepthShader.MV_Loc, 1, GL_FALSE, &MV[0][0]);
        glUniformMatrix4fv(particleDepthShader.P_Loc, 1, GL_FALSE, &P[0][0]);
        glUniform3fv(particleDepthShader.lDir_Loc, 1, &lDir[0]);
        glUniform4fv(particleDepthShader.camPos_Loc, 1, &camPos[0]);
        glUniform1fv(particleDepthShader.radius_Loc, 1, &radius);

        // Clear the buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glCullFace(GL_BACK);

        //Send VAO to the GPU
        glBindVertexArray(vao);
        #ifdef USE_TESS_SHADER
           glDrawArrays(GL_PATCHES, 0, n_particles); //TessShader
        #else
            glDrawArrays(GL_POINTS, 0, n_particles); //GeomShader
        #endif

        /*--------------------------------------------------------------------------------------------*/

        glfwSwapBuffers(window);
        ++frames_last_second;
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, 1);
        }

        /*---------------------- FPS DISPLAY HANDLING ------------------------------------*/
        second_accumulator += delta_time;
        if (second_accumulator.count() >= 1.0) {
            float newFPS = static_cast<float>( frames_last_second / second_accumulator.count());
            setWindowFPS(window, newFPS);
            frames_last_second = 0;
            second_accumulator = std::chrono::duration<double>(0);
        }
        /*--------------------------------------------------------------------------------*/
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


GLuint makeTextureBuffer(int w, int h, GLenum format, GLint internalFormat) {
    GLuint buffertex;

    glGenTextures(1, &buffertex);
    glBindTexture(GL_TEXTURE_2D, buffertex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, format, GL_FLOAT, NULL);

    return buffertex;
}

void generateFBOs(){

    // ---------Background FBO-----------
    // Depth buffer
    GLuint depthBuffer;
    glGenRenderbuffers(1, &depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, WIDTH, HEIGHT);

    // FBO for background
    GLuint backgroundFBO;
    glGenFramebuffers(1, &backgroundFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, backgroundFBO);
    GLuint backgroundTexture = makeTextureBuffer(WIDTH, HEIGHT, GL_RGBA, GL_RGBA32F);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, backgroundTexture, 0);

    // Attach depth
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
    // ----------------------------------------

    // Depth buffer, low-res.
    GLuint depthBufferLowres;
    glGenRenderbuffers(1, &depthBufferLowres);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBufferLowres);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, WIDTH/RESOLUTION, HEIGHT/RESOLUTION);

    //-------------------DepthFBO
    GLuint particleFBO[2];
    GLuint particleTexture[2];
    for(int i = 0; i < 2; i++) {
        glGenFramebuffers(1, &particleFBO[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, particleFBO[i]);
        particleTexture[i] = makeTextureBuffer(WIDTH / RESOLUTION, HEIGHT / RESOLUTION, GL_RED, GL_R32F);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, particleTexture[i], 0);
        // Attach depth
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufferLowres);
    }

    //-----------------FBOs for particle velocity
    GLuint velocityFBO;
    glGenFramebuffers(1, &velocityFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, velocityFBO);
    GLuint particleVelocityTexture = makeTextureBuffer(WIDTH / RESOLUTION, HEIGHT / RESOLUTION, GL_RED, GL_R32F);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, particleVelocityTexture, 0);
    // Attach depth
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufferLowres);


    //---------------------FBOs for particle thickness
    GLuint particleThicknessFBO[2];
    GLuint particleThicknessTexture[2];
    for(int i = 0; i < 2; i++) {
        glGenFramebuffers(1, &particleThicknessFBO[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, particleThicknessFBO[i]);
        particleThicknessTexture[i] = makeTextureBuffer(WIDTH / RESOLUTION, HEIGHT / RESOLUTION, GL_RED, GL_R32F);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, particleThicknessTexture[i], 0);
        // Attach depth
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufferLowres);
    }

    //Unbind framebuffers
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //-----------------------Final color FBO
    GLuint particleColorFBO;
    glGenFramebuffers(1, &particleColorFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, particleColorFBO);
    GLuint particleColorTexture = makeTextureBuffer(WIDTH / RESOLUTION, HEIGHT / RESOLUTION, GL_RGBA, GL_RGBA32F);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, particleColorTexture, 0);
    // Attach depth
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufferLowres);
}