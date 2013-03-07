#ifndef MESH_H
#define MESH_H

#include "Common.h"

#include <list>
#include <vector>
#include <algorithm>
#include <assert.h>

struct Vertex
{
    float u;
    float v;
    vec3f position;
    vec3f normal;
};

class Mesh
{
public:

    Mesh( int numBuckets = 2048 )
    {
        this->numBuckets = numBuckets;
        buckets = new std::list<int>[numBuckets];
    }

    ~Mesh()
    {
        delete [] buckets;
        buckets = NULL;
    }

    void Clear()
    {
        for ( int i = 0; i < numBuckets; ++i )
            buckets[i].clear();
        indexBuffer.clear();
        vertexBuffer.clear();
    }

    void AddTriangle( const Vertex & a, const Vertex & b, const Vertex & c )
    {
        indexBuffer.push_back( AddVertex( a ) );
        indexBuffer.push_back( AddVertex( b ) );
        indexBuffer.push_back( AddVertex( c ) );
    }

    int GetNumIndices() const { return indexBuffer.size(); }
    int GetNumVertices() const { return vertexBuffer.size(); }
    int GetNumTriangles() const { return indexBuffer.size() / 3; }

    Vertex * GetVertexBuffer() { assert( vertexBuffer.size() ); return &vertexBuffer[0]; }

    int * GetIndexBuffer() { assert( indexBuffer.size() ); return &indexBuffer[0]; }

    int GetLargestBucketSize() const
    {
        int largest = 0;
        for ( int i = 0; i < numBuckets; ++i )
        {
            if ( buckets[i].size() > largest )
                largest = buckets[i].size();
        }
        return largest;
    }

    float GetAverageBucketSize() const
    {
        float average = 0.0f;
        for ( int i = 0; i < numBuckets; ++i )
            average += buckets[i].size();
        return average / numBuckets;
    }

    int GetNumZeroBuckets() const
    {
        int numZero = 0;
        for ( int i = 0; i < numBuckets; ++i )
            if ( buckets[i].size() == 0 )
                numZero++;
        return numZero;
    }

protected:

    int GetGridCellBucket( int x, int y, int z )
    {
        uint32_t data[3] = { x, y, z };
        return hash( (const uint8_t*) &data[0], 12 ) % numBuckets;
    }

    int AddVertex( const Vertex & vertex, float grid = 0.1f, float epsilon = 0.001f )
    {
        const float epsilonSquared = epsilon * epsilon;
        
        const float inverseGrid = 1.0f / grid;

        int x = (int) floor( vertex.position.x() * inverseGrid );
        int y = (int) floor( vertex.position.y() * inverseGrid );
        int z = (int) floor( vertex.position.z() * inverseGrid );

        int bucketIndex = GetGridCellBucket( x, y, z );

        std::list<int> & bucket = buckets[bucketIndex];

        int index = -1;

        for ( std::list<int>::iterator itor = bucket.begin(); itor != bucket.end(); ++itor )
        {
            const int i = *itor;
            const Vertex & v = vertexBuffer[i];
            vec3f dp = v.position - vertex.position;
            vec3f dn = v.normal - vertex.normal;
            if ( fabs( v.u - vertex.u ) < epsilonSquared &&
                 fabs( v.v - vertex.v ) < epsilonSquared &&
                 length_squared( dp ) < epsilonSquared && 
                 length_squared( dn ) < epsilonSquared )
            {
                index = i;
                break;
            }
        }

        if ( index != -1 )
            return index;

        vertexBuffer.push_back( vertex );

        index = vertexBuffer.size() - 1;

        const float vx = vertex.position.x();
        const float vy = vertex.position.y();
        const float vz = vertex.position.z();

        for ( int ix = -1; ix <= 1; ++ix )
        {
            for ( int iy = -1; iy <= 1; ++iy )
            {
                for ( int iz = -1; iz <= 1; ++iz )
                {
                    const float x1 = grid * ( x + ix ) - epsilon;
                    const float y1 = grid * ( y + iy ) - epsilon;
                    const float z1 = grid * ( z + iz ) - epsilon;

                    const float x2 = grid * ( x + ix + 1 ) + epsilon;
                    const float y2 = grid * ( y + iy + 1 ) + epsilon;
                    const float z2 = grid * ( z + iz + 1 ) + epsilon;

                    if ( vx >= x1 && vx <= x2 &&
                         vy >= y1 && vy <= y2 &&
                         vz >= z1 && vz <= z2 )
                    {
                        buckets[ GetGridCellBucket( x + ix, y + iy, z + iz ) ].push_back( index );
                    }
                }
            }
        }

        return index;
    }

private:

    std::vector<Vertex> vertexBuffer;
    
    std::vector<int> indexBuffer;

    int numBuckets;
    std::list<int> * buckets;
};

