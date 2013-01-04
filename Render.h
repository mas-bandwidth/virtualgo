#ifndef RENDER_H
#define RENDER_H

#include "Stone.h"
#include "Board.h"
#include "Mesh.h"

#if PLATFORM == PLATFORM_MAC
#include <OpenGl/gl.h>
#include <OpenGl/glu.h>
#include <OpenGL/glext.h>
#include <OpenGL/OpenGL.h>
#endif

void ClearScreen( int displayWidth, int displayHeight, float r = 0, float g = 0, float b = 0 )
{
    glViewport( 0, 0, displayWidth, displayHeight );
    glDisable( GL_SCISSOR_TEST );
    glClearStencil( 0 );
    glClearColor( r, g, b, 1 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
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

                vec3f a( bottom_x1, y2, bottom_z1 );
                vec3f b( top_x1, y1, top_z1 );
                vec3f c( top_x2, y1, top_z2 );
                vec3f d( bottom_x2, y2, bottom_z2 );

                vec3f center( 0, center_y, 0 );

                vec3f na = normalize( a - center );
                vec3f nb = normalize( b - center );
                vec3f nc = normalize( c - center );
                vec3f nd = normalize( d - center );

                glNormal3f( na.x(), na.y(), na.z() );
                glVertex3f( a.x(), a.y(), a.z() );
                
                glNormal3f( nb.x(), nb.y(), nb.z() );
                glVertex3f( b.x(), b.y(), b.z() );

                glNormal3f( nc.x(), nc.y(), nc.z() );
                glVertex3f( c.x(), c.y(), c.z() );
                
                glNormal3f( nd.x(), nd.y(), nd.z() );
                glVertex3f( d.x(), d.y(), d.z() );
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

                vec3f a( top_x1, y1, top_z1 );
                vec3f b( bottom_x1, y2, bottom_z1 );
                vec3f c( bottom_x2, y2, bottom_z2 );
                vec3f d( top_x2, y1, top_z2 );

                vec3f center( 0, center_y, 0 );

                vec3f na = normalize( a - center );
                vec3f nb = normalize( b - center );
                vec3f nc = normalize( c - center );
                vec3f nd = normalize( d - center );

                glNormal3f( na.x(), na.y(), na.z() );
                glVertex3f( a.x(), a.y(), a.z() );
                
                glNormal3f( nb.x(), nb.y(), nb.z() );
                glVertex3f( b.x(), b.y(), b.z() );

                glNormal3f( nc.x(), nc.y(), nc.z() );
                glVertex3f( c.x(), c.y(), c.z() );
                
                glNormal3f( nd.x(), nd.y(), nd.z() );
                glVertex3f( d.x(), d.y(), d.z() );
            }
        }
    }

    glEnd();
}

void SubdivideBiconvexMesh( Mesh & mesh,
                            const Biconvex & biconvex, 
                            bool i, bool j, bool k,
                            vec3f a, vec3f b, vec3f c,
                            vec3f an, vec3f bn, vec3f cn,
                            vec3f sphereCenter,
                            bool clockwise,
                            float h, int depth, int subdivisions )
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

        SubdivideBiconvexMesh( mesh, biconvex, i, j, false, a, e, d, an, en, dn, sphereCenter, clockwise, h, depth, subdivisions );
        SubdivideBiconvexMesh( mesh, biconvex, false, j, k, e, b, f, en, bn, fn, sphereCenter, clockwise, h, depth, subdivisions );
        SubdivideBiconvexMesh( mesh, biconvex, i, false, k, d, f, c, dn, fn, cn, sphereCenter, clockwise, h, depth, subdivisions );
        SubdivideBiconvexMesh( mesh, biconvex, false, false, false, d, e, f, dn, en, fn, sphereCenter, clockwise, h, depth, subdivisions );
    }
    else
    {
        if ( !clockwise )
        {
            glNormal3f( an.x(), an.y(), an.z() );
            glVertex3f( a.x(), a.y(), a.z() );

            glNormal3f( bn.x(), bn.y(), bn.z() );
            glVertex3f( b.x(), b.y(), b.z() );

            glNormal3f( cn.x(), cn.y(), cn.z() );
            glVertex3f( c.x(), c.y(), c.z() );
        }
        else
        {
            glNormal3f( an.x(), an.y(), an.z() );
            glVertex3f( a.x(), a.y(), a.z() );

            glNormal3f( cn.x(), cn.y(), cn.z() );
            glVertex3f( c.x(), c.y(), c.z() );

            glNormal3f( bn.x(), bn.y(), bn.z() );
            glVertex3f( b.x(), b.y(), b.z() );
        }
    }
}

void GenerateBiconvexMesh( Mesh & mesh, const Biconvex & biconvex, int subdivisions = 5 )
{
    // HACK: render with glBegin/glEnd first to verify -- then convert to vertex buffer
    glBegin( GL_TRIANGLES );

    const float h = biconvex.GetBevel() / 2;

    const float bevelCircleRadius = biconvex.GetBevelCircleRadius();

    const int numTriangles = 5;

    const float deltaAngle = 360.0f / numTriangles;

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

        SubdivideBiconvexMesh( mesh, biconvex, false, false, true, a, b, c, an, bn, cn, sphereCenter, false, h, 0, subdivisions );
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

        SubdivideBiconvexMesh( mesh, biconvex, false, false, true, a, b, c, an, bn, cn, sphereCenter, true, -h, 0, subdivisions );
    }

    // bevel

    const int numBevelRings = 16;
    const int numBevelSegments = 256;

    const float segmentAngle = 2*pi / numBevelSegments;

    const float torusMajorRadius = biconvex.GetBevelTorusMajorRadius();
    const float torusMinorRadius = biconvex.GetBevelTorusMinorRadius();

    const float delta_z = biconvex.GetBevel() / numBevelRings;

    for ( int i = 0; i < numBevelRings; ++i )
    {
        const float z1 = biconvex.GetBevel() / 2 - i * delta_z;
        const float z2 = biconvex.GetBevel() / 2 - (i+1) * delta_z;

        for ( int j = 0; j < numBevelSegments; ++j )
        {
            const float angle1 = j * segmentAngle;
            const float angle2 = angle1 + segmentAngle;

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

            vec3f na = normalize( a - circleCenter1 );
            vec3f nb = normalize( b - circleCenter1 );
            vec3f nc = normalize( c - circleCenter2 );
            vec3f nd = normalize( d - circleCenter2 );

            glNormal3f( na.x(), na.y(), na.z() );
            glVertex3f( a.x(), a.y(), a.z() );
            
            glNormal3f( nb.x(), nb.y(), nb.z() );
            glVertex3f( b.x(), b.y(), b.z() );

            glNormal3f( nc.x(), nc.y(), nc.z() );
            glVertex3f( c.x(), c.y(), c.z() );
            
            glNormal3f( na.x(), na.y(), na.z() );
            glVertex3f( a.x(), a.y(), a.z() );
            
            glNormal3f( nc.x(), nc.y(), nc.z() );
            glVertex3f( c.x(), c.y(), c.z() );
            
            glNormal3f( nd.x(), nd.y(), nd.z() );
            glVertex3f( d.x(), d.y(), d.z() );
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
            glVertex3f( x + dx, 0.0f, y + dy );
            glVertex3f( x, 0.0f, y + dy );
            glVertex3f( x, 0.0f, y );

            y += dy;
        }

        x += dx;
    }

    glEnd();
}

void RenderMesh( Mesh & mesh )
{
    glBegin( GL_TRIANGLES );

    // TODO

    glEnd();
}

#endif
