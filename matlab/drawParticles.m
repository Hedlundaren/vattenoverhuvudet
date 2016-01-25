function drawParticles(particles)
  for j = 1:length(particles)
      pos = particles(j).position;
      plot(pos(1), pos(2), 'ob');
      axis([0 10 0 10]);
      hold on
  end
end