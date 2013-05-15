#ifndef PHYSICS_H
#define PHYSICS_H

#include <vector>
#include "Board.h"
#include "StoneData.h"
#include "StoneInstance.h"
#include "SceneGrid.h"
#include "RigidBody.h"
#include "CollisionDetection.h"
#include "CollisionResponse.h"
#include "Telemetry.h"
#include <set>

struct PhysicsParameters
{
    float dt;
    int iterations;
    int rotationSubsteps;

    vec3f gravity;

    float e;
    float u;
    
    float ceiling;

    float rollingFactorA;
    float rollingFactorB;
    float rollingSpeedA;
    float rollingSpeedB;
    
    float dampingThreshold;
    float dampingFactor;
    
    float deactivateTime;
    float deactivateLinearThreshold;
    float deactivateAngularThreshold;

    PhysicsParameters()
    {
        dt = 1.0f / 60.0f;
        iterations = 1;
        rotationSubsteps = 1;

        e = 0.5f;

        // size 40
        u = 0.35f;

        // size 32 
        //u = 0.5f;
        
        ceiling = 100.0f;
        
        gravity = vec3f(0,0,0);

        // size 40
        rollingFactorA = 0.75f;
        rollingFactorB = 0.975f;
        rollingSpeedA = 0.25f;
        rollingSpeedB = 1.0f;

        /*
        // size 32
        rollingFactorA = 0.75f;
        rollingFactorB = 0.99f;
        rollingSpeedA = 0.25f;
        rollingSpeedB = 1.0f;
        */
        
        dampingThreshold = 0.01f;
        dampingFactor = 0.999f;

        deactivateTime = 0.25f;
        deactivateLinearThreshold = 0.1f * 0.1f;
        deactivateAngularThreshold = 0.1f * 0.1f;
    }
};

struct IdPair
{
    uint32_t a : 16;
    uint32_t b : 16;

    bool operator < ( const IdPair & other ) const
    {
        return *((uint32_t*)this) < *((uint32_t*)&other);
    }
};

struct ObjectPair
{
    IdPair id_pair;
    StoneInstance * a;
    StoneInstance * b;
};

typedef std::set<IdPair> ObjectSet;

inline void FindOverlappingObjects( SceneGrid & sceneGrid,
                                    float boundingSphereRadiusSquared,
                                    std::vector<StoneInstance> & stones,
                                    std::vector<ObjectPair> & overlappingObjects )
{
    static ObjectSet objectSet;
    objectSet.clear();
    
    int nx, ny, nz;
    sceneGrid.GetIntegerBounds( nx, ny, nz );
    
    for ( int iz = 0; iz < nz; ++iz )
    {
        for ( int iy = 0; iy < ny; ++iy )
        {
            for ( int ix = 0; ix < nx; ++ix )
            {
                int index = sceneGrid.GetCellIndex( ix, iy, iz );
                
                const Cell & cell = sceneGrid.GetCell( index );
                    
                for ( int i = 0; i < cell.objects.size(); ++i )
                {
                    uint16_t a = cell.objects[i];

                    for ( int j = 0; j < cell.objects.size(); ++j )
                    {
                        uint16_t b = cell.objects[j];
                        
                        if ( a == b )
                            continue;

                        IdPair id_pair;
                        
                        if ( a < b )
                        {
                            id_pair.a = a;
                            id_pair.b = b;
                        }
                        else
                        {
                            id_pair.a = b;
                            id_pair.b = a;
                        }
                        
                        if ( objectSet.find( id_pair ) == objectSet.end() )
                        {
                            ObjectPair objectPair;
                            objectPair.id_pair = id_pair;
                            objectPair.a = FindStoneInstance( id_pair.a, stones );
                            objectPair.b = FindStoneInstance( id_pair.b, stones );
                            assert( objectPair.a );
                            assert( objectPair.b );
                            if ( length_squared( objectPair.a->rigidBody.position - objectPair.b->rigidBody.position ) < boundingSphereRadiusSquared * 4 )
                            {
                                objectSet.insert( id_pair );
                                overlappingObjects.push_back( objectPair );
                            }
                        }
                    }
                }

                // todo: compare base cell vs. the adjecent cells +1

                //   a: (+1,0,0)
                //   b: (0,+1,0)
                //   c: (0,0,+1)
                //   d: (+1,+1,0)
                //   e: (0,+1,+1)
                //   f: (+1,0,+1)
                //   g: (+1,+1,+1)
            }
        }
    }
}

