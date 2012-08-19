/*
    Virtual Go
    A networked physics simulation of a go board and stones
    Copyright (c) 2005-2012, Glenn Fiedler. All rights reserved.
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

    Biconvex( float _width, float _height )
    {
        width = _width;
        height = _height;

        sphereRadius = ( width*width + height*height ) / ( 4 * height );
        sphereRadiusSquared = sphereRadius * sphereRadius;
        sphereOffset = sphereRadius - height/2;
        sphereDot = dot( vec3f(0,1,0), normalize( vec3f( width/2, sphereOffset, 0 ) ) );

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
    float sphereDot;                        // dot product threshold for detecting circle edge vs. sphere surface collision

    float circleRadius;                     // the radius of the circle edge at the intersection of the spheres surfaces

    float boundingSphereRadius;             // bounding sphere radius for biconvex shape
    float boundingSphereRadiusSquared;      // bounding sphere radius squared
};

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

template <typename T> void swap( T & a, T & b )
{
    T tmp = a;
    a = b;
    b = tmp;
}

inline void BiconvexSupport_LocalSpace( const Biconvex & biconvex, 
                                        vec3f axis, 
                                        float & s1, 
                                        float & s2 )
{
    const float sphereDot = biconvex.GetSphereDot();
    if ( fabs( dot( axis, vec3f(0,1,0) ) ) < sphereDot )
    {
        // in this orientation the span is the circle edge projected onto the line
        const float circleRadius = biconvex.GetCircleRadius();
        vec3f point = normalize( vec3f( axis.x(), 0, axis.z() ) ) * circleRadius;
        s2 = dot( point, axis );
        s1 = -s2;
    }
    else
    {
        // in this orientation the span is the intersection of the spans of both spheres
        const float sphereOffset = biconvex.GetSphereOffset();
        const float sphereRadius = biconvex.GetSphereRadius();
        float t1 = dot( vec3f(0,-sphereOffset,0), axis );          // bottom sphere
        float t2 = dot( vec3f(0,sphereOffset,0), axis );           // top sphere
        if ( t1 > t2 )
            swap( t1, t2 );
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
    //const float sphereDot = biconvex.GetSphereDot();
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
            swap( t1, t2 );
        s1 = t2 - sphereRadius;
        s2 = t1 + sphereRadius;
    }
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

/*
    Rigid body class and support functions.
    We need a nice way to cache the local -> world,
    world -> local and position for a given rigid body.

    The rigid body transform class lets us do this,
    it's fundamentally a 4x4 rigid body transform matrix
    but with the inverse cached, as well as position, rotation
    and rotation inverse for quick lookup.

    (I'm not too concerned about memory usage at this point)

    In the future, we may want to split up into structure
    of arrays instead when batch processing, but for now
    this is just fine :)
*/

void PrintVector( vec3f vector )
{
    printf( "[%f,%f,%f]\n", vector.x(), vector.y(), vector.z() );
}

void PrintVector( vec4f vector )
{
    printf( "[%f,%f,%f,%f]\n", vector.x(), vector.y(), vector.z(), vector.w() );
}

void PrintMatrix( mat4f matrix )
{
    printf( "[%f,%f,%f,%f,\n %f,%f,%f,%f\n %f,%f,%f,%f\n %f,%f,%f,%f]\n", 
        simd4f_get_x(matrix.value.x), 
        simd4f_get_x(matrix.value.y), 
        simd4f_get_x(matrix.value.z), 
        simd4f_get_x(matrix.value.w), 
        simd4f_get_y(matrix.value.x), 
        simd4f_get_y(matrix.value.y), 
        simd4f_get_y(matrix.value.z), 
        simd4f_get_y(matrix.value.w), 
        simd4f_get_z(matrix.value.x), 
        simd4f_get_z(matrix.value.y), 
        simd4f_get_z(matrix.value.z), 
        simd4f_get_z(matrix.value.w), 
        simd4f_get_w(matrix.value.x), 
        simd4f_get_w(matrix.value.y), 
        simd4f_get_w(matrix.value.z), 
        simd4f_get_w(matrix.value.w) );
}

struct quat4f
{
    float x,y,z,w;

    quat4f()
    {
        // ...
    }

    quat4f( float w, float x, float y, float z )
    {
        this->w = w;
        this->x = x;
        this->y = y;
        this->z = z;
    }

    static quat4f identity()
    {
        quat4f q;
        q.x = 0;
        q.y = 0;
        q.z = 0;
        q.w = 1;
        return q;
    }

    static quat4f axisRotation( float angleRadians, vec3f axis )
    {
        const float a = angleRadians * 0.5f;
        const float s = (float) sin( a );
        const float c = (float) cos( a );
        quat4f q;
        q.w = c;
        q.x = axis.x() * s;
        q.y = axis.y() * s;
        q.z = axis.z() * s;
        return q;
    }

    float length() const
    {
        return sqrt( x*x + y*y + z*z + w*w );
    }

    void toMatrix( mat4f & matrix ) const
    {
        float fTx  = 2.0f*x;
        float fTy  = 2.0f*y;
        float fTz  = 2.0f*z;
        float fTwx = fTx*w;
        float fTwy = fTy*w;
        float fTwz = fTz*w;
        float fTxx = fTx*x;
        float fTxy = fTy*x;
        float fTxz = fTz*x;
        float fTyy = fTy*y;
        float fTyz = fTz*y;
        float fTzz = fTz*z;

        const float array[] = 
        {
            // todo: not sure if this is correct order or not (may be transposed?!)
            1.0f - ( fTyy + fTzz ), fTxy - fTwz, fTxz + fTwy, 0,
            fTxy + fTwz, 1.0f - ( fTxx + fTzz ), fTyz - fTwx, 0,
            fTxz - fTwy, fTyz + fTwx, 1.0f - ( fTxx + fTyy ), 0,
            0, 0, 0, 1
        };

        matrix.load( array );
    }

