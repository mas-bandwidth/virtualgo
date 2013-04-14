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
#include "Intersection.h"
#include "CollisionDetection.h"

using namespace platform;

int main( int argc, char * argv[] )
{   
    bool playback = false;
    bool video = false;

    for ( int i = 1; i < argc; ++i )
    {
        if ( strcmp( argv[i], "playback" ) == 0 )
        {
            printf( "playback\n" );
            playback = true;
        }
        else if ( strcmp( argv[i], "video" ) == 0 )
        {
            printf( "video\n" );
            video = true;
        }
    }

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

    // create 2 pixel buffer objects, you need to delete them when program exits.
    // glBufferDataARB with NULL pointer reserves only memory space.
    const int NumPBOs = 2;
    GLuint pboIds[NumPBOs];
    int index = 0;
    const int dataSize = displayWidth * displayHeight * 3;
    if ( video )
    {
        glGenBuffersARB( NumPBOs, pboIds );
        for ( int i = 0; i < NumPBOs; ++i )
        {
            glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[i] );
            glBufferDataARB( GL_PIXEL_UNPACK_BUFFER_ARB, dataSize, 0, GL_STREAM_DRAW_ARB );
        }
        glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, 0 );
    }

    // record input to a file
    // read it back in playback mode for recording video
    FILE * inputFile = fopen( "output/recordedInputs", playback ? "rb" : "wb" );
    if ( !inputFile )
    {
        printf( "failed to open input file\n" );
        return 1;
    }

    mat4f rotation = mat4f::identity();

    double t = 0.0f;

    bool quit = false;

    bool prevSpace = false;

    bool rotating = true;

    float smoothedRotation = 0.0f;

    const float dt = 1.0f / 60.0f;

    unsigned int frame = 0;

    while ( !quit )
    {
        UpdateEvents();

        Input input;
        
        if ( !playback )
        {
            input = platform::Input::Sample();
            fwrite( &input, sizeof( platform::Input ), 1, inputFile );
            fflush( inputFile );
        }
        else
        {
            const int size = sizeof( platform::Input );
            if ( !fread( &input, size, 1, inputFile ) )
                quit = true;
        }

        if ( input.quit )
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

            glScalef( 0.6f, 0.6f, 0.6f );

            vec3f lightPosition( -1, +2, -5 );

            GLfloat light_ambient[] = { 0.7, 0.7, 0.7, 1.0 };
            GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
            GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };

            glLightfv( GL_LIGHT0, GL_AMBIENT, light_ambient );
            glLightfv( GL_LIGHT0, GL_DIFFUSE, light_diffuse );
            glLightfv( GL_LIGHT0, GL_SPECULAR, light_specular );

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

            GLfloat lightAmbientColor[] = { 1, 1, 1, 1.0 };
            glLightModelfv( GL_LIGHT_MODEL_AMBIENT, lightAmbientColor );

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

            glColor4f(1,1,1,1);

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

        // record to video

        if ( video )
        {
            // "index" is used to read pixels from framebuffer to a PBO
            // "nextIndex" is used to update pixels in the other PBO
            index = ( index + 1 ) % NumPBOs;
            int prevIndex = ( index + NumPBOs - 1 ) % NumPBOs;

            // set the target framebuffer to read
            glReadBuffer( GL_FRONT );

            // read pixels from framebuffer to PBO
            // glReadPixels() should return immediately.
            glBindBufferARB( GL_PIXEL_PACK_BUFFER_ARB, pboIds[index] );
            glReadPixels( 0, 0, displayWidth, displayHeight, GL_BGR, GL_UNSIGNED_BYTE, 0 );
            if ( frame > (unsigned) NumPBOs )
            {
                // map the PBO to process its data by CPU
                glBindBufferARB( GL_PIXEL_PACK_BUFFER_ARB, pboIds[prevIndex] );
                GLubyte * ptr = (GLubyte*) glMapBufferARB( GL_PIXEL_PACK_BUFFER_ARB,
                                                           GL_READ_ONLY_ARB );
                if ( ptr )
                {
                    char filename[256];
                    sprintf( filename, "output/frame-%05d.tga", frame - NumPBOs );
                    #ifdef LETTERBOX
                    WriteTGA( filename, displayWidth, displayHeight - 80, ptr + displayWidth * 3 * 40 );
                    #else
                    WriteTGA( filename, displayWidth, displayHeight, ptr );
                    #endif
                    glUnmapBufferARB( GL_PIXEL_PACK_BUFFER_ARB );
                }
            }

            // back to conventional pixel operation
            glBindBufferARB( GL_PIXEL_PACK_BUFFER_ARB, 0 );
        }

        // update the display
        
        UpdateDisplay( video ? 0 : 1 );

        // update time

        t += dt;

        frame++;
    }

    CloseDisplay();

    return 0;
}
