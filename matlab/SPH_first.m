%% Initiate
% Here you can forget your past, just like "Men In Black"
clear all
close all
clc

parameters = struct(...
    'dt',0.05, ...
    'mass',1, ...
    'kernelSize',1, ...
    'gasConstantK',1, ...
    'viscosityConstant', 1, ...
    'restDensity', 0, ...
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
figure
i = 6;
while true

    particles(i).position = [parameters.leftBound+0.5*rand, parameters.topBound-0.5*rand];
    particles(i+1).position = [parameters.leftBound+0.5*rand, parameters.topBound-0.5*rand];
    particles(i+2).position = [parameters.leftBound+0.5*rand, parameters.topBound-0.5*rand];
    particles(i).velocity = [7-rand*0.5 -rand];
    particles(i+1).velocity = [7-rand*0.5 -rand];
    particles(i+2).velocity = [7-rand*0.5 -rand];

    
    particles = calculateForces(particles, parameters);
    particles = performTimestep(particles, parameters.dt);
    particles = checkBoundaries(particles, parameters);
    clf;
    hold on
    
    drawParticles(particles);

    axis([(parameters.leftBound-1) (parameters.rightBound+1) (parameters.bottomBound-1) (parameters.topBound+1)]);
    

    i = i + 3;

    pause(0.01);
end