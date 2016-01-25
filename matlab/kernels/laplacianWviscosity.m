function [laplacian] = laplacianWviscosity(r, h)
	radius = norm(r);

	if radius < h && radius >= 0
		laplacian = (45 / (pi * h^6)) * (h - radius);
	else
		laplacian = 0;
	end
end