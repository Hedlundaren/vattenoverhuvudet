function drawParticles(particles)
%DRAWPARTICLES
%   Draws all particles in their new positions

  for j = 1:length(particles)
      pos = particles(j).position;
      plot(pos(1), pos(2), '.b');
  end
end