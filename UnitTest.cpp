#include "Common.h"
#include "Biconvex.h"
#include "RigidBody.h"
#include "Intersection.h"
#include "InertiaTensor.h"
#include "CollisionDetection.h"
#include "SceneGrid.h"

#include "UnitTest++/UnitTest++.h"
#include "UnitTest++/TestRunner.h"
#include "UnitTest++/TestReporterStdout.h"

#define CHECK_CLOSE_VEC3( value, expected, epsilon ) CHECK_CLOSE( length( value - expected ), 0.0f, epsilon )

SUITE( SceneGrid )
{
    TEST( scene_grid_integer_bounds )
    {
        SceneGrid sceneGrid;
        sceneGrid.Initialize( 4.0f, 16.0f, 16.0f, 16.0f );

        int nx,ny,nz;
        sceneGrid.GetIntegerBounds( nx, ny, nz );
        CHECK( nx == 4 );
        CHECK( ny == 4 );
        CHECK( nz == 4 );
    }

    TEST( scene_grid_cell_coordinates )
    {
        SceneGrid sceneGrid;
        sceneGrid.Initialize( 4.0f, 16.0f, 16.0f, 16.0f );

        int ix,iy,iz;
        sceneGrid.GetCellCoordinates( vec3f(1,1,1), ix, iy, iz );
        CHECK( ix == 2 );
        CHECK( iy == 2 );
        CHECK( iz == 0 );

        sceneGrid.GetCellCoordinates( vec3f(-100,-100,-100), ix, iy, iz );
        CHECK( ix == 0 );
        CHECK( iy == 0 );
        CHECK( iz == 0 );

        int nx,ny,nz;
        sceneGrid.GetIntegerBounds( nx, ny, nz );

        sceneGrid.GetCellCoordinates( vec3f(+100,+100,+100), ix, iy, iz );
        CHECK( ix == nx - 1 );
        CHECK( iy == ny - 1 );
        CHECK( iz == nz - 1 );
    }

    TEST( scene_grid_cell_index )
    {
        SceneGrid sceneGrid;
        sceneGrid.Initialize( 4.0f, 16.0f, 16.0f, 16.0f );

        int index = sceneGrid.GetCellIndex( 0, 0, 0 );
        CHECK( index == 0 );

        int nx,ny,nz;
        sceneGrid.GetIntegerBounds( nx, ny, nz );

        index = sceneGrid.GetCellIndex( 100, 100, 100 );
        CHECK( index == (nx-1) + (ny-1)*nx + (nz-1)*nx*ny );

        index = sceneGrid.GetCellIndex( 2, 0, 0 );
        CHECK( index == 2 );

        index = sceneGrid.GetCellIndex( 0, 2, 0 );
        CHECK( index == 2*4 );

        index = sceneGrid.GetCellIndex( 0, 0, 2 );
        CHECK( index == 2*4*4 );
    }

    TEST( scene_grid_add_object )
    {
        SceneGrid sceneGrid;
        sceneGrid.Initialize( 4.0f, 16.0f, 16.0f, 16.0f );

        uint16_t id = 1;
        vec3f position(0,0,0);

        sceneGrid.AddObject( id, position );

        int ix,iy,iz;
        sceneGrid.GetCellCoordinates( position, ix, iy, iz );

        int index = sceneGrid.GetCellIndex( ix, iy, iz );

        Cell & cell = sceneGrid.GetCell( index );

        CHECK( std::find( cell.objects.begin(), cell.objects.end(), id ) != cell.objects.end() );
    }

    TEST( scene_grid_remove_object )
    {
        SceneGrid sceneGrid;
        sceneGrid.Initialize( 4.0f, 16.0f, 16.0f, 16.0f );

        uint16_t id = 1;
        vec3f position(0,0,0);

        sceneGrid.AddObject( id, position );

        int ix,iy,iz;
        sceneGrid.GetCellCoordinates( position, ix, iy, iz );
        int index = sceneGrid.GetCellIndex( ix, iy, iz );
        Cell & cell = sceneGrid.GetCell( index );
        CHECK( std::find( cell.objects.begin(), cell.objects.end(), id ) != cell.objects.end() );

        sceneGrid.RemoveObject( id, position );
        CHECK( std::find( cell.objects.begin(), cell.objects.end(), id ) == cell.objects.end() );
    }

    TEST( scene_grid_move_object )
    {
        SceneGrid sceneGrid;
        sceneGrid.Initialize( 4.0f, 16.0f, 16.0f, 16.0f );

        uint16_t id = 1;
        vec3f position1(0,0,0);
        vec3f position2(10,10,10);

        sceneGrid.AddObject( id, position1 );

        int ix,iy,iz;
        sceneGrid.GetCellCoordinates( position1, ix, iy, iz );
        int index = sceneGrid.GetCellIndex( ix, iy, iz );
        Cell & cell = sceneGrid.GetCell( index );
        CHECK( std::find( cell.objects.begin(), cell.objects.end(), id ) != cell.objects.end() );

        sceneGrid.MoveObject( id, position1, position2 );
        CHECK( std::find( cell.objects.begin(), cell.objects.end(), id ) == cell.objects.end() );

        sceneGrid.GetCellCoordinates( position2, ix, iy, iz );
        index = sceneGrid.GetCellIndex( ix, iy, iz );
        cell = sceneGrid.GetCell( index );
        CHECK( std::find( cell.objects.begin(), cell.objects.end(), id ) != cell.objects.end() );
    }
}

