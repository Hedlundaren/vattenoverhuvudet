function newParticles = calculateForces( particles, mass, kernelSize )
%CALCULATEFORCES Summary of this function goes here
%   Detailed explanation goes here

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
    for j=1:length(particles)
        % Calculate particle j's pressure force on i
        
        % Calculate particle j's viscosity force on i
        
        % Calculate particle j's tension force on i
    end
    
    % Add any external forces on i
end

newParticles = particles;

end