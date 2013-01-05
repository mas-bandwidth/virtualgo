/*
    Go stone tesselation demo.
    Copyright (c) 2004-2013, Glenn Fiedler. All rights reserved.
*/

#include "Common.h"
#include "Render.h"
#include "Stone.h"
#include "Mesh.h"
#include "Platform.h"
#include "Biconvex.h"
#include "RigidBody.h"
#include "Collision.h"
#include "Intersection.h"

using namespace platform;

enum Mode
{
    Naive,
    Subdivision,
    SubdivisionWithBevel
};

int main()
{
    srand( time( NULL ) );

    printf( "[tesselation]\n" );

    Biconvex biconvex( 2.2f, 1.13f );

    Biconvex biconvexWithBevel( 2.2f, 1.13f, 0.22f );

    Mode mode = Naive;

    RigidBody rigidBody;
    rigidBody.mass = 1.0f;
    rigidBody.inverseMass = 1.0f / rigidBody.mass;
    CalculateBiconvexInertiaTensor( rigidBody.mass, biconvex, rigidBody.inertiaTensor, rigidBody.inverseInertiaTensor );

    Mesh mesh;
    GenerateBiconvexMesh( mesh, biconvex );
    
    int displayWidth, displayHeight;
    GetDisplayResolution( displayWidth, displayHeight );

    #ifdef LETTERBOX
    displayWidth = 1280;
    displayHeight = 800;
    #endif

    printf( "display resolution is %d x %d\n", displayWidth, displayHeight );

    if ( !OpenDisplay( "Tesselation Demo", displayWidth, displayHeight, 32 ) )
    {
        printf( "error: failed to open display" );
        return 1;
    }

    HideMouseCursor();

    // setup for render

    glEnable( GL_LINE_SMOOTH );
    glEnable( GL_POLYGON_SMOOTH );
    glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
    glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );

    glEnable( GL_LIGHTING );
    glEnable( GL_LIGHT0 );

    glShadeModel( GL_SMOOTH );

    GLfloat lightAmbientColor[] = { 0.5, 0.5, 0.5, 1.0 };
    glLightModelfv( GL_LIGHT_MODEL_AMBIENT, lightAmbientColor );

    glLightf( GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.01f );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glLineWidth( 1 );
    glColor4f( 0.5f,0.5f,0.5f,1 );
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );

    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    mat4f rotation = mat4f::identity();

    bool prevSpace = false;
    bool rotating = true;
    bool quit = false;

    float smoothedRotation = 0.0f;

    const float dt = 1.0f / 60.0f;

    uint64_t frame = 0;

    while ( true )
    {
        UpdateEvents();

        platform::Input input;
        
        input = platform::Input::Sample();

        if ( input.escape || input.quit )
            break;

        if ( input.one )
        {
            mode = Naive;
        }
        else if ( input.two )
        {
            mode = Subdivision;
            mesh.Clear();
            GenerateBiconvexMesh( mesh, biconvex );
            printf( "num vertices = %d, num zero buckets = %d, average bucket = %.1f, largest bucket = %d (2)\n", 
                mesh.GetNumVertices(), 
                mesh.GetNumZeroBuckets(),
                mesh.GetAverageBucketSize(), 
                mesh.GetLargestBucketSize() );
        }
        else if ( input.three )
        {
            mode = SubdivisionWithBevel;
            mesh.Clear();
            GenerateBiconvexMesh( mesh, biconvexWithBevel );
            printf( "num vertices = %d, num zero buckets = %d, average bucket = %.1f, largest bucket = %d (3)\n", 
                mesh.GetNumVertices(), 
                mesh.GetNumZeroBuckets(),
                mesh.GetAverageBucketSize(), 
                mesh.GetLargestBucketSize() );
        }

        if ( input.space )
        {
            if ( !prevSpace )
                rotating = !rotating;

            prevSpace = true;
        }
        else
            prevSpace = false;

        ClearScreen( displayWidth, displayHeight );

        if ( frame > 20 )
        {
            const float fov = 25.0f;
            glMatrixMode( GL_PROJECTION );
            glLoadIdentity();
            gluPerspective( fov, (float) displayWidth / (float) displayHeight, 0.1f, 100.0f );

            float flipX[] = { -1,0,0,0,
                               0,1,0,0,
                               0,0,1,0,
                               0,0,0,1 };

            glMultMatrixf( flipX );

            glMatrixMode( GL_MODELVIEW );
            glLoadIdentity();
            gluLookAt( -5, 0, 0, 
                       0, 0, 0, 
                       0, 1, 0 );

            // set light position

            GLfloat lightPosition[] = { -10, 3, 10, 1 };
            glLightfv( GL_LIGHT0, GL_POSITION, lightPosition );

            // render stone

            const float targetRotation = rotating ? 0.28f : 0.0f;
            smoothedRotation += ( targetRotation - smoothedRotation ) * 0.15f;
            mat4f deltaRotation = mat4f::axisRotation( smoothedRotation, vec3f(1,-2,3) );
            rotation = rotation * deltaRotation;

            RigidBodyTransform biconvexTransform( vec3f(0,0,0), rotation );

            glPushMatrix();

            float opengl_transform[16];
            biconvexTransform.localToWorld.store( opengl_transform );
            glMultMatrixf( opengl_transform );

            if ( mode == Naive )
                RenderBiconvex_Naive( biconvex );
            else
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
