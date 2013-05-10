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

inline void UpdatePhysics( float dt, 
                           const Board & board, 
                           const StoneData & stoneData, 
                           std::vector<StoneInstance> & stones,
                           Telemetry & telemetry, 
                           const Frustum & frustum,
                           const vec3f & gravity, 
                           float ceiling )
{
    // update stone physics

    const int iterations = 1;

    const float iteration_dt = dt / iterations;

    for ( int i = 0; i < iterations; ++i )
    {
        for ( int j = 0; j < stones.size(); ++j )
        {
            StoneInstance & stone = stones[j];

            if ( !stone.rigidBody.active )
                continue;

            stone.rigidBody.linearMomentum += gravity * stone.rigidBody.mass * iteration_dt;

            stone.rigidBody.UpdateMomentum();

            stone.rigidBody.position += stone.rigidBody.linearVelocity * iteration_dt;

            quat4f spin;
            AngularVelocityToSpin( stone.rigidBody.orientation, stone.rigidBody.angularVelocity, spin );
            const int rotation_substeps = 10;
            const float rotation_substep_dt = iteration_dt / rotation_substeps;
            for ( int j = 0; j < rotation_substeps; ++j )
            {
                stone.rigidBody.orientation += spin * rotation_substep_dt;
                stone.rigidBody.orientation = normalize( stone.rigidBody.orientation );
            }        
            
            stone.rigidBody.UpdateTransform();

            StaticContact contact;

            bool iteration_collided = false;
            
            const float e = 0.5f;
            const float u = 0.5f;
        
            // collision between stone and near plane

            vec4f nearPlane( 0, 0, -1, -ceiling );
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

            // collision between stone and ground plane
            
            if ( StonePlaneCollision( stoneData.biconvex, vec4f(0,0,1,0), stone.rigidBody, contact ) )
            {
                ApplyCollisionImpulseWithFriction( contact, e, u );
                iteration_collided = true;
                telemetry.SetCollision( COLLISION_GroundPlane );
            }

            // collision between stone and board

            bool selected = false;          // todo: if stone is selected push out along board
            
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
                if ( length_squared( stone.rigidBody.angularMomentum ) > 0.0001f )
                {
                    const float factor_a = DecayFactor( 0.75f, iteration_dt );
                    const float factor_b = DecayFactor( 0.99f, iteration_dt );
                    
                    const float momentum = length( stone.rigidBody.angularMomentum );

                    const float a = 0.25f;
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
                }
                else
                {
                    stone.rigidBody.angularMomentum = vec3f( 0, 0, 0 );
                }
            }

            // apply damping

            const float linear_factor = DecayFactor( 0.999f, iteration_dt );
            const float angular_factor = DecayFactor( 0.999f, iteration_dt );

            stone.rigidBody.linearMomentum *= linear_factor;
            stone.rigidBody.angularMomentum *= angular_factor;
        }

        // deactivate stones at rest

        for ( int i = 0; i < stones.size(); ++i )
        {
            StoneInstance & stone = stones[i];

            if ( !stone.rigidBody.active )
                continue;

            stone.rigidBody.UpdateMomentum();

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
