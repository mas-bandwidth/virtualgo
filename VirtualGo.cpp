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

    float GetBoundingSphereRadius() const { return boundingSphereRadius; }

private:

    float width;                            // width of biconvex solid
    float height;                           // height of biconvex solid

    float sphereRadius;                     // radius of spheres that intersect to generate this biconvex solid
    float sphereRadiusSquared;              // radius squared
    float sphereOffset;                     // vertical offset from biconvex origin to center of spheres

    float boundingSphereRadius;             // bounding sphere radius for biconvex shape
    float boundingSphereRadiusSquared;      // bounding sphere radius squared
};

inline bool IntersectRaySphere( vectorial::vec3f rayStart, 
                                vectorial::vec3f rayDirection, 
                                vectorial::vec3f sphereCenter, 
                                float sphereRadius, 
                                float sphereRadiusSquared, 
                                float & t )
{
    vectorial::vec3f delta = sphereCenter - rayStart;
    const float distanceSquared = vectorial::dot( delta, delta );
    const float timeClosest = vectorial::dot( delta, rayDirection );
    if ( timeClosest < 0 )
        return false;                   // ray points away from sphere
    const float timeHalfChordSquared = sphereRadiusSquared - distanceSquared + timeClosest*timeClosest;
    if ( timeHalfChordSquared < 0 )
        return false;                   // ray misses sphere
    t = timeClosest - sqrt( timeHalfChordSquared );
    return true;
}

inline bool IntersectRayBiconvex_LocalSpace( vectorial::vec3f rayStart, 
                                             vectorial::vec3f rayDirection, 
                                             const Biconvex & biconvex,
                                             float & t, 
                                             vectorial::vec3f & point, 
                                             vectorial::vec3f & normal )
{
    const float sphereOffset = biconvex.GetSphereOffset();
    const float sphereRadius = biconvex.GetSphereRadius();
    const float sphereRadiusSquared = biconvex.GetSphereRadiusSquared();

    // intersect ray with bottom sphere
    vectorial::vec3f bottomSphereCenter( 0, -sphereOffset, 0 );
    if ( IntersectRaySphere( rayStart, rayDirection, bottomSphereCenter, sphereRadius, sphereRadiusSquared, t ) )
    {
        point = rayStart + rayDirection * t;
        if ( point.y() >= 0 )
        {
            normal = vectorial::normalize( point - bottomSphereCenter );
            return true;
        }        
    }

    // intersect ray with top sphere
    vectorial::vec3f topSphereCenter( 0, sphereOffset, 0 );
    if ( IntersectRaySphere( rayStart, rayDirection, topSphereCenter, sphereRadius, sphereRadiusSquared, t ) )
    {
        point = rayStart + rayDirection * t;
        if ( point.y() <= 0 )
        {
            normal = vectorial::normalize( point - topSphereCenter );
            return true;
        }
    }

    return false;
}

