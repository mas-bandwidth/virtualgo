/*
    Dynamics Demo for "Virtual Go" Physics Tutorial @ GDC 2013
    Copyright (c) 2005-2013, Glenn Fiedler. All rights reserved.
*/

#include "Common.h"
#include "Render.h"
#include "StoneData.h"
#include "StoneInstance.h"
#include "Platform.h"
#include "Biconvex.h"
#include "RigidBody.h"
#include "CollisionDetection.h"
#include "CollisionResponse.h"
#include "MeshGenerators.h"
#include "Intersection.h"
#include <vector>

using namespace platform;

const float FOV = 40.0f;
const float Near = 0.1f;
const float Far = 100.0f;

enum Mode
{
    Nothing,
    LinearMotionStrobe,
    LinearMotionSmooth,
    Gravity,
    AngularMotion,
    Combination,
    NumModes
};

Mode mode = Nothing;

StoneData stoneData;
StoneInstance stoneInstance;

std::vector<RigidBody> snapshots;
float snapshotAccumulator = FLT_MAX;

void RandomStone( const Biconvex & biconvex, RigidBody & rigidBody, Mode mode )
{
    rigidBody.orientation = quat4f(1,0,0,0);
    rigidBody.linearMomentum = vec3f(0,0,0);
    rigidBody.angularMomentum = vec3f(0,0,0);

    if ( mode == LinearMotionSmooth || mode == LinearMotionStrobe )
    {
        rigidBody.position = vec3f(-2.5f,0,0);
        rigidBody.linearMomentum = vec3f(0,0,0);
    }
    else if ( mode == Gravity || mode == Combination )
    {
        rigidBody.position = vec3f(-5.75f,0,-6.1f);
        rigidBody.linearMomentum = vec3f(13.5f,0,41.5f);
    }
    else
    {
        rigidBody.position = vec3f(0,0,0);
    }

    if ( mode == AngularMotion || mode == Combination )
        rigidBody.angularMomentum = vec3f(10,1,5);

    rigidBody.UpdateMomentum();
    rigidBody.UpdateTransform();

    snapshots.clear();
    snapshotAccumulator = FLT_MAX;
}

