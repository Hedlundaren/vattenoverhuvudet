

using namespace std;

/**********************************
* Class Particle               *
***********************************/

class Particle
{
public:

    //Constructor
    Particle();
    ~Particle();

    float position [3] = {0.0f, 0.0f, 0.0f};
    float velocity [3] = {0.0f, 0.0f, 0.0f};
    float force [3] = {0.0f, 0.0f, 0.0f};
    float density = 0.0f;
    float pressure = 0.0f;
    float cs = 0.0f;

protected:


private:

};

Particle::Particle(){
    position[0] = rand();
    position[1] = rand();
    position[2] = rand();
}

ostream& operator<<(ostream& out, const Particle& p)
{
    out << "x: " << p.position[0] << " y: " << p.position[1] << " z: " << p.position[2] << endl;

    return out;
}
