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
    'viscosityConstant',1, ...
    'restDensity', 0, ...
    'leftBound', 0, ...
    'rightBound', 10, ...
    'bottomBound', 0, ...
    'topBound', 10);

%% Struct
field1 = 'f'; value1 = 'some text';
s = struct(field1,value1)

%% One Particle

field1 = 'density'; value1 = 1.2;
field2 = 'position'; value2 = [1.0 2.0];
field3 = 'velocity'; value3 = [0.0 0.0];
field4 = 'pressure'; value4 = 0.0;
field5 = 'force'; value5 = [0.0 0.0];
% Surface tension?

particle = struct(...
    field1,value1, ...
    field2,value2, ...
    field3,value3, ...
    field4,value4, ...
    field5,value5);

%% Several Particles

for i = 1:50
    particles(i) = particle;
    
    % Give each particle random velocity and position
    particles(i).position = [rand*10 rand*10];
    particles(i).velocity = 3*[(2*rand-1) (2*rand-1)];
end

%% Draw Particles

%drawParticles(particles);

%% Calculate Properties
figure;
while true
    %tic;
    
    particles = calculateForces(particles, parameters);
    particles = performTimestep(particles, parameters.dt);
    particles = checkBoundaries(particles, parameters);
    clf;
    hold on
    
    drawParticles(particles);
    axis([-1 11 -1 11]);
    
    %toc
    pause(0.0001);
end