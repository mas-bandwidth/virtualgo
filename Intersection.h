#ifndef INTERSECTION_H
#define INTERSECTION_H

#include "Board.h"

inline bool IntersectRayPlane( vec3f rayStart,
                               vec3f rayDirection,
                               vec3f planeNormal,
                               float planeDistance,
                               float & t,
                               float epsilon = 0.001f )
{
    // IMPORTANT: we only consider intersections *in front* the ray start, eg. t >= 0
    const float d = dot( rayDirection, planeNormal );
    if ( d > -epsilon )
        return false;
    t = - ( dot( rayStart, planeNormal ) + planeDistance ) / d;
    assert( t >= 0 );
    return true;
}

inline bool IntersectRaySphere( vec3f rayStart,
                                vec3f rayDirection, 
                                vec3f sphereCenter, 
                                float sphereRadius, 
                                float sphereRadiusSquared, 
                                float & t )
{
    vec3f delta = sphereCenter - rayStart;
    const float distanceSquared = dot( delta, delta );
    const float timeClosest = dot( delta, rayDirection );
    if ( timeClosest < 0 )
        return false;                   // ray points away from sphere
    const float timeHalfChordSquared = sphereRadiusSquared - distanceSquared + timeClosest*timeClosest;
    if ( timeHalfChordSquared < 0 )
        return false;                   // ray misses sphere
    t = timeClosest - sqrt( timeHalfChordSquared );
    if ( t < 0 )
        return false;                   // ray started inside sphere. we only want one sided collisions from outside of sphere
    return true;
}

inline bool IntersectRayBiconvex_LocalSpace( vec3f rayStart, 
                                             vec3f rayDirection, 
                                             const Biconvex & biconvex,
                                             float & t, 
                                             vec3f & point, 
                                             vec3f & normal )
{
    const float sphereOffset = biconvex.GetSphereOffset();
    const float sphereRadius = biconvex.GetSphereRadius();
    const float sphereRadiusSquared = biconvex.GetSphereRadiusSquared();

    // intersect ray with bottom sphere
    vec3f bottomSphereCenter( 0, -sphereOffset, 0 );
    if ( IntersectRaySphere( rayStart, rayDirection, bottomSphereCenter, sphereRadius, sphereRadiusSquared, t ) )
    {
        point = rayStart + rayDirection * t;
        if ( point.y() >= 0 )
        {
            normal = normalize( point - bottomSphereCenter );
            return true;
        }        
    }

    // intersect ray with top sphere
    vec3f topSphereCenter( 0, sphereOffset, 0 );
    if ( IntersectRaySphere( rayStart, rayDirection, topSphereCenter, sphereRadius, sphereRadiusSquared, t ) )
    {
        point = rayStart + rayDirection * t;
        if ( point.y() <= 0 )
        {
            normal = normalize( point - topSphereCenter );
            return true;
        }
    }

    return false;
}

inline float IntersectPlaneBiconvex_LocalSpace( vec3f planeNormal,
                                                float planeDistance,
                                                const Biconvex & biconvex,
                                                vec3f & point,
                                                vec3f & normal )
{
    const float sphereDot = biconvex.GetSphereDot();
    const float planeNormalDot = fabs( dot( vec3f(0,1,0), planeNormal ) );
    if ( planeNormalDot > sphereDot )
    {
        // sphere surface collision
        const float sphereRadius = biconvex.GetSphereRadius();
        const float sphereOffset = planeNormal.y() < 0 ? -biconvex.GetSphereOffset() : +biconvex.GetSphereOffset();
        vec3f sphereCenter( 0, sphereOffset, 0 );
        point = sphereCenter - normalize( planeNormal ) * sphereRadius;
    }
    else
    {
        // circle edge collision
        const float circleRadius = biconvex.GetCircleRadius();
        point = normalize( vec3f( -planeNormal.x(), 0, -planeNormal.z() ) ) * circleRadius;
    }

    normal = planeNormal;

    return dot( -planeNormal, point ) + planeDistance;
}

