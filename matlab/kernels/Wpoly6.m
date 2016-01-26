function [ w ] = Wpoly6( r, h )
%WPOLY6 Smoothing kernel
%   Used for most common calculations (e.g. density and surface tension)

    radius = norm(r);

    if radius < h && radius >= 0
        w = (315/(64*pi*h^9)) * (h^2 - radius^2)^3;
    else
        w = 0;
    end
end

