function newParticles = calculateForces( particles, parameters )
%CALCULATEFORCES - Calculate all forces acting on the particles
%   Calculate density, pressure force, viscosity force, tension force and
%   external forces.

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
    
    cs = 0;
    n = [0 0];
    laplacianCs = 0;
    
    pressureForce = [0 0];
    viscosityForce = [0 0];
    
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
        
        % Calculate "color" for particle j
        cs = cs + parameters.mass * (1 / particles(j).density) * ...
            Wpoly6(relativePosition, parameters.kernelSize);
        
        % Calculate gradient of "color" for particle j
        n = n + parameters.mass * (1 / particles(j).density) * ...
            gradWpoly6(relativePosition, parameters.kernelSize);
        
        % Calculate laplacian of "color" for particle j
        laplacianCs = laplacianCs + parameters.mass * (1 / particles(j).density) * ...
            laplacianWpoly6(relativePosition, parameters.kernelSize);
    end
    
    if (norm(n) < parameters.nThreshold)
        tensionForce = [0 0];
    else
        k = - laplacianCs / norm(n);
        tensionForce = parameters.sigma * k * n;
    end
    
    % Add any external forces on i
    externalForce = parameters.gravity;
    
    particles(i).force = pressureForce + viscosityForce + ...
        tensionForce + externalForce;
end

newParticles = particles;

end