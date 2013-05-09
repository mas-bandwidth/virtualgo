#ifndef RENDER_H
#define RENDER_H

void CalculateFrustumPlanes( const mat4f & clipMatrix, Frustum & frustum )
{
    float clipData[16];

    clipMatrix.store( clipData );

    #define clip(i,j) clipData[j+i*4]

    const vec3f left( clip(0,3) + clip(0,0),
                      clip(1,3) + clip(1,0),
                      clip(2,3) + clip(2,0) );
    
    const vec3f right( clip(0,3) - clip(0,0),
                       clip(1,3) - clip(1,0),
                       clip(2,3) - clip(2,0) );

    const vec3f bottom( clip(0,3) + clip(0,1),
                        clip(1,3) + clip(1,1),
                        clip(2,3) + clip(2,1) );

    const vec3f top( clip(0,3) - clip(0,1),
                     clip(1,3) - clip(1,1),
                     clip(2,3) - clip(2,1) );

    const vec3f front( clip(0,3) + clip(0,2),
                       clip(1,3) + clip(1,2),
                       clip(2,3) + clip(2,2) );

    const vec3f back( clip(0,3) - clip(0,2),
                      clip(1,3) - clip(1,2),
                      clip(2,3) - clip(2,2) );

    const float left_d = - ( clip(3,3) + clip(3,0) );
    const float right_d = - ( clip(3,3) - clip(3,0) );

    const float bottom_d = - ( clip(3,3) + clip(3,1) );
    const float top_d = - ( clip(3,3) - clip(3,1) );

    const float front_d = - ( clip(3,3) + clip(3,2) );
    const float back_d = - ( clip(3,3) - clip(3,2) );

    const float left_length = length( left );
    const float right_length = length( right );
    const float top_length = length( top );
    const float bottom_length = length( bottom );
    const float front_length = length( front );
    const float back_length = length( back );

    frustum.left = vec4f( left.x(),
                          left.y(),
                          left.z(),
                          left_d ) * 1.0f / left_length;

    frustum.right = vec4f( right.x(),
                           right.y(),
                           right.z(),
                           right_d ) * 1.0f / right_length;

    frustum.top = vec4f( top.x(),
                         top.y(),
                         top.z(),
                         top_d ) * 1.0f / top_length;

    frustum.bottom = vec4f( bottom.x(),
                            bottom.y(),
                            bottom.z(),
                            bottom_d ) * 1.0f / bottom_length;

    frustum.front = vec4f( front.x(),
                           front.y(),
                           front.z(),
                           front_d ) * 1.0f / front_length;

    frustum.back = vec4f( back.x(),
                          back.y(),
                          back.z(),
                          back_d ) * 1.0f / back_length;
}

void MakeShadowMatrix( const vec4f & plane, const vec4f & light, mat4f & shadowMatrix )
{
    // http://math.stackexchange.com/questions/320527/projecting-a-point-on-a-plane-through-a-matrix
    
    const float planeDotLight = dot( plane, light );
    
    float data[16];
    
    data[0] = planeDotLight - light.x() * plane.x();
    data[4] = -light.x() * plane.y();
    data[8] = -light.x() * plane.z();
    data[12] = -light.x() * plane.w();
    
    data[1] = -light.y() * plane.x();
    data[5] = planeDotLight - light.y() * plane.y();
    data[9] = -light.y() * plane.z();
    data[13] = -light.y() * plane.w();
    
    data[2] = -light.z() * plane.x();
    data[6] = -light.z() * plane.y();
    data[10] = planeDotLight - light.z() * plane.z();
    data[14] = -light.z() * plane.w();
    
    data[3] = -light.w() * plane.x();
    data[7] = -light.w() * plane.y();
    data[11] = -light.w() * plane.z();
    data[15] = planeDotLight - light.w() * plane.w();
    
    shadowMatrix.load( data );
}

