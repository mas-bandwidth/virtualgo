/*
    Virtual Go
    A networked simulation of a go board and stones
*/

#include "Platform.h"
#include <stdio.h>
#include <assert.h>
#include "vectorial/vec2f.h"
#include "vectorial/vec3f.h"
#include "vectorial/vec4f.h"
#include "vectorial/mat4f.h"
#include "UnitTest++/UnitTest++.h"
#include "UnitTest++/TestRunner.h"
#include "UnitTest++/TestReporterStdout.h"

#if PLATFORM == PLATFORM_MAC
#include <OpenGl/gl.h>
#include <OpenGl/glu.h>
#include <OpenGL/glext.h>
#include <OpenGL/OpenGL.h>
#endif
   
using namespace platform;
using namespace vectorial;

class Biconvex
{
public:

    Biconvex( float _width, float _height )
    {
        width = _width;
        height = _height;

        sphereRadius = ( width*width + height*height ) / ( 4 * height );
        sphereRadiusSquared = sphereRadius * sphereRadius;
        sphereOffset = sphereRadius - height/2;
        sphereDot = dot( vec3f(0,1,0), vec3f(width/2,0,0) - vec3f(0,-sphereOffset,0) );

        circleRadius = width / 2;

        boundingSphereRadius = width * 0.5f;
        boundingSphereRadiusSquared = boundingSphereRadius * boundingSphereRadius;

        assert( sphereOffset > 0.0f );
        assert( sphereRadius > 0.0f );
    }

    float GetWidth() const { return width; }
    float GetHeight() const { return height; }

    float GetSphereRadius() const { return sphereRadius; }
    float GetSphereRadiusSquared() const { return sphereRadiusSquared; }
    float GetSphereOffset() const { return sphereOffset; }
    float GetSphereDot() const { return sphereDot; }

    float GetCircleRadius() const { return circleRadius; }

    float GetBoundingSphereRadius() const { return boundingSphereRadius; }

private:

    float width;                            // width of biconvex solid
    float height;                           // height of biconvex solid

    float sphereRadius;                     // radius of spheres that intersect to generate this biconvex solid
    float sphereRadiusSquared;              // radius squared
    float sphereOffset;                     // vertical offset from biconvex origin to center of spheres
    float sphereDot;                        // dot product of "up" with vector from to sphere center to "P" on biconvex edge circle

    float circleRadius;                     // the radius of the circle edge at the intersection of the spheres surfaces

    float boundingSphereRadius;             // bounding sphere radius for biconvex shape
    float boundingSphereRadiusSquared;      // bounding sphere radius squared
};

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

inline bool PointInsideBiconvex_LocalSpace( vec3f point,
                                            const Biconvex & biconvex,
                                            float epsilon = 0.001f )
{
    const float radius = biconvex.GetSphereRadius() + epsilon;
    const float radiusSquared = radius * radius; 
    const float sphereOffset = biconvex.GetSphereOffset();

    if ( point.y() >= 0 )
    {
        // check bottom sphere (top half of biconvex)
        vec3f bottomSphereCenter( 0, -sphereOffset, 0 );
        const float distanceSquared = length_squared( point - bottomSphereCenter );
        if ( distanceSquared <= radiusSquared )
            return true;
    }
    else
    {
        // check top sphere (bottom half of biconvex)
        vec3f topSphereCenter( 0, sphereOffset, 0 );
        const float distanceSquared = length_squared( point - topSphereCenter );
        if ( distanceSquared <= radiusSquared )
            return true;
    }

    return false;
}

inline bool IsPointOnBiconvexSurface_LocalSpace( vec3f point, 
                                                 const Biconvex & biconvex,
                                                 float epsilon = 0.001f )
{
    const float innerRadius = biconvex.GetSphereRadius() - epsilon;
    const float outerRadius = biconvex.GetSphereRadius() + epsilon;
    const float innerRadiusSquared = innerRadius * innerRadius;
    const float outerRadiusSquared = outerRadius * outerRadius;
    const float sphereOffset = biconvex.GetSphereOffset();

    if ( point.y() >= 0 )
    {
        // check bottom sphere (top half of biconvex)
        vec3f bottomSphereCenter( 0, -sphereOffset, 0 );
        const float distanceSquared = length_squared( point - bottomSphereCenter );
        if ( distanceSquared >= innerRadiusSquared && distanceSquared <= outerRadiusSquared )
            return true;
    }
    else
    {
        // check top sphere (bottom half of biconvex)
        vec3f topSphereCenter( 0, sphereOffset, 0 );
        const float distanceSquared = length_squared( point - topSphereCenter );
        if ( distanceSquared >= innerRadiusSquared && distanceSquared <= outerRadiusSquared )
            return true;
    }

    return false;
}

inline void GetBiconvexSurfaceNormalAtPoint_LocalSpace( vec3f point, 
                                                        const Biconvex & biconvex,
                                                        vec3f & normal,
                                                        float epsilon = 0.001 )
{
    const float sphereOffset = biconvex.GetSphereOffset();
    if ( point.y() > epsilon )
    {
        // top half of biconvex (bottom sphere)
        vec3f bottomSphereCenter( 0, -sphereOffset, 0 );
        normal = normalize( point - bottomSphereCenter );
    }
    else if ( point.y() < -epsilon )
    {
        // bottom half of biconvex (top sphere)
        vec3f topSphereCenter( 0, sphereOffset, 0 );
        normal = normalize( point - topSphereCenter );
    }
    else
    {
        // circle edge
        normal = normalize( point );       
    }
}

