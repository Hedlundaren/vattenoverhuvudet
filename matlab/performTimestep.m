function newParticles = performTimestep( particles, dt )
%PERFORMTIMESTEP Summary of this function goes here
%   Detailed explanation goes here
for k = 1:length(particles)
    % Perform acceleration integration to receive velocity
    velocity = particles(k).velocity;
    
    particles(k).velocity = velocity + (particles(k).force / particles(k).density) * dt;
    
    % Perform velocity integration to receive position
    position = particles(k).position;
    
    position = position + particles(k).velocity * dt;
    particles(k).position = position;
end

newParticles = particles;

end