void CheckOpenGLError( const char * message )
{
    int error = glGetError();
    if ( error != GL_NO_ERROR )
    {
        printf( "opengl error: %s (%s)\n", gluErrorString( error ), message );
        exit( 1 );
    }    
}

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

    // initialize stones

    printf( "tesselating go stones...\n" );

    StoneSize stoneSize = STONE_SIZE_40;

    stoneData.Initialize( stoneSize );

    stoneInstance.Initialize( stoneData );

    stoneInstance.rigidBody.inertia = vec3f(1,1,1);
    stoneInstance.rigidBody.inertiaTensor = mat4f::identity();
    stoneInstance.rigidBody.inverseInertiaTensor = mat4f::identity();
    stoneInstance.rigidBody.UpdateTransform();

    Mesh<Vertex> mesh;
    GenerateBiconvexMesh( mesh, stoneData.biconvex );

    Mesh<Vertex> cheapMesh;
    GenerateBiconvexMesh( cheapMesh, stoneData.biconvex, 3 );

    int displayWidth, displayHeight;
    GetDisplayResolution( displayWidth, displayHeight );

    #ifdef LETTERBOX
    displayWidth = 1280;
    displayHeight = 800;
    #endif

    printf( "opening display: %d x %d\n", displayWidth, displayHeight );

    if ( !OpenDisplay( "Dynamics", displayWidth, displayHeight, 32 ) )
    {
        printf( "error: failed to open display" );
        return 1;
    }

    CheckOpenGLError( "after display open" );

    HideMouseCursor();

    // setup opengl

    glEnable( GL_LINE_SMOOTH );
    glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );

    glEnable( GL_POLYGON_SMOOTH );
    glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );

    GLfloat light_ambient[] = { 0.0, 0.0, 0.0, 1.0 };
    GLfloat light_diffuse[] = { 0.7, 0.7, 0.7, 1.0 };
    GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };

    glLightfv( GL_LIGHT0, GL_AMBIENT, light_ambient );
    glLightfv( GL_LIGHT0, GL_DIFFUSE, light_diffuse );
    glLightfv( GL_LIGHT0, GL_SPECULAR, light_specular );

    glLightfv( GL_LIGHT1, GL_AMBIENT, light_ambient );
    glLightfv( GL_LIGHT1, GL_DIFFUSE, light_diffuse );
    glLightfv( GL_LIGHT1, GL_SPECULAR, light_specular );

    glLightfv( GL_LIGHT2, GL_AMBIENT, light_ambient );
    glLightfv( GL_LIGHT2, GL_DIFFUSE, light_diffuse );
    glLightfv( GL_LIGHT2, GL_SPECULAR, light_specular );

    glLightfv( GL_LIGHT3, GL_AMBIENT, light_ambient );
    glLightfv( GL_LIGHT3, GL_DIFFUSE, light_diffuse );
    glLightfv( GL_LIGHT3, GL_SPECULAR, light_specular );

    glShadeModel( GL_SMOOTH );

    GLfloat lightAmbientColor[] = { 0.75, 0.75, 0.75, 1.0 };
    glLightModelfv( GL_LIGHT_MODEL_AMBIENT, lightAmbientColor );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );

    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    glDisable( GL_COLOR_MATERIAL );

    CheckOpenGLError( "after opengl setup" );

    bool quit = false;

    srand( time( NULL ) );

    const float normal_dt = 1.0f / 60.0f;
    const float slowmo_dt = normal_dt * 0.1f;

    float dt = normal_dt;

    unsigned int frame = 0;

    bool prevOne = false;
    bool prevTwo = false;
    bool prevThree = false;
    bool prevFour = false;
    bool prevFive = false;
    bool prevSpace = false;
    bool prevEnter = false;
    bool slowmo = false;

    float moveAccumulator = 0;

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

    while ( !quit )
    {
        CheckOpenGLError( "frame start" );

        UpdateEvents();

        platform::Input input;
        
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
        {
            slowmo = !slowmo;
            if ( mode == Nothing )
                dt = slowmo_dt;
        }
        prevSpace = input.space;

        if ( input.enter && !prevEnter )
        {
            RandomStone( stoneData.biconvex, stoneInstance.rigidBody, mode );
        }
        prevEnter = input.enter;

        if ( input.one && !prevOne )
        {
            mode = LinearMotionStrobe;
            moveAccumulator = 0;
            RandomStone( stoneData.biconvex, stoneInstance.rigidBody, mode );
        }
        prevOne = input.one;

        if ( input.two && !prevTwo )
        {
            mode = LinearMotionSmooth;
            RandomStone( stoneData.biconvex, stoneInstance.rigidBody, mode );
        }
        prevTwo = input.two;

        if ( input.three && !prevThree )
        {
            mode = Gravity;
            RandomStone( stoneData.biconvex, stoneInstance.rigidBody, mode );
        }
        prevThree = input.three;

        if ( input.four && !prevFour )
        {
            mode = AngularMotion;
            RandomStone( stoneData.biconvex, stoneInstance.rigidBody, mode );
        }
        prevFour = input.four;

        if ( input.five && !prevFive )
        {
            mode = Combination;
            RandomStone( stoneData.biconvex, stoneInstance.rigidBody, mode );
        }
        prevFive = input.five;

        glDepthMask( GL_TRUE );

        ClearScreen( displayWidth, displayHeight );

        if ( mode == Nothing )
        {
            UpdateDisplay( 1 );
            continue;
        }

        // setup lights

        GLfloat lightPosition0[] = { 250, -1000, 500, 1 };
        GLfloat lightPosition1[] = { -250, 0, 250, 1 };
        GLfloat lightPosition2[] = { 100, 0, 100, 1 };
        GLfloat lightPosition3[] = { 0, -1000, 1000, 1 };
        
        glLightfv( GL_LIGHT0, GL_POSITION, lightPosition0 );
        glLightfv( GL_LIGHT1, GL_POSITION, lightPosition1 );
        glLightfv( GL_LIGHT2, GL_POSITION, lightPosition2 );
        glLightfv( GL_LIGHT3, GL_POSITION, lightPosition3 );

        // update camera

        glMatrixMode( GL_PROJECTION );

        glLoadIdentity();

        gluPerspective( FOV, (float) displayWidth / (float) displayHeight, Near, Far );

        glMatrixMode( GL_MODELVIEW );

        glLoadIdentity();

        float cameraDist = 10;

        if ( mode == LinearMotionStrobe || mode == LinearMotionSmooth )
            cameraDist = 6.4430f;
        else if ( mode == AngularMotion )
            cameraDist = 4.0f;

        gluLookAt( 0, -cameraDist, 0,
                   0, 0, 0, 
                   0, 0, 1 );

        // update stone physics

        const float target_dt = slowmo ? slowmo_dt : normal_dt;
        const float tightness = ( target_dt < dt ) ? 0.2f : 0.1f;
        dt += ( target_dt - dt ) * tightness;

        if ( mode == LinearMotionStrobe )
        {
            stoneInstance.rigidBody.linearMomentum = vec3f(0,0,0);
            moveAccumulator += dt;
            if ( moveAccumulator >= 1 )
            {
                moveAccumulator -= 1;
                stoneInstance.rigidBody.position += vec3f(2.5f,0,0);
            }
        }
        else if ( mode == LinearMotionSmooth )
        {
            stoneInstance.rigidBody.linearMomentum = vec3f(2.5f,0,0);
        }

        if ( mode == Gravity || mode == Combination )
        {
            const float gravity = 9.8f * 10;    // cms/sec^2
            stoneInstance.rigidBody.linearMomentum += vec3f(0,0,-gravity) * stoneInstance.rigidBody.mass * dt;
        }

        stoneInstance.rigidBody.UpdateMomentum();

        stoneInstance.rigidBody.position += stoneInstance.rigidBody.linearVelocity * dt;

        quat4f spin;
        AngularVelocityToSpin( stoneInstance.rigidBody.orientation, stoneInstance.rigidBody.angularVelocity, spin );
        stoneInstance.rigidBody.orientation += spin * dt;
        stoneInstance.rigidBody.orientation = normalize( stoneInstance.rigidBody.orientation );

        stoneInstance.rigidBody.UpdateTransform();

        // update snapshots

        float strobeTime = 1.0f;
        if ( mode != LinearMotionSmooth )
            strobeTime = 0.1f;
        if ( mode == Gravity || mode == Combination )
            strobeTime = 0.0585f;

        snapshotAccumulator += dt;
        if ( snapshotAccumulator >= strobeTime && snapshots.size() < 30 )
        {
            snapshots.push_back( stoneInstance.rigidBody );
            if ( snapshotAccumulator != FLT_MAX )
                snapshotAccumulator -= strobeTime;
            else
                snapshotAccumulator = dt;
        }

        // render snapshots

        if ( mode != LinearMotionStrobe && mode != AngularMotion )
        {
            glDisable( GL_LIGHTING );

            glLineWidth( 2.0f );

            glColor4f( 0.25f, 0.25f, 0.25f, 1 );

            glEnable( GL_DEPTH_TEST );
            glDepthMask( GL_FALSE );

            for ( int i = 0; i < snapshots.size(); ++i )
            {
                glPushMatrix();

                RigidBody & rigidBody = snapshots[i];

                const RigidBodyTransform & biconvexTransform = rigidBody.transform;
                float opengl_transform[16];
                biconvexTransform.localToWorld.store( opengl_transform );
                glMultMatrixf( opengl_transform );

                RenderMesh( cheapMesh );

                glPopMatrix();
            }
        }

        // setup lights for stone render

        glEnable( GL_LIGHT0 );
        glEnable( GL_LIGHT1 );
        glEnable( GL_LIGHT2 );
        glEnable( GL_LIGHT3 );

        // render stone

        glEnable( GL_LIGHTING );

        glPushMatrix();

        const RigidBodyTransform & biconvexTransform = stoneInstance.rigidBody.transform;
        float opengl_transform[16];
        biconvexTransform.localToWorld.store( opengl_transform );
        glMultMatrixf( opengl_transform );

        glEnable( GL_BLEND ); 
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        glEnable( GL_DEPTH_TEST );
        glDepthMask( GL_FALSE );

        GLfloat mat_ambient[] = { 0.25, 0.25, 0.25, 1.0 };
        GLfloat mat_diffuse[] = { 0.45, 0.45, 0.45, 1.0 };
        GLfloat mat_specular[] = { 0.1, 0.1, 0.1, 1.0 };
        GLfloat mat_shininess[] = { 100.0 };

        glMaterialfv( GL_FRONT, GL_AMBIENT, mat_ambient );
        glMaterialfv( GL_FRONT, GL_DIFFUSE, mat_diffuse );
        glMaterialfv( GL_FRONT, GL_SPECULAR, mat_specular );
        glMaterialfv( GL_FRONT, GL_SHININESS, mat_shininess );

        glLineWidth( 3 );

        RenderMesh( mesh );

        glPopMatrix();

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

        CheckOpenGLError( "frame end" );

        frame++;
    }

    CloseDisplay();

    return 0;
}
