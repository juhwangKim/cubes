// Copyright © 2015, The Network Protocol Company, Inc. All Rights Reserved.

#include "protocol.h"
#include "network.h"
#include "shared.h"
#include "world.h"
#include "game.h"
#include <stdio.h>
#include <signal.h>

struct Server
{
    Socket * socket = nullptr;
};

void server_init( Server & server )
{
    server.socket = new Socket( ServerPort );

    printf( "listening on port %d\n", server.socket->GetPort() );
}

void server_free( Server & server )
{
    printf( "shutting down\n" );

    delete server.socket;
    server = Server();
}

void server_tick( World & world )
{
//    printf( "%d-%d: %f [%+.4f]\n", (int) world.frame, (int) world.tick, world.time, TickDeltaTime );

    world_tick( world );
}

void server_frame( World & world, double real_time, double frame_time, double jitter )
{
    //printf( "%d: %f [%+.2fms]\n", (int) frame, real_time, jitter * 1000 );
    
    for ( int i = 0; i < TicksPerServerFrame; ++i )
    {
        server_tick( world );
    }
}

static volatile int quit = 0;

void interrupt_handler( int dummy )
{
    quit = 1;
}

int server_main( int argc, char ** argv )
{
    printf( "starting server\n" );

    Server server;

    server_init( server );

    World world;
    world_init( world );
    world_setup_cubes( world );

    const double start_time = platform_time();

    double previous_frame_time = start_time;
    double next_frame_time = previous_frame_time + ServerFrameDeltaTime;

    signal( SIGINT, interrupt_handler );

    while ( !quit )
    {
        const double time_to_sleep = max( 0.0, next_frame_time - platform_time() - AverageSleepJitter );

        platform_sleep( time_to_sleep );

        const double real_time = platform_time();

        const double frame_time = next_frame_time;

        const double jitter = real_time - frame_time;

        server_frame( world, real_time, frame_time, jitter );

        const double end_of_frame_time = platform_time();

        int num_frames_advanced = 0;
        while ( next_frame_time < end_of_frame_time + ServerFrameSafety * ServerFrameDeltaTime )
        {
            next_frame_time += ServerFrameDeltaTime;
            num_frames_advanced++;
        }

        const int num_dropped_frames = max( 0, num_frames_advanced - 1 );

        if ( num_dropped_frames > 0 )
        {
            printf( "dropped frame %d (%d)\n", (int) world.frame, num_dropped_frames );
        }

        previous_frame_time = next_frame_time - ServerFrameDeltaTime;

        world.frame++;
    }

    world_free( world );

    server_free( server );

    return 0;
}

int main( int argc, char ** argv )
{
    return server_main( argc, argv ); 
}
