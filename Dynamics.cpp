/*
    Dynamics Demo for "Virtual Go" Physics Tutorial @ GDC 2013
    Copyright (c) 2005-2013, Glenn Fiedler. All rights reserved.
*/

#include "Common.h"
#include "Render.h"
#include "Stone.h"
#include "Platform.h"
#include "Biconvex.h"
#include "RigidBody.h"
#include "CollisionDetection.h"
#include "CollisionResponse.h"
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

Stone stone;

std::vector<RigidBody> snapshots;
float snapshotAccumulator = FLT_MAX;

void RandomStone( const Biconvex & biconvex, RigidBody & rigidBody, Mode mode )
{
    rigidBody.orientation = quat4f(1,0,0,0);
    rigidBody.linearMomentum = vec3f(0,0,0);
    rigidBody.angularMomentum = vec3f(0,0,0);

    if ( mode == LinearMotionSmooth || mode == LinearMotionStrobe )
    {
        rigidBody.position = vec3f(-5,0,0);
        rigidBody.linearMomentum = vec3f(0,0,0);
    }
    else if ( mode == Gravity || mode == Combination )
    {
        rigidBody.position = vec3f(-5,-4,0);
        rigidBody.linearMomentum = vec3f(13.5f,38,0);
    }
    else
    {
        rigidBody.position = vec3f(0,0,0);
    }

    if ( mode == AngularMotion || mode == Combination )
        rigidBody.angularMomentum = vec3f(10,-20,-15);

    rigidBody.Update();

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

int main()
{
    // initialize stones

    printf( "tesselating go stones...\n" );

    StoneSize stoneSize = STONE_SIZE_32;

    stone.Initialize( stoneSize );
    stone.rigidBody.inertia = vec3f(1,1,1);
    stone.rigidBody.inertiaTensor = mat4f::identity();
    stone.rigidBody.inverseInertiaTensor = mat4f::identity();

    Mesh mesh;
    GenerateBiconvexMesh( mesh, stone.biconvex );

    Mesh cheapMesh;
    GenerateBiconvexMesh( cheapMesh, stone.biconvex, 3 );

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
    GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
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

    GLfloat lightAmbientColor[] = { 0.2, 0.2, 0.2, 1.0 };
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

    uint64_t frame = 0;

    bool prevOne = false;
    bool prevTwo = false;
    bool prevThree = false;
    bool prevFour = false;
    bool prevFive = false;
    bool prevSpace = false;
    bool prevEnter = false;
    bool slowmo = false;

    float moveAccumulator = 0;

    while ( !quit )
    {
        CheckOpenGLError( "frame start" );

        UpdateEvents();

        platform::Input input;
        
        input = platform::Input::Sample();

        if ( input.quit )
            quit = true;

        if ( input.space && !prevSpace )
        {
            slowmo = !slowmo;
        }
        prevSpace = input.space;

        if ( input.enter && !prevEnter )
        {
            RandomStone( stone.biconvex, stone.rigidBody, mode );
            dt = normal_dt;
            slowmo = false;
        }
        prevEnter = input.enter;

        if ( input.one && !prevOne )
        {
            mode = LinearMotionStrobe;
            moveAccumulator = 0;
            dt = normal_dt;
            slowmo = false;
            RandomStone( stone.biconvex, stone.rigidBody, mode );
        }
        prevOne = input.one;

        if ( input.two && !prevTwo )
        {
            mode = LinearMotionSmooth;
            dt = normal_dt;
            slowmo = false;
            RandomStone( stone.biconvex, stone.rigidBody, mode );
        }
        prevTwo = input.two;

        if ( input.three && !prevThree )
        {
            mode = Gravity;
            dt = normal_dt;
            slowmo = false;
            RandomStone( stone.biconvex, stone.rigidBody, mode );
        }
        prevThree = input.three;

        if ( input.four && !prevFour )
        {
            mode = AngularMotion;
            dt = normal_dt;
            slowmo = false;
            RandomStone( stone.biconvex, stone.rigidBody, mode );
        }
        prevFour = input.four;

        if ( input.five && !prevFive )
        {
            mode = Combination;
            dt = normal_dt;
            slowmo = false;
            RandomStone( stone.biconvex, stone.rigidBody, mode );
        }
        prevFive = input.five;

        glDepthMask( GL_TRUE );

        ClearScreen( displayWidth, displayHeight );

        if ( mode == Nothing )
        {
            UpdateDisplay( 1 );
            frame++;
            continue;
        }

        // setup lights

        GLfloat lightPosition0[] = { 250, 1000, -500, 1 };
        GLfloat lightPosition1[] = { -250, 0, -250, 1 };
        GLfloat lightPosition2[] = { 100, 0, -100, 1 };
        GLfloat lightPosition3[] = { 0, +1000, +1000, 1 };
        
        glLightfv( GL_LIGHT0, GL_POSITION, lightPosition0 );
        glLightfv( GL_LIGHT1, GL_POSITION, lightPosition1 );
        glLightfv( GL_LIGHT2, GL_POSITION, lightPosition2 );
        glLightfv( GL_LIGHT3, GL_POSITION, lightPosition3 );

        // update camera

        glMatrixMode( GL_PROJECTION );

        glLoadIdentity();

        gluPerspective( FOV, (float) displayWidth / (float) displayHeight, Near, Far );

        float flipX[] = { -1,0,0,0,
                           0,1,0,0,
                           0,0,1,0,
                           0,0,0,1 };

        glMultMatrixf( flipX );

        glMatrixMode( GL_MODELVIEW );

        glLoadIdentity();

        gluLookAt( 0, 0, -12, 
                   0, 0, 0, 
                   0, 1, 0 );

        // update stone physics

        const float target_dt = slowmo ? slowmo_dt : normal_dt;
        const float tightness = ( target_dt < dt ) ? 0.2f : 0.1f;
        dt += ( target_dt - dt ) * tightness;

        if ( mode == LinearMotionStrobe )
        {
            stone.rigidBody.linearMomentum = vec3f(0,0,0);
            moveAccumulator += dt;
            if ( moveAccumulator > 1 )
            {
                moveAccumulator -= 1;
                stone.rigidBody.position += vec3f(5,0,0);
            }
        }
        else if ( mode == LinearMotionSmooth )
        {
            stone.rigidBody.linearMomentum = vec3f(5,0,0);
        }

        if ( mode == Gravity || mode == Combination )
        {
            const float gravity = 9.8f * 10;    // cms/sec^2
            stone.rigidBody.linearMomentum += vec3f(0,-gravity,0) * stone.rigidBody.mass * dt;
        }

        stone.rigidBody.Update();

        stone.rigidBody.position += stone.rigidBody.linearVelocity * dt;
        quat4f spin = AngularVelocityToSpin( stone.rigidBody.orientation, stone.rigidBody.angularVelocity );
        stone.rigidBody.orientation += spin * dt;
        stone.rigidBody.orientation = normalize( stone.rigidBody.orientation );

        // update snapshots

        snapshotAccumulator += dt;
        if ( snapshotAccumulator > 0.1f && snapshots.size() < 30 )
        {
            snapshots.push_back( stone.rigidBody );
            snapshotAccumulator = 0;
        }

        // render snapshots

        glDisable( GL_LIGHTING );

        glColor4f( 0.25f, 0.25f, 0.25f, 1 );

        glEnable( GL_DEPTH_TEST );
        glDepthMask( GL_FALSE );

        for ( int i = 0; i < snapshots.size(); ++i )
        {
            glPushMatrix();

            RigidBody & rigidBody = snapshots[i];

            RigidBodyTransform biconvexTransform( rigidBody.position, rigidBody.orientation );
            float opengl_transform[16];
            biconvexTransform.localToWorld.store( opengl_transform );
            glMultMatrixf( opengl_transform );

            RenderMesh( cheapMesh );

            glPopMatrix();
        }

        // setup lights for stone render

        glEnable( GL_LIGHT0 );
        glEnable( GL_LIGHT1 );
        glEnable( GL_LIGHT2 );
        glEnable( GL_LIGHT3 );

        // render stone

        glEnable( GL_LIGHTING );

        glPushMatrix();

        RigidBodyTransform biconvexTransform( stone.rigidBody.position, stone.rigidBody.orientation );
        float opengl_transform[16];
        biconvexTransform.localToWorld.store( opengl_transform );
        glMultMatrixf( opengl_transform );

        glEnable( GL_BLEND ); 
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glLineWidth( 1 );
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

        RenderMesh( mesh );

        glPopMatrix();

        // update the display
        
        UpdateDisplay( 1 );

        // update time

        frame++;

        CheckOpenGLError( "frame end" );
    }

    CloseDisplay();

    return 0;
}
