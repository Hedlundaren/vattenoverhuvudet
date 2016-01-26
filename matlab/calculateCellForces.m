function newCenterCellParticles = calculateCellForces( centerCellParticles, neighbouringCellsParticles, parameters )
%CALCULATECELLFORCES Summary of this function goes here
%   Detailed explanation goes here

n_center = length(centerCellParticles);
n_neighbouring = length(neighbouringCellsParticles);

% Calculate their densities
for i=1:n_center
    iPressure = (particles(i).density - parameters.restDensity) * parameters.gasConstantK;
    
    cs = 0;
    n = [0 0];
    laplacianCs = 0;
    
    pressureForce = [0 0];
    viscosityForce = [0 0];
    
    for j=1:n_center
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
    
    % Ugly code duplication but whatevveaaaaaa
    for j=1:n_neighbouring
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
    
    % Sum all forces and call it a day
    centerCellParticles(i).force = pressureForce + viscosityForce + ...
        tensionForce + externalForce;
end

newCenterCellParticles = centerCellParticles;

end