inline float IntersectRayStone( const Biconvex & biconvex, 
                                const RigidBodyTransform & biconvexTransform,
                                vec3f rayStart, 
                                vec3f rayDirection, 
                                vec3f & point, 
                                vec3f & normal )
{
    vec3f local_rayStart = TransformPoint( biconvexTransform.worldToLocal, rayStart );
    vec3f local_rayDirection = TransformVector( biconvexTransform.worldToLocal, rayDirection );
    
    vec3f local_point, local_normal;

    float t;

    bool result = IntersectRayBiconvex_LocalSpace( local_rayStart, 
                                                   local_rayDirection,
                                                   biconvex,
                                                   t,
                                                   local_point,
                                                   local_normal );

    if ( result )
    {
        point = TransformPoint( biconvexTransform.localToWorld, local_point );
        normal = TransformVector( biconvexTransform.localToWorld, local_normal );
        return t;
    }

    return -1;
}

inline float IntersectRayBoard( const Board & board,
                                vec3f rayStart,
                                vec3f rayDirection,
                                vec3f & point,
                                vec3f & normal,
                                float epsilon = 0.001f )
{ 
    // first check with the primary surface
    // statistically speaking this is the most likely
    {
        float t;
        if ( IntersectRayPlane( rayStart, rayDirection, vec3f(0,1,0), 0, t, epsilon ) )
        {
            point = rayStart + rayDirection * t;
            normal = vec3f(0,1,0);
            const float w = board.GetHalfWidth();
            const float h = board.GetHalfHeight();
            const float px = point.x();
            const float pz = point.z();
            if ( px >= -w && px <= w && pz >= -h && pz <= h )
                return t;
        }
    }

    // todo: other cases
    // left side, right side, top side, bottom side

    return -1;
}

enum BoardEdges
{
    BOARD_EDGE_None = 0,
    BOARD_EDGE_Left = 1,
    BOARD_EDGE_Top = 2,
    BOARD_EDGE_Right = 4,
    BOARD_EDGE_Bottom = 8
};

enum StoneBoardCollisionType
{
    STONE_BOARD_COLLISION_None = 0xFFFFFFFF,             // not colliding with the board
    STONE_BOARD_COLLISION_Primary = BOARD_EDGE_None,     // common case: collision with the primary surface (the plane at y = 0)
    STONE_BOARD_COLLISION_LeftSide = BOARD_EDGE_Left,
    STONE_BOARD_COLLISION_TopSide = BOARD_EDGE_Top,
    STONE_BOARD_COLLISION_RightSide = BOARD_EDGE_Right,
    STONE_BOARD_COLLISION_BottomSide = BOARD_EDGE_Bottom,
    STONE_BOARD_COLLISION_TopLeftCorner = BOARD_EDGE_Top | BOARD_EDGE_Left,
    STONE_BOARD_COLLISION_TopRightCorner = BOARD_EDGE_Top | BOARD_EDGE_Right,
    STONE_BOARD_COLLISION_BottomRightCorner = BOARD_EDGE_Bottom | BOARD_EDGE_Right,
    STONE_BOARD_COLLISION_BottomLeftCorner = BOARD_EDGE_Bottom | BOARD_EDGE_Left
};

inline StoneBoardCollisionType DetermineStoneBoardCollisionType( const Board & board, vec3f position, float radius )
{
    // stone is above board surface by more than the radius
    // of the bounding sphere, no collision is possible!
    const float thickness = board.GetThickness();
    const float y = position.y();
    if ( y > thickness + radius )
        return STONE_BOARD_COLLISION_None;

    // some collision is possible, determine whether we are potentially
    // colliding width the edges of the board. the common case is that we are not!

    const float x = position.x();
    const float z = position.z();

    const float w = board.GetHalfWidth();
    const float h = board.GetHalfHeight();
    const float r = radius;

    uint32_t edges = BOARD_EDGE_None;

    // todo: we can optimize this and cache the various
    // min/max bounds per-axis because multiple stones
    // with the same bounding radius are colliding with
    // the same board every frame

    if ( x <= -w + r )                            // IMPORTANT: assumption that the board width/height is large 
        edges |= BOARD_EDGE_Left;                 // relative to the bounding sphere, eg. that only one corner
    else if ( x >= w - r )                        // would potentially be intersecting with a stone at any time
        edges |= BOARD_EDGE_Right;

    if ( z <= -h + r )
        edges |= BOARD_EDGE_Top;
    else if ( z >= h - r )
        edges |= BOARD_EDGE_Bottom;

    // common case: stone bounding sphere is entirely within the primary
    // surface and cannot intersect with corners or edges of the board
    if ( edges == 0 )
        return STONE_BOARD_COLLISION_Primary;

    // rare case: no collision if the stone is further than the bounding
    // sphere radius from the sides of the board along the x or z axes.
    if ( x < -w - r || x > w + r || z < -h - r || z > h + r )
        return STONE_BOARD_COLLISION_None;

    // otherwise: the edge bitfield maps to the set of collision cases
    // these collision cases indicate which sides and corners need to be
    // tested in addition to the primary surface.
    return (StoneBoardCollisionType) edges;
}

