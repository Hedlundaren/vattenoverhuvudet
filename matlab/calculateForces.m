function newParticles = calculateForces( particles, mass, kernelSize )
%CALCULATEFORCES Summary of this function goes here
%   Detailed explanation goes here
restDensity = 0;
gasConstantK = 1;

% Set all forces to zero and calculate their densities
for i=1:length(particles)
    particles(i).force = 0;
    
    density = 0;
    
    for j=1:length(particles)
        relativePosition = particles(i).position - particles(j).position;
        density = density + mass * Wpoly6(relativePosition, kernelSize);
    end
    
    particles(i).density = density;
end

for i=1:length(particles)
    iPressure = (particles(i).density - restDensity) * gasConstantK;
    
    pressureForce = [0 0];
    viscosityForce = [0 0];
    tensionForce = [0 0];
    
    for j=1:length(particles)
        relativePosition = particles(i).position - particles(j).position;
        
        % Calculate particle j's pressure force on i
        jPressure = (particles(j).density - restDensity) * gasConstantK;
        pressureForce = pressureForce - mass * ...
            ((iPressure + jPressure)/(2*particles(j).density)) * ...
            gradWspiky(relativePosition, kernelSize);
        
        % Calculate particle j's viscosity force on i
        
        
        % Calculate particle j's tension force on i
    end
    
    % Add any external forces on i
    externalForce = [0 -1];
    
    particles(i).force = pressureForce + viscosityForce + ...
        tensionForce + externalForce;
end

newParticles = particles;

end