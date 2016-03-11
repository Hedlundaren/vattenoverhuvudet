#include <iostream>
#include <chrono>
#include <vector>
#include <iomanip>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifdef _WIN32
#include "GL/glew.h"
#endif

#include "GLFW/glfw3.h"

#include "rendering/VoxelGrid.hpp"
#include "common/tic_toc.hpp"
#include "common/GLerror.hpp"

#include "rendering/ShaderProgram.hpp"
#include "rendering/texture.hpp"
#include "math/randomized.hpp"
#include "common/Rotator.hpp"

#include "ParticleSimulator.hpp"
#include "OpenCL/OpenClParticleSimulator.hpp"
#include "CppParticleSimulator.hpp"

//#define USE_TESS_SHADER
//You still need to change comments in vert- and frag-shaders
#include "nanogui/nanogui.h"

#include "HeightMap.hpp"
#include "rendering/VoxelGrid.hpp"

nanogui::Screen *screen;

using std::cout;
using std::endl;

ParticleSimulator *createSimulator(std::vector<glm::vec3> &positions, std::vector<glm::vec3> &velocities,
                                   Parameters &params);

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
nanogui::TextBox *fpsBox;
bool hmap_wireframe = false;
bool render_particle_grid = false;

static int WIDTH = 720;
static int HEIGHT = 720;
#define RESOLUTION 1
#define SMOOTHING_ITERATIONS 120

// A terrain
struct {
    float* heightData;
    GLuint heightTexture;

    GLuint envTexture;
    GLuint lowTexture;
    GLuint highTexture;
} terrain;