inline void UpdatePhysics( const PhysicsParameters & params,
                           const Board & board, 
                           const StoneData & stoneData, 
                           SceneGrid & sceneGrid,
                           std::vector<StoneInstance> & stones,
                           Telemetry & telemetry, 
                           const Frustum & frustum )
{
    // update stone physics

    const float iteration_dt = params.dt / params.iterations;

    for ( int i = 0; i < params.iterations; ++i )
    {
        // =======================================================================
        // 0. keep track of previous position per-stone before any adjustments
        // =======================================================================
        
        vec3f previousPosition[MaxStones];
        
        for ( int j = 0; j < stones.size(); ++j )
        {
            StoneInstance & stone = stones[j];

            previousPosition[j] = stone.rigidBody.position;
        }
        
        // =======================================================================
        // 1. integrate motion
        // =======================================================================

        for ( int j = 0; j < stones.size(); ++j )
        {
            StoneInstance & stone = stones[j];

            if ( !stone.rigidBody.active )
                continue;

            if ( !stone.selected )
                stone.rigidBody.linearMomentum += params.gravity * stone.rigidBody.mass * iteration_dt;

            stone.rigidBody.UpdateMomentum();

            if ( !stone.selected )
                stone.rigidBody.position += stone.rigidBody.linearVelocity * iteration_dt;
            
            quat4f spin;
            AngularVelocityToSpin( stone.rigidBody.orientation, stone.rigidBody.angularVelocity, spin );
            const float rotation_substep_dt = iteration_dt / params.rotationSubsteps;
            for ( int j = 0; j < params.rotationSubsteps; ++j )
            {
                stone.rigidBody.orientation += spin * rotation_substep_dt;
                stone.rigidBody.orientation = normalize( stone.rigidBody.orientation );
            }
            
            stone.rigidBody.UpdateTransform();
        }

        // =======================================================================
        // 2. collide all objects against static planes, floor, board etc.
        // =======================================================================

        for ( int j = 0; j < stones.size(); ++j )
        {
            StoneInstance & stone = stones[j];

            if ( !stone.rigidBody.active )
                continue;

            StaticContact contact;

            bool iteration_collided = false;
            
            // collision between stone and near plane

            vec4f nearPlane( 0, 0, -1, -params.ceiling );
            if ( StonePlaneCollision( stoneData.biconvex, nearPlane, stone.rigidBody, contact ) )
            {
                ApplyCollisionImpulseWithFriction( contact, params.e, params.u );
                telemetry.SetCollision( COLLISION_NearPlane );
                iteration_collided = true;
            }

            // collision between stone and left plane

            if ( StonePlaneCollision( stoneData.biconvex, frustum.left, stone.rigidBody, contact ) )
            {
                ApplyCollisionImpulseWithFriction( contact, params.e, params.u );
                telemetry.SetCollision( COLLISION_LeftPlane );
                iteration_collided = true;
            }

            // collision between stone and right plane

            if ( StonePlaneCollision( stoneData.biconvex, frustum.right, stone.rigidBody, contact ) )
            {
                ApplyCollisionImpulseWithFriction( contact, params.e, params.u );
                telemetry.SetCollision( COLLISION_RightPlane );
                iteration_collided = true;
            }

            // collision between stone and top plane

            if ( StonePlaneCollision( stoneData.biconvex, frustum.top, stone.rigidBody, contact ) )
            {
                ApplyCollisionImpulseWithFriction( contact, params.e, params.u );
                telemetry.SetCollision( COLLISION_TopPlane );
                iteration_collided = true;
            }

            // collision between stone and bottom plane

            if ( StonePlaneCollision( stoneData.biconvex, frustum.bottom, stone.rigidBody, contact ) )
            {
                ApplyCollisionImpulseWithFriction( contact, params.e, params.u );
                telemetry.SetCollision( COLLISION_BottomPlane );
                iteration_collided = true;
            }

            // collision between stone and ground plane
            
            if ( StonePlaneCollision( stoneData.biconvex, vec4f(0,0,1,0), stone.rigidBody, contact ) )
            {
                ApplyCollisionImpulseWithFriction( contact, params.e, params.u );
                telemetry.SetCollision( COLLISION_GroundPlane );
                iteration_collided = true;
            }

            // collision between stone and board

            if ( StoneBoardCollision( stoneData.biconvex, board, stone.rigidBody, contact, true, stone.selected ) )
            {
                ApplyCollisionImpulseWithFriction( contact, params.e, params.u );
                telemetry.SetCollision( COLLISION_Board );
                iteration_collided = true;
            }

            // this is a *massive* hack to approximate rolling/spinning
            // friction and it is completely made up and not accurate at all!

            if ( iteration_collided )
            {
                if ( length_squared( stone.rigidBody.angularMomentum ) > 0.0001f )
                {
                    const float factor_a = DecayFactor( params.rollingFactorA, iteration_dt );
                    const float factor_b = DecayFactor( params.rollingFactorB, iteration_dt );
                    
                    const float momentum = length( stone.rigidBody.angularMomentum );

                    const float a = params.rollingSpeedA;
                    const float b = params.rollingSpeedB;
                    
                    if ( momentum >= b )
                    {
                        stone.rigidBody.angularMomentum *= factor_b;
                    }
                    else if ( momentum <= a )
                    {
                        stone.rigidBody.angularMomentum *= factor_a;
                    }
                    else
                    {
                        const float alpha = ( momentum - a ) / ( b - a );
                        const float factor = factor_a * ( 1 - alpha ) + factor_b * alpha;
                        stone.rigidBody.angularMomentum *= factor;
                    }
                }
                else
                {
                    stone.rigidBody.angularMomentum = vec3f( 0, 0, 0 );
                }
            }

            // apply air resistance damping

            if ( length_squared( stone.rigidBody.linearMomentum ) > params.dampingThreshold ||
                 length_squared( stone.rigidBody.angularMomentum ) > params.dampingThreshold )
            {
                const float linear_factor = DecayFactor( params.dampingFactor, iteration_dt );
                const float angular_factor = DecayFactor( params.dampingFactor, iteration_dt );

                stone.rigidBody.linearMomentum *= linear_factor;
                stone.rigidBody.angularMomentum *= angular_factor;
            }

            // apply select damping

            if ( stone.selected )
                stone.rigidBody.angularMomentum *= SelectDamping;
        }

        // =======================================================================
        // 3. update stone positions in scene grid post static-collision
        // =======================================================================

        for ( int j = 0; j < stones.size(); ++j )
        {
            StoneInstance & stone = stones[j];
            
            if ( !stone.rigidBody.active )
                continue;

            sceneGrid.MoveObject( stone.id, previousPosition[j], stone.rigidBody.position );
            
            previousPosition[j] = stone.rigidBody.position;
        }        
        
        // =======================================================================
        // 4. collide stones against other stones
        // =======================================================================
        
        static std::vector<ObjectPair> potentiallyOverlappingObjects;
        potentiallyOverlappingObjects.clear();

        FindOverlappingObjects( sceneGrid, 
                                stoneData.biconvex.GetBoundingSphereRadiusSquared(),
                                stones,
                                potentiallyOverlappingObjects );

        /*
        static float accum = 0;
        accum += params.dt;
        if ( accum > 1 )
        {
            printf( "overlapping objects: %d\n", (int) potentiallyOverlappingObjects.size() );
            accum = 0;
        }
        */

        for ( int j = 0; j < potentiallyOverlappingObjects.size(); ++j )
        {
            ObjectPair & pair = potentiallyOverlappingObjects[j];
            
            StoneInstance * a = pair.a;
            StoneInstance * b = pair.b;

            vec3f & position_a = a->rigidBody.position;
            vec3f & position_b = b->rigidBody.position;

            vec3f difference = position_a - position_b;

            float distance = length( difference );

            vec3f axis = distance > 0.00001f ? normalize( difference ) : vec3f(0,1,0);

            const float boundingSphereRadius = stoneData.biconvex.GetBoundingSphereRadius();

            const float penetration = 2 * boundingSphereRadius - distance;

            if ( !a->selected )
                position_a += axis * penetration / 2;
            
            if ( !b->selected )
                position_b -= axis * penetration / 2;
            
            // todo: apply simple linear impulse response
            
            // todo: treat selected stones as if they have infinite mass!
            
            a->rigidBody.Activate();
            b->rigidBody.Activate();
            
            a->rigidBody.UpdateTransform();
            b->rigidBody.UpdateTransform();
        }
                
        // =======================================================================
        // 5. update stone positions in scene grid post dynamic-collision
        // =======================================================================
        
        for ( int j = 0; j < stones.size(); ++j )
        {
            StoneInstance & stone = stones[j];
            
            if ( !stone.rigidBody.active )
                continue;
            
            sceneGrid.MoveObject( stone.id, previousPosition[j], stone.rigidBody.position );

            stone.rigidBody.UpdateTransform();
        }        
    }

    // =======================================================================
    // 6. detect stones at rest and deactivate them
    // =======================================================================

    for ( int i = 0; i < stones.size(); ++i )
    {
        StoneInstance & stone = stones[i];

        if ( !stone.rigidBody.active )
            continue;

        stone.rigidBody.UpdateMomentum();

        // hackfix: post-collision response there is some velocity remaining up
        // this velocity causes the stone to drift and never come to rest.
        // I just clear less than certain amount of z momentum to zero to fix!
        float linear_z = stone.rigidBody.linearMomentum.z();
        if ( linear_z > 0 && linear_z < 1.0f * stone.rigidBody.mass )
        {
            stone.rigidBody.linearMomentum = vec3f( stone.rigidBody.linearMomentum.x(), stone.rigidBody.linearMomentum.y(), 0 );
            stone.rigidBody.UpdateMomentum();
        }

        if ( length_squared( stone.rigidBody.linearVelocity ) < params.deactivateLinearThreshold &&
             length_squared( stone.rigidBody.angularVelocity ) < params.deactivateAngularThreshold )
        {
            stone.rigidBody.deactivateTimer += params.dt;
            if ( stone.rigidBody.deactivateTimer >= params.deactivateTime )
                stone.rigidBody.Deactivate();
        }
        else
            stone.rigidBody.deactivateTimer = 0;
    }
}

#endif