#if OPENGL_ES2

    enum
    {
        UNIFORM_MODELVIEWPROJECTION_MATRIX,
        UNIFORM_NORMAL_MATRIX,
        UNIFORM_LIGHT_POSITION,
        UNIFORM_ALPHA,
        NUM_UNIFORMS
    };

    void gluUnProject( GLfloat winx, GLfloat winy, GLfloat winz,
                       const mat4f inverseClipMatrix,
                       const GLint viewport[4],
                       GLfloat *objx, GLfloat *objy, GLfloat *objz )
    {
        vec4f in( ( ( winx - viewport[0] ) / viewport[2] ) * 2 - 1,
                  ( ( winy - viewport[1] ) / viewport[3] ) * 2 - 1,
                  ( winz ) * 2 - 1,
                  1.0f );

        vec4f out = inverseClipMatrix * in;
        
        *objx = out.x() / out.w();
        *objy = out.y() / out.w();
        *objz = out.z() / out.w();
    }

    void GetPickRay( const mat4f & inverseClipMatrix, float screen_x, float screen_y, vec3f & rayStart, vec3f & rayDirection )
    {
        GLint viewport[4];
        glGetIntegerv( GL_VIEWPORT, viewport );

        const float displayHeight = viewport[3];

        float x = screen_x;
        float y = displayHeight - screen_y;

        GLfloat x1,y1,z1;
        GLfloat x2,y2,z2;

        gluUnProject( x, y, 0, inverseClipMatrix, viewport, &x1, &y1, &z1 );
        gluUnProject( x, y, 1, inverseClipMatrix, viewport, &x2, &y2, &z2 );

        vec3f ray1( x1,y1,z1 );
        vec3f ray2( x2,y2,z2 );

        rayStart = ray1;
        rayDirection = normalize( ray2 - ray1 );
    }

    float GetShadowAlpha( const vec3f & stonePosition )
    {
        const float ShadowFadeStart = 5.0f;
        const float ShadowFadeFinish = 20.0f;

        const float z = stonePosition.z();

        if ( z < ShadowFadeStart )
            return 1.0f;
        else if ( z < ShadowFadeFinish )
            return 1.0f - ( z - ShadowFadeStart ) / ( ShadowFadeFinish - ShadowFadeStart );
        else
            return 0.0f;
    }

#else

    #include "Config.h"
    #include "Board.h"
    #include "Biconvex.h"
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

    void RenderBiconvexNaive( const Biconvex & biconvex, int numSegments = 128, int numRings = 32 )
    {
        glBegin( GL_TRIANGLES );

        const float sphereRadius = biconvex.GetSphereRadius();
        const float segmentAngle = 2*pi / numSegments;

        // bottom surface of biconvex
        {
            const float center_z = biconvex.GetSphereOffset();
            const float delta_z = biconvex.GetHeight() / 2 / numRings;

            for ( int i = 0; i < numRings; ++i )
            {
                const float z1 = -i * delta_z;
                const float s1 = z1 - center_z;
                const float r1 = sqrt( sphereRadius*sphereRadius - (s1*s1) );
                const float z2 = -(i+1) * delta_z;
                const float s2 = z2 - center_z;
                const float r2 = sqrt( sphereRadius*sphereRadius - (s2*s2) );

                for ( int j = 0; j < numSegments; ++j )
                {
                    const float angle1 = j * segmentAngle;
                    const float angle2 = angle1 + segmentAngle;
                    
                    const float top_x1 = cos( angle1 ) * r1;
                    const float top_y1 = sin( angle1 ) * r1;    
                    const float top_x2 = cos( angle2 ) * r1;
                    const float top_y2 = sin( angle2 ) * r1;

                    const float bottom_x1 = cos( angle1 ) * r2;
                    const float bottom_y1 = sin( angle1 ) * r2;    
                    const float bottom_x2 = cos( angle2 ) * r2;
                    const float bottom_y2 = sin( angle2 ) * r2;

                    vec3f a( top_x1, top_y1, z1 );
                    vec3f b( bottom_x1, bottom_y1, z2 );
                    vec3f c( bottom_x2, bottom_y2, z2 );
                    vec3f d( top_x2, top_y2, z1 );

                    vec3f center( 0, 0, center_z );

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
            const float center_z = -biconvex.GetSphereOffset();
            const float delta_z = biconvex.GetHeight() / 2 / numRings;

            for ( int i = 0; i < numRings; ++i )
            {
                const float z1 = i * delta_z;
                const float s1 = z1 - center_z;
                const float r1 = sqrt( sphereRadius*sphereRadius - (s1*s1) );
                const float z2 = (i+1) * delta_z;
                const float s2 = z2 - center_z;
                const float r2 = sqrt( sphereRadius*sphereRadius - (s2*s2) );

                for ( int j = 0; j < numSegments; ++j )
                {
                    const float angle1 = j * segmentAngle;
                    const float angle2 = angle1 + segmentAngle;
                    
                    const float top_x1 = cos( angle1 ) * r1;
                    const float top_y1 = sin( angle1 ) * r1;    
                    const float top_x2 = cos( angle2 ) * r1;
                    const float top_y2 = sin( angle2 ) * r1;

                    const float bottom_x1 = cos( angle1 ) * r2;
                    const float bottom_y1 = sin( angle1 ) * r2;
                    const float bottom_x2 = cos( angle2 ) * r2;
                    const float bottom_y2 = sin( angle2 ) * r2;

                    vec3f a( bottom_x1, bottom_y1, z2 );
                    vec3f b( top_x1, top_y1, z1 );
                    vec3f c( top_x2, top_y2, z1 );
                    vec3f d( bottom_x2, bottom_y2, z2 );

                    vec3f center( 0, 0, center_z );

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

    void RenderMesh( Mesh<Vertex> & mesh )
    {
        glBegin( GL_TRIANGLES );

        int numTriangles = mesh.GetNumTriangles();
        uint16_t * indexBuffer = mesh.GetIndexBuffer();
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

#endif
