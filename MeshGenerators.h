#ifndef MESH_GENERATORS_H
#define MESH_GENERATORS_H

#include "Mesh.h"
#include "Biconvex.h"
#include "Board.h"

void SubdivideBiconvexMesh( Mesh<Vertex> & mesh,
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

        vec3f bevelOffset( 0, 0, h );

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

        v1.position = a;
        v1.normal = an;

        v2.position = b;
        v2.normal = bn;

        v3.position = c;
        v3.normal = cn;

        if ( !clockwise )
            mesh.AddTriangle( v1, v3, v2 );
        else
            mesh.AddTriangle( v1, v2, v3 );
    }
}

void GenerateBiconvexMesh( Mesh<Vertex> & mesh, const Biconvex & biconvex, int subdivisions = 5, int numTriangles = 5, float epsilon = 0.001f )
{
    const float bevelCircleRadius = biconvex.GetBevelCircleRadius();

    const float deltaAngle = 360.0f / numTriangles;

    const float h = biconvex.GetBevel() / 2;

    const float texture_a = 1 / biconvex.GetWidth();
    const float texture_b = 0.5f;

    // top

    for ( int i = 0; i < numTriangles; ++i )
    {
        mat4f r1 = mat4f::axisRotation( deltaAngle * i, vec3f(0,0,1) );
        mat4f r2 = mat4f::axisRotation( deltaAngle * ( i + 1 ), vec3f(0,0,1) );

        vec3f a = vec3f( 0, 0, -biconvex.GetSphereOffset() + biconvex.GetSphereRadius() );
        vec3f b = transformPoint( r1, vec3f( 0, bevelCircleRadius, h ) );
        vec3f c = transformPoint( r2, vec3f( 0, bevelCircleRadius, h ) );

        const vec3f sphereCenter = vec3f( 0, 0, -biconvex.GetSphereOffset() );

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
            if ( vertices[i].position.z() < h + epsilon )
                circleAngles.push_back( atan2( vertices[i].position.y(), vertices[i].position.x() ) );
        }

        std::sort( circleAngles.begin(), circleAngles.end() );

        // tesselate the bevel

        const int numBevelRings = 4;

        const float torusMajorRadius = biconvex.GetBevelTorusMajorRadius();
        const float torusMinorRadius = biconvex.GetBevelTorusMinorRadius();

        const float delta_z = biconvex.GetBevel() / numBevelRings;

        for ( int i = 0; i < numBevelRings; ++i )
        {
            const float z1 = bevel / 2 - i * delta_z;
            const float z2 = bevel / 2 - (i+1) * delta_z;

            for ( int j = 0; j < circleAngles.size(); ++j )
            {
                const float angle1 = circleAngles[j];
                const float angle2 = circleAngles[(j+1)%circleAngles.size()];

                vec3f circleCenter1 = vec3f( cos( angle1 ), sin( angle1 ), 0 ) * torusMajorRadius;
                vec3f circleCenter2 = vec3f( cos( angle2 ), sin( angle2 ), 0 ) * torusMajorRadius;

                vec3f circleUp( 0, 0, 1 );

                vec3f circleRight1 = normalize( circleCenter1 );
                vec3f circleRight2 = normalize( circleCenter2 );

                const float circleX1 = sqrt( torusMinorRadius*torusMinorRadius - z1*z1 );
                const float circleX2 = sqrt( torusMinorRadius*torusMinorRadius - z2*z2 );

                vec3f a = circleCenter1 + circleX1 * circleRight1 + z1 * circleUp;
                vec3f b = circleCenter1 + circleX2 * circleRight1 + z2 * circleUp;
                vec3f c = circleCenter2 + circleX2 * circleRight2 + z2 * circleUp;
                vec3f d = circleCenter2 + circleX1 * circleRight2 + z1 * circleUp;

                vec3f an = normalize( a - circleCenter1 );
                vec3f bn = normalize( b - circleCenter1 );
                vec3f cn = normalize( c - circleCenter2 );
                vec3f dn = normalize( d - circleCenter2 );

                Vertex v1,v2,v3;

                v1.position = a;
                v1.normal = an;

                v2.position = c;
                v2.normal = cn;

                v3.position = b;
                v3.normal = bn;

                mesh.AddTriangle( v1, v3, v2 );

                v1.position = a;
                v1.normal = an;

                v2.position = d;
                v2.normal = dn;

                v3.position = c;
                v3.normal = cn;

                mesh.AddTriangle( v1, v3, v2 );
            }
        }
    }

    // bottom

    for ( int i = 0; i < numTriangles; ++i )
    {
        mat4f r1 = mat4f::axisRotation( deltaAngle * i, vec3f(0,0,1) );
        mat4f r2 = mat4f::axisRotation( deltaAngle * ( i + 1 ), vec3f(0,0,1) );

        vec3f a = vec3f( 0, 0, biconvex.GetSphereOffset() - biconvex.GetSphereRadius() );
        vec3f b = transformPoint( r1, vec3f( 0, bevelCircleRadius, -h ) );
        vec3f c = transformPoint( r2, vec3f( 0, bevelCircleRadius, -h ) );

        const vec3f sphereCenter = vec3f( 0, 0, biconvex.GetSphereOffset() );

        vec3f an = normalize( a - sphereCenter );
        vec3f bn = normalize( b - sphereCenter );
        vec3f cn = normalize( c - sphereCenter );

        SubdivideBiconvexMesh( mesh, biconvex, false, false, true, a, b, c, an, bn, cn, sphereCenter, false, -h, 0, subdivisions, texture_a, texture_b );//0, 0 );
    }
}

