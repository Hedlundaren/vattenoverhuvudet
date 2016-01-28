#include <iostream>
#include "Particle.h"
#include "kernels.h"
//#include <GLFW/glfw3.h>

using namespace std;

int main() {
    const int n_Particles = 100;

    Particle particles[n_Particles];

    for(int i = 0; i < n_Particles; i++) {
        particles[i] = Particle();
    }

    cout << "Hello, World!" << endl;
    return 0;
}
