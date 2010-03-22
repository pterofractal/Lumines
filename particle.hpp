#ifndef PARTICLE_HPP
#define PARTICLE_HPP

#include <gtkmm.h>
#include "algebra.hpp"

class Particle 
{
	public:
		Particle();
		Particle(Point3D pos, float radius, Vector3D velocity, float decay, int colIndex);
		virtual ~Particle();
		
		float getDecay();
		Point3D getPos();
		float getRadius();
		Vector3D getVelocity();	
		float getAlpha();
		void setAlpha();
		void setColour(int colIndex);
		int getColour();
		bool step(float t);
	protected:

	private:
	float decay;
	Point3D pos;
	float radius;
	Vector3D velocity;
	Vector3D accel;
	float alpha;
	int colourIndex;
};
#endif
