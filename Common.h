#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include "vectorial/vec2f.h"
#include "vectorial/vec3f.h"
#include "vectorial/vec4f.h"
#include "vectorial/mat4f.h"

using namespace vectorial;

const float pi = 3.14159265358979f;

inline int random( int maximum )
{
    assert( maximum > 0 );
    int randomNumber = 0;
    randomNumber = rand() % maximum;
    return randomNumber;
}

uint32_t hash( const uint8_t * data, uint32_t length, uint32_t hash = 0 )
{
    for ( uint32_t i = 0; i < length; ++i )
    {
        hash += data[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    return hash;
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

float DegToRad( float degrees )
{
    return ( degrees / 360.0f ) * 2 * pi;
}

inline float max( float v1, float v2 )
{
    return v1 > v2 ? v1 : v2;
}

inline float clamp( float v, float min, float max )
{
    if ( v > max )
        return max;
    if ( v < min )
        return min;
    return v;
}

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
            1.0f - ( fTyy + fTzz ), fTxy + fTwz, fTxz - fTwy, 0,
            fTxy - fTwz, 1.0f - ( fTxx + fTzz ), fTyz + fTwx, 0, 
            fTxz + fTwy, fTyz - fTwx, 1.0f - ( fTxx + fTyy ), 0,
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
    // hack: slow version -- original code is commented out. it does not seem to be getting correct w coordinate?!
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

#endif