    void toAxisAngle( vec3f & axis, float & angle, const float epsilonSquared = 0.001f * 0.001f ) const
    {
        const float squareLength = x*x + y*y + z*z;
        if ( squareLength > epsilonSquared )
        {
            angle = 2.0f * (float) acos( w );
            const float inverseLength = 1.0f / (float) sqrt( squareLength );
            axis = vec3f( x, y, z ) * inverseLength;
        }
        else
        {
            axis = vec3f(1,0,0);
            angle = 0.0f;
        }
    }
};

// todo: quat4f should probably just be vec4f derived. fast operations like add, mult by scalar at least...

inline quat4f multiply( const quat4f & q1, const quat4f & q2 )
{
    quat4f result;
    result.w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z;
    result.x = q1.w*q2.x + q1.x*q2.w + q1.y*q2.z - q1.z*q2.y;
    result.y = q1.w*q2.y - q1.x*q2.z + q1.y*q2.w + q1.z*q2.x;
    result.z = q1.w*q2.z + q1.x*q2.y - q1.y*q2.x + q1.z*q2.w;
    return result;
}

inline quat4f normalize( const quat4f & q, const float epsilon = 0.0001f )
{
    const float length = q.length();
    assert( length > epsilon  );
    float inv = 1.0f / length;
    quat4f result;
    result.x = q.x * inv;
    result.y = q.y * inv;
    result.z = q.z * inv;
    result.w = q.w * inv;
    return result;
}

inline quat4f & operator += ( quat4f & q1, const quat4f & q2 )
{
    q1.x += q2.x;
    q1.y += q2.y;
    q1.z += q2.z;
    q1.w += q2.w;
    return q1;
}

inline quat4f operator + ( quat4f & q1, const quat4f & q2 )
{
    quat4f result;
    result.x = q1.x + q2.x;
    result.y = q1.y + q2.y;
    result.z = q1.z + q2.z;
    result.w = q1.w + q2.w;
    return result;
}

inline quat4f operator * ( const quat4f & q1, const quat4f & q2 )
{
    return multiply( q1, q2 );
}

inline quat4f operator * ( const quat4f & q, float s )
{
    quat4f result;
    result.x = q.x * s;
    result.y = q.y * s;
    result.z = q.z * s;
    result.w = q.w * s;
    return result;
}

inline quat4f operator * ( float s, const quat4f & q )
{
    quat4f result;
    result.x = q.x * s;
    result.y = q.y * s;
    result.z = q.z * s;
    result.w = q.w * s;
    return result;
}

struct RigidBody
{
    vec3f position;
    quat4f orientation;
    vec3f linearVelocity;
    vec3f angularVelocity;

    RigidBody()
    {
        position = vec3f(0,0,0);
        orientation = quat4f::identity();
        linearVelocity = vec3f(0,0,0);
        angularVelocity = vec3f(0,0,0);
    }
};

inline quat4f AngularVelocityToSpin( const quat4f & orientation, vec3f angularVelocity )
{
    return 0.5f * quat4f( 0, angularVelocity.x(), angularVelocity.y(), angularVelocity.z() ) * orientation;
}

inline mat4f RigidBodyInverse( mat4f matrix )
{
    /*
        How to invert a rigid body matrix
        http://graphics.stanford.edu/courses/cs248-98-fall/Final/q4.html
    */

    mat4f inverse = matrix;
    
    vec4f translation = matrix.value.w;

    inverse.value.w = simd4f_create(0,0,0,1);
    simd4x4f_transpose_inplace( &inverse.value );

    inverse.value.w = simd4f_create( -dot( matrix.value.x, translation ),
                                     -dot( matrix.value.y, translation ),
                                     -dot( matrix.value.z, translation ),
                                     1.0f );

    return inverse;
}

struct RigidBodyTransform
{
    mat4f localToWorld;
    mat4f worldToLocal;

    RigidBodyTransform( vec3f position, mat4f rotation = mat4f::identity() )
    {
        localToWorld = rotation;
        localToWorld.value.w = simd4f_create( position.x(), position.y(), position.z(), 1 );
        worldToLocal = RigidBodyInverse( localToWorld );
    }

    RigidBodyTransform( vec3f position, quat4f rotation )
    {
        rotation.toMatrix( localToWorld );
        localToWorld.value.w = simd4f_create( position.x(), position.y(), position.z(), 1 );
        worldToLocal = RigidBodyInverse( localToWorld );
    }

    vec3f GetPosition() const
    {
        return localToWorld.value.w;
    }

    vec3f GetUp() const
    {
        return transformVector( localToWorld, vec3f(0,1,0) );
    }
};

inline vec3f TransformPoint( mat4f matrix, vec3f point )
{
    return transformPoint( matrix, point );
}

inline vec3f TransformVector( mat4f matrix, vec3f normal )
{
    return transformVector( matrix, normal );
}

inline vec4f TransformPlane( mat4f matrix, vec4f plane )
{
    // todo: code below doesn't seem to get proper w coordinate

    vec3f normal( plane.x(), plane.y(), plane.z() );
    float d = plane.w();
    vec3f point = normal * d;
    normal = TransformVector( matrix, normal );
    point = TransformPoint( matrix, point );
    d = dot( point, normal );
    return vec4f( normal.x(), normal.y(), normal.z(), d );

    // IMPORTANT: to transform a plane (nx,ny,nz,d) by a matrix multiply it by the inverse of the transpose
    // http://www.opengl.org/discussion_boards/showthread.php/159564-Clever-way-to-transform-plane-by-matrix
    /*
    mat4f m = RigidBodyInverse( transpose( matrix ) );
    return m * plane;
    */
}

/*
    Go board.

    We model the go board as an axis aligned rectangular prism.
    The top surface of the go board is along the y = 0 plane for simplicity.
    Collision with the top surface is the common case, however we also need
    to consider collisions with the side planes, the top edges and the top
    corners.
*/

class Board
{
public:

    Board( float width, float height, float thickness )
    {
        this->width = width;
        this->height = height;
        this->thickness = thickness;
        halfWidth = width / 2;
        halfHeight = height / 2;
    }

    float GetWidth() const
    {
        return width;
    }

    float GetHeight() const
    {
        return height;
    }

    float GetThickness() const
    {
        return thickness;
    }

    float GetHalfWidth() const
    {
        return halfWidth;
    }

    float GetHalfHeight() const
    {
        return halfHeight;
    }

private:

