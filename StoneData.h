#ifndef STONE_DATA_H
#define STONE_DATA_H

#include "Biconvex.h"
#include "InertiaTensor.h"

// stone sizes from http://www.kurokigoishi.co.jp/english/seihin/goishi/index.html

enum StoneSize
{
    STONE_SIZE_22,
    STONE_SIZE_25,
    STONE_SIZE_28,
    STONE_SIZE_30,
    STONE_SIZE_31,
    STONE_SIZE_32,
    STONE_SIZE_33,
    STONE_SIZE_34,
    STONE_SIZE_35,
    STONE_SIZE_36,
    STONE_SIZE_37,
    STONE_SIZE_38,
    STONE_SIZE_39,
    STONE_SIZE_40,
    STONE_SIZE_NumValues
};

const float StoneHeight[] = 
{
    0.63f,
    0.70f,
    0.75f,
    0.80f,
    0.84f,
    0.88f,
    0.92f,
    0.95f,
    0.98f,
    1.01f,
    1.04f,
    1.07f,
    1.10f,
    1.13f
};

inline float GetStoneWidth( StoneSize stoneSize, bool black = false )
{
	return 2.2f + 0.2f; //+ ( black ? 0.3f : 0 );
}

inline float GetStoneHeight( StoneSize stoneSize, bool black = false )
{
	return StoneHeight[stoneSize];// + ( black ? 0.3f : 0 );
}

struct StoneData
{
    void Initialize( StoneSize stoneSize, 
                     float bevel = 0.1f, 
                     float mass = 1.0f, 
                     bool black = false )
    {
        this->mass = mass;
        biconvex = Biconvex( GetStoneWidth( stoneSize, black ), GetStoneHeight( stoneSize, black ), bevel );
        CalculateBiconvexInertiaTensor( mass, biconvex, inertia, inertiaTensor, inverseInertiaTensor );
    }

    Biconvex biconvex;

    float mass;

    vec3f inertia;
    mat4f inertiaTensor;
    mat4f inverseInertiaTensor;
};

inline vec3f NearestPointOnStone( const Biconvex & biconvex, 
                                  const RigidBodyTransform & biconvexTransform, 
                                  const vec3f & point )
{
	vec3f localPoint = TransformPoint( biconvexTransform.worldToLocal, point );
    vec3f nearestLocal = GetNearestPointOnBiconvexSurface_LocalSpace( localPoint, biconvex );
    return TransformPoint( biconvexTransform.localToWorld, nearestLocal );
}

#endif
