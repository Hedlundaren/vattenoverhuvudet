function drawParticles(particles)
  for j = 1:length(particles)
      pos = particles(j).position;
      plot(pos(1), pos(2), 'ob');
  end
end