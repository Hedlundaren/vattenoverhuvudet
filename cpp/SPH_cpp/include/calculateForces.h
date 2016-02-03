
#ifndef CFORCES_H
#define CFORCES_H

#include <iostream>
#include "glm/glm.hpp"
#include "glm/ext.hpp"

float calcPressureForce(glm::vec3 p);

float calcViscosityForce(glm::vec3 p);

float calcTensionForce(glm::vec3 p);



#endif
