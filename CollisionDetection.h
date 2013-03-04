#ifndef COLLISION_DETECTION_H
#define COLLISION_DETECTION_H

#include "Board.h"
#include "Biconvex.h"
#include "Intersection.h"
#include <stdint.h>

// --------------------------------------------------------------------------

struct StaticContact
{
    RigidBody * rigidBody;
    vec3f point;
    vec3f normal;
    float depth;
};

struct DynamicContact
{
    RigidBody * a;
    RigidBody * b;
    vec3f point;
    vec3f normal;
};

inline bool StoneBoardCollision( const Biconvex & biconvex,
                                 const Board & board, 
                                 RigidBody & rigidBody,
                                 StaticContact & contact );

inline bool StoneFloorCollision( const Biconvex & biconvex,
                                 const Board & board, 
                                 RigidBody & rigidBody,
                                 StaticContact & contact );

// --------------------------------------------------------------------------

inline void ClosestFeaturesBiconvexPlane_LocalSpace( vec3f planeNormal,
                                                     float planeDistance,
                                                     const Biconvex & biconvex,
                                                     vec3f & biconvexPoint,
                                                     vec3f & biconvexNormal,
                                                     vec3f & planePoint )
{
    const float sphereDot = biconvex.GetSphereDot();
    const float planeNormalDot = fabs( dot( vec3f(0,1,0), planeNormal ) );
    if ( planeNormalDot > sphereDot )
    {
        // sphere surface collision
        const float sphereRadius = biconvex.GetSphereRadius();
        const float sphereOffset = planeNormal.y() < 0 ? -biconvex.GetSphereOffset() : +biconvex.GetSphereOffset();
        vec3f sphereCenter( 0, sphereOffset, 0 );
        biconvexPoint = sphereCenter - normalize( planeNormal ) * sphereRadius;
        biconvexNormal = normalize( biconvexPoint - sphereCenter );
    }
    else
    {
        // circle edge collision
        const float circleRadius = biconvex.GetCircleRadius();
        biconvexPoint = normalize( vec3f( -planeNormal.x(), 0, -planeNormal.z() ) ) * circleRadius;
        biconvexNormal = normalize( biconvexPoint );
    }

    planePoint = biconvexPoint - planeNormal * ( dot( biconvexPoint, planeNormal ) - planeDistance );
}

inline void ClosestFeaturesStoneBoard( const Board & board, 
                                       const Biconvex & biconvex, 
                                       const RigidBodyTransform & biconvexTransform,
                                       vec3f & stonePoint,
                                       vec3f & stoneNormal,
                                       vec3f & boardPoint,
                                       vec3f & boardNormal )
{
    vec3f biconvexPosition = biconvexTransform.GetPosition();

    const float boundingSphereRadius = biconvex.GetWidth() * 0.5f;

    StoneBoardCollisionType collisionType = DetermineStoneBoardCollisionType( board, biconvexPosition, boundingSphereRadius );

    const float thickness = board.GetThickness();

    //if ( collisionType == STONE_BOARD_COLLISION_Primary )
    {
        // common case: collision with primary surface of board only
        // no collision with edges or corners of board is possible

        vec4f plane = TransformPlane( biconvexTransform.worldToLocal, vec4f(0,1,0,thickness) );

        vec3f local_stonePoint;
        vec3f local_stoneNormal;
        vec3f local_boardPoint;

        ClosestFeaturesBiconvexPlane_LocalSpace( vec3f( plane.x(), plane.y(), plane.z() ), plane.w(), 
                                                 biconvex, 
                                                 local_stonePoint,
                                                 local_stoneNormal,
                                                 local_boardPoint );

        stonePoint = TransformPoint( biconvexTransform.localToWorld, local_stonePoint );
        stoneNormal = TransformVector( biconvexTransform.localToWorld, local_stoneNormal );
        boardPoint = TransformPoint( biconvexTransform.localToWorld, local_boardPoint );
        boardNormal = vec3f(0,1,0);
    }
    /*
    else if ( collisionType == STONE_BOARD_COLLISION_LeftSide )
    {
        // todo
    }
    else
    {
        // not implemented yet!
    }
    */
}

// -----------------------------------------------------------------------

bool StoneBoardCollision( const Biconvex & biconvex,
                          const Board & board, 
                          RigidBody & rigidBody,
                          StaticContact & contact )
{
    // detect collision with the board

    float depth;
    vec3f point, normal;
    if ( !IntersectStoneBoard( board, biconvex, RigidBodyTransform( rigidBody.position, rigidBody.orientation ), point, normal, depth ) )
        return false;

    // project the stone out of collision with the board

    rigidBody.position += normal * depth;

    // fill the contact information for the caller

    vec3f stonePoint, stoneNormal, boardPoint, boardNormal;

    ClosestFeaturesStoneBoard( board, biconvex, 
                               RigidBodyTransform( rigidBody.position, rigidBody.orientation ), 
                               stonePoint, stoneNormal, boardPoint, boardNormal );

    contact.rigidBody = &rigidBody;
    contact.point = boardPoint;
    contact.normal = boardNormal;
    contact.depth = depth;

    return true;
}

bool StoneFloorCollision( const Biconvex & biconvex,
                          const Board & board, 
                          RigidBody & rigidBody,
                          StaticContact & contact )
{
    const float boundingSphereRadius = biconvex.GetBoundingSphereRadius();

    vec3f biconvexPosition = rigidBody.position;

    RigidBodyTransform biconvexTransform( rigidBody.position, rigidBody.orientation );

    float s1,s2;
    vec3f biconvexUp = biconvexTransform.GetUp();
    vec3f biconvexCenter = biconvexTransform.GetPosition();
    BiconvexSupport_WorldSpace( biconvex, biconvexCenter, biconvexUp, vec3f(0,1,0), s1, s2 );
    
    if ( s1 > 0 )
        return false;

    float depth = -s1;

    rigidBody.position += vec3f(0,depth,0);

    vec4f plane = TransformPlane( biconvexTransform.worldToLocal, vec4f(0,1,0,0) );

    vec3f local_stonePoint;
    vec3f local_stoneNormal;
    vec3f local_floorPoint;

    ClosestFeaturesBiconvexPlane_LocalSpace( vec3f( plane.x(), plane.y(), plane.z() ), 
                                             plane.w(), 
                                             biconvex, 
                                             local_stonePoint,
                                             local_stoneNormal,
                                             local_floorPoint );

    contact.rigidBody = &rigidBody;
    contact.point = TransformPoint( biconvexTransform.localToWorld, local_floorPoint );
    contact.normal = vec3f(0,1,0);
    contact.depth = depth;

    return true;
}

// -----------------------------------------------------------------------

#endif
