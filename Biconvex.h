#ifndef BICONVEX_H
#define BICONVEX_H

#include <algorithm>
#include <float.h>

/*
    Biconvex solid.

    The biconvex solid is the intersection of two equally sized spheres

    The two spheres of specific radius are placed vertically relative to 
    each other at a specific distance to generate a biconvex solid with
    the desired width (circle diameter) and height (y axis top to bottom).
*/

class Biconvex
{
public:

    Biconvex()
    {
        memset( this, 0, sizeof( Biconvex ) );
    }

    Biconvex( float _width, float _height, float _bevel = 0 )
    {
        width = _width;
        height = _height;
        bevel = _bevel;

        sphereRadius = ( width*width + height*height ) / ( 4 * height );
        sphereRadiusSquared = sphereRadius * sphereRadius;
        sphereOffset = sphereRadius - height/2;
        sphereDot = dot( vec3f(0,0,1), normalize( vec3f( width/2, sphereOffset, 0 ) ) );

        circleRadius = width / 2;

        boundingSphereRadius = width * 0.5f;
        boundingSphereRadiusSquared = boundingSphereRadius * boundingSphereRadius;

        assert( sphereOffset >= 0.0f );
        assert( sphereRadius > 0.0f );

        const float z = ( bevel/2 + sphereOffset );
        bevelCircleRadius = sqrt( sphereRadiusSquared - z*z );
        bevelTorusMajorRadius = ( sphereOffset * bevelCircleRadius ) / ( sphereOffset + bevel/2 );
        bevelTorusMinorRadius = length( vec3f( bevelCircleRadius, 0, bevel/2 ) - vec3f( bevelTorusMajorRadius, 0, 0 ) );
    }

    float GetWidth() const { return width; }
    float GetHeight() const { return height; }
    float GetBevel() const { return bevel; }

    float GetSphereRadius() const { return sphereRadius; }
    float GetSphereRadiusSquared() const { return sphereRadiusSquared; }
    float GetSphereOffset() const { return sphereOffset; }
    float GetSphereDot() const { return sphereDot; }

    float GetBoundingSphereRadius() const { return boundingSphereRadius; }

    float GetCircleRadius() const { return circleRadius; }

    float GetBevelCircleRadius() const { return bevelCircleRadius; }
    float GetBevelTorusMajorRadius() const { return bevelTorusMajorRadius; }
    float GetBevelTorusMinorRadius() const { return bevelTorusMinorRadius; }

private:

    float width;                            // width of biconvex solid
    float height;                           // height of biconvex solid
    float bevel;                            // height of bevel measured vertically looking at the stone side-on

    float sphereRadius;                     // radius of spheres that intersect to generate this biconvex solid
    float sphereRadiusSquared;              // radius squared
    float sphereOffset;                     // vertical offset from biconvex origin to center of spheres
    float sphereDot;                        // dot product threshold for detecting circle edge vs. sphere surface collision

    float circleRadius;                     // the radius of the circle edge at the intersection of the spheres surfaces

    float boundingSphereRadius;             // bounding sphere radius for biconvex shape
    float boundingSphereRadiusSquared;      // bounding sphere radius squared

    float bevelCircleRadius;                // radius of circle on sphere at start of bevel. if no bevel then equal to circle radius
    float bevelTorusMajorRadius;            // the major radius of the torus generating the bevel
    float bevelTorusMinorRadius;            // the minor radius of the torus generating the bevel
};

