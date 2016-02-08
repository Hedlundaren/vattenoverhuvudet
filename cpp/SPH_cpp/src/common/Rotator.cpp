#include "common/Rotator.hpp"

void KeyTranslator::init(GLFWwindow *window) {
     horizontal = 0.0;
     zoom = 0.0;
     lastTime = glfwGetTime();
};

void KeyTranslator::poll(GLFWwindow *window) {

	double currentTime, elapsedTime;

	currentTime = glfwGetTime();
	elapsedTime = currentTime - lastTime;
	lastTime = currentTime;

	if(glfwGetKey(window, GLFW_KEY_RIGHT)) {
		horizontal += elapsedTime*5.0; //Move right with speed 5*dt
	}

	if(glfwGetKey(window, GLFW_KEY_LEFT)) {
		horizontal -= elapsedTime*5.0; //Move left with speed 5*dt

	}

	if(glfwGetKey(window, GLFW_KEY_UP)) {
		zoom -= elapsedTime*2.0; // Zoom in with speed 3*dt
	}

	if(glfwGetKey(window, GLFW_KEY_DOWN)) {
		zoom += elapsedTime*2.0; // Zoom out with speed 3*dt
	}
}


void MouseRotator::init(GLFWwindow *window) {
    phi = 0.0;
    theta = 0.0;
    glfwGetCursorPos(window, &lastX, &lastY);
	lastLeft = GL_FALSE;
	lastRight = GL_FALSE;
}

void MouseRotator::poll(GLFWwindow *window) {

  double currentX;
  double currentY;
  int currentLeft;
  int currentRight;
  double moveX;
  double moveY;
  int windowWidth;
  int windowHeight;

  // Find out where the mouse pointer is, and which buttons are pressed
  glfwGetCursorPos(window, &currentX, &currentY);
  currentLeft = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
  currentRight = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT); //Not used yet
  glfwGetWindowSize( window, &windowWidth, &windowHeight );

  if(currentLeft && lastLeft) { // If a left button drag is in progress
    moveX = currentX - lastX;
    moveY = currentY - lastY;
  	phi += M_PI * moveX/windowWidth; // Longest drag rotates 180 degrees
	if (phi > M_PI*2.0) phi = 0.0f;
	if (phi < 0.0) phi = M_PI*2.0f;

  	theta += M_PI * moveY/windowHeight; // Longest drag rotates 180 deg
	if (theta > M_PI*2.0) theta = 0.0f;
	if (theta < 0.0f) theta = M_PI*2.0f;
  }
  lastLeft = currentLeft;
  lastRight = currentRight;
  lastX = currentX;
  lastY = currentY;
}
