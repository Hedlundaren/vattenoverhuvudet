function newParticles = performTimestep( particles, dt )
%PERFORMTIMESTEP
%   Euler time step

for k = 1:length(particles)
    % Perform acceleration integration to receive velocity
    velocity = particles(k).velocity;
    
    particles(k).velocity = velocity + (particles(k).force / particles(k).density) * dt;
    
    % Perform velocity integration to receive position
    position = particles(k).position;
    
    position = position + particles(k).velocity * dt;
    particles(k).position = position;
end

%Update to new positions
newParticles = particles;

end