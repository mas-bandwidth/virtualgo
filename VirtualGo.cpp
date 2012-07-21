/*
    Virtual Go
    A networked simulation of a go board and stones
*/

#include <stdio.h>
#include <assert.h>
#include "vectorial/simd4f.h"  

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

int main()
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