inline bool PointInsideBiconvex_LocalSpace( vec3f point,
                                            const Biconvex & biconvex,
                                            float epsilon = 0.001f )
{
    const float radius = biconvex.GetSphereRadius() + epsilon;
    const float radiusSquared = radius * radius; 
    const float sphereOffset = biconvex.GetSphereOffset();

    if ( point.z() >= 0 )
    {
        // check bottom sphere (top half of biconvex)
        vec3f bottomSphereCenter( 0, 0, -sphereOffset );
        const float distanceSquared = length_squared( point - bottomSphereCenter );
        if ( distanceSquared <= radiusSquared )
            return true;
    }
    else
    {
        // check top sphere (bottom half of biconvex)
        vec3f topSphereCenter( 0, 0, sphereOffset );
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

    if ( point.z() >= 0 )
    {
        // check bottom sphere (top half of biconvex)
        vec3f bottomSphereCenter( 0, 0, -sphereOffset );
        const float distanceSquared = length_squared( point - bottomSphereCenter );
        if ( distanceSquared >= innerRadiusSquared && distanceSquared <= outerRadiusSquared )
            return true;
    }
    else
    {
        // check top sphere (bottom half of biconvex)
        vec3f topSphereCenter( 0, 0, sphereOffset );
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
    if ( point.z() > epsilon )
    {
        // top half of biconvex (bottom sphere)
        vec3f bottomSphereCenter( 0, 0, -sphereOffset );
        normal = normalize( point - bottomSphereCenter );
    }
    else if ( point.z() < -epsilon )
    {
        // bottom half of biconvex (top sphere)
        vec3f topSphereCenter( 0, 0, sphereOffset );
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
    const float sphereOffset = point.z() > 0 ? -biconvex.GetSphereOffset() : +biconvex.GetSphereOffset();
    vec3f sphereCenter( 0, 0, sphereOffset );
    vec3f a = sphereCenter + normalize( point - sphereCenter ) * sphereRadius;
    vec3f b = normalize( vec3f( point.x(), point.y(), 0 ) ) * circleRadius;
    if ( sphereOffset * a.z() > 0 )         // IMPORTANT: only consider "a" if on same half of biconvex as point
        return b;
    const float pointDot = fabs( dot( vec3f(0,0,1), normalize(point) ) );
    if ( pointDot < 1.0f - epsilon )
    {
        const float sqr_distance_a = length_squared( point - a );
        const float sqr_distance_b = length_squared( point - b );
        if ( sqr_distance_b < sqr_distance_a )
            return b;
    }
    return a;
}

inline void BiconvexSupport_LocalSpace( const Biconvex & biconvex, 
                                        vec3f axis, 
                                        float & s1, 
                                        float & s2 )
{
    const float sphereDot = biconvex.GetSphereDot();
    if ( fabs( dot( axis, vec3f(0,0,1) ) ) < sphereDot )
    {
        // in this orientation the span is the circle edge projected onto the line
        const float circleRadius = biconvex.GetCircleRadius();
        vec3f point = normalize( vec3f( axis.x(), axis.y(), 0 ) ) * circleRadius;
        s2 = dot( point, axis );
        s1 = -s2;
    }
    else
    {
        // in this orientation the span is the intersection of the spans of both spheres
        const float sphereOffset = biconvex.GetSphereOffset();
        const float sphereRadius = biconvex.GetSphereRadius();
        float t1 = dot( vec3f(0,0,-sphereOffset), axis );          // bottom sphere
        float t2 = dot( vec3f(0,0,sphereOffset), axis );           // top sphere
        if ( t1 > t2 )
            std::swap( t1, t2 );
        s1 = t2 - sphereRadius;
        s2 = t1 + sphereRadius;
    }
}

inline void BiconvexSupport_WorldSpace( const Biconvex & biconvex, 
                                        vec3f biconvexCenter,
                                        vec3f biconvexUp,
                                        vec3f axis, 
                                        float & s1,
                                        float & s2 )
{
    // same function but for the case where the biconvex
    // solid is not centered around (0,0,0) and is rotated
    // (this is the commmon case for the "other" biconvex)
    const float sphereDot = biconvex.GetSphereDot();
    if ( fabs( dot( axis, biconvexUp ) ) < sphereDot )
    {
        // in this orientation the span is the circle edge projected onto the line
        const float circleRadius = biconvex.GetCircleRadius();
        const float center_t = dot( biconvexCenter, axis );
        const float radius_t = dot( normalize( axis - biconvexUp * dot(biconvexUp,axis) ) * circleRadius, axis );
        s1 = center_t - radius_t;
        s2 = center_t + radius_t;
    }
    else
    {
        // in this orientation the span is the intersection of the spans of both spheres
        const float sphereOffset = biconvex.GetSphereOffset();
        const float sphereRadius = biconvex.GetSphereRadius();
        float t1 = dot( biconvexCenter - biconvexUp * sphereOffset, axis );          // bottom sphere
        float t2 = dot( biconvexCenter + biconvexUp * sphereOffset, axis );          // top sphere
        if ( t1 > t2 )
            std::swap( t1, t2 );
        s1 = t2 - sphereRadius;
        s2 = t1 + sphereRadius;
    }
}

struct NearestPoint
{
    vec3f biconvexPoint;
    vec3f linePoint;
};

inline void GetNearestPoint_Biconvex_Line( const Biconvex & biconvex, 
                                           vec3f biconvexCenter,
                                           vec3f biconvexUp,
                                           vec3f lineOrigin,
                                           vec3f lineDirection,
                                           vec3f & biconvexPoint,
                                           vec3f & linePoint )
{
    /*
        We have maximum three potential cases for nearest point.

            1. nearest point on the top sphere (bottom biconvex sphere surface)
            2. nearest point on the bottom sphere (top biconvex sphere surface)
            3. nearest point on the biconvex circle edge (two candidates on either side of circle)

        For cases 1&2 it is possible that the nearest point on the sphere 
        is not on the biconvex surface, in this cases these points are ignored.

        This could be coded much more efficiently but it's good enough for now!
    */

    const int MaxPoints = 4;

    int numPoints = 0;
    NearestPoint point[MaxPoints];

    // top sphere  -->  bottom of biconvex
    {
        const float sphereOffset = biconvex.GetSphereOffset();
        const float sphereRadius = biconvex.GetSphereRadius();

        const vec3f sphereCenter = biconvexCenter + biconvexUp * sphereOffset;
    
        const vec3f projectedCenter = lineOrigin + dot( sphereCenter - lineOrigin, lineDirection ) * lineDirection;

        if ( length_squared( sphereCenter - projectedCenter ) > 0.001f )
        {
            vec3f axis = normalize( projectedCenter - sphereCenter );

            vec3f spherePoint = sphereCenter + axis * sphereRadius;

            if ( dot( spherePoint, biconvexUp ) < dot( biconvexCenter, biconvexUp ) )
            {
                point[numPoints].linePoint = projectedCenter;
                point[numPoints].biconvexPoint = spherePoint;
                numPoints++;
            }
        }
    }

    // bottom sphere  --> top of biconvex
    {
        const float sphereOffset = biconvex.GetSphereOffset();
        const float sphereRadius = biconvex.GetSphereRadius();

        const vec3f sphereCenter = biconvexCenter - biconvexUp * sphereOffset;
    
        const vec3f projectedCenter = lineOrigin + dot( sphereCenter - lineOrigin, lineDirection ) * lineDirection;

        if ( length_squared( sphereCenter - projectedCenter ) > 0.001f )
        {
            vec3f axis = normalize( projectedCenter - sphereCenter );

            vec3f spherePoint = sphereCenter + axis * sphereRadius;

            if ( dot( spherePoint, biconvexUp ) > dot( biconvexCenter, biconvexUp ) )
            {
                point[numPoints].linePoint = projectedCenter;
                point[numPoints].biconvexPoint = spherePoint;
                numPoints++;
            }
        }
    }

    // circle edge
    {
        const float circleRadius = biconvex.GetCircleRadius();

        const vec3f circleCenter = biconvexCenter;
    
        const vec3f projectedCenter = lineOrigin + dot( circleCenter - lineOrigin, lineDirection ) * lineDirection;

        if ( length_squared( circleCenter - projectedCenter ) > 0.001f )
        {
            vec3f axis = normalize( projectedCenter - circleCenter );

            const vec3f biconvexLeft = cross( cross( biconvexUp, axis ), biconvexUp );

            point[numPoints].biconvexPoint = circleCenter - biconvexLeft * circleRadius;
            point[numPoints].linePoint = lineOrigin + dot( point[numPoints].biconvexPoint - lineOrigin, lineDirection ) * lineDirection;
            numPoints++;

            point[numPoints].biconvexPoint = circleCenter + biconvexLeft * circleRadius;
            point[numPoints].linePoint = lineOrigin + dot( point[numPoints].biconvexPoint - lineOrigin, lineDirection ) * lineDirection;
            numPoints++;
        }
        else
        {
            // degenerate case -- line goes through middle of biconvex
            point[numPoints].biconvexPoint = circleCenter;
            point[numPoints].linePoint = projectedCenter;
            numPoints++;
        }
    }

    assert( numPoints > 0 );

    float nearestDistanceSquared = FLT_MAX;
    NearestPoint * nearestPoint = NULL;

    for ( int i = 0; i < numPoints; ++i )
    {
        const float distanceSquared = length_squared( point[i].biconvexPoint - point[i].linePoint );
        if ( distanceSquared < nearestDistanceSquared )
        {
            nearestDistanceSquared = distanceSquared;
            nearestPoint = &point[i];
        }
    }

    assert( nearestPoint );

    biconvexPoint = nearestPoint->biconvexPoint;
    linePoint = nearestPoint->linePoint; 
}

#define TEST_BICONVEX_AXIS( name, axis )                                            \
{                                                                                   \
    float s1,s2,t1,t2;                                                              \
    BiconvexSupport_WorldSpace( biconvex, position_a, up_a, axis, s1, s2 );         \
    BiconvexSupport_WorldSpace( biconvex, position_b, up_b, axis, t1, t2 );         \
    if ( s2 + epsilon < t1 || t2 + epsilon < s1 )                                   \
        return false;                                                               \
}

/*
#define TEST_BICONVEX_AXIS( name, axis )                                            \
{                                                                                   \
    printf( "-----------------------------------\n" );                              \
    printf( name ":\n" );                                                           \
    printf( "-----------------------------------\n" );                              \
    float s1,s2,t1,t2;                                                              \
    BiconvexSupport_WorldSpace( biconvex, position_a, up_a, axis, s1, s2 );         \
    BiconvexSupport_WorldSpace( biconvex, position_b, up_b, axis, t1, t2 );         \
    printf( "(%f,%f) | (%f,%f)\n", s1, s2, t1, t2 );                                \
    if ( s2 + epsilon < t1 || t2 + epsilon < s1 )                                   \
    {                                                                               \
        printf( "not intersecting\n" );                                             \
        return false;                                                               \
    }                                                                               \
}
*/

bool Biconvex_SAT( const Biconvex & biconvex,
                   vec3f position_a,
                   vec3f position_b,
                   vec3f up_a,
                   vec3f up_b,
                   float epsilon = 0.001f )
{
    const float sphereOffset = biconvex.GetSphereOffset();

    vec3f top_a = position_a + up_a * sphereOffset;
    vec3f top_b = position_b + up_b * sphereOffset;

    vec3f bottom_a = position_a - up_a * sphereOffset;
    vec3f bottom_b = position_b - up_b * sphereOffset;

    TEST_BICONVEX_AXIS( "primary", normalize( position_b - position_a ) );
    TEST_BICONVEX_AXIS( "top_a|top_b", normalize( top_b - top_a ) );
    TEST_BICONVEX_AXIS( "top_a|bottom_b", normalize( bottom_b - top_a ) );
    TEST_BICONVEX_AXIS( "bottom_a|top_b", normalize( top_b - bottom_a ) );
    TEST_BICONVEX_AXIS( "bottom_b|top_a", normalize( bottom_b - bottom_a ) );

    return true;
}

#endif
