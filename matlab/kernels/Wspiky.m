function [ w ] = Wspiky( r, h )
%WSPIKY Summary of this function goes here
%   Detailed explanation goes here
    radius = norm(r);

    if radius < h && radius >= 0
        w = (15/(pi*h^6)) * (h - radius)^3;
    else
        w = 0;
        
    end
end

