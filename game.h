// Copyright © 2015, The Network Protocol Company, Inc. All Rights Reserved.

#ifndef GAME_H
#define GAME_H

#include "entity.h"
#include "cubes.h"
#include "physics.h"

struct Input
{
    bool left;
    bool right;
    bool up;
    bool down;
    bool push;
    bool pull;

    Input()
    {
        left = false;
        right = false;
        up = false;
        down = false;
        push = false;
        pull = false;
    }

    bool operator == ( const Input & other )
    {
        return left == other.left    &&
              right == other.right   &&
                 up == other.up      &&
               down == other.down    &&
               push == other.push    &&
               pull == other.pull;
    }
    
    bool operator != ( const Input & other )
    {
        return ! ( (*this) == other );
    }
};

inline void ProcessPlayerInput( EntityManager * entity_manager, PhysicsManager * physics_manager, int player_id, const Input & input, uint64_t tick, double t, float dt )
{
    assert( entity_manager );
    assert( physics_manager );

    const int player_entity_index = ENTITY_PLAYER_BEGIN + player_id;

    assert( entity_manager->GetType( player_entity_index ) == ENTITY_TYPE_CUBE );

    CubeEntity * player_cube = (CubeEntity*) entity_manager->GetEntity( player_entity_index );

    assert( player_cube );

    const float strafe_force = 120.0f;

    float force_x = 0.0f;
    float force_y = 0.0f;
    
    force_x -= strafe_force * (int) input.left;
    force_x += strafe_force * (int) input.right;
    force_y += strafe_force * (int) input.up;
    force_y -= strafe_force * (int) input.down;

    vec3f force( force_x, force_y, 0.0f );

    if ( !input.push )
    {
        if ( dot( player_cube->linear_velocity, force ) < 0.0f )
            force *= 2.0f;
    }

    physics_manager->ApplyForce( player_cube->physics_index, force );

    if ( input.push )
    {
        // todo: driving wobble from tick is bad so I changed to t but needs to be retuned.

        // bobbing force on player cube
        {
            const float wobble_x = sin(t*0.1+1) + sin(t*0.05f+3) + sin(t+10);
            const float wobble_y = sin(t*0.1+2) + sin(t*0.05f+4) + sin(t+11);
            const float wobble_z = sin(t*0.1+3) + sin(t*0.05f+5) + sin(t+12);
            vec3f force = vec3f( wobble_x, wobble_y, wobble_z ) * 2.0f;
            physics_manager->ApplyForce( player_cube->physics_index, force );
        }
        
        // bobbing torque on player cube
        {
            const float wobble_x = sin(t*0.05+10) + sin(t*0.05f+22)  + sin(t*1.1f);
            const float wobble_y = sin(t*0.09+5)  + sin(t*0.045f+16) + sin(t*1.11f);
            const float wobble_z = sin(t*0.05+4)  + sin(t*0.055f+9)  + sin(t*1.12f);
            vec3f torque = vec3f( wobble_x, wobble_y, wobble_z ) * 1.5f;
            physics_manager->ApplyTorque( player_cube->physics_index, torque );
        }

        // todo: untangle wtf this is supposed to do
        /*
        // calculate velocity tilt for player cube

        vec3f targetUp(0,0,1);
        {
            vec3f strafe( force.x(), force.y(), 0 );
            float speed = velocity.length();
            if ( length_speed > 0.001f )
            {
                math::Vector axis = -force[playerId].cross( math::Vector(0,0,1) );
                axis.normalize();
                float tilt = speed * 0.1f;
                if ( tilt > 0.25f )
                    tilt = 0.25f;
                targetUp = math::Quaternion( tilt, axis ).transform( targetUp );
            }
        }
        */
        
        // stay upright torque on player cube
        vec3f target_up(0,0,1);
        {
            vec3f current_up = player_cube->orientation.transform( vec3f(0,0,1) );
            vec3f axis = cross( target_up, current_up );
            float angle = acos( dot( target_up, current_up ) );
            if ( angle > 0.5f )
                angle = 0.5f;
            vec3f torque = - 100 * axis * angle;
            physics_manager->ApplyTorque( player_cube->physics_index, torque );
        }

        // todo: this linear damping is not framerate independent
        
        // apply damping to player cube
        player_cube->angular_velocity *= 0.95f;
        player_cube->linear_velocity *= vec3f( 0.96f, 0.96f, 0.999f );
    }
    
    vec3f push_origin = player_cube->position;

    push_origin += vec3f( 0, 0, -0.1f );

    if ( input.push || input.pull )
    {
        for ( int i = 0; i < MaxEntities; ++i )
        {
            if ( ( entity_manager->GetFlag( i ) & ENTITY_FLAG_ALLOCATED ) == 0 )
                continue;

            if ( entity_manager->GetType( i ) != ENTITY_TYPE_CUBE )
                continue;

            auto cube = (CubeEntity*) entity_manager->GetEntity( i );

            assert( cube );

            const float mass = cube->size * cube->size * cube->size;

            if ( input.push )
            {
                vec3f difference = cube->position - push_origin;

                if ( cube == player_cube )
                    difference -= vec3f( 0, 0, 0.7f );
                
                const float distance_squared = length_squared( difference );

                if ( distance_squared > 0.0001f && distance_squared < 4.0f )
                {
                    const float distance = sqrt( distance_squared );

                    vec3f direction = difference / distance;

                    float magnitude = 1.0f / distance_squared * 200.0f;
                    if ( magnitude > 500.0f )
                        magnitude = 500.0f;

                    vec3f force = direction * magnitude;

                    if ( cube != player_cube )
                        force *= mass;

                    if ( cube == player_cube && input.pull )
                        force *= 20;

                    if ( cube->authority == 0 || cube->authority == player_id )
                    {
                        physics_manager->ApplyForce( cube->physics_index, force );
                        cube->authority = player_id;
                        // todo: authority time needed? probably.
                    }
                }
            }
            else if ( input.pull )
            {
                if ( cube != player_cube )
                {
                    vec3f origin( player_cube->position.x(), player_cube->position.y(), 0.0f );

                    vec3f difference = cube->position - origin;

                    const float distance_squared = length_squared( difference );
                    
                    const float effective_radius_squared = 1.8f * 1.8f;

                    if ( distance_squared > 0.2f*0.2f && distance_squared < effective_radius_squared )
                    {
                        const float distance = sqrt( distance_squared );

                        vec3f direction = difference / distance;

                        float magnitude = 1.0f / distance_squared * 200.0f;
                        
                        if ( magnitude > 2000.0f )
                            magnitude = 2000.0f;

                        vec3f force = - direction * magnitude;

                        if ( cube->authority == player_id || cube->authority == MaxPlayers )
                        {
                            physics_manager->ApplyForce( cube->physics_index, force * mass );
                            cube->authority = player_id;
                            // todo: authority time needed? probably.
                        }
                    }
                }
            }
        }
    }
}

#endif // #ifndef GAME_H