void SubdivideBiconvexMesh( Mesh & mesh,
                            const Biconvex & biconvex, 
                            bool i, bool j, bool k,
                            vec3f a, vec3f b, vec3f c,
                            vec3f an, vec3f bn, vec3f cn,
                            vec3f sphereCenter,
                            bool clockwise,
                            float h, int depth, int subdivisions,
                            float texture_a, float texture_b )
{
    // edges: i = c -> a
    //        j = a -> b
    //        k = b -> c

    // vertices:
    //
    //        a
    //
    //     e     d 
    //
    //  b     f     c

    if ( depth < subdivisions )
    {
        vec3f d = ( a + c ) / 2;
        vec3f e = ( a + b ) / 2;
        vec3f f = ( b + c ) / 2;

        vec3f dn, en, fn;

        const float sphereRadius = biconvex.GetSphereRadius();
        const float bevelCircleRadius = biconvex.GetBevelCircleRadius();

        vec3f bevelOffset( 0, h, 0 );

        if ( i )
        {
            d = normalize( d - bevelOffset ) * bevelCircleRadius + bevelOffset;
            dn = normalize( d - sphereCenter );
        }
        else
        {
            dn = normalize( d - sphereCenter );
            d = sphereCenter + dn * sphereRadius;
        }

        if ( j )
        {
            e = normalize( e - bevelOffset ) * bevelCircleRadius + bevelOffset;
            en = normalize( e - sphereCenter );
        }
        else
        {
            en = normalize( e - sphereCenter );
            e = sphereCenter + en * sphereRadius;
        }

        if ( k )
        {
            f = normalize( f - bevelOffset ) * bevelCircleRadius + bevelOffset;
            fn = normalize( f - sphereCenter );
        }
        else
        {
            fn = normalize( f - sphereCenter );
            f = sphereCenter + fn * sphereRadius;
        }

        depth++;

        // vertices:
        //
        //        a
        //
        //     e     d 
        //
        //  b     f     c

        SubdivideBiconvexMesh( mesh, biconvex, i, j, false, a, e, d, an, en, dn, sphereCenter, clockwise, h, depth, subdivisions, texture_a, texture_b );
        SubdivideBiconvexMesh( mesh, biconvex, false, j, k, e, b, f, en, bn, fn, sphereCenter, clockwise, h, depth, subdivisions, texture_a, texture_b );
        SubdivideBiconvexMesh( mesh, biconvex, i, false, k, d, f, c, dn, fn, cn, sphereCenter, clockwise, h, depth, subdivisions, texture_a, texture_b );
        SubdivideBiconvexMesh( mesh, biconvex, false, false, false, d, e, f, dn, en, fn, sphereCenter, clockwise, h, depth, subdivisions, texture_a, texture_b );
    }
    else
    {
        Vertex v1,v2,v3;

        v1.u = a.x() * texture_a + texture_b;
        v1.v = a.z() * texture_a + texture_b;
        v1.position = a;
        v1.normal = an;

        v2.u = b.x() * texture_a + texture_b;
        v2.v = b.z() * texture_a + texture_b;
        v2.position = b;
        v2.normal = bn;

        v3.u = c.x() * texture_a + texture_b;
        v3.v = c.z() * texture_a + texture_b;
        v3.position = c;
        v3.normal = cn;

        if ( !clockwise )
            mesh.AddTriangle( v1, v2, v3 );
        else
            mesh.AddTriangle( v1, v3, v2 );
    }
}

