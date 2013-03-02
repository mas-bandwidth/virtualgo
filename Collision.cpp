/*
    Collision Demo
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

using namespace platform;

enum Mode
{
    LinearCollisionResponse,
    AngularCollisionResponse,
    CollisionResponseWithFriction,
    CollisionWithBoard,
    NumModes
};

void RandomStone( const Biconvex & biconvex, RigidBody & rigidBody, Mode mode )
{
    rigidBody.position = vec3f( 0, 15.0f, 0 );
    rigidBody.orientation = quat4f::axisRotation( random_float(0,2*pi), vec3f( random_float(0.1f,1), random_float(0.1f,1), random_float(0.1f,1) ) );
    if ( mode == LinearCollisionResponse )
        rigidBody.orientation = quat4f(1,0,0,0);
    rigidBody.linearMomentum = vec3f(0,0,0);
    if ( mode < CollisionResponseWithFriction )
        rigidBody.angularMomentum = vec3f(0,0,0);
    rigidBody.Update();
}

Stone stone;
StoneSize size = STONE_SIZE_34;
Stone stoneSizes[STONE_SIZE_NumValues];

void SelectStoneSize( int newStoneSize )
{
    size = (StoneSize) clamp( newStoneSize, 0, STONE_SIZE_NumValues - 1 );
    stone.biconvex = stoneSizes[size].biconvex;
    stone.rigidBody.mass = stoneSizes[size].rigidBody.mass;
    stone.rigidBody.inverseMass = stoneSizes[size].rigidBody.inverseMass;
    stone.rigidBody.inertia = stoneSizes[size].rigidBody.inertia;
    stone.rigidBody.inertiaTensor = stoneSizes[size].rigidBody.inertiaTensor;
    stone.rigidBody.inverseInertiaTensor = stoneSizes[size].rigidBody.inverseInertiaTensor;
}

int main()
{
    printf( "[virtual go]\n" );

    Mode mode = LinearCollisionResponse;

    Board smallBoard( 14, 14, 1.0f );
    Board largeBoard( 35, 35, 0 );

    for ( int i = 0; i < STONE_SIZE_NumValues; ++i )
        stoneSizes[i].Initialize( (StoneSize)i );

    SelectStoneSize( STONE_SIZE_34 );

    RandomStone( stone.biconvex, stone.rigidBody, mode );
    
    Mesh mesh[STONE_SIZE_NumValues];
    for ( int i = 0; i < STONE_SIZE_NumValues; ++i )
        GenerateBiconvexMesh( mesh[i], stoneSizes[i].biconvex );

    int displayWidth, displayHeight;
    GetDisplayResolution( displayWidth, displayHeight );

    #ifdef LETTERBOX
    displayWidth = 1280;
    displayHeight = 800;
    #endif

    printf( "display resolution is %d x %d\n", displayWidth, displayHeight );

    if ( !OpenDisplay( "Virtual Go", displayWidth, displayHeight, 32 ) )
    {
        printf( "error: failed to open display" );
        return 1;
    }

    HideMouseCursor();

    glEnable( GL_LINE_SMOOTH );
    glEnable( GL_POLYGON_SMOOTH );
    glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
    glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );

    glEnable( GL_LIGHTING );
    glEnable( GL_LIGHT0 );

    glShadeModel( GL_SMOOTH );

    GLfloat lightAmbientColor[] = { 0.75, 0.75, 0.75, 1.0 };
    glLightModelfv( GL_LIGHT_MODEL_AMBIENT, lightAmbientColor );

    glLightf( GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.01f );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glLineWidth( 4 );
    glColor4f( 0.5f,0.5f,0.5f,1 );
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );

    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    bool quit = false;

    srand( time( NULL ) );

    const float normal_dt = 1.0f / 60.0f;
    const float slowmo_dt = normal_dt * 0.1f;

    float dt = normal_dt;

    uint64_t frame = 0;

    bool prevSpace = false;
    bool prevUp = false;
    bool prevDown = false;
    bool slowmo = false;

    int zoomLevel = 0;
    const int MaxZoomLevel = 4;

    while ( !quit )
    {
        UpdateEvents();

        platform::Input input;
        
        input = platform::Input::Sample();

        if ( input.escape || input.quit )
            quit = true;

        if ( input.space && !prevSpace )
        {
            slowmo = !slowmo;
        }
        prevSpace = input.space;

        if ( input.up && !prevUp )
        {
            zoomLevel --;
            if ( zoomLevel < 0 )
                zoomLevel = 0;
        }
        prevUp = input.up;

        if ( input.down && !prevDown )
        {
            zoomLevel ++;
            if ( zoomLevel > MaxZoomLevel - 1 )
                zoomLevel = MaxZoomLevel - 1;
        }
        prevDown = input.down;

        if ( input.alt )
        {
            if ( input.left )
                SelectStoneSize( size - 1 );
            else if ( input.right )
                SelectStoneSize( size + 1 );
        }
        else
        {
            if ( input.one )
            {
                mode = LinearCollisionResponse;
                RandomStone( stone.biconvex, stone.rigidBody, mode );
                dt = normal_dt;
                slowmo = false;
            }

            if ( input.two )
            {
                mode = AngularCollisionResponse;
                RandomStone( stone.biconvex, stone.rigidBody, mode );
                dt = normal_dt;
                slowmo = false;
            }

            if ( input.three )
            {
                mode = CollisionResponseWithFriction;
                RandomStone( stone.biconvex, stone.rigidBody, mode );
                dt = normal_dt;
                slowmo = false;
            }

            if ( input.four )
            {
                mode = CollisionWithBoard;
                RandomStone( stone.biconvex, stone.rigidBody, mode );
                dt = normal_dt;
                slowmo = false;
            }
        }

        ClearScreen( displayWidth, displayHeight );

        if ( frame > 20 )
        {
            const float fov = 40.0f;

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

            if ( zoomLevel == 0 )
            {
                gluLookAt( 10, 1, 0,
                            0, 1, 0,
                            0, 1, 0 );
            }
            else if ( zoomLevel == 1 )
            {
                gluLookAt( 20, 5, 0,
                            0, 2, 0,
                            0, 1, 0 );
            }
            else if ( zoomLevel == 2 )
            {
                gluLookAt( 30, 20, 0,
                            0, 3, 0,
                            0, 1, 0 );
            }
            else if ( zoomLevel == 3 )
            {
                gluLookAt(  0, 50, 0,
                            0, 0, 0,
                            -1, 0, 0 );
            }

            // update stone physics

            const float target_dt = slowmo ? slowmo_dt : normal_dt;
            const float tightness = ( target_dt < dt ) ? 0.2f : 0.1f;
            dt += ( target_dt - dt ) * tightness;

            const int iterations = 20;

            const float iteration_dt = dt / iterations;

            for ( int i = 0; i < iterations; ++i )
            {
                bool colliding = false;

                const float gravity = 9.8f * 10;    // cms/sec^2
                stone.rigidBody.linearMomentum += vec3f(0,-gravity,0) * stone.rigidBody.mass * iteration_dt;

                stone.rigidBody.Update();

                stone.rigidBody.position += stone.rigidBody.linearVelocity * iteration_dt;
                quat4f spin = AngularVelocityToSpin( stone.rigidBody.orientation, stone.rigidBody.angularVelocity );
                stone.rigidBody.orientation += spin * iteration_dt;
                stone.rigidBody.orientation = normalize( stone.rigidBody.orientation );

                // collision between stone and board

                const float e = 0.85f;
                const float u = 0.15f;

                Board & board = ( mode >= CollisionWithBoard ) ? smallBoard : largeBoard;

                StaticContact boardContact;
                if ( StoneBoardCollision( stone.biconvex, board, stone.rigidBody, boardContact ) )
                {
                    if ( mode == LinearCollisionResponse )
                        ApplyLinearCollisionImpulse( boardContact, e );
                    else if ( mode == AngularCollisionResponse )
                        ApplyCollisionImpulseWithFriction( boardContact, e, 0.0f );
                    else if ( mode >= CollisionResponseWithFriction )
                        ApplyCollisionImpulseWithFriction( boardContact, e, u );
                    stone.rigidBody.Update();
                }

                // collision between stone and floor

                if ( mode >= CollisionWithBoard )
                {
                    StaticContact floorContact;
                    if ( StoneFloorCollision( stone.biconvex, board, stone.rigidBody, floorContact ) )
                    {
                        if ( mode == LinearCollisionResponse )
                            ApplyLinearCollisionImpulse( floorContact, e );
                        else if ( mode == AngularCollisionResponse )
                            ApplyCollisionImpulseWithFriction( floorContact, e, 0.0f );
                        else if ( mode >= CollisionResponseWithFriction )
                            ApplyCollisionImpulseWithFriction( floorContact, e, u );
                        stone.rigidBody.Update();
                    }
                }
            }

            // render board

            glLineWidth( 5 );
            glColor4f( 0.8f,0.8f,0.8f,1 );

            Board & board = ( mode >= CollisionWithBoard ) ? smallBoard : largeBoard;

            RenderBoard( board );

            // render stone

            glPushMatrix();

            RigidBodyTransform biconvexTransform( stone.rigidBody.position, stone.rigidBody.orientation );
            float opengl_transform[16];
            biconvexTransform.localToWorld.store( opengl_transform );
            glMultMatrixf( opengl_transform );

            glEnable( GL_BLEND ); 
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glLineWidth( 1 );
            glColor4f( 1, 1, 1, 1 );
            RenderMesh( mesh[size] );

            glPopMatrix();
        }

        // update the display
        
        UpdateDisplay( 1 );

        // update time

        frame++;
    }

    CloseDisplay();

    return 0;
}
