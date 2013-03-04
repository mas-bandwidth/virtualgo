#include "Common.h"
#include "Biconvex.h"
#include "RigidBody.h"
#include "Intersection.h"
#include "InertiaTensor.h"
#include "CollisionDetection.h"

#include "UnitTest++/UnitTest++.h"
#include "UnitTest++/TestRunner.h"
#include "UnitTest++/TestReporterStdout.h"

#define CHECK_CLOSE_VEC3( value, expected, epsilon ) CHECK_CLOSE( length( value - expected ), 0.0f, epsilon )

SUITE( Intersection )
{
    TEST( stone_board_collision_type )
    {
        Board board( 9 );

        const float w = board.GetWidth() * 0.5f;
        const float h = board.GetHeight() * 0.5f;
        const float t = board.GetThickness();

        const float radius = 1.0f;

        bool broadPhaseReject = false;

        CHECK( DetermineStoneBoardRegion( board, vec3f(0,0,0), radius, broadPhaseReject ) == STONE_BOARD_REGION_Primary );
        CHECK( broadPhaseReject == false );

        CHECK( DetermineStoneBoardRegion( board, vec3f(0,-100,0), radius, broadPhaseReject ) == STONE_BOARD_REGION_Primary );
        CHECK( broadPhaseReject == false );

        CHECK( DetermineStoneBoardRegion( board, vec3f(-w,0,0), radius, broadPhaseReject ) == STONE_BOARD_REGION_LeftSide );
        CHECK( broadPhaseReject == false );
        
        CHECK( DetermineStoneBoardRegion( board, vec3f(+w,0,0), radius, broadPhaseReject ) == STONE_BOARD_REGION_RightSide );
        CHECK( broadPhaseReject == false );
        
        CHECK( DetermineStoneBoardRegion( board, vec3f(0,0,-h), radius, broadPhaseReject ) == STONE_BOARD_REGION_TopSide );
        CHECK( broadPhaseReject == false );
        
        CHECK( DetermineStoneBoardRegion( board, vec3f(0,0,+h), radius, broadPhaseReject ) == STONE_BOARD_REGION_BottomSide );
        CHECK( broadPhaseReject == false );

        CHECK( DetermineStoneBoardRegion( board, vec3f(-w,0,-h), radius, broadPhaseReject ) == STONE_BOARD_REGION_TopLeftCorner );
        CHECK( broadPhaseReject == false );

        CHECK( DetermineStoneBoardRegion( board, vec3f(+w,0,-h), radius, broadPhaseReject ) == STONE_BOARD_REGION_TopRightCorner );
        CHECK( broadPhaseReject == false );

        CHECK( DetermineStoneBoardRegion( board, vec3f(+w,0,+h), radius, broadPhaseReject ) == STONE_BOARD_REGION_BottomRightCorner );
        CHECK( broadPhaseReject == false );

        CHECK( DetermineStoneBoardRegion( board, vec3f(-w,0,+h), radius, broadPhaseReject ) == STONE_BOARD_REGION_BottomLeftCorner );
        CHECK( broadPhaseReject == false );

        CHECK( DetermineStoneBoardRegion( board, vec3f(0,t+radius + 0.01f,0), radius, broadPhaseReject ) == STONE_BOARD_REGION_Primary );
        CHECK( broadPhaseReject == true );
    }

    TEST( stone_board_collision_none )
    {
        Board board( 9 );

        const float w = board.GetWidth() * 0.5f;
        const float h = board.GetHeight() * 0.5f;
        const float t = board.GetThickness();

        Biconvex biconvex( 2.0f, 1.0f );

        const float r = biconvex.GetBoundingSphereRadius();

        mat4f localToWorld = mat4f::identity();
        mat4f worldToLocal = mat4f::identity();

        float depth;
        vec3f point, normal;

        CHECK( !IntersectStoneBoard( board, biconvex, RigidBodyTransform( vec3f(0,t+r*2,0) ), point, normal, depth ) );
        CHECK( !IntersectStoneBoard( board, biconvex, RigidBodyTransform( vec3f(-w-r*2,0,0) ), point, normal, depth ) );
        CHECK( !IntersectStoneBoard( board, biconvex, RigidBodyTransform( vec3f(+w+r*2,0,0) ), point, normal, depth ) );
        CHECK( !IntersectStoneBoard( board, biconvex, RigidBodyTransform( vec3f(0,0,-h-r*2) ), point, normal, depth ) );
        CHECK( !IntersectStoneBoard( board, biconvex, RigidBodyTransform( vec3f(0,0,+h+r*2) ), point, normal, depth ) );
    }

    TEST( stone_board_collision_primary )
    {
        const float epsilon = 0.001f;

        Board board( 9 );

        const float w = board.GetWidth() * 0.5f;
        const float h = board.GetHeight() * 0.5f;
        const float t = board.GetThickness();

        Biconvex biconvex( 2.0f, 1.0f );

        float depth;
        vec3f point, normal;

        // test at origin with identity rotation
        {
            RigidBodyTransform biconvexTransform( vec3f(0,0,0), mat4f::identity() );

            CHECK( IntersectStoneBoard( board, biconvex, biconvexTransform, point, normal, depth ) );

            CHECK_CLOSE( depth, 1.0f, epsilon );
            CHECK_CLOSE_VEC3( point, vec3f(0,-0.5f,0), epsilon );
            CHECK_CLOSE_VEC3( normal, vec3f(0,1,0), epsilon );
        }
        
        // test away from origin with identity rotation
        {
            RigidBodyTransform biconvexTransform( vec3f(w/2,0,h/2), mat4f::identity() );

            CHECK( IntersectStoneBoard( board, biconvex, biconvexTransform, point, normal, depth ) );

            CHECK_CLOSE( depth, 1.0f, epsilon );
            CHECK_CLOSE_VEC3( point, vec3f(w/2,-0.5f,h/2), epsilon );
            CHECK_CLOSE_VEC3( normal, vec3f(0,1,0), epsilon );
        }

        // test at origin flipped upside down (180 degrees)
        {
            RigidBodyTransform biconvexTransform( vec3f(0,0,0), mat4f::axisRotation( 180, vec3f(0,0,1) ) );

            CHECK( IntersectStoneBoard( board, biconvex, biconvexTransform, point, normal, depth ) );

            CHECK_CLOSE( depth, 1.0f, epsilon );
            CHECK_CLOSE_VEC3( point, vec3f(0,-0.5f,0), epsilon );
            CHECK_CLOSE_VEC3( normal, vec3f(0,1,0), epsilon );
        }

        // test away from origin flipped upside down (180degrees)
        {
            RigidBodyTransform biconvexTransform( vec3f(w/2,0,h/2), mat4f::axisRotation( 180, vec3f(0,0,1) ) );

            CHECK( IntersectStoneBoard( board, biconvex, biconvexTransform, point, normal, depth ) );

            CHECK_CLOSE( depth, 1.0f, epsilon );
            CHECK_CLOSE_VEC3( point, vec3f(w/2,-0.5f,h/2), epsilon );
            CHECK_CLOSE_VEC3( normal, vec3f(0,1,0), epsilon );
        }

        // test away from origin rotated 90 degrees clockwise around z axis
        {
            RigidBodyTransform biconvexTransform( vec3f(w/2,0,h/2), mat4f::axisRotation( 90, vec3f(0,0,1) ) );

            CHECK( IntersectStoneBoard( board, biconvex, biconvexTransform, point, normal, depth ) );

            CHECK_CLOSE( depth, 1.5f, epsilon );
            CHECK_CLOSE_VEC3( point, vec3f(w/2,-1,h/2), epsilon );
            CHECK_CLOSE_VEC3( normal, vec3f(0,1,0), epsilon );
        }

        // test away from origin rotated 90 degrees counter-clockwise around z axis
        {
            RigidBodyTransform biconvexTransform( vec3f(w/2,0,h/2), mat4f::axisRotation( -90, vec3f(0,0,1) ) );

            CHECK( IntersectStoneBoard( board, biconvex, biconvexTransform, point, normal, depth ) );

            CHECK_CLOSE( depth, 1.5f, epsilon );
            CHECK_CLOSE_VEC3( point, vec3f(w/2,-1,h/2), epsilon );
            CHECK_CLOSE_VEC3( normal, vec3f(0,1,0), epsilon );
        }

        // test away from origin rotated 90 degrees clockwise around x axis
        {
            RigidBodyTransform biconvexTransform( vec3f(w/2,0,h/2), mat4f::axisRotation( 90, vec3f(1,0,0) ) );

            CHECK( IntersectStoneBoard( board, biconvex, biconvexTransform, point, normal, depth ) );

            CHECK_CLOSE( depth, 1.5f, epsilon );
            CHECK_CLOSE_VEC3( point, vec3f(w/2,-1,h/2), epsilon );
            CHECK_CLOSE_VEC3( normal, vec3f(0,1,0), epsilon );
        }

        // test away from origin rotated 90 degrees counter-clockwise around x axis
        {
            RigidBodyTransform biconvexTransform( vec3f(w/2,0,h/2), mat4f::axisRotation( -90, vec3f(1,0,0) ) );

            CHECK( IntersectStoneBoard( board, biconvex, biconvexTransform, point, normal, depth ) );

            CHECK_CLOSE( depth, 1.5f, epsilon );
            CHECK_CLOSE_VEC3( point, vec3f(w/2,-1,h/2), epsilon );
            CHECK_CLOSE_VEC3( normal, vec3f(0,1,0), epsilon );
        }
    }

    TEST( stone_board_collision_left_side )
    {
        // ...
    }

    TEST( stone_board_collision_right_side )
    {
        // ...
    }

    TEST( stone_board_collision_top_side )
    {
        // ...
    }

    TEST( stone_board_collision_bottom_side )
    {
        // ...
    }

    TEST( stone_board_collision_top_left_corner )
    {
        // ...
    }

    TEST( stone_board_collision_top_right_corner )
    {
        // ...
    }

    TEST( stone_board_collision_bottom_right_corner )
    {
        // ...
    }

    TEST( stone_board_collision_bottom_left_corner )
    {
        // ...
    }

    // =====================================================================================

    TEST( biconvex )
    {
        Biconvex biconvex( 2.0f, 1.0f );
        const float epsilon = 0.001f;
        CHECK_CLOSE( biconvex.GetWidth(), 2.0f, epsilon );
        CHECK_CLOSE( biconvex.GetHeight(), 1.0f, epsilon );
        CHECK_CLOSE( biconvex.GetSphereRadius(), 1.25f, epsilon );
        CHECK_CLOSE( biconvex.GetSphereOffset(), 0.75f, epsilon );
        CHECK_CLOSE( biconvex.GetSphereDot(), 0.599945f, epsilon );
    }    

    TEST( intersect_ray_sphere_hit )
    {
        const float epsilon = 0.001f;
        vec3f rayStart(0,0,-10);
        vec3f rayDirection(0,0,1);
        vec3f sphereCenter(0,0,0);
        float sphereRadius = 1.0f;
        float sphereRadiusSquared = sphereRadius * sphereRadius;
        float t = 0;
        bool hit = IntersectRaySphere( rayStart, rayDirection, sphereCenter, sphereRadius, sphereRadiusSquared, t );
        CHECK( hit );
        CHECK_CLOSE( t, 9.0f, epsilon );
    }

    TEST( intersect_ray_sphere_miss )
    {
        const float epsilon = 0.001f;
        vec3f rayStart(0,50,-10);
        vec3f rayDirection(0,0,1);
        vec3f sphereCenter(0,0,0);
        float sphereRadius = 1.0f;
        float sphereRadiusSquared = sphereRadius * sphereRadius;
        float t = 0;
        bool hit = IntersectRaySphere( rayStart, rayDirection, sphereCenter, sphereRadius, sphereRadiusSquared, t );
        CHECK( !hit );
    }

    TEST( intersect_ray_sphere_inside )
    {
        // IMPORTANT: we define a ray starting inside the sphere as a miss
        // we only care about spheres that we hit from the outside, eg. one sided
        const float epsilon = 0.001f;
        vec3f rayStart(0,0,0);
        vec3f rayDirection(0,0,1);
        vec3f sphereCenter(0,0,0);
        float sphereRadius = 1.0f;
        float sphereRadiusSquared = sphereRadius * sphereRadius;
        float t = 0;
        bool hit = IntersectRaySphere( rayStart, rayDirection, sphereCenter, sphereRadius, sphereRadiusSquared, t );
        CHECK( !hit );
    }

    TEST( point_inside_biconvex )
    {
        Biconvex biconvex( 2.0f, 1.0f );
        CHECK( PointInsideBiconvex_LocalSpace( vec3f(0,0,0), biconvex ) );
        CHECK( PointInsideBiconvex_LocalSpace( vec3f(-1,0,0), biconvex ) );
        CHECK( PointInsideBiconvex_LocalSpace( vec3f(+1,0,0), biconvex ) );
        CHECK( PointInsideBiconvex_LocalSpace( vec3f(0,0,-1), biconvex ) );
        CHECK( PointInsideBiconvex_LocalSpace( vec3f(0,0,+1), biconvex ) );
        CHECK( PointInsideBiconvex_LocalSpace( vec3f(0,0.5,0), biconvex ) );
        CHECK( PointInsideBiconvex_LocalSpace( vec3f(0,-0.5,0), biconvex ) );
        CHECK( !PointInsideBiconvex_LocalSpace( vec3f(0,0.5,0.5f), biconvex ) );
    }

    TEST( point_on_biconvex_surface )
    {
        Biconvex biconvex( 2.0f, 1.0f );
        CHECK( IsPointOnBiconvexSurface_LocalSpace( vec3f(-1,0,0), biconvex ) );
        CHECK( IsPointOnBiconvexSurface_LocalSpace( vec3f(+1,0,0), biconvex ) );
        CHECK( IsPointOnBiconvexSurface_LocalSpace( vec3f(0,0,-1), biconvex ) );
        CHECK( IsPointOnBiconvexSurface_LocalSpace( vec3f(0,0,+1), biconvex ) );
        CHECK( IsPointOnBiconvexSurface_LocalSpace( vec3f(0,0.5,0), biconvex ) );
        CHECK( IsPointOnBiconvexSurface_LocalSpace( vec3f(0,-0.5,0), biconvex ) );
        CHECK( !IsPointOnBiconvexSurface_LocalSpace( vec3f(0,0,0), biconvex ) );
        CHECK( !IsPointOnBiconvexSurface_LocalSpace( vec3f(10,10,10), biconvex ) );
    }

    TEST( biconvex_surface_normal_at_point )
    {
        const float epsilon = 0.001f;
        Biconvex biconvex( 2.0f, 1.0f );
        vec3f normal(0,0,0);

        GetBiconvexSurfaceNormalAtPoint_LocalSpace( vec3f(1,0,0), biconvex, normal );
        CHECK_CLOSE_VEC3( normal, vec3f(1,0,0), epsilon );

        GetBiconvexSurfaceNormalAtPoint_LocalSpace( vec3f(-1,0,0), biconvex, normal );
        CHECK_CLOSE_VEC3( normal, vec3f(-1,0,0), epsilon );

        GetBiconvexSurfaceNormalAtPoint_LocalSpace( vec3f(0,0,1), biconvex, normal );
        CHECK_CLOSE_VEC3( normal, vec3f(0,0,1), epsilon );

        GetBiconvexSurfaceNormalAtPoint_LocalSpace( vec3f(0,0,-1), biconvex, normal );
        CHECK_CLOSE_VEC3( normal, vec3f(0,0,-1), epsilon );

        GetBiconvexSurfaceNormalAtPoint_LocalSpace( vec3f(0,0.5,0), biconvex, normal );
        CHECK_CLOSE_VEC3( normal, vec3f(0,1,0), epsilon );

        GetBiconvexSurfaceNormalAtPoint_LocalSpace( vec3f(0,-0.5,0), biconvex, normal );
        CHECK_CLOSE_VEC3( normal, vec3f(0,-1,0), epsilon );
    }

    TEST( nearest_point_on_biconvex_surface )
    {
        const float epsilon = 0.001f;
        Biconvex biconvex( 2.0f, 1.0f );
        vec3f nearest;

        nearest = GetNearestPointOnBiconvexSurface_LocalSpace( vec3f(0,10,0), biconvex );
        CHECK_CLOSE_VEC3( nearest, vec3f(0,0.5f,0), epsilon );

        nearest = GetNearestPointOnBiconvexSurface_LocalSpace( vec3f(0,-10,0), biconvex );
        CHECK_CLOSE_VEC3( nearest, vec3f(0,-0.5f,0), epsilon );

        nearest = GetNearestPointOnBiconvexSurface_LocalSpace( vec3f(-10,0,0), biconvex );
        CHECK_CLOSE_VEC3( nearest, vec3f(-1,0,0), epsilon );

        nearest = GetNearestPointOnBiconvexSurface_LocalSpace( vec3f(10,0,0), biconvex );
        CHECK_CLOSE_VEC3( nearest, vec3f(1,0,0), epsilon );

        nearest = GetNearestPointOnBiconvexSurface_LocalSpace( vec3f(0,0,-10), biconvex );
        CHECK_CLOSE_VEC3( nearest, vec3f(0,0,-1), epsilon );

        nearest = GetNearestPointOnBiconvexSurface_LocalSpace( vec3f(0,0,10), biconvex );
        CHECK_CLOSE_VEC3( nearest, vec3f(0,0,1), epsilon );
    }

    TEST( intersect_plane_biconvex_bottom )
    {
        const float epsilon = 0.001f;
        Biconvex biconvex( 2.0f, 1.0f );
        vec3f planeNormal(0,1,0);
        float planeDistance = -10;
        vec3f point, normal;
        float penetrationDepth = IntersectPlaneBiconvex_LocalSpace( planeNormal, planeDistance, biconvex, point, normal );
        CHECK_CLOSE( penetrationDepth, -9.5, epsilon );
        CHECK_CLOSE_VEC3( point, vec3f(0,-0.5,0), epsilon );
        CHECK_CLOSE_VEC3( normal, vec3f(0,1,0), epsilon );
    }

    TEST( intersect_plane_biconvex_top )
    {
        const float epsilon = 0.001f;
        Biconvex biconvex( 2.0f, 1.0f );
        vec3f planeNormal(0,-1,0);
        float planeDistance = -10;
        vec3f point, normal;
        float penetrationDepth = IntersectPlaneBiconvex_LocalSpace( planeNormal, planeDistance, biconvex, point, normal );
        CHECK_CLOSE( penetrationDepth, -9.5, epsilon );
        CHECK_CLOSE_VEC3( point, vec3f(0,+0.5,0), epsilon );
        CHECK_CLOSE_VEC3( normal, vec3f(0,-1,0), epsilon );
    }

    TEST( intersect_plane_biconvex_side )
    {
        const float epsilon = 0.001f;
        Biconvex biconvex( 2.0f, 1.0f );
        vec3f planeNormal(-1,0,0);
        float planeDistance = -10;
        vec3f point, normal;
        float penetrationDepth = IntersectPlaneBiconvex_LocalSpace( planeNormal, planeDistance, biconvex, point, normal );
        CHECK_CLOSE( penetrationDepth, -9, epsilon );
        CHECK_CLOSE_VEC3( point, vec3f(1,0,0), epsilon );
        CHECK_CLOSE_VEC3( normal, vec3f(-1,0,0), epsilon );
    }

    TEST( biconvex_support_local_space )
    {
        const float epsilon = 0.001f;

        Biconvex biconvex( 2.0f, 1.0f );

        float s1,s2;

        BiconvexSupport_LocalSpace( biconvex, vec3f(0,1,0), s1, s2 );
        CHECK_CLOSE( s1, -0.5f, epsilon );
        CHECK_CLOSE( s2, 0.5f, epsilon );

        BiconvexSupport_LocalSpace( biconvex, vec3f(0,-1,0), s1, s2 );
        CHECK_CLOSE( s1, -0.5f, epsilon );
        CHECK_CLOSE( s2, 0.5f, epsilon );

        BiconvexSupport_LocalSpace( biconvex, vec3f(1,0,0), s1, s2 );
        CHECK_CLOSE( s1, -1.0f, epsilon );
        CHECK_CLOSE( s2, 1.0f, epsilon );

        BiconvexSupport_LocalSpace( biconvex, vec3f(-1,0,0), s1, s2 );
        CHECK_CLOSE( s1, -1.0f, epsilon );
        CHECK_CLOSE( s2, 1.0f, epsilon );

        BiconvexSupport_LocalSpace( biconvex, vec3f(0,0,1), s1, s2 );
        CHECK_CLOSE( s1, -1.0f, epsilon );
        CHECK_CLOSE( s2, 1.0f, epsilon );

        BiconvexSupport_LocalSpace( biconvex, vec3f(0,0,-1), s1, s2 );
        CHECK_CLOSE( s1, -1.0f, epsilon );
        CHECK_CLOSE( s2, 1.0f, epsilon );
    }

    TEST( biconvex_support_world_space )
    {
        const float epsilon = 0.001f;

        Biconvex biconvex( 2.0f, 1.0f );
        vec3f biconvexCenter(10,0,0);
        vec3f biconvexUp(1,0,0);

        float s1,s2;

        BiconvexSupport_WorldSpace( biconvex, biconvexCenter, biconvexUp, vec3f(0,1,0), s1, s2 );
        CHECK_CLOSE( s1, -1.0f, epsilon );
        CHECK_CLOSE( s2, 1.0f, epsilon );

        BiconvexSupport_WorldSpace( biconvex, biconvexCenter, biconvexUp, vec3f(0,-1,0), s1, s2 );
        CHECK_CLOSE( s1, -1.0f, epsilon );
        CHECK_CLOSE( s2, 1.0f, epsilon );

        BiconvexSupport_WorldSpace( biconvex, biconvexCenter, biconvexUp, vec3f(1,0,0), s1, s2 );
        CHECK_CLOSE( s1, 9.5f, epsilon );
        CHECK_CLOSE( s2, 10.5f, epsilon );

        BiconvexSupport_WorldSpace( biconvex, biconvexCenter, biconvexUp, vec3f(-1,0,0), s1, s2 );
        CHECK_CLOSE( s1, -10.5f, epsilon );
        CHECK_CLOSE( s2, -9.5f, epsilon );

        BiconvexSupport_WorldSpace( biconvex, biconvexCenter, biconvexUp, vec3f(0,0,1), s1, s2 );
        CHECK_CLOSE( s1, -1.0f, epsilon );
        CHECK_CLOSE( s2, 1.0f, epsilon );

        BiconvexSupport_WorldSpace( biconvex, biconvexCenter, biconvexUp, vec3f(0,0,-1), s1, s2 );
        CHECK_CLOSE( s1, -1.0f, epsilon );
        CHECK_CLOSE( s2, 1.0f, epsilon );
    }

    TEST( biconvex_sat )
    {
        const float epsilon = 0.001f;

        Biconvex biconvex( 2.0f, 1.0f );

        CHECK( Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,0,0), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );
        CHECK( Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,0,0), vec3f(0,1,0), vec3f(0,-1,0), epsilon ) );
        CHECK( Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,0,0), vec3f(0,1,0), vec3f(1,0,0), epsilon ) );
        CHECK( Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,0,0), vec3f(0,1,0), vec3f(-1,0,0), epsilon ) );
        CHECK( Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,0,0), vec3f(0,1,0), vec3f(0,0,1), epsilon ) );
        CHECK( Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,0,0), vec3f(0,1,0), vec3f(0,0,-1), epsilon ) );

        CHECK( Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(1,0,0), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );
        CHECK( Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(-1,0,0), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );
        CHECK( Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,1,0), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );
        CHECK( Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,-1,0), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );
        CHECK( Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,0,1), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );
        CHECK( Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,0,-1), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );

        CHECK( !Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(10,0,0), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );
        CHECK( !Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(-10,0,0), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );
        CHECK( !Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,10,0), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );
        CHECK( !Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,-10,0), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );
        CHECK( !Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,0,10), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );
        CHECK( !Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,0,-10), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );

        CHECK( !Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(10,0,0), vec3f(0,1,0), vec3f(1,0,0), epsilon ) );
        CHECK( !Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(-10,0,0), vec3f(0,1,0), vec3f(1,0,0), epsilon ) );
        CHECK( !Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,10,0), vec3f(0,1,0), vec3f(1,0,0), epsilon ) );
        CHECK( !Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,-10,0), vec3f(0,1,0), vec3f(1,0,0), epsilon ) );
        CHECK( !Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,0,10), vec3f(0,1,0), vec3f(1,0,0), epsilon ) );
        CHECK( !Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,0,-10), vec3f(0,1,0), vec3f(1,0,0), epsilon ) );
    }
}

class MyTestReporter : public UnitTest::TestReporterStdout
{
    virtual void ReportTestStart( UnitTest::TestDetails const & details )
    {
        printf( "test_%s\n", details.testName );
    }
};

int main( int argc, char * argv[] )
{
    MyTestReporter reporter;

    UnitTest::TestRunner runner( reporter );

    return runner.RunTestsIf( UnitTest::Test::GetTestList(), NULL, UnitTest::True(), 0 );
}
