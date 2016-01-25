function newParticles = checkBoundaries( particles, parameters )
%CHECKBOUNDARIES Summary of this function goes here
%   Detailed explanation goes here
for i=1:length(particles)
    position = particles(i).position;
    velocity = particles(i).velocity;
    if(position(2) < parameters.bottomBound || position(2) > parameters.topBound)
        position(2) = max(min(position(2),parameters.topBound),parameters.bottomBound);
        velocity(2) = -velocity(2);
    end
    if(position(1) < parameters.leftBound || position(1) > parameters.rightBound)
        position(1) = max(min(position(1),parameters.rightBound),parameters.leftBound);
        velocity(1) = -velocity(1);
    end
    particles(i).position = position;
    particles(i).velocity = velocity;
end
newParticles = particles;
end

