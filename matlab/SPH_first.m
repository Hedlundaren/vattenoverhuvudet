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
    'gasConstantK',1, ...
    'viscosityConstant', 5, ...
    'restDensity', 0, ...
    'sigma', 72e-3, ...
    'nThreshold', 0.02, ...
    'gravity', [0 -9.82], ...
    'leftBound', 0, ...
    'rightBound', 2, ...
    'bottomBound', 0, ...
    'topBound', 5, ...
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


for i = 1:50
    particles(i) = particle;
    particles(i).position = [parameters.leftBound+rand*(parameters.rightBound - parameters.leftBound), ...
                             parameters.bottomBound+0.5*rand];
    particles(i).velocity = [0 0];
end

for i = 51: 70
    particles(i) = particle;
    particles(i).position = [ 0.6*rand - 0.3 + (parameters.leftBound + parameters.rightBound)/2, parameters.topBound - 0.5*rand ];
    particles(i).velocity = [0 -5];
end

%% Calculate Properties
figure;
while true
    1+1
    tic;

    particles = calculateForces(particles, parameters);
    toc;
    tic;
    particles = performTimestep(particles, parameters.dt);
    toc;
    tic;
    particles = checkBoundaries(particles, parameters);
    toc;
    clf;
    hold on
    
    drawParticles(particles);

    axis([(parameters.leftBound-1) (parameters.rightBound+1) (parameters.bottomBound-1) (parameters.topBound+1)]);

    pause(0.0001);
end