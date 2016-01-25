function performTimestep( particles )
%PERFORMTIMESTEP Summary of this function goes here
%   Detailed explanation goes here
for k = 1:length(particles)
    position = particles(k).position;
    
    position = position + particles(k).velocity * dt;
    particles(k).position = position;
end

end