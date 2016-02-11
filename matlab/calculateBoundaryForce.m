function boundaryForce = calculateBoundaryForce( particle, parameters )
%UNTITLED Summary of this function goes here
%   Detailed explanation goes here
boundaryForce = [0 0];

% BOTTOM BOUND
r = [0 (particle.position(2) - parameters.bottomBound)];
radius = norm(r);

if radius < parameters.kernelSize && radius >= 0
    boundaryForce = -parameters.mass * 100*gradWspiky(r, parameters.kernelSize);
end

% LEFT BOUND
r = [(particle.position(1) - parameters.leftBound) 0];
radius = norm(r);

if radius < parameters.kernelSize && radius >= 0
    boundaryForce = boundaryForce - parameters.mass * 100*gradWspiky(r, parameters.kernelSize);
end
    
% RIGHT BOUND
r = [(particle.position(1) - parameters.rightBound) 0];
radius = norm(r);

if radius < parameters.kernelSize && radius >= 0
    boundaryForce = boundaryForce - parameters.mass * 100*gradWspiky(r, parameters.kernelSize);
end
    



end

