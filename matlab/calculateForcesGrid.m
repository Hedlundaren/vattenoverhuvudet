function newParticles = calculateForcesGrid(  particles, parameters  )
%CALCULATEFORCESGRID Summary of this function goes here
%   Detailed explanation goes here
emptyParticleArray = struct(...
    'density', {}, ...
    'position',{}, ...
    'velocity',{}, ...
    'pressure',{}, ...
    'force',{}, ...
    'cs', {});

cell = struct(...
    'particles', emptyParticleArray);

n_rows = ceil((parameters.rightBound - parameters.leftBound) / parameters.kernelSize);
n_cols = ceil((parameters.topBound - parameters.bottomBound) / parameters.kernelSize);

% Zero-element grid cells along edges for "padding"
cellGrid = repmat(cell, n_rows + 2, n_cols + 2);

% Generate cell grid and put each particle in correct cell
for i=1:length(particles)
    % + 1 for zero-padded grid
    row = ceil(particles(i).position(2) / parameters.kernelSize) + 1;
    col = ceil(particles(i).position(1) / parameters.kernelSize) + 1;
    
    cellGrid(row, col).particles(length(cellGrid(row, col).particles) + 1) = particles(i);
end

% Calculate each particle's density
for row=2:(n_rows+1)
   for col=2:(n_cols+1)
       centerCellParticles = cellGrid(row, col).particles;
       neighbouringCellsParticles = [cat(2, cellGrid([row-1 row+1], [col-1 col col+1]).particles) ...
           cat(2, cellGrid(row, [col-1 col+1]).particles)];
       
       cellGrid(row, col).particles = ...
           calculateCellDensities(centerCellParticles, neighbouringCellsParticles, parameters);
   end
end

% Calculate each particle's resulting force
for row=2:(n_rows+1)
   for col=2:(n_cols+1)
       centerCellParticles = cellGrid(row, col).particles;
       neighbouringCellsParticles = [cat(2, cellGrid([row-1 row+1], [col-1 col col+1]).particles) ...
           cat(2, cellGrid(row, [col-1 col+1]).particles)];
       
       cellGrid(row, col).particles = ...
           calculateCellForces(centerCellParticles, neighbouringCellsParticles, parameters);
   end
end

newParticles = cat(2, cellGrid.particles);

end

