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

    const float ideal = 2.5f;       // todo: parameterize this

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

    int numTriangles = mesh.GetNumTriangles();
    int * indexBuffer = mesh.GetIndexBuffer();
    Vertex * vertexBuffer = mesh.GetVertexBuffer();

    for ( int i = 0; i < numTriangles; ++i )
    {
        const int index_a = indexBuffer[i*3];
        const int index_b = indexBuffer[i*3+1];
        const int index_c = indexBuffer[i*3+2];

        const Vertex & a = vertexBuffer[index_a];
        const Vertex & b = vertexBuffer[index_b];
        const Vertex & c = vertexBuffer[index_c];

        glNormal3f( a.normal.x(), a.normal.y(), a.normal.z() );
        glVertex3f( a.position.x(), a.position.y(), a.position.z() );

        glNormal3f( b.normal.x(), b.normal.y(), b.normal.z() );
        glVertex3f( b.position.x(), b.position.y(), b.position.z() );

        glNormal3f( c.normal.x(), c.normal.y(), c.normal.z() );
        glVertex3f( c.position.x(), c.position.y(), c.position.z() );
    }

    glEnd();
}

void RenderBiconvexNaive( const Biconvex & biconvex, int numSegments = 128, int numRings = 32 )
{
    glBegin( GL_TRIANGLES );

    const float sphereRadius = biconvex.GetSphereRadius();
    const float segmentAngle = 2*pi / numSegments;

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
                
                glNormal3f( na.x(), na.y(), na.z() );
                glVertex3f( a.x(), a.y(), a.z() );
                
                glNormal3f( nc.x(), nc.y(), nc.z() );
                glVertex3f( c.x(), c.y(), c.z() );
                
                glNormal3f( nd.x(), nd.y(), nd.z() );
                glVertex3f( d.x(), d.y(), d.z() );
            }
        }
    }

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
                
                glNormal3f( na.x(), na.y(), na.z() );
                glVertex3f( a.x(), a.y(), a.z() );
                
                glNormal3f( nc.x(), nc.y(), nc.z() );
                glVertex3f( c.x(), c.y(), c.z() );
                
                glNormal3f( nd.x(), nd.y(), nd.z() );
                glVertex3f( d.x(), d.y(), d.z() );
            }
        }
    }

    glEnd();
}

#endif
