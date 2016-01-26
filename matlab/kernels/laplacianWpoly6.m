function laplacian = laplacianWpoly6( r, h )
%LAPLACIANWPOLY6 - Laplacian of Wpoly6
%   Used for curvatore of surface (k(cs))

radius = norm(r);

if radius < h && radius >= 0
    laplacian = (315/(64*pi*h^9)) * (24 * radius^2 * (h^2 - radius^2) - 6 * (h^2 - radius^2)^2);
else
    laplacian = 0;
end
end