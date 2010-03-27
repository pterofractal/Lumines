#include "particle.hpp"
#include <iostream>

Particle::Particle(Point3D position, float radius, Vector3D velocity, float d, float *col, Vector3D acceleration, int shape)
{
	pos = position;
	rad = radius;
	vel = velocity;
	decay = d;
	colour = col;
		this->shape = shape;
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

void Particle::setColourIndex(int colIndex)
{
	colourIndex = colIndex;
}

int Particle::getColourIndex()
{
	return colourIndex;
}
float* Particle::getColour()
{
	return colour;
}

int Particle::getShape()
{
	return shape;
}

Particle::~Particle()
{
	
}