function [ gradient ] = gradWpoly6( r, h )
%DWPOLY6 Summary of this function goes here
%   Detailed explanation goes here
    radius = norm(r);

    if radius < h && radius >= 0
        gradient = - ((315/(64*pi*h^9)) * 6 *(h^2 - radius^2)^3) * r;
    else
        gradient = [0 0];
    end

end

