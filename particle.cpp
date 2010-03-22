#include "particle.hpp"
#include <iostream>

Particle::Particle(Point3D position, float rad, Vector3D vel, float d, int colIndex)
{
	pos = position;
	radius = rad;
	velocity = vel;
	decay = d;
	colourIndex = colIndex;
	// Defaults
	alpha = 1.f;
//	accel = 0.f;
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
	return radius;
}

Vector3D Particle::getVelocity()
{
	return velocity;
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
	pos = pos + t * velocity;

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