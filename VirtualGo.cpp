/*
    Virtual Go
    A networked simulation of a go board and stones
*/

#include <stdio.h>
#include <assert.h>
#include "vectorial/vec2f.h"
#include "vectorial/vec3f.h"
#include "vectorial/vec4f.h"
#include "vectorial/mat4f.h"
   
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

    float GetBoundingSphereRadius() const { return boundingSphereRadius; }

private:

    float width;                            // width of biconvex solid
    float height;                           // height of biconvex solid

    float sphereRadius;                     // radius of spheres that intersect to generate this biconvex solid
    float sphereRadiusSquared;              // radius squared
    float sphereOffset;                     // vertical offset from biconvex origin to center of spheres
    float sphereDot;                        // dot product of "up" with vector from to sphere center to "P" on biconvex edge circle

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
                                                          const Biconvex & biconvex )
{
    const float circleRadius = biconvex.GetWidth() / 2;
    const float sphereRadius = biconvex.GetSphereRadius();
    const float sphereOffset = point.y() > 0 ? -biconvex.GetSphereOffset() : +biconvex.GetSphereOffset();
    vec3f sphereCenter( 0, sphereOffset, 0 );
    vec3f a = normalize( point - sphereCenter ) * sphereRadius;
    vec3f b = normalize( point - vec3f(0,point.y(),0) ) * circleRadius;
    const float sqr_distance_a = length_squared( point - a );
    const float sqr_distance_b = length_squared( point - b );
    if ( sqr_distance_a < sqr_distance_b )
        return a;
    else
        return b;
}

inline bool IntersectPlaneBiconvex_LocalSpace( vec3f planeNormal,
                                               float planeDistance,
                                               const Biconvex & biconvex,
                                               vec3f & point,
                                               vec3f & normal,
                                               float depth )
{
    const float sphereDot = biconvex.GetSphereDot();
    const float planeNormalDot = fabs( dot( vec3f(0,1,0), planeNormal ) );
    if ( planeNormalDot > sphereDot )
    {
        // sphere surface collision
        const float sphereRadius = biconvex.GetSphereRadius();
        const float sphereOffset = planeNormal.y() < 0 ? -biconvex.GetSphereOffset() : +biconvex.GetSphereOffset();
        vec3f sphereCenter( 0, sphereOffset, 0 );
        point = normalize( sphereCenter - planeNormal ) * sphereRadius;
    }
    else
    {
        // circle edge collision
        const float circleRadius = biconvex.GetWidth() / 2;
        point = normalize( vec3f( -planeNormal.x(), 0, -planeNormal.z() ) ) * circleRadius;
    }
    normal = planeNormal;
    depth = dot( -planeNormal, point );
    return depth > 0;                           // todo: probably want some epsilon here
}

