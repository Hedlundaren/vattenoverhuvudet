function laplacian = laplacianWpoly6( r, h )
%LAPLACIANWPOLY6 Summary of this function goes here
%   Detailed explanation goes here
radius = norm(r);

if radius < h && radius >= 0
    laplacian = ((315/(64*pi*h^9)) * 24 * radius^2 * (h^2 - radius^2));
else
    laplacian = 0;
end
end