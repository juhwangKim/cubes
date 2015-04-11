// Copyright © 2015, The Network Protocol Company, Inc. All Rights Reserved.

#ifndef CUBES_H
#define CUBES_H

#include "entity.h"
#include "physics.h"
#include "shared.h"

struct CubeEntity : public Entity
{
    float size;
    CubeEntity() : size( 1.0f ) {}
};

struct CubeManager
{
    EntityManager * entity_manager;
    PhysicsManager * physics_manager;
    bool allocated[MaxCubes];
    int entity_index_to_cube_index[MaxCubes];
    CubeEntity cubes[MaxCubes];

    CubeManager( EntityManager * entity_manager, PhysicsManager * physics_manager )
    {
        assert( entity_manager );
        assert( physics_manager );
        this->entity_manager = entity_manager;
        this->physics_manager = physics_manager;
        memset( allocated, 0, sizeof( allocated ) );
        for ( int i = 0; i < MaxCubes; ++i )
            entity_index_to_cube_index[i] = -1;
    }

    CubeEntity * CreateCube( const vec3f & position, float size, int required_entity_index = ENTITY_NULL )
    {
        const int entity_index = entity_manager->Allocate( ENTITY_TYPE_CUBE, required_entity_index );
        if ( entity_index == ENTITY_NULL )
            return nullptr;
        for ( int i = 0; i < MaxCubes; ++i )
        {
            if ( !allocated[i] )
            {
                allocated[i] = true;
                entity_index_to_cube_index[entity_index] = i;
                cubes[i].entity_index = entity_index;

                PhysicsObjectState initial_state;
                initial_state.position = position;
                initial_state.scale = size;
                initial_state.enabled = true;
                cubes[i].physics_index = physics_manager->AddObject( initial_state, PHYSICS_SHAPE_CUBE );
                
                return &cubes[i];
            }
        }
        return nullptr;
    }

    void DestroyCube( CubeEntity * cube_entity )
    {
        assert( cube_entity );
        const int entity_index = cube_entity->entity_index;
        assert( entity_index >= 0 );
        assert( entity_index < MaxEntities );
        const int cube_index = entity_index_to_cube_index[entity_index];
        assert( cube_index >= 0 );
        assert( cube_index < MaxCubes );
        allocated[cube_index] = false;
        entity_index_to_cube_index[entity_index] = -1;
        cubes[cube_index] = CubeEntity();
        entity_manager->Free( entity_index );
    }

    void Update( double t, float dt )
    {
        // ...
    }
};

#endif // #ifndef CUBES_H