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

    ParticleSimulator *simulator;

#ifdef USE_OPENCL_SIMULATION
    simulator = new OpenClParticleSimulator();
#else
    simulator = new CppParticleSimulator();
#endif

    //Generate particles
    int n = -1;
    cout << "How many particles? ";
    std::cin >> n;
    const int n_particles = n;

    Parameters params(n_particles);
    Parameters::set_default_parameters(params);

    screen = new Screen;
    screen->initialize(window, true);
    setNanoScreenCallbacksGLFW(window, screen);
    createGUI(screen, params);

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
        cout << "bg color = " << glm::to_string(params.bg_color) << endl;
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

    /* No need to store a pointer, the data structure will be automatically
       freed when the parent window is deleted */
    new Label(window, "Push buttons", "sans-bold");

    Button *b = new Button(window, "Plain button");
    b->setCallback([] { cout << "pushed!" << endl; });
    b = new Button(window, "Styled", ENTYPO_ICON_ROCKET);
    b->setBackgroundColor(Color(0, 0, 255, 25));
    b->setCallback([] { cout << "pushed!" << endl; });

    new Label(window, "Toggle buttons", "sans-bold");
    b = new Button(window, "Toggle me");
    b->setFlags(Button::ToggleButton);
    b->setChangeCallback([](bool state) { cout << "Toggle button state: " << state << endl; });

    new Label(window, "Radio buttons", "sans-bold");
    b = new Button(window, "Radio button 1");
    b->setFlags(Button::RadioButton);
    b = new Button(window, "Radio button 2");
    b->setFlags(Button::RadioButton);

    new Label(window, "A tool palette", "sans-bold");
    Widget *tools = new Widget(window);
    tools->setLayout(new BoxLayout(Orientation::Horizontal,
                                   Alignment::Middle, 0, 6));

    b = new ToolButton(tools, ENTYPO_ICON_CLOUD);
    b = new ToolButton(tools, ENTYPO_ICON_FF);
    b = new ToolButton(tools, ENTYPO_ICON_COMPASS);
    b = new ToolButton(tools, ENTYPO_ICON_INSTALL);

    new Label(window, "Popup buttons", "sans-bold");
    PopupButton *popupBtn = new PopupButton(window, "Popup", ENTYPO_ICON_EXPORT);
    Popup *popup = popupBtn->popup();
    popup->setLayout(new GroupLayout());
    new Label(popup, "Arbitrary widgets can be placed here");
    new CheckBox(popup, "A check box");
    popupBtn = new PopupButton(popup, "Recursive popup", ENTYPO_ICON_FLASH);
    popup = popupBtn->popup();
    popup->setLayout(new GroupLayout());
    new CheckBox(popup, "Another check box");

    window = new Window(screen, "Basic widgets");
    window->setPosition(Vector2i(200, 15));
    window->setLayout(new GroupLayout());

    new Label(window, "Message dialog", "sans-bold");
    tools = new Widget(window);
    tools->setLayout(new BoxLayout(Orientation::Horizontal,
                                   Alignment::Middle, 0, 6));
    b = new Button(tools, "Info");
    b->setCallback([&] {
        auto dlg = new MessageDialog(screen, MessageDialog::Type::Information, "Title",
                                     "This is an information message");
        dlg->setCallback([](int result) { cout << "Dialog result: " << result << endl; });
    });
    b = new Button(tools, "Warn");
    b->setCallback([&] {
        auto dlg = new MessageDialog(screen, MessageDialog::Type::Warning, "Title", "This is a warning message");
        dlg->setCallback([](int result) { cout << "Dialog result: " << result << endl; });
    });
    b = new Button(tools, "Ask");
    b->setCallback([&] {
        auto dlg = new MessageDialog(screen, MessageDialog::Type::Warning, "Title", "This is a question message", "Yes",
                                     "No", true);
        dlg->setCallback([](int result) { cout << "Dialog result: " << result << endl; });
    });

    new Label(window, "File dialog", "sans-bold");
    tools = new Widget(window);
    tools->setLayout(new BoxLayout(Orientation::Horizontal,
                                   Alignment::Middle, 0, 6));
    b = new Button(tools, "Open");
    b->setCallback([&] {
        cout << "File dialog result: " << file_dialog(
                {{"png", "Portable Network Graphics"},
                 {"txt", "Text file"}}, false) << endl;
    });
    b = new Button(tools, "Save");
    b->setCallback([&] {
        cout << "File dialog result: " << file_dialog(
                {{"png", "Portable Network Graphics"},
                 {"txt", "Text file"}}, true) << endl;
    });

    new Label(window, "Combo box", "sans-bold");
    new ComboBox(window, {"Combo box item 1", "Combo box item 2", "Combo box item 3"});
    new Label(window, "Check box", "sans-bold");
    CheckBox *cb = new CheckBox(window, "Flag 1",
                                [](bool state) { cout << "Check box 1 state: " << state << endl; }
    );
    cb->setChecked(true);
    cb = new CheckBox(window, "Flag 2",
                      [](bool state) { cout << "Check box 2 state: " << state << endl; }
    );

    new Label(window, "Slider and text box", "sans-bold");

    Widget *panel = new Widget(window);
    panel->setLayout(new BoxLayout(Orientation::Horizontal,
                                   Alignment::Middle, 0, 20));

    Slider *slider = new Slider(panel);
    slider->setValue(0.5f);
    slider->setFixedWidth(80);

    TextBox *textBox = new TextBox(panel);
    textBox->setFixedSize(Vector2i(60, 25));
    textBox->setValue("50");
    textBox->setUnits("%");
    slider->setCallback([textBox](float value) {
        textBox->setValue(std::to_string((int) (value * 100)));
    });
    slider->setFinalCallback([&](float value) {
        cout << "Final slider value: " << (int) (value * 100) << endl;
    });
    textBox->setFixedSize(Vector2i(60, 25));
    textBox->setFontSize(20);
    textBox->setAlignment(TextBox::Alignment::Right);

    window = new Window(screen, "Misc. widgets");
    window->setPosition(Vector2i(425, 15));
    window->setLayout(new GroupLayout());
    new Label(window, "Color wheel", "sans-bold");
    new ColorWheel(window);
    new Label(window, "Function graph", "sans-bold");
    Graph *graph = new Graph(window, "Some function");
    graph->setHeader("E = 2.35e-3");
    graph->setFooter("Iteration 89");
    VectorXf &func = graph->values();
    func.resize(100);
    for (int i = 0; i < 100; ++i)
        func[i] = 0.5f * (0.5f * std::sin(i / 10.f) +
                          0.5f * std::cos(i / 23.f) + 1);

    window = new Window(screen, "Grid of small widgets");
    window->setPosition(Vector2i(425, 288));
    GridLayout *layout =
            new GridLayout(Orientation::Horizontal, 2,
                           Alignment::Middle, 15, 5);
    layout->setColAlignment(
            {Alignment::Maximum, Alignment::Fill});
    layout->setSpacing(0, 10);
    window->setLayout(layout);

    {
        new Label(window, "Floating point :", "sans-bold");
        textBox = new TextBox(window);
        textBox->setEditable(true);
        textBox->setFixedSize(Vector2i(100, 20));
        textBox->setValue("50");
        textBox->setUnits("GiB");
        textBox->setDefaultValue("0.0");
        textBox->setFontSize(16);
        textBox->setFormat("[-]?[0-9]*\\.?[0-9]+");
    }

    {
        new Label(window, "Positive integer :", "sans-bold");
        textBox = new TextBox(window);
        textBox->setEditable(true);
        textBox->setFixedSize(Vector2i(100, 20));
        textBox->setValue("50");
        textBox->setUnits("Mhz");
        textBox->setDefaultValue("0.0");
        textBox->setFontSize(16);
        textBox->setFormat("[1-9][0-9]*");
    }

    {
        new Label(window, "Checkbox :", "sans-bold");

        cb = new CheckBox(window, "Check me");
        cb->setFontSize(16);
        cb->setChecked(true);
    }

    new Label(window, "Combo box :", "sans-bold");
    ComboBox *cobo =
            new ComboBox(window, {"Item 1", "Item 2", "Item 3"});
    cobo->setFontSize(16);
    cobo->setFixedSize(Vector2i(100, 20));

    new Label(window, "Color button :", "sans-bold");
    popupBtn = new PopupButton(window, "", 0);
    popupBtn->setBackgroundColor(Color(255, 120, 0, 255));
    popupBtn->setFontSize(16);
    popupBtn->setFixedSize(Vector2i(100, 20));
    popup = popupBtn->popup();
    popup->setLayout(new GroupLayout());

    ColorWheel *colorwheel = new ColorWheel(popup);
    colorwheel->setColor(popupBtn->backgroundColor());

    Button *colorBtn = new Button(popup, "Pick");
    colorBtn->setFixedSize(Vector2i(100, 25));
    Color c = colorwheel->color();
    colorBtn->setBackgroundColor(c);

    colorwheel->setCallback([=](const Color &value) {
        cout << "Color wheel" << endl;
        colorBtn->setBackgroundColor(value);
        p->bg_color.r = value.r();
        p->bg_color.g = value.g();
        p->bg_color.b = value.b();
    });

    colorBtn->setChangeCallback([colorBtn, popupBtn](bool pushed) {
        cout << "Color button" << endl;
        if (pushed) {
            popupBtn->setBackgroundColor(colorBtn->backgroundColor());
            popupBtn->setPushed(false);
        }
    });

    screen->performLayout();
}