function newParticles = calculateForces( particles, parameters )
%CALCULATEFORCES Summary of this function goes here
%   Detailed explanation goes here

% Set all forces to zero and calculate their densities
for i=1:length(particles)
    particles(i).force = 0;
    
    density = 0;
    
    for j=1:length(particles)
        relativePosition = particles(i).position - particles(j).position;
        density = density + parameters.mass * Wpoly6(relativePosition, parameters.kernelSize);
    end
    
    particles(i).density = density;
end

for i=1:length(particles)
    iPressure = (particles(i).density - parameters.restDensity) * parameters.gasConstantK;
    
    pressureForce = [0 0];
    viscosityForce = [0 0];
    tensionForce = [0 0];
    
    for j=1:length(particles)
        relativePosition = particles(i).position - particles(j).position;
        
        % Calculate particle j's pressure force on i
        jPressure = (particles(j).density - parameters.restDensity) * parameters.gasConstantK;
        pressureForce = pressureForce - parameters.mass * ...
            ((iPressure + jPressure)/(2*particles(j).density)) * ...
            gradWspiky(relativePosition, parameters.kernelSize);
        
        % Calculate particle j's viscosity force on i
        viscosityForce = viscosityForce + parameters.viscosityConstant * ...
            parameters.mass * ((particles(j).velocity - particles(i).velocity)/particles(j).density) * ...
            laplacianWviscosity(relativePosition, parameters.kernelSize);
        
        % Calculate particle j's tension force on i
    end
    
    % Add any external forces on i
    externalForce = [0 -1];
    
    particles(i).force = pressureForce + viscosityForce + ...
        tensionForce + externalForce;
end

newParticles = particles;

end