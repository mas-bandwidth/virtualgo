#ifndef STONE_INSTANCE_H
#define STONE_INSTANCE_H

#include "StoneData.h"
#include "RigidBody.h"
#include <map>

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

        visualOffset = vec3f(0,0,0);
    }

    void UpdateVisualTransform()
    {
        if ( length_squared( visualOffset ) > 0.001f * 0.001f )
        {
            if ( selected )
                visualOffset *= 0.25f;
            else
                visualOffset *= 0.7f;

            visualTransform = rigidBody.rotation;
            visualTransform.value.w = simd4f_create( rigidBody.position.x() + visualOffset.x(),
                                                     rigidBody.position.y() + visualOffset.y(),
                                                     rigidBody.position.z() + visualOffset.z(), 1 );

        }
        else
        {
            visualOffset = vec3f(0,0,0);
            visualTransform = rigidBody.transform.localToWorld;
        }
    }

    uint32_t id : 16;
    uint32_t white : 1;
    uint32_t selected : 1;
    uint32_t constrained : 1;
    uint32_t constraintRow : 5;
    uint32_t constraintColumn : 5;

    float deleteTimer;

    vec3f constraintPosition;

    vec3f visualOffset;
    mat4f visualTransform;

    RigidBody rigidBody;
};

typedef std::map<uint16_t,int> StoneMap;

StoneInstance * FindStoneInstance( uint16_t id, std::vector<StoneInstance> & stones, StoneMap & stoneMap )
{
    StoneMap::iterator itor = stoneMap.find( id );
    if ( itor == stoneMap.end() )
        return NULL;
    const int stoneIndex = itor->second;
    return &stones[stoneIndex];
}

#endif
