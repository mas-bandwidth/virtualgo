/*
    Picking Demo
    Copyright (c) 2005-2013, Glenn Fiedler. All rights reserved.
*/

#include "Common.h"
#include "Render.h"
#include "Stone.h"
#include "Platform.h"
#include "Biconvex.h"
#include "RigidBody.h"
#include "Intersection.h"
#include "CollisionDetection.h"

using namespace platform;

int main()
{
    printf( "[picking demo]\n" );

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

    if ( !OpenDisplay( "Picking Demo", displayWidth, displayHeight, 32 ) )
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
    bool rotating = true;
    bool quit = false;

    float smoothedRotation = 0.0f;

    srand( time( NULL ) );

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
            const float fov = 25.0f;
            gluPerspective( fov, (float) displayWidth / (float) displayHeight, 0.1f, 100.0f );

            float flipX[] = { -1,0,0,0,
                               0,1,0,0,
                               0,0,1,0,
                               0,0,0,1 };

            glMultMatrixf( flipX );

            glMatrixMode( GL_MODELVIEW );
            glLoadIdentity();
            gluLookAt( 0, 5, -20.0f, 
                       0, 0, 0, 
                       0, 1, 0 );

            glEnable( GL_CULL_FACE );
            glCullFace( GL_BACK );
            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

            // render board

            glLineWidth( 1 );

            Board board( 20.0f, 20.0f, 1.0f );

            glColor4f( 0.5f,0.5f,0.5f,1 );

            RenderBoard( board );

            // render stone

            static bool firstFrame = 0;
            const float targetRotation = rotating ? 0.28f : 0.0f;
            if ( firstFrame )
                smoothedRotation = targetRotation;
            else
                smoothedRotation += ( targetRotation - smoothedRotation ) * 0.15f;
            firstFrame = false;
            mat4f deltaRotation = mat4f::axisRotation( smoothedRotation, vec3f(1,2,3) );
            rotation = rotation * deltaRotation;

            RigidBodyTransform biconvexTransform( vec3f(0,biconvex.GetHeight()/2,0), rotation );

            glPushMatrix();

            float opengl_transform[16];
            biconvexTransform.localToWorld.store( opengl_transform );
            glMultMatrixf( opengl_transform );

            glEnable( GL_BLEND ); 
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glColor4f( 0.5f,0.5f,0.5f,1 );
            RenderMesh( mesh );

            glPopMatrix();

            // render nearest intersection with picking ray: stone or board
            glLineWidth( 2 );
            {
                int mouse_x, mouse_y;
                GetMousePosition( mouse_x, mouse_y );

                vec3f rayStart, rayDirection;
                GetMousePickRay( mouse_x, mouse_y, rayStart, rayDirection );

                vec3f board_point, board_normal, stone_point, stone_normal;
                const float board_t = IntersectRayBoard( board, rayStart, rayDirection, board_point, board_normal );
                const float stone_t = IntersectRayStone( biconvex, biconvexTransform, rayStart, rayDirection, stone_point, stone_normal );

                // display stone intersection if it is nearest or the board was not hit
                if ( ( stone_t >= 0.0f && board_t < 0.0f ) || ( stone_t >= 0.0f && stone_t < board_t ) )
                {
                    glColor4f( 0.7f,0,0,1 );
                    glBegin( GL_LINES );
                    vec3f p1 = stone_point;
                    vec3f p2 = stone_point + stone_normal * 0.5f;
                    glVertex3f( p1.x(), p1.y(), p1.z() );
                    glVertex3f( p2.x(), p2.y(), p2.z() );
                    glEnd();
                }

                // display board intersection if it is nearest, or we did not hit the stone
                if ( ( board_t >= 0.0f && stone_t < 0.0f ) || ( board_t >= 0.0f && board_t < stone_t ) )
                {
                    glColor4f( 0.7f,0,0,1 );
                    glBegin( GL_LINES );
                    vec3f p1 = board_point;
                    vec3f p2 = board_point + board_normal * 0.5f;
                    glVertex3f( p1.x(), p1.y(), p1.z() );
                    glVertex3f( p2.x(), p2.y(), p2.z() );
                    glEnd();

                    // if we hit the board, then render yellow line between board intersection
                    // and the nearest point on the stone. this we can test that nearest point
                    // on biconvex fn is working properly.
                    vec3f nearest = NearestPointOnStone( biconvex, biconvexTransform, board_point );
                    glColor4f( 0.9f,0.9f,0.3f,1 );
                    glBegin( GL_LINES );
                    glVertex3f( p1.x(), p1.y(), p1.z() );
                    glVertex3f( nearest.x(), nearest.y(), nearest.z() );
                    glEnd();
                }
            }
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
