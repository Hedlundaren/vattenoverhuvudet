#pragma once

// Includes
#include <stdlib.h>
#include <stdio.h>
#include <cmath>
#include <GL/gl.h>


GLuint makeTextureBuffer(int w, int h, GLenum format, GLint internalFormat);

GLuint loadTexture(const char *filename);

char* loadFile(char* name);
GLuint genFloatTexture(float *data, int width, int height);

float* loadPGM(const char* fileName, int w, int h);