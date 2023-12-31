#ifndef RENDER_H
#define RENDER_H

#include "Stone.h"
#include "Board.h"
#include "Mesh.h"

#define GL_SILENCE_DEPRECATION

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

void RenderGrid( float z, float size, float gridWidth, float gridHeight )
{
    glBegin( GL_QUADS );

    const float dx = gridWidth;
    const float dy = gridHeight;

    float x = - ( size - 1 ) * 0.5f * gridWidth;

    for ( int i = 0; i < size - 1; ++i )
    {
        float y = - ( size - 1 ) * 0.5f * gridHeight;

        for ( int j = 0; j < size - 1; ++j )
        {
            glVertex3f( x + dx, y, z );
            glVertex3f( x + dx, y + dy, z );
            glVertex3f( x, y + dy, z );
            glVertex3f( x, y, z );

            y += dy;
        }

        x += dx;
    }

    glEnd();
}

void RenderBoard( const Board & board )
{
    glBegin( GL_QUADS );

    const float width = board.GetWidth();
    const float height = board.GetHeight();
    const float thickness = board.GetThickness();

    // top of board

    glNormal3f( 0,0,1 );

    glTexCoord2f( 1, 0 );   glVertex3f( width/2, -height/2, thickness );
    glTexCoord2f( 1, 1 );   glVertex3f( width/2, height/2, thickness );
    glTexCoord2f( 0, 1 );   glVertex3f( -width/2, height/2, thickness );
    glTexCoord2f( 0, 0 );   glVertex3f( -width/2, -height/2, thickness );

    // front side

    glNormal3f( 0,-1,0 );
    
    glVertex3f( width/2, -height/2, 0 );
    glVertex3f( width/2, -height/2, thickness );
    glVertex3f( -width/2, -height/2, thickness );
    glVertex3f( -width/2, -height/2, 0 );

    // back side

    glNormal3f( 0,+1,0 );
    glVertex3f( width/2, height/2, thickness );
    glVertex3f( width/2, height/2, 0 );
    glVertex3f( -width/2, height/2, 0 );
    glVertex3f( -width/2, height/2, thickness );

    // left side

    glNormal3f( -1,0,0 );
    glVertex3f( -width/2, -height/2, 0 );
    glVertex3f( -width/2, -height/2, thickness );
    glVertex3f( -width/2, +height/2, thickness );
    glVertex3f( -width/2, +height/2, 0 );

    // right side

    glNormal3f( +1,0,0 );
    glVertex3f( width/2, -height/2, thickness );
    glVertex3f( width/2, -height/2, 0 );
    glVertex3f( width/2, +height/2, 0 );
    glVertex3f( width/2, +height/2, thickness );

    glEnd();
}

void RenderMesh( Mesh<Vertex,int> & mesh )
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

#endif
