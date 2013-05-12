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
        u = 0.5f;
        
        ceiling = 100.0f;
        
        gravity = vec3f(0,0,0);

        rollingFactorA = 0.75f;
        rollingFactorB = 0.99f;
        rollingSpeedA = 0.25f;
        rollingSpeedB = 1.0f;
        
        dampingThreshold = 0.5f;
        dampingFactor = 0.9995f;

        deactivateTime = 0.1f;
        deactivateLinearThreshold = 0.1f * 0.1f;
        deactivateAngularThreshold = 0.05f * 0.05f;
    }
};

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
        // 1. integrate object motion for this iteration
        // =======================================================================

        for ( int j = 0; j < stones.size(); ++j )
        {
            StoneInstance & stone = stones[j];

            if ( !stone.rigidBody.active )
                continue;

            vec3f previousPosition = stone.rigidBody.position;

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

            sceneGrid.MoveObject( stone.id, previousPosition, stone.rigidBody.position );
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

            // apply damping

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
        // 3. collide stones against other stones
        // =======================================================================

        // ...
    }

    // =======================================================================
    // 4. detect stones at rest and deactivate them
    // =======================================================================

    for ( int i = 0; i < stones.size(); ++i )
    {
        StoneInstance & stone = stones[i];

        if ( !stone.rigidBody.active )
            continue;

        stone.rigidBody.UpdateMomentum();

        if ( length_squared( stone.rigidBody.linearVelocity ) < params.deactivateLinearThreshold &&
             length_squared( stone.rigidBody.angularVelocity ) < params.deactivateAngularThreshold )
        {
            stone.rigidBody.deactivateTimer += iteration_dt;
            if ( stone.rigidBody.deactivateTimer >= params.deactivateTime )
                stone.rigidBody.Deactivate();
        }
        else
            stone.rigidBody.deactivateTimer = 0;
    }
}

#endif