inline bool IntersectPlaneBiconvex_LocalSpace( vectorial::vec3f planeNormal, 
                                               float planeDistance,
                                               const Biconvex & biconvex,
                                               vectorial::vec3f & point,
                                               vectorial::vec3f & normal )
{
    // TODO
    /*
        Biconvex *biconvex = *(Biconvex**) dGeomGetClassData(self);

        assert(biconvex);

        dBodyID body = dGeomGetBody(self);

        if (!body)
            return 0;

        const dReal *center = dBodyGetPosition(body);
        
        dVector4 plane;
        dGeomPlaneGetParams(other, plane);

        const dReal *rotation = dGeomGetRotation(self);

        const dReal radius = biconvex->radius;

        dVector3 up;
        up[0] = rotation[2];
        up[1] = rotation[6];
        up[2] = rotation[10];

        assert(Mathematics::Float::equal((float)(up[0]*up[0]+up[1]*up[1]+up[2]*up[2]), 1));

        // top sphere

        const dReal topDot = up[0]*plane[0] + up[1]*plane[1] + up[2]*plane[2];

        if (topDot>=0)
        {
            dVector3 topCenter;
            
            topCenter[0] = center[0] + biconvex->offset * up[0];
            topCenter[1] = center[1] + biconvex->offset * up[1];
            topCenter[2] = center[2] + biconvex->offset * up[2];

            const dReal d = plane[0]*topCenter[0] + plane[1]*topCenter[1] + plane[2]*topCenter[2] - plane[3];

            if (d<radius && d>0)
            {
                // fill in contact information

                contact[0].pos[0] = topCenter[0] - plane[0] * d;
                contact[0].pos[1] = topCenter[1] - plane[1] * d;
                contact[0].pos[2] = topCenter[2] - plane[2] * d;
                contact[0].normal[0] = plane[0];
                contact[0].normal[1] = plane[1];
                contact[0].normal[2] = plane[2];
                contact[0].depth = radius - d;
                contact[0].g1 = self;
                contact[0].g2 = other;

                // calculate contact dot relative to up

                dVector3 delta;

                delta[0] = center[0] - contact[0].pos[0];
                delta[1] = center[1] - contact[0].pos[1];
                delta[2] = center[2] - contact[0].pos[2];

                dNormalize3(delta);

                const dReal contactDot = delta[0]*up[0] + delta[1]*up[1] + delta[2]*up[2];

                if (contactDot<0)
                {
                    // edge contact

                    dVector4 centerPlane;

                    centerPlane[0] = up[0];
                    centerPlane[1] = up[1];
                    centerPlane[2] = up[2];
                    centerPlane[3] = centerPlane[0]*center[0] + centerPlane[1]*center[1] + centerPlane[2]*center[2];

                    // project on to center plane

                    const dReal centerDepth = contact[0].pos[0]*centerPlane[0] + contact[0].pos[1]*centerPlane[1] + contact[0].pos[2]*centerPlane[2] - centerPlane[3];

                    contact[0].pos[0] -= centerPlane[0]*centerDepth;
                    contact[0].pos[1] -= centerPlane[1]*centerDepth;
                    contact[0].pos[2] -= centerPlane[2]*centerDepth;

                    assert(Mathematics::Float::equal((float)(contact[0].pos[0]*centerPlane[0] + contact[0].pos[1]*centerPlane[1] + contact[0].pos[2]*centerPlane[2] - centerPlane[3]), 0));

                    // normalize contact point to radius

                    dVector3 radial;

                    radial[0] = center[0] - contact[0].pos[0];
                    radial[1] = center[1] - contact[0].pos[1];
                    radial[2] = center[2] - contact[0].pos[2];

                    dNormalize3(radial);

                    contact[0].pos[0] = center[0] - radial[0] * biconvex->bound;
                    contact[0].pos[1] = center[1] - radial[1] * biconvex->bound;
                    contact[0].pos[2] = center[2] - radial[2] * biconvex->bound;

                    // recalculate contact depth based on new contact point

                    contact[0].depth = -(plane[0]*contact[0].pos[0] + plane[1]*contact[0].pos[1] + plane[2]*contact[0].pos[2] - plane[3]);
                }

                return 1;
            }
        }

        // bottom sphere

        const dReal bottomDot = -topDot;

        if (bottomDot>=0)
        {
            dVector3 bottomCenter;
            
            bottomCenter[0] = center[0] - biconvex->offset * up[0];
            bottomCenter[1] = center[1] - biconvex->offset * up[1];
            bottomCenter[2] = center[2] - biconvex->offset * up[2];

            const dReal d = plane[0]*bottomCenter[0] + plane[1]*bottomCenter[1] + plane[2]*bottomCenter[2] - plane[3];

            if (d<radius && d>0)
            {
                // fill in contact information

                contact[0].pos[0] = bottomCenter[0] - plane[0] * d;
                contact[0].pos[1] = bottomCenter[1] - plane[1] * d;
                contact[0].pos[2] = bottomCenter[2] - plane[2] * d;
                contact[0].normal[0] = plane[0];
                contact[0].normal[1] = plane[1];
                contact[0].normal[2] = plane[2];
                contact[0].depth = radius - d;
                contact[0].g1 = self;
                contact[0].g2 = other;

                // calculate contact dot relative to up

                dVector3 delta;

                delta[0] = center[0] - contact[0].pos[0];
                delta[1] = center[1] - contact[0].pos[1];
                delta[2] = center[2] - contact[0].pos[2];

                dNormalize3(delta);

                const dReal contactDot = delta[0]*up[0] + delta[1]*up[1] + delta[2]*up[2];

                if (contactDot>0)
                {
                    // edge contact

                    dVector4 centerPlane;

                    centerPlane[0] = -up[0];
                    centerPlane[1] = -up[1];
                    centerPlane[2] = -up[2];
                    centerPlane[3] = centerPlane[0]*center[0] + centerPlane[1]*center[1] + centerPlane[2]*center[2];

                    // project on to center plane

                    const dReal centerDepth = contact[0].pos[0]*centerPlane[0] + contact[0].pos[1]*centerPlane[1] + contact[0].pos[2]*centerPlane[2] - centerPlane[3];

                    contact[0].pos[0] -= centerPlane[0]*centerDepth;
                    contact[0].pos[1] -= centerPlane[1]*centerDepth;
                    contact[0].pos[2] -= centerPlane[2]*centerDepth;

                    assert(Mathematics::Float::equal((float)(contact[0].pos[0]*centerPlane[0] + contact[0].pos[1]*centerPlane[1] + contact[0].pos[2]*centerPlane[2] - centerPlane[3]), 0));

                    // normalize contact point to radius

                    dVector3 radial;

                    radial[0] = center[0] - contact[0].pos[0];
                    radial[1] = center[1] - contact[0].pos[1];
                    radial[2] = center[2] - contact[0].pos[2];

                    dNormalize3(radial);

                    contact[0].pos[0] = center[0] - radial[0] * biconvex->bound;
                    contact[0].pos[1] = center[1] - radial[1] * biconvex->bound;
                    contact[0].pos[2] = center[2] - radial[2] * biconvex->bound;

                    // recalculate contact depth based on new contact point

                    contact[0].depth = -(plane[0]*contact[0].pos[0] + plane[1]*contact[0].pos[1] + plane[2]*contact[0].pos[2] - plane[3]);
                }

                return 1;
            }
        }
        
        return 0;
    */

    return false;
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
        printf( "=======================================\n" );
    }

    {
        vectorial::vec3f rayStart(0,0,-10);
        vectorial::vec3f rayDirection(0,0,1);
        vectorial::vec3f sphereCenter(0,0,0);
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
        vectorial::vec3f planeNormal(0,1,0);
        float planeDistance = 0;
        vectorial::vec3f point, normal;
        bool collided = IntersectPlaneBiconvex_LocalSpace( planeNormal, planeDistance, biconvex, point, normal );

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
        }
        else
            printf( " + no collision!\n" );
        printf( "=======================================\n" );
    } 
}
