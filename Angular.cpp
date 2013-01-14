/*
    Angular Motion Demo
    Copyright (c) 2005-2013, Glenn Fiedler. All rights reserved.
*/

#include "Common.h"
#include "Render.h"
#include "Stone.h"
#include "Platform.h"
#include "Biconvex.h"

using namespace platform;

int main()
{
    printf( "[angular motion demo]\n" );

    Biconvex biconvex( 2.2f, 1.13f, 0.1f );

    Mesh mesh;
    GenerateBiconvexMesh( mesh, biconvex );

    int displayWidth, displayHeight;
    GetDisplayResolution( displayWidth, displayHeight );

    #ifdef LETTERBOX
    displayWidth = 1280;
    displayHeight = 800;
    #endif

    printf( "display resolution is %d x %d\n", displayWidth, displayHeight );

    if ( !OpenDisplay( "Angular Motion Demo", displayWidth, displayHeight, 32 ) )
    {
        printf( "error: failed to open display" );
        return 1;
    }

    ShowMouseCursor();

    glEnable( GL_LINE_SMOOTH );
    glEnable( GL_POLYGON_SMOOTH );
    glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
    glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );

    mat4f rotation = mat4f::identity();

    double t = 0.0f;

    bool prevSpace = false;
    bool quit = false;

    srand( time( NULL ) );

    bool prevCollision = false;

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
        {
            // todo: relaunch object
        }
        prevSpace = input.space;

        ClearScreen( displayWidth, displayHeight );

        if ( frame > 20 )
        {
            const float fov = 25.0f;

            glMatrixMode( GL_PROJECTION );

            glLoadIdentity();
            
            float flipX[] = { -1,0,0,0,
                               0,1,0,0,
                               0,0,1,0,
                               0,0,0,1 };
            
            glMultMatrixf( flipX );

            gluPerspective( fov, (float) displayWidth / (float) displayHeight, 0.1f, 100.0f );

            glMatrixMode( GL_MODELVIEW );    

            glLoadIdentity();

            gluLookAt( 0, 5, -25.0f, 
                       0, 4, 0, 
                       0, 1, 0 );

            glEnable( GL_CULL_FACE );
            glCullFace( GL_BACK );
            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

            // render board

            Board board( 20.0f, 20.0f, 1.0f );

            glLineWidth( 1 );
            glColor4f( 0.5f,0.5f,0.5f,1 );
            RenderBoard( board );

#if 0

            // update stone physics

            RigidBodyTransform biconvexTransform( rigidBody.position, rigidBody.orientation );

            bool colliding = false;

            const float gravity = 9.8f * 10;    // cms/sec^2
            rigidBody.linearVelocity += vec3f(0,-gravity,0) * step_dt;

            // integrate with velocities post collision response

            rigidBody.position += rigidBody.linearVelocity * step_dt;
            quat4f spin = AngularVelocityToSpin( rigidBody.orientation, rigidBody.angularVelocity );
            rigidBody.orientation += spin * step_dt;
            rigidBody.orientation = normalize( rigidBody.orientation );

            // render stone

            RigidBodyTransform biconvexTransform = RigidBodyTransform( rigidBody.position, rigidBody.orientation );

            glPushMatrix();

            float opengl_transform[16];
            biconvexTransform.localToWorld.store( opengl_transform );
            glMultMatrixf( opengl_transform );

            glEnable( GL_BLEND ); 
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glLineWidth( 1 );
            glColor4f( 0.5f,0.5f,0.5f,1 );
            RenderMesh( mesh );

            glPopMatrix();

#endif
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
