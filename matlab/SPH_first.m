%% Initiate
% Here you can forget your past, just like "Men In Black"
clear all
close all
clc

%% Struct
field1 = 'f'; value1 = 'some text';
s = struct(field1,value1)

%% One Particle

field1 = 'mass'; value1 = 1.0;
field2 = 'density'; value2 = 1.2;
field3 = 'viscosity'; value3 = 3.2;
field4 = 'location'; value4 = [1.0 2.0];
field5 = 'velocity'; value5 = [0.0 0.0];
% Surface tension?

particle = struct(...
    field1,value1, ...
    field2,value2, ...
    field3,value3, ...
    field4,value4, ...
    field5,value5);

disp(['mass: ' num2str(particle.mass)]);

%% Several Particles

for i = 1:100
   particles(i) = particle; 
   
   % Give each particle random velocity and position
   particles(i).location = [rand*10 rand*10];
   particles(i).velocity = [rand rand];
end

%% Draw Particles

for j = 1:length(particles)
    pos = particles(j).location;
    plot(pos(1), pos(2), 'ob');
    axis([0 10 0 10]);
    hold on
end

%% Calculate Properties
