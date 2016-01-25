function [gradient] = gradWspiky(r, h)
	radius = norm(r);

	if radius < h && radius > 0
		gradient = (15 / (pi * h^6)) * 3 * (h - radius)^2 * (- r / radius);
	else
		gradient = zeros(1, length(r));
	end
end