void GenerateFloorMesh( Mesh<TexturedVertex> & mesh, float w = 60, float h = 60, float uv = 4.5f, float zbias = -0.04f )
{
    TexturedVertex a,b,c,d;

    const float z = 0.0f + zbias;

    a.position = vec3f( -w, -h, z );
    a.normal = vec3f( 0, 0, 1 );
    a.texCoords = vec2f( 0, 0 );

    b.position = vec3f( -w, h, z );
    b.normal = vec3f( 0, 0, 1 );
    b.texCoords = vec2f( 0, uv );

    c.position = vec3f( w, h, z );
    c.normal = vec3f( 0, 0, 1 );
    c.texCoords = vec2f( uv, uv );

    d.position = vec3f( w, -h, z );
    d.normal = vec3f( 0, 0, 1 );
    d.texCoords = vec2f( uv, 0 );

    mesh.AddTriangle( a, c, b );
    mesh.AddTriangle( a, d, c );
}

void GenerateBoardMesh( Mesh<TexturedVertex> & mesh, const Board & board, const float zbias = -0.03f )
{
    TexturedVertex a,b,c,d;

    const float w = board.GetHalfWidth();
    const float h = board.GetHalfHeight();
    const float z = board.GetThickness() + zbias;

    // primary surface

    a.position = vec3f( -w, -h, z );
    a.normal = vec3f( 0, 0, 1 );
    a.texCoords = vec2f( 0, 0 );

    b.position = vec3f( -w, h, z );
    b.normal = vec3f( 0, 0, 1 );
    b.texCoords = vec2f( 0, 1 );

    c.position = vec3f( w, h, z );
    c.normal = vec3f( 0, 0, 1 );
    c.texCoords = vec2f( 1, 1 );

    d.position = vec3f( w, -h, z );
    d.normal = vec3f( 0, 0, 1 );
    d.texCoords = vec2f( 1, 0 );

    mesh.AddTriangle( a, c, b );
    mesh.AddTriangle( a, d, c );

    // left side

    a.position = vec3f( -w, h, z );
    a.normal = vec3f( -1, 0, 0 );
    a.texCoords = vec2f( 0, 0 );

    b.position = vec3f( -w, -h, z );
    b.normal = vec3f( -1, 0, 0 );
    b.texCoords = vec2f( 0, 1 );

    c.position = vec3f( -w, -h, 0 );
    c.normal = vec3f( -1, 0, 0 );
    c.texCoords = vec2f( 1, 1 );

    d.position = vec3f( -w, +h, 0 );
    d.normal = vec3f( -1, 0, 0 );
    d.texCoords = vec2f( 1, 0 );

    mesh.AddTriangle( a, c, b );
    mesh.AddTriangle( a, d, c );

    // right side

    a.position = vec3f( +w, -h, z );
    a.normal = vec3f( 1, 0, 0 );
    a.texCoords = vec2f( 0, 0 );

    b.position = vec3f( +w, h, z );
    b.normal = vec3f( 1, 0, 0 );
    b.texCoords = vec2f( 0, 1 );

    c.position = vec3f( +w, h, 0 );
    c.normal = vec3f( 1, 0, 0 );
    c.texCoords = vec2f( 1, 1 );

    d.position = vec3f( +w, -h, 0 );
    d.normal = vec3f( 1, 0, 0 );
    d.texCoords = vec2f( 1, 0 );

    mesh.AddTriangle( a, c, b );
    mesh.AddTriangle( a, d, c );

    // top side

    a.position = vec3f( +w, h, z );
    a.normal = vec3f( 0, 1, 0 );
    a.texCoords = vec2f( 0, 0 );

    b.position = vec3f( -w, h, z );
    b.normal = vec3f( 0, 1, 0 );
    b.texCoords = vec2f( 0, 1 );

    c.position = vec3f( -w, h, 0 );
    c.normal = vec3f( 0, 1, 0 );
    c.texCoords = vec2f( 1, 1 );

    d.position = vec3f( +w, h, 0 );
    d.normal = vec3f( 0, 1, 0 );
    d.texCoords = vec2f( 1, 0 );

    mesh.AddTriangle( a, c, b );
    mesh.AddTriangle( a, d, c );

    // bottom side

    a.position = vec3f( -w, -h, z );
    a.normal = vec3f( 0, -1, 0 );
    a.texCoords = vec2f( 0, 0 );

    b.position = vec3f( +w, -h, z );
    b.normal = vec3f( 0, -1, 0 );
    b.texCoords = vec2f( 0, 1 );

    c.position = vec3f( +w, -h, 0 );
    c.normal = vec3f( 0, -1, 0 );
    c.texCoords = vec2f( 1, 1 );

    d.position = vec3f( -w, -h, 0 );
    d.normal = vec3f( 0, -1, 0 );
    d.texCoords = vec2f( 1, 0 );

    mesh.AddTriangle( a, c, b );
    mesh.AddTriangle( a, d, c );
}

