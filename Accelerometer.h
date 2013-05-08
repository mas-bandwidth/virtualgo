#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

#include "Config.h"

class Accelerometer
{
public:

	Accelerometer()
	{
	    rawAcceleration = vec3f(0,0,-1);
	    smoothedAcceleration = vec3f(0,0,-1);
	    jerkAcceleration = vec3f(0,0,0);
	    up = vec3f(0,0,1);
	    down = -up;
	}

	void Update( const vec3f & inputAcceleration )
	{
	    rawAcceleration = inputAcceleration;
	    
	    smoothedAcceleration += ( rawAcceleration - smoothedAcceleration ) * AccelerometerTightness;
	   	jerkAcceleration = rawAcceleration - smoothedAcceleration;

	   	if ( length_squared( smoothedAcceleration ) > 0.00001f )
	   		up = normalize( -smoothedAcceleration );
	   	else
	   		up = vec3f(0,0,1);

	   	down = -up;
	}

	const vec3f & GetRawAcceleration() const
	{
		return rawAcceleration;
	}

	const vec3f & GetSmoothedAcceleration() const
	{
		return smoothedAcceleration;
	}

	const vec3f & GetJerkAcceleration() const
	{
		return jerkAcceleration;
	}

	const vec3f & GetUp() const
	{
		return up;
	}

	const vec3f & GetDown() const
	{
		return down;
	}

private:

    vec3f rawAcceleration;                 // raw data from the accelometer
    vec3f smoothedAcceleration;            // smoothed acceleration = gravity
    vec3f jerkAcceleration;                // jerk acceleration = short motions, above a threshold = bump the board
    vec3f up;							   // normalized up direction
    vec3f down;							   // down vector. always -up
};

#endif
