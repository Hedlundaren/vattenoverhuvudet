function newCenterCellParticles = calculateCellDensities( centerCellParticles, neighbouringCellsParticles, parameters )
%CALCULATECELLDENSITIES Summary of this function goes here
%   Detailed explanation goes here
n_center = length(centerCellParticles);
n_neighbouring = length(neighbouringCellsParticles);

% Calculate their densities
for i=1:n_center
    density = 0;
    
    for j=1:n_center
        relativePosition = centerCellParticles(i).position - centerCellParticles(j).position;
        density = density + parameters.mass * Wpoly6(relativePosition, parameters.kernelSize);
    end
    
    for j=1:n_neighbouring
        relativePosition = centerCellParticles(i).position - neighbouringCellsParticles(j).position;
        density = density + parameters.mass * Wpoly6(relativePosition, parameters.kernelSize);
    end
    
    centerCellParticles(i).density = density;
end

newCenterCellParticles = centerCellParticles;

end