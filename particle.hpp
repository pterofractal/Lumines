#ifndef PARTICLE_HPP
#define PARTICLE_HPP

#include <gtkmm.h>
#include "algebra.hpp"

class Particle 
{
	public:
		Particle();
		Particle(Point3D pos, float radius, Vector3D velocity, float decay, float *col, Vector3D acceleration, int shape = 0);
		virtual ~Particle();
		
		float getDecay();
		Point3D getPos();
		float getRadius();
		Vector3D getVelocity();	
		float getAlpha();
		void setAlpha();
		void setColourIndex(int colIndex);
		int getColourIndex();
		float* getColour();
		bool step(float t);
		int getShape();
		
		
		
	protected:

	private:

	float *colour;
	float decay;
	Point3D pos;
	float rad;
	Vector3D vel;
	Vector3D accel;
	float alpha;
	int colourIndex;
	int shape;
};
#endif