inline vec3f GetNearestPointOnBiconvexSurface_LocalSpace( vec3f point, 
                                                          const Biconvex & biconvex,
                                                          float epsilon = 0.001f )
{
    const float circleRadius = biconvex.GetCircleRadius();
    const float sphereRadius = biconvex.GetSphereRadius();
    const float sphereOffset = point.y() > 0 ? -biconvex.GetSphereOffset() : +biconvex.GetSphereOffset();
    vec3f sphereCenter( 0, sphereOffset, 0 );
    vec3f a = sphereCenter + normalize( point - sphereCenter ) * sphereRadius;
    vec3f b = normalize( vec3f( point.x(), 0, point.z() ) ) * circleRadius;
    if ( sphereOffset * a.y() > 0 )         // IMPORTANT: only consider "a" if on same half of biconvex as point
        return b;
    const float sphereDot = biconvex.GetSphereDot();
    const float pointDot = fabs( dot( vec3f(0,1,0), normalize(point) ) );
    if ( pointDot < 1.0f - epsilon )
    {
        const float sqr_distance_a = length_squared( point - a );
        const float sqr_distance_b = length_squared( point - b );
        if ( sqr_distance_b < sqr_distance_a )
            return b;
    }
    return a;
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
        point = sphereCenter + ( normalize( sphereCenter - planeNormal ) * sphereRadius );
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

template <typename T> void swap( T & a, T & b )
{
    T tmp = a;
    a = b;
    b = tmp;
}

inline void BiconvexSupport_LocalSpace( Biconvex & biconvex, 
                                        vec3f direction, 
                                        float & s1, 
                                        float & s2 )
{
    const float sphereDot = biconvex.GetSphereDot();
    if ( fabs( dot( direction, vec3f(0,1,0) ) ) < sphereDot )
    {
        // in this orientation the span is the circle edge projected onto the line
        const float circleRadius = biconvex.GetCircleRadius();
        vec3f point = normalize( vec3f( direction.x(), 0, direction.z() ) ) * circleRadius;
        s2 = dot( point, direction );
        s1 = -s2;
    }
    else
    {
        // in this orientation the span is the intersection of the spans of both spheres
        const float sphereOffset = biconvex.GetSphereOffset();
        const float sphereRadius = biconvex.GetSphereRadius();
        float t1 = dot( vec3f(0,-sphereOffset,0), direction );          // bottom sphere
        float t2 = dot( vec3f(0,sphereOffset,0), direction );           // top sphere
        if ( t1 > t2 )
            swap( t1, t2 );
        s1 = t2 - sphereRadius;
        s2 = t1 + sphereRadius;
    }
}

// todo: get the local space one making sense first!
/*
inline void BiconvexSupport_WorldSpace( Biconvex & biconvex, 
                                        vec3f biconvexCenter,
                                        vec3f biconvexUp,
                                        vec3f direction, 
                                        float & s1,
                                        float & s2 )
{
    // same function but for the case where the biconvex
    // solid is not centered around (0,0,0) and is rotated
    // (this is the commmon case for the "other" biconvex)
    const float sphereOffset = biconvex.GetSphereOffset();
    const float sphereRadius = biconvex.GetSphereRadius();
    float t1 = dot( biconvexCenter + biconvexUp * sphereOffset, direction );          // top sphere
    float t2 = dot( biconvexCenter - biconvexUp * sphereOffset, direction );          // bottom sphere
    if ( t1 > t2 )
        swap( t1, t2 );
    s1 = t2 - sphereRadius;
    s2 = t1 + sphereRadius;
}
*/

#if VIRTUALGO_CONSOLE

    int main()
    {
        Biconvex biconvex( 2.0f, 1.0f );
        printf( "hello world\n" );
        return 0;
    }

#elif VIRTUALGO_TEST

    #define CHECK_CLOSE_VEC3( value, expected, epsilon ) CHECK_CLOSE( length( value - expected ), 0.0f, epsilon )

    SUITE( Intersection )
    {
        TEST( biconvex )
        {
            Biconvex biconvex( 2.0f, 1.0f );
            const float epsilon = 0.001f;
            CHECK_CLOSE( biconvex.GetWidth(), 2.0f, epsilon );
            CHECK_CLOSE( biconvex.GetHeight(), 1.0f, epsilon );
            CHECK_CLOSE( biconvex.GetSphereRadius(), 1.25f, epsilon );
            CHECK_CLOSE( biconvex.GetSphereOffset(), 0.75f, epsilon );
            CHECK_CLOSE( biconvex.GetSphereDot(), 0.75f, epsilon );
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

        TEST( biconvex_support )
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

#else

    int main()
    {
        printf( "[virtual go]\n" );

        int displayWidth, displayHeight;
        GetDisplayResolution( displayWidth, displayHeight );

        #ifdef LETTERBOX
        displayWidth = 1280;
        displayHeight = 800;
        #endif

        printf( "display resolution is %d x %d\n", displayWidth, displayHeight );

        HideMouseCursor();
        
        if ( !OpenDisplay( "Virtual Go", displayWidth, displayHeight ) )
        {
            printf( "error: failed to open display" );
            return 1;
        }
        
        bool quit = false;
        while ( !quit )
        {
            platform::Input input;
            
            input = platform::Input::Sample();

            if ( input.escape )
                quit = true;

            glViewport( 0, 0, displayWidth, displayHeight );
            glDisable( GL_SCISSOR_TEST );
            glClearStencil( 0 );
            glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );     
            glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
            
            UpdateDisplay( 1 );
        }

        CloseDisplay();

        return 0;
    }

#endif
