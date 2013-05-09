#ifndef PHYSICS_H
#define PHYSICS_H

#include <vector>
#include "Board.h"
#include "StoneData.h"
#include "StoneInstance.h"
#include "RigidBody.h"
#include "CollisionDetection.h"
#include "CollisionResponse.h"
#include "Telemetry.h"

inline void UpdatePhysics( float dt, const Board & board, const StoneData & stoneData, std::vector<StoneInstance> & stones,
                           Telemetry & telemetry, const Frustum & frustum,
                           const vec3f & gravity, bool selected, bool locked, float smoothZoom )
{
   // update stone physics

    #if MULTIPLE_STONES
    const int iterations = 1;
    #else
    const int iterations = 10;
    #endif

    const float iteration_dt = dt / iterations;

    for ( int i = 0; i < iterations; ++i )
    {
        for ( int j = 0; j < stones.size(); ++j )
        {
            StoneInstance & stone = stones[j];

            if ( !stone.rigidBody.active )
                continue;

            #if !MULTIPLE_STONES            
            if ( !selected )
            #endif
                stone.rigidBody.linearMomentum += gravity * stone.rigidBody.mass * iteration_dt;

            stone.rigidBody.UpdateMomentum();

            if ( !selected )
                stone.rigidBody.position += stone.rigidBody.linearVelocity * iteration_dt;

            const int rotation_substeps = 1;
            const float rotation_substep_dt = iteration_dt / rotation_substeps;
            for ( int j = 0; j < rotation_substeps; ++j )
            {
                quat4f spin;
                AngularVelocityToSpin( stone.rigidBody.orientation, stone.rigidBody.angularVelocity, spin );
                stone.rigidBody.orientation += spin * rotation_substep_dt;
                stone.rigidBody.orientation = normalize( stone.rigidBody.orientation );
            }        
            
            stone.rigidBody.UpdateTransform();

            StaticContact contact;

            bool iteration_collided = false;
            
            const float e = 0.5f;
            const float u = 0.5f;
        
            if ( !locked )
            {
                // collision between stone and near plane

                #if MULTIPLE_STONES
                vec4f nearPlane( 0, 0, -1, -smoothZoom * 0.5f );
                #else
                vec4f nearPlane( 0, 0, -1, -smoothZoom );
                #endif
                
                if ( StonePlaneCollision( stoneData.biconvex, nearPlane, stone.rigidBody, contact ) )
                {
                    ApplyCollisionImpulseWithFriction( contact, e, u );
                    iteration_collided = true;
                    telemetry.SetCollision( COLLISION_NearPlane );
                }

                // collision between stone and left plane

                if ( StonePlaneCollision( stoneData.biconvex, frustum.left, stone.rigidBody, contact ) )
                {
                    ApplyCollisionImpulseWithFriction( contact, e, u );
                    iteration_collided = true;
                    telemetry.SetCollision( COLLISION_LeftPlane );
                }

                // collision between stone and right plane

                if ( StonePlaneCollision( stoneData.biconvex, frustum.right, stone.rigidBody, contact ) )
                {
                    ApplyCollisionImpulseWithFriction( contact, e, u );
                    iteration_collided = true;
                    telemetry.SetCollision( COLLISION_RightPlane );
                }

                // collision between stone and top plane

                if ( StonePlaneCollision( stoneData.biconvex, frustum.top, stone.rigidBody, contact ) )
                {
                    ApplyCollisionImpulseWithFriction( contact, e, u );
                    iteration_collided = true;
                    telemetry.SetCollision( COLLISION_TopPlane );
                }

                // collision between stone and bottom plane

                if ( StonePlaneCollision( stoneData.biconvex, frustum.bottom, stone.rigidBody, contact ) )
                {
                    ApplyCollisionImpulseWithFriction( contact, e, u );
                    iteration_collided = true;
                    telemetry.SetCollision( COLLISION_BottomPlane );
                }
            }

            // collision between stone and ground plane
            
            if ( StonePlaneCollision( stoneData.biconvex, vec4f(0,0,1,0), stone.rigidBody, contact ) )
            {
                ApplyCollisionImpulseWithFriction( contact, e, u );
                iteration_collided = true;
                telemetry.SetCollision( COLLISION_GroundPlane );
            }

            // collision between stone and board

            if ( StoneBoardCollision( stoneData.biconvex, board, stone.rigidBody, contact, true, selected ) )
            {
                ApplyCollisionImpulseWithFriction( contact, e, u );
                iteration_collided = true;
                telemetry.SetCollision( COLLISION_Board );
            }

            // this is a *massive* hack to approximate rolling/spinning
            // friction and it is completely made up and not accurate at all!

            if ( iteration_collided )
            {
                #if LOCKED

                    const float factor = DecayFactor( 0.9f, iteration_dt );
                    stone.rigidBody.angularMomentum *= factor;

                #else

                    float momentum = length( stone.rigidBody.angularMomentum );
                
                    #if MULTIPLE_STONES
                    const float factor_a = DecayFactor( 0.75f, iteration_dt );
                    const float factor_b = DecayFactor( 0.99f, iteration_dt );
                    #else
                    const float factor_a = 0.9925f;
                    const float factor_b = 0.9995f;
                    #endif
                    
                    const float a = 0.0f;
                    const float b = 1.0f;
                    
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

                #endif
            }

            // apply damping

            #if MULTIPLE_STONES
            const float linear_factor = 0.99999f;
            const float angular_factor = 0.99999f;
            #else
            const float linear_factor = DecayFactor( 0.999f, iteration_dt );
            const float angular_factor = DecayFactor( 0.999f, iteration_dt );
            #endif

            stone.rigidBody.linearMomentum *= linear_factor;
            stone.rigidBody.angularMomentum *= angular_factor;
        }

        // deactivate stones at rest

        for ( int i = 0; i < stones.size(); ++i )
        {
            StoneInstance & stone = stones[i];

            if ( length_squared( stone.rigidBody.linearVelocity ) < DeactivateLinearThresholdSquared &&
                 length_squared( stone.rigidBody.angularVelocity ) < DeactivateAngularThresholdSquared )
            {
                stone.rigidBody.deactivateTimer += dt;
                if ( stone.rigidBody.deactivateTimer >= DeactivateTime )
                    stone.rigidBody.Deactivate();
            }
            else
                stone.rigidBody.deactivateTimer = 0;
        }
    }
}

#endif
