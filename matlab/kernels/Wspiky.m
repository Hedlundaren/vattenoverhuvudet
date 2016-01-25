function [ w ] = Wspiky( radius, h )
%WSPIKY Summary of this function goes here
%   Detailed explanation goes here
    if radius < h && radius >= 0
        w = (15/(pi*h^6)) * (h - radius)^3;
    else
        w = 0;
        
    end
end

