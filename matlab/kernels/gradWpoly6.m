function [ gradient ] = gradWpoly6( r, h )
%GRADWPOLY6 - Gradient of Wpoly6
%   Used for surface normal (n)

radius = norm(r);

if radius < h && radius >= 0
    gradient = - ((315/(64*pi*h^9)) * 6 * (h^2 - radius^2)^2) * r;
else
    gradient = zeros(1, length(r));
end

end