inline bool IntersectStoneBoard( const Board & board, 
                                 const Biconvex & biconvex, 
                                 const RigidBodyTransform & biconvexTransform,
                                 float epsilon = 0.0001f )
{
    const float boundingSphereRadius = biconvex.GetBoundingSphereRadius();

    vec3f biconvexPosition = biconvexTransform.GetPosition();

    StoneBoardCollisionType collisionType = DetermineStoneBoardCollisionType( board, biconvexPosition, boundingSphereRadius );

    const float thickness = board.GetThickness();

    if ( collisionType == STONE_BOARD_COLLISION_Primary )
    {
        float s1,s2;
        vec3f biconvexUp = biconvexTransform.GetUp();
        vec3f biconvexCenter = biconvexTransform.GetPosition();
        BiconvexSupport_WorldSpace( biconvex, biconvexCenter, biconvexUp, vec3f(0,1,0), s1, s2 );
        return s1 <= thickness;
    }

    // todo: other cases

    return false;
}

inline bool IntersectStoneBoard( const Board & board, 
                                 const Biconvex & biconvex, 
                                 const RigidBodyTransform & biconvexTransform,
                                 vec3f & point,
                                 vec3f & normal,
                                 float & depth,
                                 float epsilon = 0.0001f )
{
    const float boundingSphereRadius = biconvex.GetBoundingSphereRadius();

    vec3f biconvexPosition = biconvexTransform.GetPosition();

    StoneBoardCollisionType collisionType = DetermineStoneBoardCollisionType( board, biconvexPosition, boundingSphereRadius );

    const float thickness = board.GetThickness();

    if ( collisionType == STONE_BOARD_COLLISION_Primary )
    {
        // common case: collision with primary surface of board only
        // no collision with edges or corners of board is possible

        vec4f plane = TransformPlane( biconvexTransform.worldToLocal, vec4f(0,1,0,thickness) );

        vec3f local_point;
        vec3f local_normal;
        depth = IntersectPlaneBiconvex_LocalSpace( vec3f( plane.x(), plane.y(), plane.z() ), 
                                                   plane.w(), biconvex, local_point, local_normal );
        if ( depth >= 0.0f )
        {
            point = TransformPoint( biconvexTransform.localToWorld, local_point );
            normal = TransformVector( biconvexTransform.localToWorld, local_normal );
            return true;
        }
    }
    /*
    else if ( collisionType == STONE_BOARD_COLLISION_LeftSide )
    {
        assert( false );
    }
    else if ( collisionType == STONE_BOARD_COLLISION_RightSide )
    {
        assert( false );
    }
    else if ( collisionType == STONE_BOARD_COLLISION_TopSide )
    {
        assert( false );
    }
    else if ( collisionType == STONE_BOARD_COLLISION_BottomSide )
    {
        assert( false );
    }
    else if ( collisionType == STONE_BOARD_COLLISION_TopLeftCorner )
    {
        assert( false );
    }
    else if ( collisionType == STONE_BOARD_COLLISION_TopRightCorner )
    {
        assert( false );
    }
    else if ( collisionType == STONE_BOARD_COLLISION_BottomRightCorner )
    {
        assert( false );
    }
    else if ( collisionType == STONE_BOARD_COLLISION_BottomLeftCorner )
    {
        assert( false );
    }
    */

    return false;
}

#endif