void GenerateBiconvexMesh( Mesh & mesh, const Biconvex & biconvex, int subdivisions = 5, int numTriangles = 5, float epsilon = 0.001f )
{
    const float bevelCircleRadius = biconvex.GetBevelCircleRadius();

    const float deltaAngle = 360.0f / numTriangles;

    const float h = biconvex.GetBevel() / 2;

    const float texture_a = 1 / biconvex.GetWidth();
    const float texture_b = 0.5f;

    // top

    for ( int i = 0; i < numTriangles; ++i )
    {
        mat4f r1 = mat4f::axisRotation( deltaAngle * i, vec3f(0,1,0) );
        mat4f r2 = mat4f::axisRotation( deltaAngle * ( i + 1 ), vec3f(0,1,0) );

        vec3f a = vec3f( 0, -biconvex.GetSphereOffset() + biconvex.GetSphereRadius(), 0 );
        vec3f b = transformPoint( r1, vec3f( 0, h, bevelCircleRadius ) );
        vec3f c = transformPoint( r2, vec3f( 0, h, bevelCircleRadius ) );

        const vec3f sphereCenter = vec3f( 0, -biconvex.GetSphereOffset(), 0 );

        vec3f an = normalize( a - sphereCenter );
        vec3f bn = normalize( b - sphereCenter );
        vec3f cn = normalize( c - sphereCenter );

        SubdivideBiconvexMesh( mesh, biconvex, false, false, true, a, b, c, an, bn, cn, sphereCenter, true, h, 0, subdivisions, texture_a, texture_b);
    }

    // bevel

    const float bevel = biconvex.GetBevel();

    if ( bevel > 0.001f )
    {
        // find bottom circle edge of biconvex top

        std::vector<float> circleAngles;

        int numVertices = mesh.GetNumVertices();
        Vertex * vertices = mesh.GetVertexBuffer();
        for ( int i = 0; i < numVertices; ++i )
        {
            if ( vertices[i].position.y() < h + epsilon )
                circleAngles.push_back( atan2( vertices[i].position.z(), vertices[i].position.x() ) );
        }

        std::sort( circleAngles.begin(), circleAngles.end() );

        // tesselate the bevel

        const int numBevelRings = 8;

        const float torusMajorRadius = biconvex.GetBevelTorusMajorRadius();
        const float torusMinorRadius = biconvex.GetBevelTorusMinorRadius();

        const float delta_y = biconvex.GetBevel() / numBevelRings;

        for ( int i = 0; i < numBevelRings; ++i )
        {
            const float y1 = bevel / 2 - i * delta_y;
            const float y2 = bevel / 2 - (i+1) * delta_y;

            for ( int j = 0; j < circleAngles.size(); ++j )
            {
                const float angle1 = circleAngles[j];
                const float angle2 = circleAngles[(j+1)%circleAngles.size()];

                vec3f circleCenter1 = vec3f( cos( angle1 ), 0, sin( angle1 ) ) * torusMajorRadius;
                vec3f circleCenter2 = vec3f( cos( angle2 ), 0, sin( angle2 ) ) * torusMajorRadius;

                vec3f circleUp( 0, 1, 0 );

                vec3f circleRight1 = normalize( circleCenter1 );
                vec3f circleRight2 = normalize( circleCenter2 );

                const float circleX1 = sqrt( torusMinorRadius*torusMinorRadius - y1*y1 );
                const float circleX2 = sqrt( torusMinorRadius*torusMinorRadius - y2*y2 );

                vec3f a = circleCenter1 + circleX1 * circleRight1 + y1 * circleUp;
                vec3f b = circleCenter1 + circleX2 * circleRight1 + y2 * circleUp;
                vec3f c = circleCenter2 + circleX2 * circleRight2 + y2 * circleUp;
                vec3f d = circleCenter2 + circleX1 * circleRight2 + y1 * circleUp;

                vec3f an = normalize( a - circleCenter1 );
                vec3f bn = normalize( b - circleCenter1 );
                vec3f cn = normalize( c - circleCenter2 );
                vec3f dn = normalize( d - circleCenter2 );

                Vertex v1,v2,v3;

                v1.u = a.x() * texture_a + texture_b;
                v1.v = a.z() * texture_a + texture_b;
                v1.position = a;
                v1.normal = an;

                v2.u = b.x() * texture_a + texture_b;
                v2.v = b.z() * texture_a + texture_b;
                v2.position = b;
                v2.normal = bn;

                v3.u = c.x() * texture_a + texture_b;
                v3.v = c.z() * texture_a + texture_b;
                v3.position = c;
                v3.normal = cn;

                mesh.AddTriangle( v1, v2, v3 );

                v1.u = a.x() * texture_a + texture_b;
                v1.v = a.z() * texture_a + texture_b;
                v1.position = a;
                v1.normal = an;

                v2.u = c.x() * texture_a + texture_b;
                v2.v = c.z() * texture_a + texture_b;
                v2.position = c;
                v2.normal = cn;

                v3.u = d.x() * texture_a + texture_b;
                v3.v = d.z() * texture_a + texture_b;
                v3.position = d;
                v3.normal = dn;

                mesh.AddTriangle( v1, v2, v3 );
            }
        }
    }

    // bottom

    for ( int i = 0; i < numTriangles; ++i )
    {
        mat4f r1 = mat4f::axisRotation( deltaAngle * i, vec3f(0,1,0) );
        mat4f r2 = mat4f::axisRotation( deltaAngle * ( i + 1 ), vec3f(0,1,0) );

        vec3f a = vec3f( 0, biconvex.GetSphereOffset() - biconvex.GetSphereRadius(), 0 );
        vec3f b = transformPoint( r1, vec3f( 0, -h, bevelCircleRadius ) );
        vec3f c = transformPoint( r2, vec3f( 0, -h, bevelCircleRadius ) );

        const vec3f sphereCenter = vec3f( 0, biconvex.GetSphereOffset(), 0 );

        vec3f an = normalize( a - sphereCenter );
        vec3f bn = normalize( b - sphereCenter );
        vec3f cn = normalize( c - sphereCenter );

        SubdivideBiconvexMesh( mesh, biconvex, false, false, true, a, b, c, an, bn, cn, sphereCenter, false, -h, 0, subdivisions, texture_a, texture_b );//0, 0 );
    }
}

#endif