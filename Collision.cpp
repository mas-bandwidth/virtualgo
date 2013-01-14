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
#include "Collision.h"
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
    rigidBody.linearVelocity = vec3f(0,0,0);
    rigidBody.angularVelocity = vec3f(0,0,0);
}

int main()
{
    printf( "[virtual go]\n" );

    Mode mode = LinearCollisionResponse;

    Biconvex biconvex( 2.2f, 1.13f, 0.1f );

    RigidBody rigidBody;
    rigidBody.mass = 1.0f;
    rigidBody.inverseMass = 1.0f / rigidBody.mass;
    CalculateBiconvexInertiaTensor( rigidBody.mass, biconvex, rigidBody.inertiaTensor, rigidBody.inverseInertiaTensor );

    RandomStone( biconvex, rigidBody, mode );

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

    ShowMouseCursor();

    glEnable( GL_LINE_SMOOTH );
    glEnable( GL_POLYGON_SMOOTH );
    glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
    glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );

    double t = 0.0f;

    bool prevSpace = false;
    bool prevEnter = false;
    bool quit = false;

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
            RandomStone( biconvex, rigidBody, mode );
        prevSpace = input.space;

        if ( input.enter && !prevEnter )
            rigidBody.linearVelocity += vec3f( -2,0,0 );
        prevEnter = input.enter;

        if ( input.one )
        {
            mode = LinearCollisionResponse;
            RandomStone( biconvex, rigidBody, mode );
        }

        if ( input.two )
        {
            mode = AngularCollisionResponse;
            RandomStone( biconvex, rigidBody, mode );
        }

        if ( input.three )
        {
            mode = CollisionResponseWithFriction;
            RandomStone( biconvex, rigidBody, mode );
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

            // update stone physics

            // hack: substeps are required for the impulse
            // to look stable and not jitter at rest. why?!
            const int steps = 1;

            const float step_dt = dt / steps;

            for ( int i = 0; i < steps; ++i )
            {
                RigidBodyTransform biconvexTransform( rigidBody.position, rigidBody.orientation );

                bool colliding = false;

                const float gravity = 9.8f * 10;    // cms/sec^2
                rigidBody.linearVelocity += vec3f(0,-gravity,0) * step_dt;

                if ( mode == LinearCollisionResponse )
                {
                    // if object is penetrating with the board, push it out

                    biconvexTransform = RigidBodyTransform( rigidBody.position, rigidBody.orientation );

                    float depth;
                    vec3f point, normal;
                    if ( IntersectStoneBoard( board, biconvex, biconvexTransform, point, normal, depth ) )
                    {
                        rigidBody.position += normal * depth;
                        colliding = true;
                    }

                    biconvexTransform = RigidBodyTransform( rigidBody.position, rigidBody.orientation );

                    // apply linear collision response

                    if ( colliding )
                    {
                        vec3f stonePoint, stoneNormal, boardPoint, boardNormal;
                        ClosestFeaturesStoneBoard( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal );

                        const vec3f velocityAtPoint = rigidBody.linearVelocity;

                        const float e = 0.5f;

                        const float k = rigidBody.inverseMass;

                        const float j = max( - ( 1 + e ) * dot( velocityAtPoint, boardNormal ) / k, 0 );

                        rigidBody.linearVelocity += j * rigidBody.inverseMass * boardNormal;
                    }
                }
                else if ( mode == AngularCollisionResponse || mode == CollisionResponseWithFriction )
                {
                    // detect collision

                    biconvexTransform = RigidBodyTransform( rigidBody.position, rigidBody.orientation );

                    float depth;
                    vec3f point, normal;
                    if ( IntersectStoneBoard( board, biconvex, biconvexTransform, point, normal, depth ) )
                    {
                        rigidBody.position += normal * depth;
                        colliding = true;
                    }

                    if ( colliding )
                    {
                        biconvexTransform = RigidBodyTransform( rigidBody.position, rigidBody.orientation );

                        vec3f stonePoint, stoneNormal, boardPoint, boardNormal;
                        ClosestFeaturesStoneBoard( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal );

                        vec3f p = stonePoint;
                        vec3f n = boardNormal;

                        vec3f r = p - rigidBody.position;

                        vec3f vp = rigidBody.linearVelocity + cross( rigidBody.angularVelocity, r );

                        const float vn = dot( vp, n );

                        if ( vn < 0 )
                        {
                            // calculate kinetic energy before collision response

                            const float ke_before = rigidBody.GetKineticEnergy();

                            // calculate inverse inertia tensor in world space

                            mat4f rotation;
                            rigidBody.orientation.toMatrix( rotation );
                            mat4f transposeRotation = transpose( rotation );
                            mat4f i = rotation * rigidBody.inverseInertiaTensor * transposeRotation;

                            // apply collision impulse

                            const float e = 0.6f;

                            const float linear_k = rigidBody.inverseMass;
                            const float angular_k = dot( cross( transformVector( i, cross( r, n ) ), r ), n );

                            const float k = linear_k + angular_k;

                            const float j = - ( 1 + e ) * vn / k;

                            rigidBody.linearVelocity += j * rigidBody.inverseMass * n;
                            rigidBody.angularVelocity += j * transformVector( i, cross( r, n ) );

                            // apply friction impulse

                            if ( mode == CollisionResponseWithFriction )
                            {
                                vec3f tangent_velocity = vp - n * dot( vp, n );

                                if ( length_squared( tangent_velocity ) > 0.0001f * 0.0001f )
                                {
                                    vec3f t = normalize( tangent_velocity );

                                    float u = 0.15f;

                                    const float vt = dot( vp, t );

                                    const float kt = rigidBody.inverseMass + dot( cross( transformVector( i, cross( r, t ) ), r ), t );

                                    const float jt = clamp( -vt / kt, -u * j, u * j );

                                    rigidBody.linearVelocity += jt * rigidBody.inverseMass * t;
                                    rigidBody.angularVelocity += jt * transformVector( i, cross( r, t ) );
                                }
                            }

                            // calculate kinetic energy after collision response
                            // IMPORTANT: verify that we never add energy!

                            const float ke_after = rigidBody.GetKineticEnergy();

                            assert( ke_after <= ke_before + 0.001f );
                        }
                    }
                }

                // integrate with velocities post collision response

                rigidBody.position += rigidBody.linearVelocity * step_dt;
                quat4f spin = AngularVelocityToSpin( rigidBody.orientation, rigidBody.angularVelocity );
                rigidBody.orientation += spin * step_dt;
                rigidBody.orientation = normalize( rigidBody.orientation );

                // render stone

                biconvexTransform = RigidBodyTransform( rigidBody.position, rigidBody.orientation );

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
