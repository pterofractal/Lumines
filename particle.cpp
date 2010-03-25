#include "particle.hpp"
#include <iostream>

Particle::Particle(Point3D position, float radius, Vector3D velocity, float d, int colIndex, Vector3D acceleration)
{
	pos = position;
	rad = radius;
	vel = velocity;
	decay = d;
	colourIndex = colIndex;
	// Defaults
	alpha = 1.f;
	accel = acceleration;
}

float Particle::getDecay()
{
	return decay;
}

Point3D Particle::getPos()
{
	return pos;
}

float Particle::getRadius()
{
	return rad;
}

Vector3D Particle::getVelocity()
{
	return vel;
}

float Particle::getAlpha()
{
	return alpha;
}

bool Particle::step(float t)
{
	// Kill particle
	if (decay < 0)
		return true;
		
	// Move particle forward
	vel = vel + t * accel;
	pos = pos + t * vel;

	// Decay defines how long the particle will be alive for
	decay -= t; 
	
	// Make particle transparent as it dies
	alpha = 1 - 1/decay;
	
	if (decay <= 0)
		return true;
		
	// keep particle alive
	return false;
}

void Particle::setColour(int colIndex)
{
	colourIndex = colIndex;
}

int Particle::getColour()
{
	return colourIndex;
}
Particle::~Particle()
{
	
}