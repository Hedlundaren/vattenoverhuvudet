%% Initiate
% Here you can forget your past, just like "Men In Black"
clear all
close all
clc

FPS = 30;
simulationTime = 20; %seconds
frames = simulationTime * FPS;

n_particles = 1000;

parameters = struct(...
    'dt', 1 / FPS, ...
    'mass',1, ...
    'kernelSize',1, ...
    'gasConstantK',462, ...
    'viscosityConstant', 8.9e-4, ...
    'restDensity', 1, ...
    'sigma', 72e-3, ...
    'nThreshold', 0.02, ...
    'gravity', [0 -9.82], ...
    'leftBound', 0, ...
    'rightBound', 100, ...
    'bottomBound', 0, ...
    'topBound', 100, ...
    'wallDamper', 0.6);

%% One Particle
particle = struct(...
    'density', 0, ...
    'position',[0 0], ...
    'velocity',[0 0], ...
    'pressure',0, ...
    'force',[0 0], ...
    'cs', 0);

%% Several Particles


for i = 1:5
    particles(i) = particle
    particles(i).position = [parameters.leftBound+0.5*rand, parameters.topBound-0.5*rand];
    
    particles(i).velocity = [3 0];
    % Give each particle random velocity and position
    %%particles(i).position = ...
    %%    [rand*(parameters.rightBound-parameters.leftBound)+parameters.leftBound ...
    %%    rand*(parameters.topBound-parameters.bottomBound)+parameters.bottomBound];
    %%particles(i).velocity = 3*[(2*rand-1) (2*rand-1)];
end

%% Calculate Properties

i = 6;
frame = 1;

figure;
while frame <= frames
    if (length(particles) < n_particles)
        particles(i).position = [parameters.leftBound+0.5*rand, parameters.topBound-0.5*rand];
        particles(i+1).position = [parameters.leftBound+0.5*rand, parameters.topBound-0.5*rand];
        particles(i+2).position = [parameters.leftBound+0.5*rand, parameters.topBound-0.5*rand];
        particles(i).velocity = [7-rand*0.5 -rand];
        particles(i+1).velocity = [7-rand*0.5 -rand];
        particles(i+2).velocity = [7-rand*0.5 -rand];
    end
    tic
    particles = calculateForcesGrid(particles, parameters);
    toc
    particles = performTimestep(particles, parameters.dt);
    particles = checkBoundaries(particles, parameters);
    clf;
    hold on
    
    drawParticles(particles);

    axis([(parameters.leftBound-1) (parameters.rightBound+1) (parameters.bottomBound-1) (parameters.topBound+1)]);
    

    i = i + 3;

    pause(0.0001);

    % Render figure to PNG file
    print(['render/fluid_simulation_00' sprintf('%03d',frame)], '-dpng');
    frame = frame + 1;
end