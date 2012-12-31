#ifndef STONE_H
#define STONE_H

#include "Biconvex.h"
#include "RigidBody.h"

inline vec3f NearestPointOnStone( const Biconvex & biconvex, 
                                  const RigidBodyTransform & biconvexTransform, 
                                  vec3f point )
{
    vec3f nearest_local = GetNearestPointOnBiconvexSurface_LocalSpace( TransformPoint( biconvexTransform.worldToLocal, point ), biconvex );
    return TransformPoint( biconvexTransform.localToWorld, nearest_local );
}

#endif
