/*
    Biconvex Support demo
    Copyright (c) 2005-2013, Glenn Fiedler. All rights reserved.
*/

#include "Common.h"
#include "Render.h"
#include "Stone.h"
#include "Platform.h"
#include "Biconvex.h"
#include "RigidBody.h"
#include "Collision.h"
#include "Intersection.h"

using namespace platform;

int main()
{
    printf( "[support demo]\n" );

    Biconvex biconvex( 2.2f, 1.13f );

    Mesh mesh;
    GenerateBiconvexMesh( mesh, biconvex );

    int displayWidth, displayHeight;
    GetDisplayResolution( displayWidth, displayHeight );

    #ifdef LETTERBOX
    displayWidth = 1280;
    displayHeight = 800;
    #endif

    printf( "display resolution is %d x %d\n", displayWidth, displayHeight );

    if ( !OpenDisplay( "Support Demo", displayWidth, displayHeight, 32 ) )
    {
        printf( "error: failed to open display" );
        return 1;
    }

    HideMouseCursor();

    glEnable( GL_LINE_SMOOTH );
    glEnable( GL_POLYGON_SMOOTH );
    glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
    glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );

    mat4f rotation = mat4f::identity();

    double t = 0.0f;

    bool quit = false;

    bool prevSpace = false;

    bool rotating = true;

    float smoothedRotation = 0.0f;

    const float dt = 1.0f / 60.0f;

    uint64_t frame = 0;

    while ( !quit )
    {
        UpdateEvents();

        platform::Input input;
        
        input = platform::Input::Sample();

        if ( input.escape || input.quit )
            quit = true;

        if ( input.space && !prevSpace )
            rotating = !rotating;
        prevSpace = input.space;

        ClearScreen( displayWidth, displayHeight );

        if ( frame > 20 )
        {
            glMatrixMode( GL_PROJECTION );
            glLoadIdentity();
            glOrtho( -1.5, +1.5f, -1.0f, +1.0f, 0.1f, 100.0f );

            float flipX[] = { -1,0,0,0,
                               0,1,0,0,
                               0,0,1,0,
                               0,0,0,1 };

            glMultMatrixf( flipX );

            glScalef( 0.6f, 0.6f, 0.6f );

            vec3f lightPosition( -2, +10, 0 );

            glMatrixMode( GL_MODELVIEW );
            glLoadIdentity();
            gluLookAt( 0, 0, -5, 
                       0, 0, 0, 
                       0, 1, 0 );

            // render stone

            const float targetRotation = rotating ? 0.28f : 0;
            smoothedRotation += ( targetRotation - smoothedRotation ) * 0.15f;
            mat4f deltaRotation = mat4f::axisRotation( smoothedRotation, vec3f(1,2,3) );
            rotation = rotation * deltaRotation;

            RigidBodyTransform biconvexTransform( vec3f(0,0,0), rotation );

            glEnable( GL_LIGHTING );
            glEnable( GL_LIGHT0 );

            glShadeModel( GL_SMOOTH );

            GLfloat lightAmbientColor[] = { 0.75, 0.75, 0.75, 1.0 };
            glLightModelfv( GL_LIGHT_MODEL_AMBIENT, lightAmbientColor );

            glLightf( GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.01f );

            GLfloat position[4];
            position[0] = lightPosition.x();
            position[1] = lightPosition.y();
            position[2] = lightPosition.z();
            position[3] = 1.0f;
            glLightfv( GL_LIGHT0, GL_POSITION, position );

            glEnable( GL_CULL_FACE );
            glCullFace( GL_BACK );
            
            glPushMatrix();

            float opengl_transform[16];
            biconvexTransform.localToWorld.store( opengl_transform );
            glMultMatrixf( opengl_transform );

            RenderMesh( mesh );

            glPopMatrix();

            // visualize biconvex support

            vec3f biconvexCenter = biconvexTransform.GetPosition();
            vec3f biconvexUp = biconvexTransform.GetUp();
            float s1,s2;
            BiconvexSupport_WorldSpace( biconvex, biconvexCenter, biconvexUp, vec3f(1,0,0), s1, s2 );

            glDisable( GL_LIGHTING );
            
            glEnable( GL_BLEND ); 
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            
            glLineStipple( 10, 0xAAAA );
            glEnable( GL_LINE_STIPPLE );
            glColor4f( 0.8,0.8,0.8,1 );
            glLineWidth( 5 );

            glBegin( GL_LINES );
            glVertex3f( s1, -10, 0 );
            glVertex3f( s1, +10, 0 );
            glVertex3f( s2, -10, 0 );
            glVertex3f( s2, +10, 0 );
            glEnd();

            glDisable( GL_LINE_STIPPLE );
            glColor4f( 1,0,0,1 );
            glLineWidth( 20 );

            glBegin( GL_LINES );
            glVertex3f( s1 - 0.01f, -1.65, 0 );
            glVertex3f( s2 + 0.01f, -1.65, 0 );
            glEnd();
        }

        // update the display
        
        UpdateDisplay( 1 );

        // update time

        t += dt;

        frame++;
    }

    CloseDisplay();

    return 0;
}
