#ifndef COLLISION_H
#define COLLISION_H

#include "Board.h"
#include <stdint.h>

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
    STONE_BOARD_COLLISION_None = 0xFFFFFFFF,             // no collision is possible (uncommon, stones are mostly on the board...)
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
    const float y = position.y();
    if ( y > radius )
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

    // todo: we need a new type of determine board type for this case
    // because we don't want to filter away "none" if we are too far
    // away to collide, because we want the closest feature to work
    // even if we are not colliding!

    /*
    StoneBoardCollisionType collisionType = DetermineStoneBoardCollisionType( board, biconvexPosition, boundingSphereRadius );

    if ( collisionType == STONE_BOARD_COLLISION_Primary )
    {
    */

        // common case: collision with primary surface of board only
        // no collision with edges or corners of board is possible

        vec4f plane = TransformPlane( biconvexTransform.worldToLocal, vec4f(0,1,0,0) );

        vec3f local_stonePoint;
        vec3f local_stoneNormal;
        vec3f local_boardPoint;

        ClosestFeaturesBiconvexPlane_LocalSpace( vec3f( plane.x(), plane.y(), plane.z() ), 
                                                 plane.w(), 
                                                 biconvex, 
                                                 local_stonePoint,
                                                 local_stoneNormal,
                                                 local_boardPoint );

        stonePoint = TransformPoint( biconvexTransform.localToWorld, local_stonePoint );
        stoneNormal = TransformVector( biconvexTransform.localToWorld, local_stoneNormal );
        boardPoint = TransformPoint( biconvexTransform.localToWorld, local_boardPoint );
        boardNormal = vec3f(0,1,0);

    /*
    }
    else
    {
        // not implemented yet!
        assert( false );
    }
    */
}

#endif