int main()
{
    {
        Biconvex biconvex( 2.0f, 1.0f );

        printf( "=======================================\n" );
        printf( "biconvex:\n" );
        printf( "=======================================\n" );
        printf( " + width = %.2f\n", biconvex.GetWidth() );
        printf( " + height = %.2f\n", biconvex.GetHeight() );
        printf( " + sphere radius = %.2f\n", biconvex.GetSphereRadius() );
        printf( " + sphere offset = %.2f\n", biconvex.GetSphereOffset() );
        printf( " + sphere dot = %.2f\n", biconvex.GetSphereDot() );
        printf( "=======================================\n" );
    }

    {
        vec3f rayStart(0,0,-10);
        vec3f rayDirection(0,0,1);
        vec3f sphereCenter(0,0,0);
        float sphereRadius = 1.0f;
        float sphereRadiusSquared = sphereRadius * sphereRadius;
        float t = 0;
        bool hit = IntersectRaySphere( rayStart, rayDirection, sphereCenter, sphereRadius, sphereRadiusSquared, t );

        printf( "=======================================\n" );
        printf( "ray vs. sphere:\n" );
        printf( "=======================================\n" );
        printf( " + ray start = (%f,%f,%f)\n", rayStart.x(), rayStart.y(), rayStart.z() );
        printf( " + ray direction = (%f,%f,%f)\n", rayDirection.x(), rayDirection.y(), rayDirection.z() );
        if ( hit )
        {
            printf( " + hit: t = %f\n", t );
        }
        else
            printf( " + missed!\n" );
        printf( "=======================================\n" );
    }

    {
        Biconvex biconvex( 2.0f, 1.0f );
        vec3f point(0,0.5,0.5f);
        bool inside = PointInsideBiconvex_LocalSpace( point, biconvex );

        printf( "=======================================\n" );
        printf( "point inside biconvex (local space):\n" );
        printf( "=======================================\n" );
        printf( " + point = (%f,%f,%f)\n", point.x(), point.y(), point.z() );
        if ( inside )
            printf( " + inside!\n" );
        else
            printf( " + outside!\n" );
        printf( "=======================================\n" );
    }

    {
        Biconvex biconvex( 2.0f, 1.0f );
        vec3f point(0,0.5,0);
        bool inside = IsPointOnBiconvexSurface_LocalSpace( point, biconvex );

        printf( "========================================\n" );
        printf( "point on biconvex surface (local space):\n" );
        printf( "========================================\n" );
        printf( " + point = (%f,%f,%f)\n", point.x(), point.y(), point.z() );
        if ( inside )
            printf( " + on surface!\n" );
        else
            printf( " + not on surface!\n" );
        printf( "========================================\n" );
    }

    {
        Biconvex biconvex( 2.0f, 1.0f );
        vec3f point(1,0,0);
        vec3f normal(0,0,0);
        GetBiconvexSurfaceNormalAtPoint_LocalSpace( point, biconvex, normal );

        printf( "=========================================\n" );
        printf( "normal on biconvex surface (local space):\n" );
        printf( "=========================================\n" );
        printf( " + point = (%f,%f,%f)\n", point.x(), point.y(), point.z() );
        printf( " + normal = (%f,%f,%f)\n", normal.x(), normal.y(), normal.z() );
        printf( "=========================================\n" );
    }

    {
        Biconvex biconvex( 2.0f, 1.0f );
        vec3f point(10,2,0);
        vec3f nearest = GetNearestPointOnBiconvexSurface_LocalSpace( point, biconvex );

        printf( "================================================\n" );
        printf( "nearest point on biconvex surface (local space):\n" );
        printf( "================================================\n" );
        printf( " + point = (%f,%f,%f)\n", point.x(), point.y(), point.z() );
        printf( " + nearest = (%f,%f,%f)\n", nearest.x(), nearest.y(), nearest.z() );
        printf( "================================================\n" );
    }

    // todo: code below doesn't seem to be getting valid results back from the fn
    {
        Biconvex biconvex( 2.0f, 1.0f );
        vec3f planeNormal(0,1,0);
        float planeDistance = 0;
        vec3f point, normal;
        float depth;
        bool collided = IntersectPlaneBiconvex_LocalSpace( planeNormal, planeDistance, biconvex, point, normal, depth );

        printf( "=======================================\n" );
        printf( "plane vs. biconvex (local space):\n" );
        printf( "=======================================\n" );
        printf( " + plane normal = (%f,%f,%f)\n", planeNormal.x(), planeNormal.y(), planeNormal.z() );
        printf( " + plane distance = %f\n", planeDistance );
        if ( collided )
        {
            printf( " + collision:\n" );
            printf( " + point = (%f,%f,%f)\n", point.x(), point.y(), point.z() );
            printf( " + normal = (%f,%f,%f)\n", normal.x(), normal.y(), normal.z() );
            printf( " + depth = %f\n", depth );
        }
        else
            printf( " + no collision!\n" );
        printf( "=======================================\n" );
    }
}
