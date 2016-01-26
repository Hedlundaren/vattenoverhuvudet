function [ w ] = Wspiky( r, h )
%WSPIKY Smoothing kernel
%   Used for pressure computations

    radius = norm(r);

    if radius < h && radius >= 0
        w = (15/(pi*h^6)) * (h - radius)^3;
    else
        w = 0;
        
    end
end

