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

        rigidBody.mass = stoneData.mass;
        rigidBody.inverseMass = 1.0f / stoneData.mass;
        rigidBody.inertia = stoneData.inertia;
        rigidBody.inertiaTensor = stoneData.inertiaTensor;
        rigidBody.inverseInertiaTensor = stoneData.inverseInertiaTensor;
    }

    uint32_t id : 16;
    uint32_t white : 1;
    uint32_t selected : 1;

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