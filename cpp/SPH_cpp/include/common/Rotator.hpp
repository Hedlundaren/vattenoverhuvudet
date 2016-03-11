#include <GLFW/glfw3.h>
#include <cmath>

#include "glm/glm.hpp"

#include "FluidInteraction.hpp"

#ifndef M_PI
#define M_PI (3.141592653589793)
#endif

class KeyTranslator {
public:
	float horizontal;
	float zoom;

private:
	double lastTime;

public:
    void init(GLFWwindow *window);
    void poll(GLFWwindow *window);
};

class MouseRotator {

public:
	float phi;
	float theta;

private:
	double lastX;
	double lastY;
    double thisX;
    double thisY;
	int lastLeft;
	int lastRight;

    int currentRight;

public:
    void init(GLFWwindow *window);
    void poll(GLFWwindow *window);

	const FluidInteraction get_fluid_interaction() const;
};
