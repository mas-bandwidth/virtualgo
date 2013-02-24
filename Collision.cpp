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
    CollisionResponseWithFriction
};

void RandomStone( const Biconvex & biconvex, RigidBody & rigidBody, Mode mode )
{
    rigidBody.position = vec3f( 0, mode > LinearCollisionResponse ? 15.0f : 10.75, 0 );
    rigidBody.orientation = quat4f::axisRotation( random_float(0,2*pi), vec3f( random_float(0.1f,1), random_float(0.1f,1), random_float(0.1f,1) ) );
    if ( mode == LinearCollisionResponse )
        rigidBody.orientation = quat4f(1,0,0,0);
    rigidBody.linearMomentum = vec3f(0,0,0);
    if ( mode != CollisionResponseWithFriction )
        rigidBody.angularMomentum = vec3f(0,0,0);
    rigidBody.Update();
}

int main()
{
    printf( "[virtual go]\n" );

    Mode mode = LinearCollisionResponse;

    Biconvex biconvex( 2.5f, 1.13f, 0.1f );

    RigidBody rigidBody;
    rigidBody.mass = 1.0f;
    rigidBody.inverseMass = 1.0f / rigidBody.mass;
    CalculateBiconvexInertiaTensor( rigidBody.mass, biconvex, rigidBody.inertia, rigidBody.inertiaTensor, rigidBody.inverseInertiaTensor );

    RandomStone( biconvex, rigidBody, mode );

    rigidBody.position = vec3f(0,-10,0);
    
    Mesh mesh;
    GenerateBiconvexMesh( mesh, biconvex );

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
    bool slowmo = false;

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

        if ( input.one )
        {
            mode = LinearCollisionResponse;
            RandomStone( biconvex, rigidBody, mode );
            dt = normal_dt;
            slowmo = false;
        }

        if ( input.two )
        {
            mode = AngularCollisionResponse;
            RandomStone( biconvex, rigidBody, mode );
            dt = normal_dt;
            slowmo = false;
        }

        if ( input.three )
        {
            mode = CollisionResponseWithFriction;
            RandomStone( biconvex, rigidBody, mode );
            dt = normal_dt;
            slowmo = false;
        }

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

            gluLookAt( +25, 5, 0,
                       0, 4, 0,
                       0, 1, 0 );
            /*
            gluLookAt( 0, 5, +25.0f, 
                       0, 4, 0, 
                       0, 1, 0 );
                       */

            // render board

            Board board( 10.0f, 10.0f, 2.0f );

            glLineWidth( 5 );
            glColor4f( 0.8f,0.8f,0.8f,1 );

            RenderBoard( board );

            // update stone physics

            bool colliding = false;

            const float target_dt = slowmo ? slowmo_dt : normal_dt;
            const float tightness = ( target_dt < dt ) ? 0.2f : 0.1f;
            dt += ( target_dt - dt ) * tightness;

            const float gravity = 9.8f * 10;    // cms/sec^2
            rigidBody.linearMomentum += vec3f(0,-gravity,0) * rigidBody.mass * dt;

            rigidBody.Update();

            rigidBody.position += rigidBody.linearVelocity * dt;
            quat4f spin = AngularVelocityToSpin( rigidBody.orientation, rigidBody.angularVelocity );
            rigidBody.orientation += spin * dt;
            rigidBody.orientation = normalize( rigidBody.orientation );

            // collision between stone and board

            const float e = 0.85f;
            const float u = 0.15f;

            StaticContact boardContact;
            if ( StoneBoardCollision( biconvex, board, rigidBody, boardContact ) )
            {
                if ( mode == LinearCollisionResponse )
                    ApplyLinearCollisionImpulse( boardContact, e );
                else if ( mode == AngularCollisionResponse )
                    ApplyCollisionImpulseWithFriction( boardContact, e, 0.0f );
                else if ( mode == CollisionResponseWithFriction )
                    ApplyCollisionImpulseWithFriction( boardContact, e, u );
                rigidBody.Update();
            }

            // collision between stone and floor

            StaticContact floorContact;
            if ( StoneFloorCollision( biconvex, board, rigidBody, floorContact ) )
            {
                if ( mode == LinearCollisionResponse )
                    ApplyLinearCollisionImpulse( floorContact, e );
                else if ( mode == AngularCollisionResponse )
                    ApplyCollisionImpulseWithFriction( floorContact, e, 0.0f );
                else if ( mode == CollisionResponseWithFriction )
                    ApplyCollisionImpulseWithFriction( floorContact, e, u );
                rigidBody.Update();
            }
    
            // render stone

            glPushMatrix();

            RigidBodyTransform biconvexTransform( rigidBody.position, rigidBody.orientation );
            float opengl_transform[16];
            biconvexTransform.localToWorld.store( opengl_transform );
            glMultMatrixf( opengl_transform );

            glEnable( GL_BLEND ); 
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glLineWidth( 1 );
            glColor4f( 1, 1, 1, 1 );
            RenderMesh( mesh );

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