SUITE( Intersection )
{
    TEST( stone_board_collision_type )
    {
        Board board;
        board.Initialize( 9 );

        const float w = board.GetWidth() * 0.5f;
        const float h = board.GetHeight() * 0.5f;
        const float t = board.GetThickness();

        const float radius = 1.0f;

        bool broadPhaseReject = false;

        CHECK( DetermineStoneBoardRegion( board, vec3f(0,0,0), radius, broadPhaseReject ) == STONE_BOARD_REGION_Primary );
        CHECK( broadPhaseReject == false );

        CHECK( DetermineStoneBoardRegion( board, vec3f(0,0,-100), radius, broadPhaseReject ) == STONE_BOARD_REGION_Primary );
        CHECK( broadPhaseReject == false );

        CHECK( DetermineStoneBoardRegion( board, vec3f(-w,0,0), radius, broadPhaseReject ) == STONE_BOARD_REGION_LeftSide );
        CHECK( broadPhaseReject == false );
        
        CHECK( DetermineStoneBoardRegion( board, vec3f(+w,0,0), radius, broadPhaseReject ) == STONE_BOARD_REGION_RightSide );
        CHECK( broadPhaseReject == false );
        
        CHECK( DetermineStoneBoardRegion( board, vec3f(0,+h,0), radius, broadPhaseReject ) == STONE_BOARD_REGION_TopSide );
        CHECK( broadPhaseReject == false );
        
        CHECK( DetermineStoneBoardRegion( board, vec3f(0,-h,0), radius, broadPhaseReject ) == STONE_BOARD_REGION_BottomSide );
        CHECK( broadPhaseReject == false );

        CHECK( DetermineStoneBoardRegion( board, vec3f(-w,+h,0), radius, broadPhaseReject ) == STONE_BOARD_REGION_TopLeftCorner );
        CHECK( broadPhaseReject == false );

        CHECK( DetermineStoneBoardRegion( board, vec3f(+w,+h,0), radius, broadPhaseReject ) == STONE_BOARD_REGION_TopRightCorner );
        CHECK( broadPhaseReject == false );

        CHECK( DetermineStoneBoardRegion( board, vec3f(+w,-h,0), radius, broadPhaseReject ) == STONE_BOARD_REGION_BottomRightCorner );
        CHECK( broadPhaseReject == false );

        CHECK( DetermineStoneBoardRegion( board, vec3f(-w,-h,0), radius, broadPhaseReject ) == STONE_BOARD_REGION_BottomLeftCorner );
        CHECK( broadPhaseReject == false );

        CHECK( DetermineStoneBoardRegion( board, vec3f(0,0,t+radius + 0.01f), radius, broadPhaseReject ) == STONE_BOARD_REGION_Primary );
        CHECK( broadPhaseReject == true );
    }

    TEST( stone_board_collision_none )
    {
        Board board;
        board.Initialize( 9 );

        const float w = board.GetWidth() * 0.5f;
        const float h = board.GetHeight() * 0.5f;
        const float t = board.GetThickness();

        Biconvex biconvex( 2.0f, 1.0f );

        const float r = biconvex.GetBoundingSphereRadius();

        mat4f localToWorld = mat4f::identity();
        mat4f worldToLocal = mat4f::identity();

        float depth;
        vec3f point, normal;

        CHECK( !IntersectStoneBoard( board, biconvex, RigidBodyTransform( vec3f(0,0,t+r*2) ), normal, depth ) );
        CHECK( !IntersectStoneBoard( board, biconvex, RigidBodyTransform( vec3f(-w-r*2,0,0) ), normal, depth ) );
        CHECK( !IntersectStoneBoard( board, biconvex, RigidBodyTransform( vec3f(+w+r*2,0,0) ), normal, depth ) );
        CHECK( !IntersectStoneBoard( board, biconvex, RigidBodyTransform( vec3f(0,-h-r*2,0) ), normal, depth ) );
        CHECK( !IntersectStoneBoard( board, biconvex, RigidBodyTransform( vec3f(0,+h+r*2,0) ), normal, depth ) );
    }

    TEST( stone_board_collision_primary )
    {
        const float epsilon = 0.001f;

        Board board;
        board.Initialize( 9 );

        const float w = board.GetWidth() * 0.5f;
        const float h = board.GetHeight() * 0.5f;
        const float t = board.GetThickness();

        Biconvex biconvex( 2.0f, 1.0f );

        float depth;
        vec3f normal;

        // test at origin with identity rotation
        {
            RigidBodyTransform biconvexTransform( vec3f(0,0,0), mat4f::identity() );

            CHECK( IntersectStoneBoard( board, biconvex, biconvexTransform, normal, depth ) );

            CHECK_CLOSE( depth, t + biconvex.GetHeight()/2, epsilon );
            CHECK_CLOSE_VEC3( normal, vec3f(0,0,1), epsilon );
        }
        
        // test away from origin with identity rotation
        {
            RigidBodyTransform biconvexTransform( vec3f(w/2,h/2,0), mat4f::identity() );

            CHECK( IntersectStoneBoard( board, biconvex, biconvexTransform, normal, depth ) );

            CHECK_CLOSE( depth, t + biconvex.GetHeight()/2, epsilon );
            CHECK_CLOSE_VEC3( normal, vec3f(0,0,1), epsilon );
        }

        // test at origin flipped upside down (180 degrees)
        {
            RigidBodyTransform biconvexTransform( vec3f(0,0,0), mat4f::axisRotation( 180, vec3f(0,1,0) ) );

            CHECK( IntersectStoneBoard( board, biconvex, biconvexTransform, normal, depth ) );

            CHECK_CLOSE( depth, t + biconvex.GetHeight()/2, epsilon );
            CHECK_CLOSE_VEC3( normal, vec3f(0,0,1), epsilon );
        }

        // test away from origin flipped upside down (180degrees)
        {
            RigidBodyTransform biconvexTransform( vec3f(w/2,h/2,0), mat4f::axisRotation( 180, vec3f(0,1,0) ) );

            CHECK( IntersectStoneBoard( board, biconvex, biconvexTransform, normal, depth ) );

            CHECK_CLOSE( depth, t + biconvex.GetHeight()/2, epsilon );
            CHECK_CLOSE_VEC3( normal, vec3f(0,0,1), epsilon );
        }

        // test away from origin rotated 90 degrees clockwise around z axis
        {
            RigidBodyTransform biconvexTransform( vec3f(w/2,h/2,0), mat4f::axisRotation( 90, vec3f(0,1,0) ) );

            CHECK( IntersectStoneBoard( board, biconvex, biconvexTransform, normal, depth ) );

            CHECK_CLOSE( depth, t + biconvex.GetWidth()/2, epsilon );
            CHECK_CLOSE_VEC3( normal, vec3f(0,0,1), epsilon );
        }

        // test away from origin rotated 90 degrees counter-clockwise around z axis
        {
            RigidBodyTransform biconvexTransform( vec3f(w/2,h/2,0), mat4f::axisRotation( -90, vec3f(0,1,0) ) );

            CHECK( IntersectStoneBoard( board, biconvex, biconvexTransform, normal, depth ) );

            CHECK_CLOSE( depth, t + biconvex.GetWidth()/2, epsilon );
            CHECK_CLOSE_VEC3( normal, vec3f(0,0,1), epsilon );
        }

        // test away from origin rotated 90 degrees clockwise around x axis
        {
            RigidBodyTransform biconvexTransform( vec3f(w/2,h/2,0), mat4f::axisRotation( 90, vec3f(1,0,0) ) );

            CHECK( IntersectStoneBoard( board, biconvex, biconvexTransform, normal, depth ) );

            CHECK_CLOSE( depth, t + biconvex.GetWidth()/2, epsilon );
            CHECK_CLOSE_VEC3( normal, vec3f(0,0,1), epsilon );
        }

        // test away from origin rotated 90 degrees counter-clockwise around x axis
        {
            RigidBodyTransform biconvexTransform( vec3f(w/2,h/2,0), mat4f::axisRotation( -90, vec3f(1,0,0) ) );

            CHECK( IntersectStoneBoard( board, biconvex, biconvexTransform, normal, depth ) );

            CHECK_CLOSE( depth, t + biconvex.GetWidth()/2, epsilon );
            CHECK_CLOSE_VEC3( normal, vec3f(0,0,1), epsilon );
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
        CHECK( PointInsideBiconvex_LocalSpace( vec3f(0,-1,0), biconvex ) );
        CHECK( PointInsideBiconvex_LocalSpace( vec3f(0,+1,0), biconvex ) );
        CHECK( PointInsideBiconvex_LocalSpace( vec3f(0,0,0.5), biconvex ) );
        CHECK( PointInsideBiconvex_LocalSpace( vec3f(0,0,-0.5), biconvex ) );
        CHECK( !PointInsideBiconvex_LocalSpace( vec3f(0,0.5f,0.5), biconvex ) );
    }

    TEST( point_on_biconvex_surface )
    {
        Biconvex biconvex( 2.0f, 1.0f );
        CHECK( IsPointOnBiconvexSurface_LocalSpace( vec3f(-1,0,0), biconvex ) );
        CHECK( IsPointOnBiconvexSurface_LocalSpace( vec3f(+1,0,0), biconvex ) );
        CHECK( IsPointOnBiconvexSurface_LocalSpace( vec3f(0,-1,0), biconvex ) );
        CHECK( IsPointOnBiconvexSurface_LocalSpace( vec3f(0,+1,0), biconvex ) );
        CHECK( IsPointOnBiconvexSurface_LocalSpace( vec3f(0,0,0.5), biconvex ) );
        CHECK( IsPointOnBiconvexSurface_LocalSpace( vec3f(0,0,-0.5), biconvex ) );
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

        nearest = GetNearestPointOnBiconvexSurface_LocalSpace( vec3f(0,0,10), biconvex );
        CHECK_CLOSE_VEC3( nearest, vec3f(0,0,0.5f), epsilon );

        nearest = GetNearestPointOnBiconvexSurface_LocalSpace( vec3f(0,0,-10), biconvex );
        CHECK_CLOSE_VEC3( nearest, vec3f(0,0,-0.5f), epsilon );

        nearest = GetNearestPointOnBiconvexSurface_LocalSpace( vec3f(-10,0,0), biconvex );
        CHECK_CLOSE_VEC3( nearest, vec3f(-1,0,0), epsilon );

        nearest = GetNearestPointOnBiconvexSurface_LocalSpace( vec3f(10,0,0), biconvex );
        CHECK_CLOSE_VEC3( nearest, vec3f(1,0,0), epsilon );

        nearest = GetNearestPointOnBiconvexSurface_LocalSpace( vec3f(0,-10,0), biconvex );
        CHECK_CLOSE_VEC3( nearest, vec3f(0,-1,0), epsilon );

        nearest = GetNearestPointOnBiconvexSurface_LocalSpace( vec3f(0,10,0), biconvex );
        CHECK_CLOSE_VEC3( nearest, vec3f(0,1,0), epsilon );
    }

    TEST( biconvex_support_local_space )
    {
        const float epsilon = 0.001f;

        Biconvex biconvex( 2.0f, 1.0f );

        float s1,s2;

        BiconvexSupport_LocalSpace( biconvex, vec3f(0,0,1), s1, s2 );
        CHECK_CLOSE( s1, -0.5f, epsilon );
        CHECK_CLOSE( s2, 0.5f, epsilon );

        BiconvexSupport_LocalSpace( biconvex, vec3f(0,0,-1), s1, s2 );
        CHECK_CLOSE( s1, -0.5f, epsilon );
        CHECK_CLOSE( s2, 0.5f, epsilon );

        BiconvexSupport_LocalSpace( biconvex, vec3f(1,0,0), s1, s2 );
        CHECK_CLOSE( s1, -1.0f, epsilon );
        CHECK_CLOSE( s2, 1.0f, epsilon );

        BiconvexSupport_LocalSpace( biconvex, vec3f(-1,0,0), s1, s2 );
        CHECK_CLOSE( s1, -1.0f, epsilon );
        CHECK_CLOSE( s2, 1.0f, epsilon );

        BiconvexSupport_LocalSpace( biconvex, vec3f(0,1,0), s1, s2 );
        CHECK_CLOSE( s1, -1.0f, epsilon );
        CHECK_CLOSE( s2, 1.0f, epsilon );

        BiconvexSupport_LocalSpace( biconvex, vec3f(0,-1,0), s1, s2 );
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
