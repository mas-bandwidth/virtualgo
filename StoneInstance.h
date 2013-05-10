#ifndef STONE_INSTANCE_H
#define STONE_INSTANCE_H

#include "StoneData.h"
#include "RigidBody.h"

struct StoneInstance
{
	void Initialize( const StoneData & stoneData, bool white = true )
	{
        this->white = white;
        rigidBody.mass = stoneData.mass;
        rigidBody.inverseMass = 1.0f / stoneData.mass;
        rigidBody.inertia = stoneData.inertia;
        rigidBody.inertiaTensor = stoneData.inertiaTensor;
        rigidBody.inverseInertiaTensor = stoneData.inverseInertiaTensor;
    }

    bool white;
    RigidBody rigidBody;
};

#endif
