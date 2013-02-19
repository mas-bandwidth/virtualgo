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
    if ( mode == LinearCollisionResponse )
        rigidBody.orientation = quat4f(1,0,0,0);
    rigidBody.linearMomentum = vec3f(0,0,0);
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

    rigidBody.position = vec3f(0,-10,0);        // hack: for the talk demo don't show anything initially
    
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

    bool quit = false;

    srand( time( NULL ) );

    const float normal_dt = 1.0f / 60.0f;
    const float slowmo_dt = normal_dt * 0.1f;

    float dt = normal_dt;

    uint64_t frame = 0;

    bool prevSpace = false;
    bool prevEnter = false;
    bool slowmo = false;
    bool collidedLastFrame = false;

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

        if ( input.enter && !prevEnter )
        {
            if ( collidedLastFrame && mode == CollisionResponseWithFriction )
            {
                vec3f direction( random_float(-1,+1), 0, random_float(-1,+1) );
                rigidBody.linearVelocity += -3 * normalize( direction );
            }
        }
        prevEnter = input.enter;

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

            gluLookAt( 0, 5, -25.0f, 
                       0, 4, 0, 
                       0, 1, 0 );

            glEnable( GL_CULL_FACE );
            glCullFace( GL_BACK );
            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

            // render board

            Board board( 40.0f, 40.0f, 1.0f );

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

            rigidBody.position += rigidBody.linearVelocity * dt;
            quat4f spin = AngularVelocityToSpin( rigidBody.orientation, rigidBody.angularVelocity );
            rigidBody.orientation += spin * dt;
            rigidBody.orientation = normalize( rigidBody.orientation );

            if ( mode == LinearCollisionResponse )
            {
                // if object is penetrating with the board, push it out

                float depth;
                vec3f point, normal;
                if ( IntersectStoneBoard( board, biconvex, RigidBodyTransform( rigidBody.position, rigidBody.orientation ), point, normal, depth ) )
                {
                    rigidBody.position += normal * depth;
                    colliding = true;
                }

                // apply linear collision response

                if ( colliding )
                {
                    vec3f stonePoint, stoneNormal, boardPoint, boardNormal;
                    ClosestFeaturesStoneBoard( board, biconvex, RigidBodyTransform( rigidBody.position, rigidBody.orientation ), stonePoint, stoneNormal, boardPoint, boardNormal );

                    const vec3f velocityAtPoint = rigidBody.linearVelocity;

                    const float e = 0.7f;

                    const float k = rigidBody.inverseMass;

                    const float j = max( - ( 1 + e ) * dot( velocityAtPoint, boardNormal ) / k, 0 );

                    rigidBody.linearMomentum += j * boardNormal;
                }
            }
            else if ( mode == AngularCollisionResponse || mode == CollisionResponseWithFriction )
            {
                // detect collision

                float depth;
                vec3f point, normal;
                if ( IntersectStoneBoard( board, biconvex, 
                                          RigidBodyTransform( rigidBody.position, rigidBody.orientation ), 
                                          point, normal, depth ) )
                {
                    rigidBody.position += normal * depth;
                    colliding = true;
                }

                if ( colliding )
                {
                    vec3f stonePoint, stoneNormal, boardPoint, boardNormal;
                    ClosestFeaturesStoneBoard( board, biconvex, RigidBodyTransform( rigidBody.position, rigidBody.orientation ), 
                                               stonePoint, stoneNormal, boardPoint, boardNormal );

                    vec3f p = stonePoint;
                    vec3f n = boardNormal;

                    vec3f r = p - rigidBody.position;

                    vec3f vp = rigidBody.linearVelocity + cross( rigidBody.angularVelocity, r );

                    const float vn = dot( vp, n );

                    if ( vn < 0 )
                    {
                        // calculate kinetic energy before collision response

                        const float ke_before_collision = rigidBody.GetKineticEnergy();

                        // calculate inverse inertia tensor in world space

                        mat4f rotation;
                        rigidBody.orientation.toMatrix( rotation );
                        mat4f transposeRotation = transpose( rotation );
                        mat4f i = rotation * rigidBody.inverseInertiaTensor * transposeRotation;

                        // apply collision impulse

                        const float e = 0.7f;

                        const float linear_k = rigidBody.inverseMass;
                        const float angular_k = dot( cross( transformVector( i, cross( r, n ) ), r ), n );

                        const float k = linear_k + angular_k;

                        const float j = - ( 1 + e ) * vn / k;

                        rigidBody.linearMomentum += j * n;
                        rigidBody.angularMomentum += j * cross( r, n );

                        const float ke_after_collision = rigidBody.GetKineticEnergy();
                        assert( ke_after_collision <= ke_before_collision );

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

                                rigidBody.linearMomentum += jt * t;
                                rigidBody.angularMomentum += jt * cross( r, t );

                                const float ke_after_friction = rigidBody.GetKineticEnergy();
                                if ( ke_after_friction > ke_after_collision )
                                {
                                    printf( "%f -> %f\n", ke_after_collision, ke_after_friction );
                                }
                                //assert( ke_after_friction <= ke_after_collision );
                            }
                        }
                    }
                }
            }

            rigidBody.Update();

            collidedLastFrame = colliding;

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