int main() {
    using namespace nanogui;

    //HeightMap::SetMaxVoxelSamplerSize(32, 4, 32);
    HeightMap::SetMaxVoxelSamplerSize(8, 2, 8);

    // normalized "kernel radius" for precomputed density-LUT
    const float density_contribution_radius = 0.1f;
    std::string nmap_name;
    cout << "Heightmap name: ";
    std::cin >> nmap_name;

    HeightMap hmap;
    if (!hmap.initFromPNGs(nmap_name)) {
        return 0;
    }

    //hmap.debug_print(50);
    tic();
    hmap.calcVoxelSamplers([](const std::vector<float> &distances, const float max_radius) {
        float sum = 0.0f;
        for (const auto distance : distances) {
            sum += (max_radius - distance);
        }
        return sum;
    }, density_contribution_radius);
    toc();

    GLFWwindow *window;

    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); //TessShader = 4 otherwise 3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); //TessShader = 0 otherwise 3
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);


    /* Request a RGBA8 buffer without MSAA */
    glfwWindowHint(GLFW_SAMPLES, 16);
    glfwWindowHint(GLFW_RED_BITS, 8);
    glfwWindowHint(GLFW_GREEN_BITS, 8);
    glfwWindowHint(GLFW_BLUE_BITS, 8);
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);

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
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

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

    hmap.initGL(glm::vec3(params.left_bound, params.bottom_bound, params.near_bound),
                glm::vec3(params.right_bound - params.left_bound,
                          0.25f * (params.top_bound - params.bottom_bound),
                          params.far_bound - params.near_bound));

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

    simulator->setupSimulation(params, positions, velocities, pos_vbo, vel_vbo,
                               hmap.get_density_voxel_sampler(),
                               hmap.get_normal_voxel_sampler(),
                               hmap.get_voxel_sampler_size());

    // Initialize voxel grid wireframe rendering
    VoxelGrid voxelGrid;
    voxelGrid.initGL(glm::vec3(params.left_bound, params.bottom_bound, params.near_bound),
                     glm::vec3(params.right_bound - params.left_bound,
                               params.top_bound - params.bottom_bound,
                               params.far_bound - params.near_bound));

    // Generate particle VAO with 2 VBOs
    GLuint part_vao = 0;
    glGenVertexArrays(1, &part_vao);
    glBindVertexArray(part_vao);
    glBindBuffer(GL_ARRAY_BUFFER, pos_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL); //position
    glBindBuffer(GL_ARRAY_BUFFER, vel_vbo);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL); //velocity

    //How many attributes do we have? Enable them!
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    // Prepare a screen quad to render postprocessed things.
    glm::vec3 quadData[] = {
            glm::vec3(-1.0f, -1.0f, 0.0f),
            glm::vec3( 1.0f, -1.0f, 0.0f),
            glm::vec3( 1.0f,  1.0f, 0.0f),
            glm::vec3(-1.0f,  1.0f, 0.0f)};
    GLuint quadElements[] = {0, 1, 3, 1, 2, 3}; //Order to write

    GLuint quadVBO = makeBO(GL_ARRAY_BUFFER, quadData, sizeof(glm::vec3)*4, GL_STATIC_DRAW);
    GLuint quadElemVBO = makeBO(GL_ELEMENT_ARRAY_BUFFER, quadElements, sizeof(GLuint)*6, GL_STATIC_DRAW);

    // Generate quad VAO with 2 VBOs
    GLuint quad_vao = 0;
    glGenVertexArrays(1, &quad_vao);
    glBindVertexArray(quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadElemVBO);

    //Unbind VAOs
    glBindVertexArray(0);

    /*----------------------------------------------------------------------------------------*/

    //Load textures
    terrain.envTexture = loadTexture("../textures/skymap_b.tga");
    //terrain.lowTexture = loadTexture("../textures/sand.tga");
    //terrain.highTexture = loadTexture("../textures/stone.tga");
    // Load terrain
    //terrain.heightData = loadPGM("../textures/grand_canyon.pgm", 4096, 4096);

    // Make terrain texture
    //terrain.heightTexture = genFloatTexture(terrain.heightData, 4096, 4096);

    /*------------------------------Generate FBOs----------------------------------------------*/

    //generateFBOs();
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
    GLuint particleThicknessFBO;
    GLuint particleThicknessTexture;
    //for(int i = 0; i < 2; i++) {
        glGenFramebuffers(1, &particleThicknessFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, particleThicknessFBO);
        particleThicknessTexture = makeTextureBuffer(WIDTH / RESOLUTION, HEIGHT / RESOLUTION, GL_RED, GL_R32F);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, particleThicknessTexture, 0);
        // Attach depth
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufferLowres);
    //}

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

    glBindRenderbuffer(GL_RENDERBUFFER, 0);
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

    //ShaderProgram backgroundShader("../shaders/simple.vert", "", "", "", "../shaders/simple.frag");
    ShaderProgram particleDepthShader("../shaders/particles.vert", "", "", "", "../shaders/particledepth.frag");
    ShaderProgram particleThicknessShader("../shaders/particles.vert", "", "", "", "../shaders/particlethickness.frag");
    ShaderProgram particleVelocityShader("../shaders/particles.vert", "", "", "", "../shaders/particlevelocity.frag");
    ShaderProgram curvatureFlowShader("../shaders/quad.vert", "", "", "", "../shaders/curvatureflow.frag");
    ShaderProgram liquidShadeShader("../shaders/quad.vert", "", "", "", "../shaders/liquidshade.frag");
    ShaderProgram compositionShader("../shaders/quad.vert", "", "", "", "../shaders/compose.frag");

    /*------------------------------------------------------------------------------------*/

    /*---------------------------Declare uniform locations-----------------------------*/
    /*
    //BACKGROUND SHADER
    backgroundShader.MV_Loc = glGetUniformLocation(backgroundShader, "MV");
    backgroundShader.P_Loc = glGetUniformLocation(backgroundShader, "P");
    backgroundShader.lDir_Loc = glGetUniformLocation(backgroundShader, "lDir");
    backgroundShader.terrainTex = glGetUniformLocation(backgroundShader, "terrainTexture");
    backgroundShader.lowTex = glGetUniformLocation(backgroundShader, "lowTexture");
    backgroundShader.highTex = glGetUniformLocation(backgroundShader, "highTexture");
    // Bind output variables
    glBindFragDataLocation(backgroundShader, 0, "outColor");
    */

    //PARTICLE_DEPTH SHADER
    particleDepthShader.MV_Loc = glGetUniformLocation(particleDepthShader, "MV");
    particleDepthShader.P_Loc = glGetUniformLocation(particleDepthShader, "P");
    particleDepthShader.screenSize_Loc = glGetUniformLocation(particleDepthShader, "screenSize");
    //particleDepthShader.terrainTex = glGetUniformLocation(particleDepthShader, "terrainTexture");
    particleDepthShader.camPos_Loc = glGetUniformLocation(particleDepthShader, "camPos");
    // Bind output variables
    glBindFragDataLocation(particleDepthShader, 0, "particleDepth");


    //PARTICLE_THICKNESS SHADER
    particleThicknessShader.MV_Loc = glGetUniformLocation(particleThicknessShader, "MV");
    particleThicknessShader.P_Loc = glGetUniformLocation(particleThicknessShader, "P");
    particleThicknessShader.screenSize_Loc = glGetUniformLocation(particleThicknessShader, "screenSize");
    //particleThicknessShader.terrainTex = glGetUniformLocation(particleThicknessShader, "terrainTexture");
    // Bind output variables
    glBindFragDataLocation(particleThicknessShader, 0, "particleThickness");

    //PARTICLE_VELOCITY SHADER
    particleVelocityShader.MV_Loc = glGetUniformLocation(particleVelocityShader, "MV");
    particleVelocityShader.P_Loc = glGetUniformLocation(particleVelocityShader, "P");
    particleVelocityShader.screenSize_Loc = glGetUniformLocation(particleVelocityShader, "screenSize");
    // Bind output variables
    glBindFragDataLocation(particleVelocityShader, 0, "velocityMap");

    //CURVATURE_FLOW SHADER
    curvatureFlowShader.P_Loc = glGetUniformLocation(curvatureFlowShader, "P");
    curvatureFlowShader.screenSize_Loc = glGetUniformLocation(curvatureFlowShader, "screenSize");
    curvatureFlowShader.particleTex = glGetUniformLocation(curvatureFlowShader, "particleTexture");
    // Bind output variables
    glBindFragDataLocation(curvatureFlowShader, 0, "outDepth");


    //LIQUID_SHADE SHADER
    liquidShadeShader.MV_Loc = glGetUniformLocation(liquidShadeShader, "MV");
    liquidShadeShader.P_Loc = glGetUniformLocation(liquidShadeShader, "P");
    liquidShadeShader.lDir_Loc = glGetUniformLocation(liquidShadeShader, "lDir");
    liquidShadeShader.screenSize_Loc = glGetUniformLocation(liquidShadeShader, "screenSize");
    liquidShadeShader.environmentTex = glGetUniformLocation(liquidShadeShader, "environmentTexture");
    liquidShadeShader.particleTex = glGetUniformLocation(liquidShadeShader, "particleTexture");
    liquidShadeShader.particleThicknessTex = glGetUniformLocation(liquidShadeShader, "particleThicknessTexture");
    liquidShadeShader.velocityTex = glGetUniformLocation(liquidShadeShader, "velocityTexture");
    // Bind output variables
    glBindFragDataLocation(liquidShadeShader, 0, "outColor");

    //COMPOSITION SHADER
    compositionShader.MV_Loc = glGetUniformLocation(compositionShader, "MV");
    compositionShader.P_Loc = glGetUniformLocation(compositionShader, "P");
    compositionShader.backgroundTex = glGetUniformLocation(compositionShader, "backgroundTexture");
    compositionShader.particleTex = glGetUniformLocation(compositionShader, "particleTexture");
    //compositionShader.terrainTex = glGetUniformLocation(compositionShader, "terrainTexture");
    // Bind output variables
    glBindFragDataLocation(compositionShader, 0, "outColor");

    /*---------------------------------------------------------------------------------------*/


    /*-----------------------------------RENDER LOOP-------------------------------------------*/

    // show the screen
    screen->setVisible(true);

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

        int w, h;
        // Update window size
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);

        //Update positions
        simulator->updateSimulation(params, dt_s);

        /*--------------------------------RENDERING---------------------------------------------*/
        //------------BACKGROUND SHADER
        /*
        //Set viewport to whole window
        glViewport(0, 0, WIDTH, HEIGHT);
        // Clear everything first thing.
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);


        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //Activate shader
        backgroundShader();
        */

        //Calculate matrices and textures
        // Get mouse and key input
        rotator.poll(window);
        trans.poll(window);
        //printf("phi = %6.2f, theta = %6.2f\n", rotator.phi, rotator.theta);
        glm::mat4 VRotX = glm::rotate(M, rotator.phi, glm::vec3(0.0f, 1.0f, 0.0f)); //Rotation about y-axis
        glm::mat4 VRotY = glm::rotate(M, rotator.theta, glm::vec3(1.0f, 0.0f, 0.0f)); //Rotation about x-axis
        glm::mat4 VTrans = glm::translate(M, glm::vec3(trans.horizontal, 0.0f, trans.zoom));

        glm::vec4 eye_position = VRotX * VRotY * glm::vec4(0.0f, 0.0f, 3 * (max_volume_side + 0.5f), 1.0f);
        glm::vec4 camPos = eye_position;
        glm::mat4 V = VTrans * glm::lookAt(glm::vec3(eye_position), scene_center, glm::vec3(0.0f, 1.0f, 0.0f));
        P = glm::perspectiveFov(50.0f, static_cast<float>(w), static_cast<float>(h), 0.1f, 100.0f);
        MV = V * M;

        //Calculate light direction
        lDir = glm::vec3(1.0f);

        // Clear the buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(params.bg_color.r,
                     params.bg_color.g,
                     params.bg_color.b,
                     1.0f);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, backgroundFBO);

        hmap.render(P, MV, hmap_wireframe);

        if (render_particle_grid) {
            voxelGrid.render(P, MV, params.kernel_size);
        }


        //----------------PARTICLE_DEPTH SHADER
        //Set viewport to low-res
        glViewport(0, 0, w/RESOLUTION, h/RESOLUTION);
        glm::mat4 P_LowRes = glm::perspectiveFov(50.0f, static_cast<float>(w/RESOLUTION), static_cast<float>(h/RESOLUTION), 0.1f, 100.0f);
        glm::vec2 screenSize = glm::vec2(w/RESOLUTION, h/RESOLUTION);

        // Activate particle depth FBO
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glBindFramebuffer(GL_FRAMEBUFFER, particleFBO[0]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w/RESOLUTION, h/RESOLUTION, 0, GL_R32F, GL_FLOAT, NULL);

        particleDepthShader();

        //Send textures
        /*glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, backgroundTexture);
        glUniform1i(particleDepthShader.terrainTex, 0);
        */

        //Send uniform variables
        glUniformMatrix4fv(particleDepthShader.MV_Loc, 1, GL_FALSE, &MV[0][0]);
        glUniformMatrix4fv(particleDepthShader.P_Loc, 1, GL_FALSE, &P_LowRes[0][0]);
        glUniform2fv(particleDepthShader.screenSize_Loc, 1, &screenSize[0]);
        glUniform4fv(particleDepthShader.camPos_Loc, 1, &camPos[0]);


        glBindVertexArray(part_vao);
        #ifdef USE_TESS_SHADER
                glDrawArrays(GL_PATCHES, 0, n_particles); //TessShader
        #else
                glDrawArrays(GL_POINTS, 0, n_particles); //GeomShader
        #endif
        glBindVertexArray(0);


        //-----------------------------PARTICLE_THICKNESS SHADER
        glBindFramebuffer(GL_FRAMEBUFFER, particleThicknessFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w/RESOLUTION, h/RESOLUTION, 0, GL_R32F, GL_FLOAT, NULL);

        particleThicknessShader();

        // Set textures
        /*glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, backgroundTexture);
        glUniform1i(particleThicknessShader.terrainTex, 0);
        */

        //Send uniform variables
        glUniformMatrix4fv(particleThicknessShader.MV_Loc, 1, GL_FALSE, &MV[0][0]);
        glUniformMatrix4fv(particleThicknessShader.P_Loc, 1, GL_FALSE, &P_LowRes[0][0]);
        glUniform2fv(particleThicknessShader.screenSize_Loc, 1, &screenSize[0]);

        glBindVertexArray(part_vao);

        // Enable additive blending and disable depth test
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glDisable(GL_DEPTH_TEST);

        #ifdef USE_TESS_SHADER
                glDrawArrays(GL_PATCHES, 0, n_particles); //TessShader
        #else
                glDrawArrays(GL_POINTS, 0, n_particles); //GeomShader
        #endif

        // Turn blending back off and depth test back on
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glBindVertexArray(0);

        //----------------------------PARTICLE_VELOCITY SHADER
        glBindFramebuffer(GL_FRAMEBUFFER, velocityFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w/RESOLUTION, h/RESOLUTION, 0, GL_R32F, GL_FLOAT, NULL);

        particleVelocityShader();

        //Send uniform variables
        glUniformMatrix4fv(particleVelocityShader.MV_Loc, 1, GL_FALSE, &MV[0][0]);
        glUniformMatrix4fv(particleVelocityShader.P_Loc, 1, GL_FALSE, &P_LowRes[0][0]);
        glUniform2fv(particleVelocityShader.screenSize_Loc, 1, &screenSize[0]);

        glBindVertexArray(part_vao);
        #ifdef USE_TESS_SHADER
                glDrawArrays(GL_PATCHES, 0, n_particles); //TessShader
        #else
                glDrawArrays(GL_POINTS, 0, n_particles); //GeomShader
        #endif
        glBindVertexArray(0);



        //-------------------------------CURVATURE_FLOW SHADER

        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        curvatureFlowShader();

        //Send uniform variables
        glUniformMatrix4fv(curvatureFlowShader.P_Loc, 1, GL_FALSE, &P_LowRes[0][0]);
        glUniform2fv(curvatureFlowShader.screenSize_Loc, 1, &screenSize[0]);
        glUniform1i(curvatureFlowShader.particleTex, 0); //Why not in loop?

        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(quad_vao);

        // Smoothing loop
        glDisable(GL_DEPTH_TEST);
        int pingpong = 0;
        for(int i = 0; i < SMOOTHING_ITERATIONS; i++) {
            // Bind no FBO
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // Bind texture
            glBindTexture(GL_TEXTURE_2D, particleTexture[pingpong]);

            // Activate proper FBO and clear
            glBindFramebuffer(GL_FRAMEBUFFER, particleFBO[1 - pingpong]);

            // Draw a quad
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

            // Switch buffers
            pingpong = 1 - pingpong;
        }
        glEnable(GL_DEPTH_TEST);
        glBindVertexArray(0);


        //----------------------------LIQUID_SHADE SHADER
        // Activate particle color FBO
        glBindFramebuffer(GL_FRAMEBUFFER, particleColorFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        liquidShadeShader();

        // Bind and set textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, particleTexture[0]);
        glUniform1i(liquidShadeShader.particleTex, 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, particleThicknessTexture);
        glUniform1i(liquidShadeShader.particleThicknessTex, 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, terrain.envTexture);
        glUniform1i(liquidShadeShader.environmentTex, 2);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, particleVelocityTexture);
        glUniform1i(liquidShadeShader.velocityTex, 3);


        // Send uniforms
        glUniformMatrix4fv(liquidShadeShader.MV_Loc, 1, GL_FALSE, &MV[0][0]);
        glUniformMatrix4fv(liquidShadeShader.P_Loc, 1, GL_FALSE, &P_LowRes[0][0]);
        glUniform2fv(liquidShadeShader.screenSize_Loc, 1, &screenSize[0]);
        glUniform3fv(liquidShadeShader.lDir_Loc, 1, &lDir[0]);


        glDisable(GL_DEPTH_TEST);
        glBindVertexArray(quad_vao);
        #ifdef USE_TESS_SHADER
                glDrawArrays(GL_PATCHES, 0, n_particles); //TessShader
        #else
                //glDrawArrays(GL_POINTS, 0, n_particles); //GeomShader
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
        #endif
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);


        //-----------------------------COMPOSITION SHADER
        //Set viewport to whole window
        glViewport(0, 0, w, h);

        // Deactivate FBOs
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Compose shader
        compositionShader();

        // Bind and set textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, terrain.envTexture);
        glUniform1i(compositionShader.backgroundTex, 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, particleColorTexture ); //   particleColorTexture
        glUniform1i(compositionShader.particleTex, 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, backgroundTexture);
        glUniform1i(compositionShader.terrainTex, 2);

        // Send uniforms
        glUniformMatrix4fv(compositionShader.MV_Loc, 1, GL_FALSE, &MV[0][0]);
        glUniformMatrix4fv(compositionShader.P_Loc, 1, GL_FALSE, &P[0][0]);

        glDisable(GL_DEPTH_TEST);
        glBindVertexArray(quad_vao);

        #ifdef USE_TESS_SHADER
                glDrawArrays(GL_PATCHES, 0, n_particles); //TessShader
        #else
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
        #endif
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);

        /*-----------------------------------------------------------------------------------*/

        screen->drawWidgets();
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

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
            params.fps = newFPS;
            std::stringstream fpsString;
            fpsString << std::fixed << std::setprecision(0) << newFPS;
            fpsBox->setValue(fpsString.str());
            frames_last_second = 0;
            second_accumulator = std::chrono::duration<double>(0);
        }
        /*--------------------------------------------------------------------------------*/
    }

    glDeleteBuffers(1, &pos_vbo);
    glDeleteBuffers(1, &vel_vbo);
    glDeleteBuffers(1, &quadElemVBO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteVertexArrays(1, &part_vao);
    glDeleteVertexArrays(1, &quad_vao);

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

    Window *window_bgcolor = new Window(screen, "Background color");
    window_bgcolor->setPosition(Vector2i(425, 288));
    GridLayout *layout =
            new GridLayout(Orientation::Horizontal, 2,
                           Alignment::Middle, 15, 5);
    layout->setColAlignment(
            {Alignment::Maximum, Alignment::Fill});
    layout->setSpacing(0, 10);
    window_bgcolor->setLayout(layout);

    ColorWheel *colorwheel = new ColorWheel(window_bgcolor);

    colorwheel->setCallback([=](const Color &value) {
        p->bg_color.r = value.r();
        p->bg_color.g = value.g();
        p->bg_color.b = value.b();
    });

    Window *window = new Window(screen, "Fluid parameters");
    window->setPosition(Vector2i(15, 15));
    window->setLayout(new GroupLayout());

    new Label(window, "Kernel size", "sans-bold");
    Widget *panel_kernel = new Widget(window);
    panel_kernel->setLayout(new BoxLayout(Orientation::Horizontal,
                                          Alignment::Minimum, 0, 25));

    Slider *slider_kernel = new Slider(panel_kernel);
    slider_kernel->setValue(p->kernel_size);
    slider_kernel->setFixedWidth(80);

    TextBox *textBox_kernel = new TextBox(panel_kernel);

    std::stringstream stream;
    stream << std::fixed << std::setprecision(1) << (double) p->kernel_size;
    textBox_kernel->setValue(stream.str());

    slider_kernel->setCallback([=](float value) {
        std::stringstream stream_kernel;
        stream_kernel << std::fixed << std::setprecision(1) << (double) p->kernel_size;
        textBox_kernel->setValue(stream_kernel.str());
        p->kernel_size = value * 2 + 0.1;
    });

    new Label(window, "Gas Constant", "sans-bold");
    Widget *panel_gas = new Widget(window);
    panel_gas->setLayout(new BoxLayout(Orientation::Horizontal,
                                       Alignment::Minimum, 0, 25));

    Slider *slider_gas = new Slider(panel_gas);
    slider_gas->setValue(p->k_gas);
    slider_gas->setFixedWidth(80);

    TextBox *textBox_gas = new TextBox(panel_gas);
    std::stringstream stream_gas;
    stream_gas << std::fixed << std::setprecision(1) << (double) p->k_gas;
    textBox_gas->setValue(stream_gas.str());

    slider_gas->setCallback([=](float value_gas) {
        std::stringstream stream_gas;
        stream_gas << std::fixed << std::setprecision(1) << (double) p->k_gas;
        textBox_gas->setValue(stream_gas.str());
        p->k_gas = value_gas;
    });

    new Label(window, "Viscosity constant", "sans-bold");
    Widget *panel_vis = new Widget(window);
    panel_vis->setLayout(new BoxLayout(Orientation::Horizontal,
                                       Alignment::Minimum, 0, 25));

    Slider *slider_vis = new Slider(panel_vis);
    slider_vis->setValue(1);
    slider_vis->setFixedWidth(80);

    TextBox *textBox_vis = new TextBox(panel_vis);
    std::stringstream stream_vis;
    stream_vis << std::fixed << std::setprecision(1) << (double) p->k_viscosity;
    textBox_vis->setValue(stream_vis.str());

    slider_vis->setCallback([=](float value_gas) {
        std::stringstream stream_vis;
        stream_vis << std::fixed << std::setprecision(1) << (double) p->k_viscosity;
        textBox_vis->setValue(stream_vis.str());
        p->k_viscosity = 20 * value_gas;
    });

    new Label(window, "Other", "sans-bold");
    CheckBox *cb = new CheckBox(window, "Gravity",
                                [=](bool state) {
                                    if (state)
                                        p->gravity.y = -9.82f;
                                    else
                                        p->gravity.y = 0.0f;
                                }
    );
    cb->setFontSize(16);
    cb->setChecked(true);

    cb = new CheckBox(window, "Heightmap wireframe",
                      [=](bool state) {
                          hmap_wireframe = state;
                      }
    );
    cb->setFontSize(16);
    cb->setChecked(false);

    cb = new CheckBox(window, "Render particle grid",
                      [=](bool state) {
                          render_particle_grid = state;
                      }
    );
    cb->setFontSize(16);
    cb->setChecked(false);

    Widget *panel_fps = new Widget(window);
    panel_fps->setLayout(new BoxLayout(Orientation::Horizontal,
                                       Alignment::Maximum, 5, 10));
    new Label(panel_fps, "FPS: ", "sans-bold");
    fpsBox = new TextBox(panel_fps);
    fpsBox->setFixedSize(Vector2i(100, 20));
    fpsBox->setDefaultValue("0.0");
    fpsBox->setFontSize(16);
    fpsBox->setFormat("[-]?[0-9]*\\.?[0-9]+");

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