    float width;                            // width of board (along x-axis)
    float height;                           // depth of board (along z-axis)
    float thickness;                        // thickness of board (along y-axis)

    float halfWidth;
    float halfHeight;
};

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

inline bool IntersectStoneBoard( const Board & board, 
                                 const Biconvex & biconvex, 
                                 const RigidBodyTransform & biconvexTransform,
                                 float epsilon = 0.001f )
{
    const float boundingSphereRadius = biconvex.GetBoundingSphereRadius();

    vec3f biconvexPosition = biconvexTransform.GetPosition();

    StoneBoardCollisionType collisionType = DetermineStoneBoardCollisionType( board, biconvexPosition, boundingSphereRadius );

    // todo: do this with SAT test -- we only want a binary result,
    // we don't care about collision point or normal!

    if ( collisionType == STONE_BOARD_COLLISION_Primary )
    {
        float s1,s2;
        vec3f biconvexUp = biconvexTransform.GetUp();
        vec3f biconvexCenter = biconvexTransform.GetPosition();
        BiconvexSupport_WorldSpace( biconvex, biconvexCenter, biconvexUp, vec3f(0,1,0), s1, s2 );
        return s1 < 0;

        /*
        vec4f plane = TransformPlane( biconvexTransform.worldToLocal, vec4f(0,1,0,0) );

        vec3f local_point;
        vec3f local_normal;
        float depth = IntersectPlaneBiconvex_LocalSpace( vec3f( plane.x(), plane.y(), plane.z() ), 
                                                         plane.w(), biconvex, local_point, local_normal );
        if ( depth > 0 )
            return true;
            */
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
                                 float epsilon = 0.001f )
{
    const float boundingSphereRadius = biconvex.GetBoundingSphereRadius();

    vec3f biconvexPosition = biconvexTransform.GetPosition();

    StoneBoardCollisionType collisionType = DetermineStoneBoardCollisionType( board, biconvexPosition, boundingSphereRadius );

    if ( collisionType == STONE_BOARD_COLLISION_Primary )
    {
        // common case: collision with primary surface of board only
        // no collision with edges or corners of board is possible

        vec4f plane = TransformPlane( biconvexTransform.worldToLocal, vec4f(0,1,0,0) );

        vec3f local_point;
        vec3f local_normal;
        depth = IntersectPlaneBiconvex_LocalSpace( vec3f( plane.x(), plane.y(), plane.z() ), 
                                                   plane.w(), biconvex, local_point, local_normal );
        if ( depth > 0 )
        {
            point = TransformPoint( biconvexTransform.localToWorld, local_point );
            normal = TransformVector( biconvexTransform.localToWorld, local_normal );
            return true;
        }
    }
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

    return false;
}

inline vec3f NearestPointOnStone( const Biconvex & biconvex, 
                                 const RigidBodyTransform & biconvexTransform, 
                                 vec3f point )
{
    vec3f nearest_local = GetNearestPointOnBiconvexSurface_LocalSpace( TransformPoint( biconvexTransform.worldToLocal, point ), biconvex );
    return TransformPoint( biconvexTransform.localToWorld, nearest_local );
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

inline bool BisectStoneBoardCollision( const Biconvex & biconvex, 
                                       const Board & board,
                                       RigidBody & rigidBody, 
                                       float dt,
                                       float & t )
{
    quat4f spin = AngularVelocityToSpin( rigidBody.orientation, rigidBody.angularVelocity );
    
    vec3f startPosition = rigidBody.position;
    quat4f startOrientation = rigidBody.orientation;

    float t0 = 0;
    float t1 = dt * 0.5f;
    float t2 = dt;

    for ( int i = 0; i < 10; ++i )
    {

        vec3f position_t0 = startPosition + rigidBody.linearVelocity * t0;
        quat4f orientation_t0 = normalize( startOrientation + spin * t0 );
        RigidBodyTransform transform_t0( position_t0, orientation_t0 );

        vec3f position_t1 = startPosition + rigidBody.linearVelocity * t1;
        quat4f orientation_t1 = normalize( startOrientation + spin * t1 );
        RigidBodyTransform transform_t1( position_t1, orientation_t1 );

        vec3f position_t2 = startPosition + rigidBody.linearVelocity * t2;
        quat4f orientation_t2 = normalize( startOrientation + spin * t2 );
        RigidBodyTransform transform_t2( position_t2, orientation_t2 );

        bool intersect_t0 = IntersectStoneBoard( board, biconvex, transform_t0 );
        bool intersect_t1 = IntersectStoneBoard( board, biconvex, transform_t1 );
        bool intersect_t2 = IntersectStoneBoard( board, biconvex, transform_t2 );

        if ( intersect_t0 )
            break;

        if ( i == 0 && !intersect_t0 && !intersect_t1 && !intersect_t2 )
        {
            t = t2;
            rigidBody.position = startPosition + rigidBody.linearVelocity * t;
            rigidBody.orientation = normalize( startOrientation + spin * t );
            return false;
        }

        if ( intersect_t1 )
        {
            t2 = t1;
            t1 = ( t2 + t0 ) / 2;
        }
        else
        {
            t0 = t1;
            t1 = ( t0 + t2 ) / 2;
        }
    }

    t = t0;
    rigidBody.position = startPosition + rigidBody.linearVelocity * t0;
    rigidBody.orientation = normalize( startOrientation + spin * t0 );

    return true;
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

const float pi = 3.14159265358979f;

float DegToRad( float degrees )
{
    return ( degrees / 360.0f ) * 2 * pi;
}

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
        TEST( stone_board_collision_type )
        {
            const float w = 10.0f;
            const float h = 10.0f;

            Board board( w*2, h*2, 0.5 );

            const float radius = 1.0f;

            CHECK( DetermineStoneBoardCollisionType( board, vec3f(0,1.1f,0), radius ) == STONE_BOARD_COLLISION_None );
            CHECK( DetermineStoneBoardCollisionType( board, vec3f(w*2,0,0), radius ) == STONE_BOARD_COLLISION_None );
            CHECK( DetermineStoneBoardCollisionType( board, vec3f(-w*2,0,0), radius ) == STONE_BOARD_COLLISION_None );
            CHECK( DetermineStoneBoardCollisionType( board, vec3f(0,0,h*2), radius ) == STONE_BOARD_COLLISION_None );
            CHECK( DetermineStoneBoardCollisionType( board, vec3f(0,0,-h*2), radius ) == STONE_BOARD_COLLISION_None );

            CHECK( DetermineStoneBoardCollisionType( board, vec3f(0,0,0), radius ) == STONE_BOARD_COLLISION_Primary );
            CHECK( DetermineStoneBoardCollisionType( board, vec3f(0,-100,0), radius ) == STONE_BOARD_COLLISION_Primary );

            CHECK( DetermineStoneBoardCollisionType( board, vec3f(-w,0,0), radius ) == STONE_BOARD_COLLISION_LeftSide );
            CHECK( DetermineStoneBoardCollisionType( board, vec3f(+w,0,0), radius ) == STONE_BOARD_COLLISION_RightSide );
            CHECK( DetermineStoneBoardCollisionType( board, vec3f(0,0,-h), radius ) == STONE_BOARD_COLLISION_TopSide );
            CHECK( DetermineStoneBoardCollisionType( board, vec3f(0,0,+h), radius ) == STONE_BOARD_COLLISION_BottomSide );

            CHECK( DetermineStoneBoardCollisionType( board, vec3f(-w,0,-h), radius ) == STONE_BOARD_COLLISION_TopLeftCorner );
            CHECK( DetermineStoneBoardCollisionType( board, vec3f(+w,0,-h), radius ) == STONE_BOARD_COLLISION_TopRightCorner );
            CHECK( DetermineStoneBoardCollisionType( board, vec3f(+w,0,+h), radius ) == STONE_BOARD_COLLISION_BottomRightCorner );
            CHECK( DetermineStoneBoardCollisionType( board, vec3f(-w,0,+h), radius ) == STONE_BOARD_COLLISION_BottomLeftCorner );
        }

        TEST( stone_board_collision_none )
        {
            const float w = 10.0f;
            const float h = 10.0f;

            Board board( w*2, h*2, 0.5 );

            Biconvex biconvex( 2.0f, 1.0f );

            const float r = biconvex.GetBoundingSphereRadius();

            mat4f localToWorld = mat4f::identity();
            mat4f worldToLocal = mat4f::identity();

            float depth;
            vec3f point, normal;

            CHECK( !IntersectStoneBoard( board, biconvex, RigidBodyTransform( vec3f(0,r*2,0) ), point, normal, depth ) );
            CHECK( !IntersectStoneBoard( board, biconvex, RigidBodyTransform( vec3f(-w-r*2,0,0) ), point, normal, depth ) );
            CHECK( !IntersectStoneBoard( board, biconvex, RigidBodyTransform( vec3f(+w+r*2,0,0) ), point, normal, depth ) );
            CHECK( !IntersectStoneBoard( board, biconvex, RigidBodyTransform( vec3f(0,0,-h-r*2) ), point, normal, depth ) );
            CHECK( !IntersectStoneBoard( board, biconvex, RigidBodyTransform( vec3f(0,0,+h+r*2) ), point, normal, depth ) );
        }

        TEST( stone_board_collision_primary )
        {
            const float epsilon = 0.001f;

            const float w = 10.0f;
            const float h = 10.0f;

            Board board( w*2, h*2, 0.5 );

            Biconvex biconvex( 2.0f, 1.0f );

            float depth;
            vec3f point, normal;

            // test at origin with identity rotation
            {
                RigidBodyTransform biconvexTransform( vec3f(0,0,0), mat4f::identity() );

                CHECK( IntersectStoneBoard( board, biconvex, biconvexTransform, point, normal, depth ) );

                CHECK_CLOSE( depth, 0.5f, epsilon );
                CHECK_CLOSE_VEC3( point, vec3f(0,-0.5f,0), epsilon );
                CHECK_CLOSE_VEC3( normal, vec3f(0,1,0), epsilon );
            }
            
            // test away from origin with identity rotation
            {
                RigidBodyTransform biconvexTransform( vec3f(w/2,0,h/2), mat4f::identity() );

                CHECK( IntersectStoneBoard( board, biconvex, biconvexTransform, point, normal, depth ) );

                CHECK_CLOSE( depth, 0.5f, epsilon );
                CHECK_CLOSE_VEC3( point, vec3f(w/2,-0.5f,h/2), epsilon );
                CHECK_CLOSE_VEC3( normal, vec3f(0,1,0), epsilon );
            }

            // test at origin flipped upside down (180 degrees)
            {
                RigidBodyTransform biconvexTransform( vec3f(0,0,0), mat4f::axisRotation( 180, vec3f(0,0,1) ) );

                CHECK( IntersectStoneBoard( board, biconvex, biconvexTransform, point, normal, depth ) );

                CHECK_CLOSE( depth, 0.5f, epsilon );
                CHECK_CLOSE_VEC3( point, vec3f(0,-0.5f,0), epsilon );
                CHECK_CLOSE_VEC3( normal, vec3f(0,1,0), epsilon );
            }

            // test away from origin flipped upside down (180degrees)
            {
                RigidBodyTransform biconvexTransform( vec3f(w/2,0,h/2), mat4f::axisRotation( 180, vec3f(0,0,1) ) );

                CHECK( IntersectStoneBoard( board, biconvex, biconvexTransform, point, normal, depth ) );

                CHECK_CLOSE( depth, 0.5f, epsilon );
                CHECK_CLOSE_VEC3( point, vec3f(w/2,-0.5f,h/2), epsilon );
                CHECK_CLOSE_VEC3( normal, vec3f(0,1,0), epsilon );
            }

            // test away from origin rotated 90 degrees clockwise around z axis
            {
                RigidBodyTransform biconvexTransform( vec3f(w/2,0,h/2), mat4f::axisRotation( 90, vec3f(0,0,1) ) );

                CHECK( IntersectStoneBoard( board, biconvex, biconvexTransform, point, normal, depth ) );

                CHECK_CLOSE( depth, 1, epsilon );
                CHECK_CLOSE_VEC3( point, vec3f(w/2,-1,h/2), epsilon );
                CHECK_CLOSE_VEC3( normal, vec3f(0,1,0), epsilon );
            }

            // test away from origin rotated 90 degrees counter-clockwise around z axis
            {
                RigidBodyTransform biconvexTransform( vec3f(w/2,0,h/2), mat4f::axisRotation( -90, vec3f(0,0,1) ) );

                CHECK( IntersectStoneBoard( board, biconvex, biconvexTransform, point, normal, depth ) );

                CHECK_CLOSE( depth, 1, epsilon );
                CHECK_CLOSE_VEC3( point, vec3f(w/2,-1,h/2), epsilon );
                CHECK_CLOSE_VEC3( normal, vec3f(0,1,0), epsilon );
            }

            // test away from origin rotated 90 degrees clockwise around x axis
            {
                RigidBodyTransform biconvexTransform( vec3f(w/2,0,h/2), mat4f::axisRotation( 90, vec3f(1,0,0) ) );

                CHECK( IntersectStoneBoard( board, biconvex, biconvexTransform, point, normal, depth ) );

                CHECK_CLOSE( depth, 1, epsilon );
                CHECK_CLOSE_VEC3( point, vec3f(w/2,-1,h/2), epsilon );
                CHECK_CLOSE_VEC3( normal, vec3f(0,1,0), epsilon );
            }

            // test away from origin rotated 90 degrees counter-clockwise around x axis
            {
                RigidBodyTransform biconvexTransform( vec3f(w/2,0,h/2), mat4f::axisRotation( -90, vec3f(1,0,0) ) );

                CHECK( IntersectStoneBoard( board, biconvex, biconvexTransform, point, normal, depth ) );

                CHECK_CLOSE( depth, 1, epsilon );
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

#else

    void ClearScreen( int displayWidth, int displayHeight )
    {
        glViewport( 0, 0, displayWidth, displayHeight );
        glDisable( GL_SCISSOR_TEST );
        glClearStencil( 0 );
        glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );     
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
    }

    void RenderBiconvex_Lines( const Biconvex & biconvex, int numSegments = 32, int numRings = 5 )
    {
        glBegin( GL_LINES );

        const float sphereRadius = biconvex.GetSphereRadius();
        const float segmentAngle = 2*pi / numSegments;

        // top surface of biconvex
        {
            const float center_y = -biconvex.GetSphereOffset();
            const float delta_y = biconvex.GetHeight() / 2 / numRings;
            for ( int i = 0; i < numRings; ++i )
            {
                const float y = i * delta_y;
                const float sy = y - center_y;
                const float radius = sqrt( sphereRadius*sphereRadius - (sy*sy) );
                for ( int j = 0; j < numSegments; ++j )
                {
                    const float angle1 = j * segmentAngle;
                    const float angle2 = angle1 + segmentAngle;
                    const float x1 = cos( angle1 ) * radius;
                    const float z1 = sin( angle1 ) * radius;    
                    const float x2 = cos( angle2 ) * radius;
                    const float z2 = sin( angle2 ) * radius;
                    glVertex3f( x1, y, z1 );
                    glVertex3f( x2, y, z2 );
                }
            }
        }

        // todo: bottom surface of biconvex
        {
            const float center_y = +biconvex.GetSphereOffset();
            const float delta_y = biconvex.GetHeight() / 2 / numRings;
            for ( int i = 0; i < numRings; ++i )
            {
                const float y = - i * delta_y;
                const float sy = y - center_y;
                const float radius = sqrt( sphereRadius*sphereRadius - (sy*sy) );
                for ( int j = 0; j < numSegments; ++j )
                {
                    const float angle1 = j * segmentAngle;
                    const float angle2 = angle1 + segmentAngle;
                    const float x1 = cos( angle1 ) * radius;
                    const float z1 = sin( angle1 ) * radius;    
                    const float x2 = cos( angle2 ) * radius;
                    const float z2 = sin( angle2 ) * radius;
                    glVertex3f( x1, y, z1 );
                    glVertex3f( x2, y, z2 );
                }
            }
        }

        glEnd();
    }

    void RenderBiconvex( const Biconvex & biconvex, int numSegments = 128, int numRings = 32 )
    {
        glBegin( GL_QUADS );

        const float sphereRadius = biconvex.GetSphereRadius();
        const float segmentAngle = 2*pi / numSegments;

        // top surface of biconvex
        {
            const float center_y = -biconvex.GetSphereOffset();
            const float delta_y = biconvex.GetHeight() / 2 / numRings;
            for ( int i = 0; i < numRings; ++i )
            {
                const float y1 = i * delta_y;
                const float s1 = y1 - center_y;
                const float r1 = sqrt( sphereRadius*sphereRadius - (s1*s1) );
                const float y2 = (i+1) * delta_y;
                const float s2 = y2 - center_y;
                const float r2 = sqrt( sphereRadius*sphereRadius - (s2*s2) );
                for ( int j = 0; j < numSegments; ++j )
                {
                    const float angle1 = j * segmentAngle;
                    const float angle2 = angle1 + segmentAngle;
                    
                    const float top_x1 = cos( angle1 ) * r1;
                    const float top_z1 = sin( angle1 ) * r1;    
                    const float top_x2 = cos( angle2 ) * r1;
                    const float top_z2 = sin( angle2 ) * r1;

                    const float bottom_x1 = cos( angle1 ) * r2;
                    const float bottom_z1 = sin( angle1 ) * r2;    
                    const float bottom_x2 = cos( angle2 ) * r2;
                    const float bottom_z2 = sin( angle2 ) * r2;

                    glVertex3f( bottom_x1, y2, bottom_z1 );
                    glVertex3f( bottom_x2, y2, bottom_z2 );

                    glVertex3f( top_x2, y1, top_z2 );
                    glVertex3f( top_x1, y1, top_z1 );
                }
            }
        }

        // bottom surface of biconvex
        {
            const float center_y = biconvex.GetSphereOffset();
            const float delta_y = biconvex.GetHeight() / 2 / numRings;
            for ( int i = 0; i < numRings; ++i )
            {
                const float y1 = -i * delta_y;
                const float s1 = y1 - center_y;
                const float r1 = sqrt( sphereRadius*sphereRadius - (s1*s1) );
                const float y2 = -(i+1) * delta_y;
                const float s2 = y2 - center_y;
                const float r2 = sqrt( sphereRadius*sphereRadius - (s2*s2) );
                for ( int j = 0; j < numSegments; ++j )
                {
                    const float angle1 = j * segmentAngle;
                    const float angle2 = angle1 + segmentAngle;
                    
                    const float top_x1 = cos( angle1 ) * r1;
                    const float top_z1 = sin( angle1 ) * r1;    
                    const float top_x2 = cos( angle2 ) * r1;
                    const float top_z2 = sin( angle2 ) * r1;

                    const float bottom_x1 = cos( angle1 ) * r2;
                    const float bottom_z1 = sin( angle1 ) * r2;    
                    const float bottom_x2 = cos( angle2 ) * r2;
                    const float bottom_z2 = sin( angle2 ) * r2;

                    glVertex3f( top_x1, y1, top_z1 );
                    glVertex3f( top_x2, y1, top_z2 );

                    glVertex3f( bottom_x2, y2, bottom_z2 );
                    glVertex3f( bottom_x1, y2, bottom_z1 );
                }
            }
        }

        glEnd();
    }

    void GetMousePickRay( int mouse_x, int mouse_y, vec3f & rayStart, vec3f & rayDirection )
    {
        // IMPORTANT: discussion about various ways to convert mouse coordinates -> pick ray
        // http://stackoverflow.com/questions/2093096/implementing-ray-picking

        GLint viewport[4];
        GLdouble modelview[16];
        GLdouble projection[16];
        glGetIntegerv( GL_VIEWPORT, viewport );
        glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
        glGetDoublev( GL_PROJECTION_MATRIX, projection );

        const float displayHeight = viewport[3];

        float x = mouse_x;
        float y = displayHeight - mouse_y;

        GLdouble x1,y1,z1;
        GLdouble x2,y2,z2;

        gluUnProject( x, y, 0, modelview, projection, viewport, &x1, &y1, &z1 );
        gluUnProject( x, y, 1, modelview, projection, viewport, &x2, &y2, &z2 );

        vec3f ray1( x1,y1,z1 );
        vec3f ray2( x2,y2,z2 );

        rayStart = ray1;
        rayDirection = normalize( ray2 - ray1 );
    }

    void RenderBoard( const Board & board )
    {
        glBegin( GL_QUADS );

        const float width = board.GetWidth();
        const float height = board.GetHeight();

        const float ideal = 1.0f;

        const int steps_x = (int) ceil( width / ideal ); 
        const int steps_y = (int) ceil( height / ideal );

        const float dx = width / steps_x;
        const float dy = height / steps_y;

        float x = - width / 2;

        for ( int i = 0; i < steps_x; ++i )
        {
            float y = - height / 2;

            for ( int j = 0; j < steps_x; ++j )
            {
                glVertex3f( x + dx, 0.0f, y );
                glVertex3f( x, 0.0f, y );
                glVertex3f( x, 0.0f, y + dy );
                glVertex3f( x + dx, 0.0f, y + dy );

                y += dy;
            }

            x += dx;
        }

        glEnd();
    }

    inline int random( int maximum )
    {
        assert( maximum > 0 );
        int randomNumber = 0;
        randomNumber = rand() % maximum;
        return randomNumber;
    }

    inline float random_float( float min, float max )
    {
        assert( max > min );
        return random( 1000000 ) / 1000000.f * ( max - min ) + min;
    }

    inline bool chance( float probability )
    {
        assert( probability >= 0.0f );
        assert( probability <= 1.0f );
        const int percent = (int) ( probability * 100.0f );
        return random(100) <= percent;
    }

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

        if ( !OpenDisplay( "Virtual Go", displayWidth, displayHeight ) )
        {
            printf( "error: failed to open display" );
            return 1;
        }

        ShowMouseCursor();

        glEnable( GL_LINE_SMOOTH );
        glEnable( GL_POLYGON_SMOOTH );
        glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
        glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );

        Biconvex biconvex( 2, 1 );

        mat4f rotation = mat4f::identity();

        double t = 0.0f;

        bool prevSpace = false;
        bool rotating = true;
        bool quit = false;

        float smoothedRotation = 0.0f;

        enum Mode
        {
            Stone,
            BiconvexSupport,
            StoneAndBoard,
            FallingStone,
            WithoutBisection,
            WithBisection
        };

        Mode mode = WithBisection;

        // for falling stone + bisection mode
        RigidBody rigidBody;
        rigidBody.position = vec3f(0,10.0f,0);
        rigidBody.angularVelocity = vec3f( random_float(-10,10), random_float(-10,10), random_float(-10,10) );

        const float dt = 1.0f / 60.0f;

        while ( !quit )
        {
            UpdateEvents();

            platform::Input input;
            
            input = platform::Input::Sample();

            if ( input.escape )
                quit = true;

            if ( input.space )
            {
                if ( !prevSpace )
                {
                    if ( mode == Stone || mode == BiconvexSupport )
                        rotating = !rotating;

                    if ( mode == FallingStone || mode == WithoutBisection || mode == WithBisection )
                    {
                        rigidBody = RigidBody();
                        rigidBody.position = vec3f(0,10.0f,0);
                        rigidBody.angularVelocity = vec3f( random_float(-10,10), random_float(-10,10), random_float(-10,10) );
                    }
                }

                prevSpace = true;
            }
            else
                prevSpace = false;

            if ( input.one )
                mode = Stone;

            if ( input.two )
                mode = BiconvexSupport;

            if ( input.three )
                mode = StoneAndBoard;

            if ( input.four )
                mode = FallingStone;

            if ( input.five )
                mode = WithoutBisection;

            if ( input.six )
                mode = WithBisection;

            ClearScreen( displayWidth, displayHeight );

            if ( mode == Stone || mode == BiconvexSupport )
            {
                if ( mode == Stone )
                {
                    const float fov = 25.0f;
                    glMatrixMode( GL_PROJECTION );
                    glLoadIdentity();
                    gluPerspective( fov, (float) displayWidth / (float) displayHeight, 0.1f, 100.0f );
                }
                else
                {
                    glMatrixMode( GL_PROJECTION );
                    glLoadIdentity();
                    glOrtho( -1.5, +1.5f, -1.0f, +1.0f, 0.1f, 100.0f );
                }

                glMatrixMode( GL_MODELVIEW );
                glLoadIdentity();
                gluLookAt( 0, 0, -5.0f, 
                           0, 0, 0, 
                           0, 1, 0 );

                // render stone

                const float targetRotation = rotating ? 0.28f : 0.0f;
                smoothedRotation += ( targetRotation - smoothedRotation ) * 0.15f;
                mat4f deltaRotation = mat4f::axisRotation( smoothedRotation, vec3f(1,2,3) );
                rotation = rotation * deltaRotation;

                RigidBodyTransform biconvexTransform( vec3f(0,0,0), rotation );

                glPushMatrix();

                float opengl_transform[16];
                biconvexTransform.localToWorld.store( opengl_transform );
                glMultMatrixf( opengl_transform );

                glEnable( GL_BLEND ); 
                glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                glLineWidth( 1 );
                glColor4f( 0.5f,0.5f,0.5f,1 );
                glEnable( GL_CULL_FACE );
                glCullFace( GL_BACK );
                glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
                RenderBiconvex( biconvex );

                glPopMatrix();

                // render intersection between stone and mouse pick ray

                int mouse_x, mouse_y;
                GetMousePosition( mouse_x, mouse_y );

                vec3f rayStart, rayDirection;
                GetMousePickRay( mouse_x, mouse_y, rayStart, rayDirection );
                
                vec3f point,normal;
                float t = IntersectRayStone( biconvex, biconvexTransform, rayStart, rayDirection, point, normal );
                if ( t >= 0 )
                {
                    glLineWidth( 2 );
                    glColor4f( 0.7f,0,0,1 );
                    glBegin( GL_LINES );
                    vec3f p1 = point;
                    vec3f p2 = point + normal * 0.1f;
                    glVertex3f( p1.x(), p1.y(), p1.z() );
                    glVertex3f( p2.x(), p2.y(), p2.z() );
                    glEnd();
                }

                if ( mode == BiconvexSupport )
                {
                    // render biconvex support along x axis

                    vec3f biconvexCenter = biconvexTransform.GetPosition();
                    vec3f biconvexUp = biconvexTransform.GetUp();
                    float s1,s2;
                    BiconvexSupport_WorldSpace( biconvex, biconvexCenter, biconvexUp, vec3f(1,0,0), s1, s2 );

                    glLineWidth( 3 );
                    glColor4f( 0,0,0.8f,1 );
                    glBegin( GL_LINES );
                    glVertex3f( s1, -10, 0 );
                    glVertex3f( s1, +10, 0 );
                    glVertex3f( s2, -10, 0 );
                    glVertex3f( s2, +10, 0 );
                    glEnd();
                }
            }
            else if ( mode == StoneAndBoard )
            {
                glMatrixMode( GL_PROJECTION );
                glLoadIdentity();
                const float fov = 25.0f;
                gluPerspective( fov, (float) displayWidth / (float) displayHeight, 0.1f, 100.0f );

                glMatrixMode( GL_MODELVIEW );
                glLoadIdentity();
                gluLookAt( 0, 5, -20.0f, 
                           0, 0, 0, 
                           0, 1, 0 );

                glEnable( GL_CULL_FACE );
                glCullFace( GL_BACK );
                glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

                // render board

                Board board( 20.0f, 20.0f, 1.0f );

                glLineWidth( 1 );
                glColor4f( 0.5f,0.5f,0.5f,1 );

                RenderBoard( board );

                // render stone

                const float targetRotation = rotating ? 0.28f : 0.0f;
                smoothedRotation += ( targetRotation - smoothedRotation ) * 0.15f;
                mat4f deltaRotation = mat4f::axisRotation( smoothedRotation, vec3f(1,2,3) );
                rotation = rotation * deltaRotation;

                RigidBodyTransform biconvexTransform( vec3f(0,biconvex.GetHeight()/2,0), rotation );

                glPushMatrix();

                float opengl_transform[16];
                biconvexTransform.localToWorld.store( opengl_transform );
                glMultMatrixf( opengl_transform );

                glEnable( GL_BLEND ); 
                glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                glLineWidth( 1 );
                glColor4f( 0.5f,0.5f,0.5f,1 );
                RenderBiconvex( biconvex, 50, 10 );

                glPopMatrix();

                // render nearest intersection with picking ray: stone or board
                {
                    int mouse_x, mouse_y;
                    GetMousePosition( mouse_x, mouse_y );

                    vec3f rayStart, rayDirection;
                    GetMousePickRay( mouse_x, mouse_y, rayStart, rayDirection );

                    vec3f board_point, board_normal, stone_point, stone_normal;
                    const float board_t = IntersectRayBoard( board, rayStart, rayDirection, board_point, board_normal );
                    const float stone_t = IntersectRayStone( biconvex, biconvexTransform, rayStart, rayDirection, stone_point, stone_normal );

                    // display stone intersection if it is nearest or the board was not hit
                    if ( ( stone_t >= 0.0f && board_t < 0.0f ) || ( stone_t >= 0.0f && stone_t < board_t ) )
                    {
                        glLineWidth( 2 );
                        glColor4f( 0.7f,0,0,1 );
                        glBegin( GL_LINES );
                        vec3f p1 = stone_point;
                        vec3f p2 = stone_point + stone_normal * 0.5f;
                        glVertex3f( p1.x(), p1.y(), p1.z() );
                        glVertex3f( p2.x(), p2.y(), p2.z() );
                        glEnd();
                    }

                    // display board intersection if it is nearest, or we did not hit the stone
                    if ( ( board_t >= 0.0f && stone_t < 0.0f ) || ( board_t >= 0.0f && board_t < stone_t ) )
                    {
                        glLineWidth( 2 );
                        glColor4f( 0.7f,0,0,1 );
                        glBegin( GL_LINES );
                        vec3f p1 = board_point;
                        vec3f p2 = board_point + board_normal * 0.5f;
                        glVertex3f( p1.x(), p1.y(), p1.z() );
                        glVertex3f( p2.x(), p2.y(), p2.z() );
                        glEnd();

                        // if we hit the board, then render yellow line between board intersection
                        // and the nearest point on the stone. this we can test that nearest point
                        // on biconvex fn is working properly.
                        vec3f nearest = NearestPointOnStone( biconvex, biconvexTransform, board_point );
                        glLineWidth( 2 );
                        glColor4f( 0.9f,0.9f,0.3f,1 );
                        glBegin( GL_LINES );
                        glVertex3f( p1.x(), p1.y(), p1.z() );
                        glVertex3f( nearest.x(), nearest.y(), nearest.z() );
                        glEnd();
                    }
                }

                // render intersection between stone and board
                {
                    float depth;
                    vec3f point,normal;
                    if ( IntersectStoneBoard( board, biconvex, biconvexTransform, point, normal, depth ) )
                    {
                        glLineWidth( 2 );
                        glColor4f( 0,0,0.85f,1 );
                        glBegin( GL_LINES );
                        vec3f p1 = point;
                        vec3f p2 = point + normal * 0.5f;
                        glVertex3f( p1.x(), p1.y(), p1.z() );
                        glVertex3f( p2.x(), p2.y(), p2.z() );
                        glEnd();
                    }
                }
            }
            else if ( mode == FallingStone || mode == WithoutBisection || mode == WithBisection )
            {
                if ( mode == FallingStone )
                {
                    const float fov = 25.0f;
                    glMatrixMode( GL_PROJECTION );
                    glLoadIdentity();
                    gluPerspective( fov, (float) displayWidth / (float) displayHeight, 0.1f, 100.0f );
    
                    glMatrixMode( GL_MODELVIEW );    
                    glLoadIdentity();
                    gluLookAt( 0, 7, -25.0f, 
                               0, 4, 0, 
                               0, 1, 0 );

                    glEnable( GL_CULL_FACE );
                    glCullFace( GL_BACK );
                    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
                }
                else if ( mode == WithoutBisection || mode == WithBisection )
                {
                    glMatrixMode( GL_PROJECTION );
                    glLoadIdentity();
                    glOrtho( -3, +3, -1, +3, -10, +10.0f );

                    glMatrixMode( GL_MODELVIEW );
                    glLoadIdentity();

                    glEnable( GL_CULL_FACE );
                    glCullFace( GL_BACK );
                    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
                }

                // render board

                Board board( 20.0f, 20.0f, 1.0f );

                glLineWidth( 1 );
                glColor4f( 0.5f,0.5f,0.5f,1 );

                RenderBoard( board );

                // update stone physics and test if board and stone are intersecting

                RigidBodyTransform biconvexTransform( rigidBody.position, rigidBody.orientation );

                bool intersecting = false;

                if ( mode != WithBisection )
                {
                    RigidBody previous = rigidBody;

                    const float scaled_dt = mode == FallingStone ? 0.25f * dt : dt;

                    const float gravity = 9.8f * 10;    // cms/sec^2
                    rigidBody.linearVelocity += vec3f(0,-gravity,0) * scaled_dt;
                    rigidBody.position += rigidBody.linearVelocity * scaled_dt;
                    quat4f spin = AngularVelocityToSpin( rigidBody.orientation, rigidBody.angularVelocity );
                    rigidBody.orientation += spin * scaled_dt;
                    rigidBody.orientation = normalize( rigidBody.orientation );

                    intersecting = IntersectStoneBoard( board, biconvex, biconvexTransform );

                    if ( intersecting )
                        rigidBody = previous;
                }
                else
                {
                    float t = 0.0f;
                    const float gravity = 9.8f * 10;    // cms/sec^2
                    rigidBody.linearVelocity += vec3f(0,-gravity,0) * dt;

                    intersecting = BisectStoneBoardCollision( biconvex, board, rigidBody, dt, t );

                    biconvexTransform = RigidBodyTransform( rigidBody.position, rigidBody.orientation );
                }

                if ( intersecting )
                {
                    rigidBody.linearVelocity = vec3f(0,0,0);
                    rigidBody.angularVelocity = vec3f(0,0,0);
                }

                // render stone

                glPushMatrix();

                float opengl_transform[16];
                biconvexTransform.localToWorld.store( opengl_transform );
                glMultMatrixf( opengl_transform );

                glEnable( GL_BLEND ); 
                glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                glLineWidth( 1 );
                glColor4f( 0.5f,0.5f,0.5f,1 );
                RenderBiconvex( biconvex, 50, 10 );

                glPopMatrix();

                if ( mode == WithoutBisection || mode == WithBisection )
                {
                    // render line representing board surface in ortho projection

                    glLineWidth( 3 );
                    glColor4f( 0,0,0.8f,1 );
                    glBegin( GL_LINES );
                    glVertex3f( -10, 0, 0 );
                    glVertex3f( +10, 0, 0 );
                    glEnd();
                }

                if ( intersecting )
                {
                    // render closest features on stone & board

                    vec3f stonePoint, stoneNormal, boardPoint, boardNormal;
                    ClosestFeaturesStoneBoard( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal );

                    glLineWidth( 3 );

                    glColor4f( 1,0,0,1 );
                    glBegin( GL_LINES );
                    vec3f p1 = boardPoint;
                    vec3f p2 = boardPoint + boardNormal;
                    glVertex3f( p1.x(), p1.y(), p1.z() );
                    glVertex3f( p2.x(), p2.y(), p2.z() );
                    glEnd();

                    glColor4f( 1,1,0,1 );
                    glBegin( GL_LINES );
                    p1 = stonePoint;
                    p2 = stonePoint + stoneNormal;
                    glVertex3f( p1.x(), p1.y(), p1.z() );
                    glVertex3f( p2.x(), p2.y(), p2.z() );
                    glEnd();
                }
            }

            // update the display
            
            UpdateDisplay( 1 );

            // update time

            if ( mode == FallingStone )
                t += 0.25f * dt;
            else
                t += dt;
        }

        CloseDisplay();

        return 0;
    }

#endif
