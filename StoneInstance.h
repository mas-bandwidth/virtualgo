#ifndef STONE_INSTANCE_H
#define STONE_INSTANCE_H

#include "StoneData.h"
#include "RigidBody.h"

struct StoneInstance
{
	void Initialize( const StoneData & stoneData, uint32_t id = 0, bool white = true )
	{
        this->id = id;
        this->white = white;
        this->selected = 0;
        this->constrained = 0;
        this->constraintRow = 0;
        this->constraintColumn = 0;

        rigidBody.mass = stoneData.mass;
        rigidBody.inverseMass = 1.0f / stoneData.mass;
        rigidBody.inertia = stoneData.inertia;
        rigidBody.inertiaTensor = stoneData.inertiaTensor;
        rigidBody.inverseInertiaTensor = stoneData.inverseInertiaTensor;

        deleteTimer = 0.0f;
    }

    uint32_t id : 16;
    uint32_t white : 1;
    uint32_t selected : 1;
    uint32_t constrained : 1;
    uint32_t constraintRow : 5;
    uint32_t constraintColumn : 5;

    float deleteTimer;

    vec3f constraintPosition;

    RigidBody rigidBody;
};

StoneInstance * FindStoneInstance( uint16_t id, std::vector<StoneInstance> & stones )
{
    // todo: need a fast version of this function
    for ( int i = 0; i < stones.size(); ++i )
    {
        if ( stones[i].id == id ) 
            return &stones[i];
    }
    return NULL;
}

#endif