void GenerateGridMesh( Mesh<TexturedVertex> & mesh, const Board & board, float zbias1 = - 0.015f, float zbias2 = -0.01f )
{
    TexturedVertex a,b,c,d;

    const BoardParams & params = board.GetParams();

    const float w = params.cellWidth;
    const float h = params.cellHeight;
    const float t = params.lineWidth * 2;       // IMPORTANT: texture is only 50% of height due to AA

    const int n = ( board.GetSize() - 1 ) / 2;

    // horizontal lines

    {
        const float z = board.GetThickness() + zbias1;

        for ( int i = -n; i <= n; ++i )
        {
            const float x1 = -n*w;
            const float x2 = +n*w;
            const float y = i * h;

            a.position = vec3f( x1, y - t/2, z );
            a.normal = vec3f( 0, 0, 1 );
            a.texCoords = vec2f(0,0);

            b.position = vec3f( x1, y + t/2, z );
            b.normal = vec3f( 0, 0, 1 );
            b.texCoords = vec2f(0,1);

            c.position = vec3f( x2, y + t/2, z );
            c.normal = vec3f( 0, 0, 1 );
            c.texCoords = vec2f(1,1);

            d.position = vec3f( x2, y - t/2, z );
            d.normal = vec3f( 0, 0, 1 );
            d.texCoords = vec2f(1,0);

            mesh.AddTriangle( a, c, b );
            mesh.AddTriangle( a, d, c );
        }
    }

    // vertical lines

    {
        const float z = board.GetThickness() + zbias2;

        for ( int i = -n; i <= +n; ++i )
        {
            const float y1 = -n*h;
            const float y2 = +n*h;
            const float x = i * w;

            a.position = vec3f( x - t/2, y1, z );
            a.normal = vec3f( 0, 0, 1 );
            a.texCoords = vec2f( 0, 0 );

            b.position = vec3f( x + t/2, y1, z );
            b.normal = vec3f( 0, 0, 1 );
            b.texCoords = vec2f( 0, 1 );

            c.position = vec3f( x + t/2, y2, z );
            c.normal = vec3f( 0, 0, 1 );
            c.texCoords = vec2f( 1, 1 );

            d.position = vec3f( x - t/2, y2, z );
            d.normal = vec3f( 0, 0, 1 );
            d.texCoords = vec2f( 1, 0 );

            mesh.AddTriangle( a, b, c );
            mesh.AddTriangle( a, c, d );
        }
    }

    // todo: need to add corners to board mesh. ideally share the same texture, eg. take 1/4 of it
}

void GenerateStarPointsMesh( Mesh<TexturedVertex> & mesh, const Board & board, float zbias = -0.005f )
{
    TexturedVertex a,b,c,d;

    const BoardParams & params = board.GetParams();

    const float r = params.starPointRadius * 2;
    const float z = board.GetThickness() + zbias;

    int numStarPoints;
    vec3f pointPosition[MaxStarPoints];
    board.GetStarPoints( pointPosition, numStarPoints );

    for ( int i = 0; i < numStarPoints; ++i )
    {
        float x = pointPosition[i].x();
        float y = pointPosition[i].y();

        const float x1 = x - r;
        const float x2 = x + r;
        const float y1 = y + r;
        const float y2 = y - r;

        a.position = vec3f( x1, y1, z );
        a.normal = vec3f( 0, 0, 1 );
        a.texCoords = vec2f(0,0);

        b.position = vec3f( x2, y1, z );
        b.normal = vec3f( 0, 0, 1 );
        b.texCoords = vec2f(0,1);

        c.position = vec3f( x2, y2, z );
        c.normal = vec3f( 0, 0, 1 );
        c.texCoords = vec2f(1,1);

        d.position = vec3f( x1, y2, z );
        d.normal = vec3f( 0, 0, 1 );
        d.texCoords = vec2f(1,0);

        mesh.AddTriangle( a, c, b );
        mesh.AddTriangle( a, d, c );
    }
}

void GenerateQuadMesh( Mesh<TexturedVertex> & mesh,
                       const vec3f & position_a,
                       const vec3f & position_b,
                       const vec3f & position_c, 
                       const vec3f & position_d,
                       const vec3f & normal )
{
    TexturedVertex a,b,c,d;

    const float uv = 1.0f;

    a.position = position_a;
    a.normal = normal;
    a.texCoords = vec2f( 0, 0 );

    b.position = position_b;
    b.normal = normal;
    b.texCoords = vec2f( uv, 0 );

    c.position = position_c;
    c.normal = normal;
    c.texCoords = vec2f( uv, uv );

    d.position = position_d;
    d.normal = normal;
    d.texCoords = vec2f( 0, uv );

    mesh.AddTriangle( a, c, b );
    mesh.AddTriangle( a, d, c );
}

#endif
