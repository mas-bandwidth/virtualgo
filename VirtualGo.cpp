/*
    Virtual Go GDC 2013 Demo
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

const int MaxZoomLevel = 5;

int zoomLevel = 2;

const float ScrollSpeed = 10.0f;

float scrollX = 0;
float scrollY = 0;
float scrollZ = 0;

enum Mode
{
    Nothing,
    LinearCollisionResponse,
    AngularCollisionResponse,
    CollisionResponseWithFriction,
    SolidColor,
    Textures,
    NumModes
};

Mode mode = Nothing;

Mode cameraMode = mode;
vec3f cameraLookAt;
vec3f cameraPosition;

const int DefaultBoardSize = 9;
const float DefaultBoardThickness = 0.5f;

Board board( DefaultBoardSize );

Stone stone;
StoneSize size = STONE_SIZE_34;
Stone stoneSizes[STONE_SIZE_NumValues];

void RandomStone( const Biconvex & biconvex, RigidBody & rigidBody, Mode mode )
{
    const float x = scrollX;
    const float z = scrollZ;
    
    rigidBody.position = vec3f( x, board.GetThickness() + 15.0f, z );
    
    //rigidBody.orientation = quat4f::axisRotation( random_float(0,2*pi), vec3f( random_float(0.1f,1), random_float(0.1f,1), random_float(0.1f,1) ) );
    
    rigidBody.orientation = quat4f::axisRotation( pi*2/3, vec3f(0,0,-1) );

    //if ( mode == LinearCollisionResponse )
    //    rigidBody.orientation = quat4f(1,0,0,0);
    
    rigidBody.linearMomentum = vec3f(0,0,0);
    
    //if ( mode < CollisionResponseWithFriction )
        rigidBody.angularMomentum = vec3f(0,0,0);

    rigidBody.Update();
}

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

void RestoreDefaults()
{
    SelectStoneSize( STONE_SIZE_34 );

    board.SetThickness( DefaultBoardThickness );

    scrollX = 0;
    scrollY = 0;
    scrollZ = 0;

    zoomLevel = 3;

    stone.rigidBody.position = vec3f( 0, board.GetThickness() + stone.biconvex.GetHeight() / 2, 0 );
    stone.rigidBody.orientation = quat4f(1,0,0,0);
    stone.rigidBody.linearMomentum = vec3f(0,0,0);
    stone.rigidBody.angularMomentum = vec3f(0,0,0);
    stone.rigidBody.Update();

    board = Board( DefaultBoardSize );
}

int main()
{
    printf( "[virtual go]\n" );

    for ( int i = 0; i < STONE_SIZE_NumValues; ++i )
        stoneSizes[i].Initialize( (StoneSize)i );

    RestoreDefaults();
    
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

    glEnable( GL_COLOR_MATERIAL );

    GLfloat lightAmbientColor[] = { 0.3, 0.3, 0.3, 1.0 };
    glLightModelfv( GL_LIGHT_MODEL_AMBIENT, lightAmbientColor );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );

    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

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
    bool prevCtrlUp = false;
    bool prevCtrlDown = false;
    bool slowmo = false;

    while ( !quit )
    {
        UpdateEvents();

        platform::Input input;
        
        input = platform::Input::Sample();

        if ( input.quit )
            quit = true;

        if ( input.escape )
            RestoreDefaults();

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

        if ( input.tab )
        {
            scrollX += ( stone.rigidBody.position.x() - scrollX ) * 0.075f;
            scrollY += ( stone.rigidBody.position.y() - scrollY ) * 0.075f;
            scrollZ += ( stone.rigidBody.position.z() - scrollZ ) * 0.075f;
        }

        const float Spin = 0.1f;

        if ( input.q )
            stone.rigidBody.angularMomentum += Spin * vec3f(1,0,0);

        if ( input.w )
            stone.rigidBody.angularMomentum += Spin * vec3f(0,1,0);

        if ( input.e )
            stone.rigidBody.angularMomentum += Spin * vec3f(0,0,1);

        if ( input.r )
            stone.rigidBody.angularMomentum *= 0.75f;

        if ( input.alt )
        {
            if ( input.left )
                SelectStoneSize( size - 1 );
            
            if ( input.right )
                SelectStoneSize( size + 1 );

            if ( input.down )
            {
                float thickness = board.GetThickness();
                thickness *= 0.75f;
                if ( thickness < 0.5f )
                    thickness = 0.5f;
                board.SetThickness( thickness );
            }

            if ( input.up )
            {
                float thickness = board.GetThickness();
                thickness *= 1.25f;
                if ( thickness > 9 )
                    thickness = 9;
                board.SetThickness( thickness );
            }

            if ( input.one )
                board = Board( 9 );

            if ( input.two )
                board = Board( 13 );

            if ( input.three )
                board = Board( 19 );
        }
        else if ( input.control )
        {
            if ( input.up && !prevCtrlUp )
            {
                zoomLevel --;
                if ( zoomLevel < 0 )
                    zoomLevel = 0;
            }
            prevCtrlUp = input.up;

            if ( input.down && !prevCtrlDown )
            {
                zoomLevel ++;
                if ( zoomLevel > MaxZoomLevel - 1 )
                    zoomLevel = MaxZoomLevel - 1;
            }
            prevCtrlDown = input.down;
        }
        else
        {
            float sx = 0;
            float sy = 0;
            float sz = 0;

            if ( input.left )
                sx = -1;

            if ( input.right )
                sx = 1;

            if ( input.up )
                sz = 1;

            if ( input.down )
                sz = -1;

            if ( input.a )
                sy = 1;

            if ( input.z )
                sy = -1;

            vec3f scroll(sx,sy,sz);

            if ( length_squared( scroll ) > 0 )
            {
                scroll = normalize( scroll ) * ScrollSpeed * normal_dt;
                scrollX += scroll.x();
                scrollY += scroll.y();
                scrollZ += scroll.z();

                if ( scrollY > 50 )
                    scrollY = 50;
            }

            if ( input.one && !prevOne )
            {
                mode = LinearCollisionResponse;
                dt = normal_dt;
                slowmo = false;
            }
            prevOne = input.one;

            if ( input.two && !prevTwo )
            {
                mode = AngularCollisionResponse;
                dt = normal_dt;
                slowmo = false;
            }
            prevTwo = input.two;

            if ( input.three && !prevThree )
            {
                mode = CollisionResponseWithFriction;
                dt = normal_dt;
                slowmo = false;
            }
            prevThree = input.three;

            if ( input.four && !prevFour )
            {
                mode = SolidColor;
                dt = normal_dt;
                slowmo = false;
            }
            prevFour = input.four;

            if ( input.five && !prevFive )
            {
                mode = SolidColor;
                dt = normal_dt;
                slowmo = false;
            }
            prevFive = input.five;

            prevCtrlUp = false;
            prevCtrlDown = false;
        }

        ClearScreen( displayWidth, displayHeight );

        if ( mode == Nothing )
        {
            UpdateDisplay( 1 );
            frame++;
            continue;
        }

        // setup lights

        GLfloat lightPosition[] = { -10, 1000, -200, 1 };
        glLightfv( GL_LIGHT0, GL_POSITION, lightPosition );

        // setup projection + modelsview

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

        // update camera

        const float x = scrollX;
        const float y = scrollY;
        const float z = scrollZ;

        vec3f targetLookAt;
        vec3f targetPosition;

        const float t = board.GetThickness();

        if ( zoomLevel == 0 )
        {
            targetLookAt = vec3f(x,y,z);
            targetPosition = vec3f(x,y,z-5);
        }
        else if ( zoomLevel == 1 )
        {
            targetLookAt = vec3f(x,y+1,z);
            targetPosition = vec3f(x,y+1,z-10);
        }
        else if ( zoomLevel == 2 )
        {
            targetLookAt = vec3f(x,y+4,z);
            targetPosition = vec3f(x,y+8,z-25);
        }
        else if ( zoomLevel == 3 )
        {
            targetLookAt = vec3f(x,y+2,z);
            targetPosition = vec3f(x,y+20,z-30);
        }
        else if ( zoomLevel == 4 )
        {
            targetLookAt = vec3f(x,y,z);
            targetPosition = vec3f(x,y+55,z);
        }

        if ( cameraMode == mode )
        {
            cameraLookAt += ( targetLookAt - cameraLookAt ) * 0.25f;
            cameraPosition += ( targetPosition - cameraPosition ) * 0.25f;
        }
        else
        {
            cameraLookAt = targetLookAt;
            cameraPosition = targetPosition;
        }

        float amountBelow = t - fmax( targetPosition.y(), targetLookAt.y() );
        if ( amountBelow > 0 )
            scrollY += amountBelow;

        cameraMode = mode;

        vec3f cameraUp = cross( normalize( cameraLookAt - cameraPosition ), vec3f(1,0,0) );

        gluLookAt( cameraPosition.x(), max( cameraPosition.y(), t ), cameraPosition.z(),
                   cameraLookAt.x(), max( cameraLookAt.y(), t ), cameraLookAt.z(),
                   cameraUp.x(), cameraUp.y(), cameraUp.z() );

        // if the stone is inside the board at the beginning of the frame
        // push it out with a spring impulse -- this gives a cool effect
        // when the board thickness is adjusted dynamically (trapoline)

        {
            StaticContact boardContact;
            if ( StoneBoardCollision( stone.biconvex, board, stone.rigidBody, boardContact ) )
                stone.rigidBody.linearMomentum += boardContact.normal * boardContact.depth * 10;
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

        // render board

        glDepthMask( GL_TRUE );

        {
            glColor4f( 0,0,0,1 );
            glDisable( GL_LIGHTING );
            glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    
            glDisable( GL_CULL_FACE );
    
            RenderBoard( board );
    
            glEnable( GL_CULL_FACE );
        }

        if ( mode >= SolidColor )
        {
            glEnable( GL_LIGHTING );
            glColor4f( 0.8f,0.45f,0,1 );            
        }
        else
        {
            glColor4f( 0.4f,0.4f,0.4f,1 );
        }

        if ( cameraPosition.x() >= -board.GetWidth() / 2 &&
             cameraPosition.x() <= board.GetWidth() / 2 &&
             cameraPosition.z() >= -board.GetHeight() / 2 &&
             cameraPosition.z() <= board.GetHeight() / 2 &&
             cameraPosition.y() <= board.GetThickness() + 0.01f )
        {
            glLineWidth( 5 );
            glBegin( GL_LINES );
            glVertex3f( -1000, board.GetThickness(), cameraPosition.z() + 1.0f );
            glVertex3f( +1000, board.GetThickness(), cameraPosition.z() + 1.0f );
            glEnd();
        }
        else
        {
            if ( mode >= SolidColor )
            {
                glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
            }
            else
            {
                glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
                glDepthMask( GL_FALSE );
                glLineWidth( 5 );
            }

            RenderBoard( board );

            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

            glLineWidth( 3 );

            if ( mode >= SolidColor )
                glColor4f( 0,0,0,1 );

            glDisable( GL_DEPTH_TEST );

            RenderGrid( board.GetThickness(), board.GetSize(), board.GetCellWidth(), board.GetCellHeight() );

            glEnable( GL_DEPTH_TEST );
        }

        // render stone

        glEnable( GL_LIGHTING );

        glPushMatrix();

        RigidBodyTransform biconvexTransform( stone.rigidBody.position, stone.rigidBody.orientation );
        float opengl_transform[16];
        biconvexTransform.localToWorld.store( opengl_transform );
        glMultMatrixf( opengl_transform );

        if ( mode < SolidColor )
        {
            glEnable( GL_BLEND ); 
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glLineWidth( 1 );
            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
            glEnable( GL_DEPTH_TEST );
            glDepthMask( GL_FALSE );
        }
        else
        {
            glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
            glDepthMask( GL_FALSE );
            glEnable( GL_DEPTH_TEST );
            glDepthMask( GL_TRUE );
        }

        glColor4f( 1, 1, 1, 1 );

        RenderMesh( mesh[size] );

        glPopMatrix();

        // update the display
        
        UpdateDisplay( 1 );

        // update time

        frame++;
    }

    CloseDisplay();

    return 0;
}
