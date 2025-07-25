/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "server.h"


/*
===============
SV_SendConfigstring

Creates and sends the server command necessary to update the CS index for the
given client
===============
*/
static void SV_SendConfigstring(client_t *client, int index)
{
	int maxChunkSize = MAX_STRING_CHARS - 24;
	int len;

	len = strlen(sv.configstrings[index]);

	if( len >= maxChunkSize ) {
		int		sent = 0;
		int		remaining = len;
		const char	*cmd;
		char	buf[MAX_STRING_CHARS];

		while (remaining > 0 ) {
			if ( sent == 0 ) {
				cmd = "bcs0";
			}
			else if( remaining < maxChunkSize ) {
				cmd = "bcs2";
			}
			else {
				cmd = "bcs1";
			}
			Q_strncpyz( buf, &sv.configstrings[index][sent],
				maxChunkSize );

			SV_SendServerCommand( client, "%s %i \"%s\"", cmd,
				index, buf );

			sent += (maxChunkSize - 1);
			remaining -= (maxChunkSize - 1);
		}
	} else {
		// standard cs, just send it
		SV_SendServerCommand( client, "cs %i \"%s\"", index,
			sv.configstrings[index] );
	}
}

/*
===============
SV_UpdateConfigstrings

Called when a client goes from CS_PRIMED to CS_ACTIVE.  Updates all
Configstring indexes that have changed while the client was in CS_PRIMED
===============
*/
void SV_UpdateConfigstrings(client_t *client)
{
	int index;

	for( index = 0; index < MAX_CONFIGSTRINGS; index++ ) {
		// if the CS hasn't changed since we went to CS_PRIMED, ignore
		if(!client->csUpdated[index])
			continue;

		// do not always send server info to all clients
		if ( index == CS_SERVERINFO && ( SV_GentityNum( client - svs.clients )->r.svFlags & SVF_NOSERVERINFO ) ) {
			continue;
		}

		// RF, don't send to bot/AI
		// Gordon: Note: might want to re-enable later for bot support
		// RF, re-enabled
		// Arnout: removed hardcoded gametype
		// Arnout: added coop
		if ( ( SV_GameIsSinglePlayer() || SV_GameIsCoop() ) && ( SV_GentityNum( client - svs.clients )->r.svFlags & SVF_BOT ) ) {
			continue;
		}

		SV_SendConfigstring(client, index);
		client->csUpdated[index] = qfalse;
	}
}

/*
===============
SV_SetConfigstring

===============
*/
void SV_SetConfigstringNoUpdate( int index, const char *val ) {
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		Com_Error( ERR_DROP, "SV_SetConfigstring: bad index %i", index );
	}

	if ( !val ) {
		val = "";
	}

	// don't bother broadcasting an update if no change
	if ( !strcmp( val, sv.configstrings[ index ] ) ) {
		return;
	}

	// change the string in sv
	Z_Free( sv.configstrings[index] );
	sv.configstrings[index] = CopyString( val );
}

void SV_SetConfigstring (int index, const char *val) {
	int		i;
	client_t	*client;

	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		Com_Error (ERR_DROP, "SV_SetConfigstring: bad index %i", index);
	}

	if ( !val ) {
		val = "";
	}

	// don't bother broadcasting an update if no change
	if ( !strcmp( val, sv.configstrings[ index ] ) ) {
		return;
	}

	// change the string in sv
	Z_Free( sv.configstrings[index] );
	sv.configstrings[index] = CopyString( val );

	// send it to all the clients if we aren't
	// spawning a new server
	if ( sv.state == SS_GAME || sv.restarting ) {

		// send the data to all relevant clients
		for (i = 0, client = svs.clients; i < sv_maxclients->integer ; i++, client++) {
			if ( client->state < CS_ACTIVE ) {
				if ( client->state == CS_PRIMED )
					client->csUpdated[ index ] = qtrue;
				continue;
			}
			// do not always send server info to all clients
			if ( index == CS_SERVERINFO && ( SV_GentityNum( i )->r.svFlags & SVF_NOSERVERINFO ) ) {
				continue;
			}

			// RF, don't send to bot/AI
			// Gordon: Note: might want to re-enable later for bot support
			// RF, re-enabled
			// Arnout: removed hardcoded gametype
			// Arnout: added coop
			if ( ( SV_GameIsSinglePlayer() || SV_GameIsCoop() ) && ( SV_GentityNum( client - svs.clients )->r.svFlags & SVF_BOT ) ) {
				continue;
			}
		
			SV_SendConfigstring(client, index);
		}
	}
}


/*
===============
SV_GetConfigstring
===============
*/
void SV_GetConfigstring( int index, char *buffer, int bufferSize ) {
	if ( bufferSize < 1 ) {
		Com_Error( ERR_DROP, "SV_GetConfigstring: bufferSize == %i", bufferSize );
	}
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		Com_Error (ERR_DROP, "SV_GetConfigstring: bad index %i", index);
	}
	if ( !sv.configstrings[index] ) {
		buffer[0] = '\0';
		return;
	}

	Q_strncpyz( buffer, sv.configstrings[index], bufferSize );
}


/*
===============
SV_SetUserinfo

===============
*/
void SV_SetUserinfo( int index, const char *val ) {
	if ( index < 0 || index >= sv_maxclients->integer ) {
		Com_Error (ERR_DROP, "SV_SetUserinfo: bad index %i", index);
	}

	if ( !val ) {
		val = "";
	}

	Q_strncpyz( svs.clients[index].userinfo, val, sizeof( svs.clients[ index ].userinfo ) );
	Q_strncpyz( svs.clients[index].name, Info_ValueForKey( val, "name" ), sizeof(svs.clients[index].name) );
	Q_strncpyz( svs.clients[index].guid, Info_ValueForKey( val, "cl_guid" ), sizeof(svs.clients[index].guid) );
}



/*
===============
SV_GetUserinfo

===============
*/
void SV_GetUserinfo( int index, char *buffer, int bufferSize ) {
	if ( bufferSize < 1 ) {
		Com_Error( ERR_DROP, "SV_GetUserinfo: bufferSize == %i", bufferSize );
	}
	if ( index < 0 || index >= sv_maxclients->integer ) {
		Com_Error (ERR_DROP, "SV_GetUserinfo: bad index %i", index);
	}
	Q_strncpyz( buffer, svs.clients[ index ].userinfo, bufferSize );
}


/*
================
SV_CreateBaseline

Entity baselines are used to compress non-delta messages
to the clients -- only the fields that differ from the
baseline will be transmitted
================
*/
static void SV_CreateBaseline( void ) {
	sharedEntity_t *ent;
	int				entnum;

	for ( entnum = 0; entnum < sv.num_entities ; entnum++ ) {
		ent = SV_GentityNum( entnum );
		if ( !ent->r.linked ) {
			continue;
		}
		ent->s.number = entnum;

		//
		// take current state as baseline
		//
		sv.svEntities[ entnum ].baseline = ent->s;
		sv.baselineUsed[ entnum ] = 1;
	}
}


/*
===============
SV_BoundMaxClients
===============
*/
static void SV_BoundMaxClients( int minimum ) {
	// get the current maxclients value
	Cvar_Get( "sv_maxclients", "20", 0 );         // NERVE - SMF - changed to 20 from 8

	// START	xkan, 10/03/2002
	// allow many bots in single player. note that this pretty much means all previous
	// settings will be ignored (including the one set through "seta sv_maxclients <num>"
	// in user profile's wolfconfig_mp.cfg). also that if the user subsequently start
	// the server in multiplayer mode, the number of clients will still be the number
	// set here, which may be wrong - we can certainly just set it to a sensible number
	// when it is not in single player mode in the else part of the if statement when
	// necessary
	if ( SV_GameIsSinglePlayer() || SV_GameIsCoop() ) {
		Cvar_Set( "sv_maxclients", "64" );
	}
	// END		xkan, 10/03/2002

	sv_maxclients->modified = qfalse;

	if ( sv_maxclients->integer < minimum ) {
		Cvar_Set( "sv_maxclients", va("%i", minimum) );
	} else if ( sv_maxclients->integer > MAX_CLIENTS ) {
		Cvar_Set( "sv_maxclients", va("%i", MAX_CLIENTS) );
	}
}


/*
===============
SV_SetSnapshotParams
===============
*/
static void SV_SetSnapshotParams( void )
{
	// PACKET_BACKUP frames is just about 6.67MB so use that even on listen servers
	svs.numSnapshotEntities = PACKET_BACKUP * MAX_GENTITIES;
}

#define USE_CLIENTS_ZONE 1


/*
===============
SV_Startup

Called when a host starts a map when it wasn't running
one before.  Successive map or map_restart commands will
NOT cause this to be called, unless the game is exited to
the menu system first.
===============
*/
static void SV_Startup( void ) {
	if ( svs.initialized ) {
		Com_Error( ERR_FATAL, "SV_Startup: svs.initialized" );
	}
	SV_BoundMaxClients( 1 );

#ifdef USE_CLIENTS_ZONE
	svs.clients = Z_TagMalloc( sv_maxclients->integer * sizeof( client_t ), TAG_CLIENTS );
	Com_Memset( svs.clients, 0, sv_maxclients->integer * sizeof( client_t ) );
#else
	// RF, avoid trying to allocate large chunk on a fragmented zone
	svs.clients = calloc( sizeof( client_t ) * sv_maxclients->integer, 1 );
	if ( !svs.clients ) {
		Com_Error( ERR_FATAL, "SV_Startup: unable to allocate svs.clients" );
	}
#endif

	SV_SetSnapshotParams();
	svs.initialized = qtrue;

	// Don't respect sv_killserver unless a server is actually running
	if ( sv_killserver->integer ) {
		Cvar_Set( "sv_killserver", "0" );
	}

	Cvar_Set( "sv_running", "1" );

	// Join the ipv6 multicast group now that a map is running so clients can scan for us on the local network.
#ifdef USE_IPV6
	NET_JoinMulticast6();
#endif
}


/*
==================
SV_ChangeMaxClients
==================
*/
void SV_ChangeMaxClients( void ) {
	int		oldMaxClients;
	int		i;
	client_t	*oldClients;
	int		count;

	// get the highest client number in use
	count = 0;
	for ( i = 0 ; i < sv_maxclients->integer ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			if (i > count)
				count = i;
		}
	}
	count++;

	oldMaxClients = sv_maxclients->integer;
	// never go below the highest client number in use
	SV_BoundMaxClients( count );
	// if still the same
	if ( sv_maxclients->integer == oldMaxClients ) {
		return;
	}

	oldClients = Hunk_AllocateTempMemory( count * sizeof(client_t) );
	// copy the clients to hunk memory
	for ( i = 0 ; i < count ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			oldClients[i] = svs.clients[i];
		}
		else {
			Com_Memset(&oldClients[i], 0, sizeof(client_t));
		}
	}

	// free old clients arrays
#ifdef USE_CLIENTS_ZONE
	Z_Free( svs.clients );
#else
	free( svs.clients );    // RF, avoid trying to allocate large chunk on a fragmented zone
#endif

	// allocate new clients
#ifdef USE_CLIENTS_ZONE
	svs.clients = Z_TagMalloc( sv_maxclients->integer * sizeof(client_t), TAG_CLIENTS );
	Com_Memset( svs.clients, 0, sv_maxclients->integer * sizeof( client_t ) );
#else
	// RF, avoid trying to allocate large chunk on a fragmented zone
	svs.clients = calloc( sizeof( client_t ) * sv_maxclients->integer, 1 );
	if ( !svs.clients ) {
		Com_Error( ERR_FATAL, "SV_Startup: unable to allocate svs.clients" );
	}
#endif

	// copy the clients over
	for ( i = 0 ; i < count ; i++ ) {
		if ( oldClients[i].state >= CS_CONNECTED ) {
			svs.clients[i] = oldClients[i];
		}
	}

	// free the old clients on the hunk
	Hunk_FreeTempMemory( oldClients );

	SV_SetSnapshotParams();
}


/*
====================
SV_SetExpectedHunkUsage

  Sets com_expectedhunkusage, so the client knows how to draw the percentage bar
====================
*/
#define HUNKUSAGE_FILENAME "hunkusage.dat"
static void SV_SetExpectedHunkUsage( const char *mapname ) {
	int handle;
	char *buf;
	const char *buftrav;
	const char *token;
	int len;

	len = FS_FOpenFileByMode( HUNKUSAGE_FILENAME, &handle, FS_READ );
	if ( len >= 0 ) { // the file exists, so read it in, strip out the current entry for this map, and save it out, so we can append the new value

		buf = (char *)Z_Malloc( len + 1 );
		memset( buf, 0, len + 1 );

		FS_Read( (void *)buf, len, handle );
		FS_FCloseFile( handle );

		// now parse the file, filtering out the current map
		buftrav = (const char*)buf;
		while ( ( token = COM_Parse( &buftrav ) ) != NULL && token[0] ) {
			if ( !Q_stricmp( token, mapname ) ) {
				// found a match
				token = COM_Parse( &buftrav );  // read the size
				if ( token && token[0] ) {
					// this is the usage
					com_expectedhunkusage = atoi( token );
					Z_Free( buf );
					return;
				}
			}
		}

		Z_Free( buf );
	}

	// just set it to a negative number,so the cgame knows not to draw the percent bar
	com_expectedhunkusage = -1;
}

/*
================
SV_ClearServer
================
*/
static void SV_ClearServer( void ) {
	int i;

	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		if ( sv.configstrings[i] ) {
			Z_Free( sv.configstrings[i] );
		}
	}

	if ( !sv_levelTimeReset->integer ) {
		i = sv.time;
		Com_Memset( &sv, 0, sizeof( sv ) );
		sv.time = i;
	} else {
		Com_Memset( &sv, 0, sizeof( sv ) );
	}
}


/*
================
SV_TouchCGameDLL
  touch the cgame DLL so that a pure client (with DLL sv_pure support) can load do the correct checks
================
*/
const char* Sys_GetDLLName( const char *name );

static void SV_TouchDLLFile( const char *module ) {
	int ref;
	const char* filename;

	filename = Sys_GetDLLName( module );
	ref = FS_TouchFileInPak( filename );

	// ydnar: so we can work the damn game
	if ( ref <= FS_GENERAL_REF && sv_pure->integer )
		Com_Error( ERR_DROP, "Failed to locate %s DLL for pure server mode", module );
}


/*
================
SV_SpawnServer

Change the server to a new map, taking all connected
clients along with it.
This is NOT called for map_restart
================
*/
void SV_SpawnServer( const char *mapname ) {
	int			i;
	int			checksum;
	qboolean	isBot;
	const char	*p, *pnames;
	char		bspname[MAX_QPATH];
	int			pakChecksum = 0; // checksum of pk3 map is in

	// ydnar: broadcast a level change to all connected clients
	if ( svs.clients && !com_errorEntered ) {
		SV_FinalCommand( "spawnserver", qfalse );
	}

	// shut down the existing game if it is running
	SV_ShutdownGameProgs();

	Com_Printf( "------ Server Initialization ------\n" );
	Com_Printf( "Server: %s\n", mapname );

	Com_sprintf( bspname, sizeof(bspname), "maps/%s.bsp", mapname );

	Sys_SetStatus( "Initializing server..." );

#ifndef DEDICATED
	// if not running a dedicated server CL_MapLoading will connect the client to the server
	// also print some status stuff
	CL_MapLoading();

	// make sure all the client stuff is unloaded
	CL_ShutdownAll();
#endif

	// clear the whole hunk because we're (re)loading the server
	Hunk_Clear();

	// clear collision map data
	CM_ClearMap();

	// timescale can be updated before SV_Frame() and cause division-by-zero in SV_RateMsec()
	Cvar_CheckRange( com_timescale, "0.001", "100", CV_FLOAT );

	// Restart renderer?
	// CL_StartHunkUsers( );

	// init client structures and svs.numSnapshotEntities
	if ( !Cvar_VariableIntegerValue( "sv_running" ) ) {
		SV_Startup();
	} else {
		// check for maxclients change
		if ( sv_maxclients->modified ) {
			SV_ChangeMaxClients();
		}
	}

#ifndef DEDICATED
	// remove pure paks that may left from client-side
	FS_ClearPureServerPaks();
	//FS_PureServerSetLoadedPaks( "", "" );
	//FS_PureServerSetReferencedPaks( "", "" );
#endif

	// clear pak references
	FS_ClearPakReferences( 0 );

	// allocate the snapshot entities on the hunk
	svs.snapshotEntities = Hunk_Alloc( sizeof(entityState_t)*svs.numSnapshotEntities, h_high );

	// initialize snapshot storage
	SV_InitSnapshotStorage();

	// toggle the server bit so clients can detect that a
	// server has changed
	svs.snapFlagServerBit ^= SNAPFLAG_SERVERCOUNT;

	// set nextmap to the same map, but it may be overridden
	// by the game startup or another console command
	Cvar_Set( "nextmap", "map_restart 0" );
//	Cvar_Set( "nextmap", va("map %s", server) );

	// try to reset level time if server is empty
	if ( !sv_levelTimeReset->integer && !sv.restartTime ) {
		for ( i = 0; i < sv_maxclients->integer; i++ ) {
			if ( svs.clients[i].state >= CS_CONNECTED ) {
				break;
			}
		}
		if ( i == sv_maxclients->integer ) {
			sv.time = 0;
		}
	}

	for ( i = 0; i < sv_maxclients->integer; i++ ) {
		// save when the server started for each client already connected
		if ( svs.clients[i].state >= CS_CONNECTED && sv_levelTimeReset->integer ) {
			svs.clients[i].oldServerTime = sv.time;
		} else {
			svs.clients[i].oldServerTime = 0;
		}
	}

	// wipe the entire per-level structure
	SV_ClearServer();
	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		sv.configstrings[i] = CopyString("");
	}

	// Ridah
	// DHM - Nerve :: We want to use the completion bar in multiplayer as well
	// Arnout: just always use it
//	if( !SV_GameIsSinglePlayer() ) {
	SV_SetExpectedHunkUsage( bspname );
//	} else {
	// just set it to a negative number,so the cgame knows not to draw the percent bar
//		Cvar_Set( "com_expectedhunkusage", "-1" );
//	}

	// make sure we are not paused
#ifndef DEDICATED
	Cvar_Set( "cl_paused", "0" );
#endif

	// get latched value
	Cvar_Get( "sv_pure", "1", CVAR_SYSTEMINFO | CVAR_LATCH );

	// get a new checksum feed and restart the file system
	srand( Com_Milliseconds() );
	Com_RandomBytes( (byte*)&sv.checksumFeed, sizeof( sv.checksumFeed ) );
	FS_Restart( sv.checksumFeed );

	Sys_SetStatus( "Loading map %s", mapname );
	CM_LoadMap( bspname, qfalse, &checksum );

	// set serverinfo visible name
	Cvar_Set( "mapname", mapname );

	Cvar_Set( "sv_mapChecksum", va( "%i",checksum ) );
	FS_FileIsInPAK( bspname, &pakChecksum, NULL );
	Cvar_Set( "sv_currentPak", va( "%d", pakChecksum ) );

	// serverid should be different each time
	sv.serverId = com_frameTime;
	sv.restartedServerId = sv.serverId; // I suppose the init here is just to be safe
	sv.checksumFeedServerId = sv.serverId;
	Cvar_Set( "sv_serverid", va( "%i", sv.serverId ) );

	// clear physics interaction links
	SV_ClearWorld();

	// media configstring setting should be done during
	// the loading stage, so connected clients don't have
	// to load during actual gameplay
	sv.state = SS_LOADING;

	Cvar_Set( "sv_serverRestarting", "1" );

	// load and spawn all other entities
	SV_InitGameProgs();

	// don't allow a map_restart if game is modified
	// Arnout: there isn't any check done against this, obsolete
//	sv_gametype->modified = qfalse;
	sv_pure->modified = qfalse;

	// run a few frames to allow everything to settle
	for ( i = 0 ; i < GAME_INIT_FRAMES ; i++ )
	{
		VM_Call( gvm, GAME_RUN_FRAME, sv.time );
        sv.time += FRAMETIME;
		svs.time += FRAMETIME;
		////SV_BotFrame (sv.time);
	}

	// create a baseline for more efficient communications
	SV_CreateBaseline();

	for ( i = 0; i < sv_maxclients->integer; i++ ) {
		// send the new gamestate to all connected clients
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			const char *denied;

			if ( svs.clients[i].netchan.remoteAddress.type == NA_BOT ) {
				if ( SV_GameIsSinglePlayer() || SV_GameIsCoop() ) {
					SV_DropClient( &svs.clients[i], "was kicked" );
					continue;
				}
				isBot = qtrue;
			}
			else {
				isBot = qfalse;
			}

			// connect the client again
			denied = GVM_ArgPtr( VM_Call( gvm, GAME_CLIENT_CONNECT, i, qfalse, isBot ) );	// firstTime = qfalse
			if ( denied ) {
				// this generally shouldn't happen, because the client
				// was connected before the level change
				SV_DropClient( &svs.clients[i], denied );
			} else {
				if( !isBot ) {
					// when we get the next packet from a connected client,
					// the new gamestate will be sent
					svs.clients[i].state = CS_CONNECTED;
				}
				else {
					client_t		*client;
					sharedEntity_t	*ent;

					client = &svs.clients[i];
					client->state = CS_ACTIVE;
					ent = SV_GentityNum( i );
					ent->s.number = i;
					client->gentity = ent;

					client->deltaMessage = client->netchan.outgoingSequence - ( PACKET_BACKUP + 1 ); // force delta reset
					client->lastSnapshotTime = svs.time - 9999; // generate a snapshot immediately

					VM_Call( gvm, GAME_CLIENT_BEGIN, i );
				}
			}
		}
	}

	// run another frame to allow things to look at all the players
	VM_Call( gvm, GAME_RUN_FRAME, sv.time );
	sv.time += FRAMETIME;
	////SV_BotFrame( sv.time );
	svs.time += FRAMETIME;

	// we want the server to reference the mp_bin pk3 that the client is expected to load from
	SV_TouchDLLFile( "cgame" );
	SV_TouchDLLFile( "ui" );

	// the server sends these to the clients so they can figure
	// out which pk3s should be auto-downloaded
	p = FS_ReferencedPakNames( NULL );
	if ( FS_ExcludeReference() ) {
		// \fs_excludeReference may mask our current ui/cgame binaries
		SV_TouchDLLFile( "cgame" );
		SV_TouchDLLFile( "ui" );
		// rebuild referenced paks list
		p = FS_ReferencedPakNames( NULL );
	}
	if( sv_pure->integer == 0 ) {
		int cgameChksum = 0, uiChksum = 0;
		FS_FileIsInPAK( SYS_DLLNAME_CGAME, &cgameChksum, NULL );
		FS_FileIsInPAK( SYS_DLLNAME_UI, &uiChksum, NULL );
		Cvar_Set( "sv_clientPaks", va("%d %d", cgameChksum, uiChksum) );
	}
	Cvar_Set( "sv_referencedPakNames", p );

	p = FS_ReferencedPakChecksums( NULL );
	Cvar_Set( "sv_referencedPaks", p );

	Cvar_Set( "sv_paks", "" );
	Cvar_Set( "sv_pakNames", "" ); // not used on client-side (except for FS_VerifyOfficialPaks :@@@@)

	if ( sv_pure->integer ) {
		int freespace, pakslen, infolen, paknameslen, numpurepaks;
		qboolean overflowed = qfalse, nameoverflowed = qfalse;
		qboolean infoTruncated = qfalse;

		p = FS_LoadedPakChecksums( &overflowed, &numpurepaks );
		pnames = FS_LoadedPakNames( qtrue, &nameoverflowed );

		pakslen = strlen( p ) + 9; // + strlen( "\\sv_paks\\" )
		paknameslen = strlen( pnames ) + 13; // strlen( "\\sv_pakNames\\" )
		freespace = SV_RemainingGameState();
		infolen = strlen( Cvar_InfoString_Big( CVAR_SYSTEMINFO, &infoTruncated ) );

		if ( infoTruncated ) {
			Com_Printf( S_COLOR_YELLOW "WARNING: truncated systeminfo!\n" );
		}

		if ( numpurepaks > 256 || pakslen > freespace || paknameslen > freespace || pakslen + paknameslen > freespace || infolen + paknameslen + pakslen >= BIG_INFO_STRING || overflowed || nameoverflowed ) {
			// switch to degraded pure mode
			// this could *potentially* lead to a false "unpure client" detection
			// which is better than guaranteed drop
			// however due to requirement for sv_pakNames in ET over Q3, it is very likely that drops will happen for vanilla clients
			if ( sv_paksOverflowMode->integer == 2 || numpurepaks > 256 ) {
				qboolean refoverflowed = qfalse, refnameoverflowed = qfalse;
				p = FS_ReferencedPakChecksums( &refoverflowed );
				pnames = FS_ReferencedPakNames( &refnameoverflowed );

				pakslen = strlen( p ) + 9; // + strlen( "\\sv_paks\\" )
				paknameslen = strlen( pnames ) + 13; // strlen( "\\sv_pakNames\\" )
				if ( pakslen > freespace || paknameslen > freespace || pakslen + paknameslen > freespace || infolen + paknameslen + pakslen >= BIG_INFO_STRING || refoverflowed || refnameoverflowed ) {
					Com_Error( ERR_FATAL, "Server is referencing too many pk3s and cannot compensate with referenced only mode" );
				}
				else {
					if ( numpurepaks > 256 ) {
						Com_Printf( S_COLOR_RED "NOTICE: using alternate sv_paks setup with referenced values only because thenumber of pure pk3s exceeds backwards compatible amount (256)\n" );
					}
					else {
						Com_Printf( S_COLOR_ORANGE "WARNING: using alternate sv_paks setup with sv_referencedPaks to avoid gamestate overflow\n" );
					}
					// the server sends these to the clients so they will only
					// load pk3s also loaded at the server
					Cvar_Set( "sv_paks", p );
					if ( *p == '\0' ) {
						Com_Printf( S_COLOR_YELLOW "WARNING: sv_pure set but no PK3 files loaded\n" );
					}
					Cvar_Set( "sv_pakNames", pnames );
				}
			}
			else {
				Com_Printf( S_COLOR_YELLOW "WARNING: skipping sv_paks setup to avoid gamestate overflow\n" );
				Com_Printf( S_COLOR_YELLOW "WARNING: clients not running ETe (%s) will be unlikely to join this server\n", Q3_VERSION );
			}
		} else {
			// the server sends these to the clients so they will only
			// load pk3s also loaded at the server
			Cvar_Set( "sv_paks", p );
			if ( *p == '\0' ) {
				Com_Printf( S_COLOR_YELLOW "WARNING: sv_pure set but no PK3 files loaded\n" );
			}
			Cvar_Set( "sv_pakNames", pnames );
		}
	}

	// save systeminfo and serverinfo strings
	SV_SetConfigstring( CS_SYSTEMINFO, Cvar_InfoString_Big( CVAR_SYSTEMINFO, NULL ) );
	cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;

	SV_SetConfigstring( CS_SERVERINFO, Cvar_InfoString( CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE, NULL ) );
	cvar_modifiedFlags &= ~CVAR_SERVERINFO;

	// NERVE - SMF
	SV_SetConfigstring( CS_WOLFINFO, Cvar_InfoString( CVAR_WOLFINFO, NULL ) );
	cvar_modifiedFlags &= ~CVAR_WOLFINFO;

	// any media configstring setting now should issue a warning
	// and any configstring changes should be reliably transmitted
	// to all clients
	sv.state = SS_GAME;

	// send a heartbeat now so the master will get up to date info
	SV_Heartbeat_f();

	Hunk_SetMark();

	Cvar_Set( "sv_serverRestarting", "0" );

	Com_Printf ("-----------------------------------\n");

	Sys_SetStatus( "Running map %s", mapname );
}


/*
===============
SV_Init

Only called at main exe startup, not for each game
===============
*/
void SV_Init( void )
{
	//int index;

	SV_AddOperatorCommands();

	if ( com_dedicated->integer )
		SV_AddDedicatedCommands();

	// serverinfo vars
	Cvar_Get( "dmflags", "0", /*CVAR_SERVERINFO*/ 0 );
	Cvar_Get( "fraglimit", "0", /*CVAR_SERVERINFO*/ 0 );
	Cvar_Get( "timelimit", "0", CVAR_SERVERINFO );

	// Rafael gameskill
//	sv_gameskill = Cvar_Get ("g_gameskill", "3", CVAR_SERVERINFO | CVAR_LATCH );
	// done

	Cvar_Get( "sv_keywords", "", CVAR_SERVERINFO );
	//Cvar_Get( "protocol", va( "%i", PROTOCOL_VERSION ), CVAR_SERVERINFO | CVAR_ROM );
	sv_mapname = Cvar_Get( "mapname", "nomap", CVAR_SERVERINFO | CVAR_ROM );
	Cvar_SetDescription( sv_mapname, "Display the name of the current map being used on a server" );
	sv_privateClients = Cvar_Get( "sv_privateClients", "0", CVAR_SERVERINFO );
	Cvar_CheckRange( sv_privateClients, "0", va( "%i", MAX_CLIENTS-1 ), CV_INTEGER );
	Cvar_SetDescription( sv_privateClients, "The number of spots, out of sv_maxclients, reserved for players with the server password (sv_privatePassword)" );
	sv_hostname = Cvar_Get( "sv_hostname", "ETHost", CVAR_SERVERINFO | CVAR_ARCHIVE );
	Cvar_SetDescription( sv_hostname, "Sets the name of the server" );
	sv_maxclients = Cvar_Get( "sv_maxclients", "20", CVAR_SERVERINFO | CVAR_LATCH );               // NERVE - SMF - changed to 20 from 8
	Cvar_CheckRange( sv_maxclients, "1", XSTRING(MAX_CLIENTS), CV_INTEGER );
	Cvar_SetDescription( sv_maxclients, "Maximum number of people allowed to join the server dedicated server" );

	sv_maxclientsPerIP = Cvar_Get( "sv_maxclientsPerIP", "3", CVAR_ARCHIVE );
	Cvar_CheckRange( sv_maxclientsPerIP, "1", NULL, CV_INTEGER );
	Cvar_SetDescription( sv_maxclientsPerIP, "Limits number of simultaneous connections from the same IP address" );

	sv_clientTLD = Cvar_Get( "sv_clientTLD", "0", CVAR_ARCHIVE_ND );
	Cvar_CheckRange( sv_clientTLD, NULL, NULL, CV_INTEGER );
	Cvar_SetDescription( sv_clientTLD, "Client country detection code" );

	sv_minRate = Cvar_Get( "sv_minRate", "0", CVAR_ARCHIVE_ND | CVAR_SERVERINFO );
	Cvar_SetDescription( sv_minRate, "Minimum server bandwidth (in bit per second) a client can use" );
	sv_maxRate = Cvar_Get( "sv_maxRate", "0", CVAR_ARCHIVE_ND | CVAR_SERVERINFO );
	Cvar_SetDescription( sv_maxRate, "Maximum server bandwidth (in bit per second) a client can use" );
	sv_floodProtect = Cvar_Get( "sv_floodProtect", "1", CVAR_ARCHIVE | CVAR_SERVERINFO );
	Cvar_SetDescription( sv_floodProtect, "Toggle server flood protection to keep players from bringing the server down" );
	sv_friendlyFire = Cvar_Get( "g_friendlyFire", "1", CVAR_SERVERINFO | CVAR_ARCHIVE );           // NERVE - SMF
	sv_maxlives = Cvar_Get( "g_maxlives", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_SERVERINFO );      // NERVE - SMF
	sv_needpass = Cvar_Get( "g_needpass", "0", CVAR_SERVERINFO | CVAR_ROM );

	// systeminfo
	//bani - added cvar_t for sv_cheats so server engine can reference it
	sv_cheats = Cvar_Get( "sv_cheats", "1", CVAR_SYSTEMINFO | CVAR_ROM );
	Cvar_SetDescription( sv_cheats, "Enable cheating commands (server side or demo playback only)" );
	sv_serverid = Cvar_Get( "sv_serverid", "0", CVAR_SYSTEMINFO | CVAR_ROM );
	sv_pure = Cvar_Get( "sv_pure", "1", CVAR_SYSTEMINFO | CVAR_LATCH );
	Cvar_SetDescription( sv_pure, "Pure server client file checksum verification:\n"
		" 0 - disabled\n"
		" 1 - enabled" );
	Cvar_Get( "sv_paks", "", CVAR_SYSTEMINFO | CVAR_ROM );
	Cvar_Get( "sv_pakNames", "", CVAR_SYSTEMINFO | CVAR_ROM );
	Cvar_Get( "sv_referencedPaks", "", CVAR_SYSTEMINFO | CVAR_ROM );
	sv_referencedPakNames = Cvar_Get( "sv_referencedPakNames", "", CVAR_SYSTEMINFO | CVAR_ROM );
	Cvar_SetDescription( sv_referencedPakNames, "Variable holds a list of all the pk3 files the server loaded data from" );
	Cvar_Get( "sv_currentPak", "0", CVAR_SERVERINFO | CVAR_ROM );
	Cvar_Get( "sv_clientPaks", "0 0", CVAR_SERVERINFO | CVAR_ROM );

	// server vars
	sv_rconPassword = Cvar_Get( "rconPassword", "", CVAR_TEMP );
	Cvar_SetDescription( sv_rconPassword, "Password for remote server commands" );
	sv_privatePassword = Cvar_Get( "sv_privatePassword", "", CVAR_TEMP );
	Cvar_SetDescription( sv_privatePassword, "Set password for private clients to login with" );
	sv_fps = Cvar_Get( "sv_fps", "20", CVAR_TEMP );
	Cvar_CheckRange( sv_fps, "10", "125", CV_INTEGER );
	Cvar_SetDescription( sv_fps, "Frames per second the server runs at" );
	sv_timeout = Cvar_Get( "sv_timeout", "240", CVAR_TEMP );
	Cvar_CheckRange( sv_timeout, "4", NULL, CV_INTEGER );
	Cvar_SetDescription( sv_timeout, "Seconds without any message before automatic client disconnect" );
	sv_zombietime = Cvar_Get( "sv_zombietime", "2", CVAR_TEMP );
	Cvar_CheckRange( sv_zombietime, "1", NULL, CV_INTEGER );
	Cvar_SetDescription( sv_zombietime, "Seconds to sink messages after disconnect" );
	Cvar_Get ("nextmap", "", CVAR_TEMP );

	sv_allowDownload = Cvar_Get( "sv_allowDownload", "1", CVAR_ARCHIVE );
	Cvar_SetDescription( sv_allowDownload, "Toggle the ability for clients to download files maps etc. from server" );

	// moved to Com_Init()
	sv_master[0] = Cvar_Get( "sv_master1", MASTER_SERVER_NAME, CVAR_ARCHIVE_ND );
	sv_master[1] = Cvar_Get( "sv_master2", "master.etlegacy.com", CVAR_ARCHIVE_ND );
	sv_master[2] = Cvar_Get( "sv_master3", "", CVAR_ARCHIVE_ND );
	sv_master[3] = Cvar_Get( "sv_master4", "", CVAR_ARCHIVE_ND );
	sv_master[4] = Cvar_Get( "sv_master5", "", CVAR_ARCHIVE_ND );
	sv_reconnectlimit = Cvar_Get( "sv_reconnectlimit", "3", 0 );
	Cvar_CheckRange( sv_reconnectlimit, "0", "12", CV_INTEGER );
	Cvar_SetDescription( sv_reconnectlimit, "Number of seconds a disconnected client should wait before next reconnect" );

	sv_tempbanmessage = Cvar_Get( "sv_tempbanmessage", "You have been kicked and are temporarily banned from joining this server.", 0 );
	sv_padPackets = Cvar_Get( "sv_padPackets", "0", CVAR_DEVELOPER );
	Cvar_SetDescription( sv_padPackets, "Adds padding bytes to network packets for rate debugging" );
	sv_killserver = Cvar_Get( "sv_killserver", "0", 0 );
	Cvar_SetDescription( sv_killserver, "Internal flag to manage server state" );
	sv_mapChecksum = Cvar_Get( "sv_mapChecksum", "", CVAR_ROM );
	Cvar_SetDescription( sv_mapChecksum, "Checksum of the currently loaded map" );
	sv_lanForceRate = Cvar_Get( "sv_lanForceRate", "1", CVAR_ARCHIVE_ND );
	Cvar_SetDescription( sv_lanForceRate, "Forces LAN clients to the maximum rate instead of accepting client setting" );

	sv_onlyVisibleClients = Cvar_Get( "sv_onlyVisibleClients", "0", 0 );       // DHM - Nerve

	sv_showAverageBPS = Cvar_Get( "sv_showAverageBPS", "0", 0 );           // NERVE - SMF - net debugging

	// NERVE - SMF - create user set cvars
	Cvar_Get( "g_userTimeLimit", "0", 0 );
	Cvar_Get( "g_userAlliedRespawnTime", "0", 0 );
	Cvar_Get( "g_userAxisRespawnTime", "0", 0 );
	Cvar_Get( "g_altStopwatchMode", "0", CVAR_ARCHIVE );
	Cvar_Get( "g_minGameClients", "8", CVAR_SERVERINFO );
	Cvar_Get( "g_complaintlimit", "6", CVAR_ARCHIVE );
	Cvar_Get( "gamestate", "-1", CVAR_WOLFINFO | CVAR_ROM );
	Cvar_Get( "g_currentRound", "0", CVAR_WOLFINFO );
	Cvar_Get( "g_nextTimeLimit", "0", CVAR_WOLFINFO );
	// -NERVE - SMF

	// TTimo - some UI additions
	// NOTE: sucks to have this hardcoded really, I suppose this should be in UI
	Cvar_Get( "g_axismaxlives", "0", 0 );
	Cvar_Get( "g_alliedmaxlives", "0", 0 );
	Cvar_Get( "g_fastres", "0", CVAR_ARCHIVE );
	Cvar_Get( "g_fastResMsec", "1000", CVAR_ARCHIVE );

	// ATVI Tracker Wolfenstein Misc #273
	Cvar_Get( "voteFlags", "0", CVAR_ROM | CVAR_SERVERINFO );

	// ATVI Tracker Wolfenstein Misc #263
	Cvar_Get( "g_antilag", "1", CVAR_ARCHIVE | CVAR_SERVERINFO );

	sv_gameType = Cvar_Get( "g_gametype", va( "%i", com_gameInfo.defaultGameType ), CVAR_SERVERINFO | CVAR_LATCH );

	sv_dl_maxRate = Cvar_Get( "sv_dl_maxRate", "2000000", CVAR_ARCHIVE );
	Cvar_CheckRange( sv_dl_maxRate, "0", NULL, CV_INTEGER );
	Cvar_SetDescription( sv_dl_maxRate, "Bandwidth allotted to PK3 file downloads via UDP, in byte/s" );

	sv_wwwDownload = Cvar_Get( "sv_wwwDownload", "0", CVAR_ARCHIVE );
	sv_wwwBaseURL = Cvar_Get( "sv_wwwBaseURL", "", CVAR_ARCHIVE );
	sv_wwwDlDisconnected = Cvar_Get( "sv_wwwDlDisconnected", "0", CVAR_ARCHIVE );
	sv_wwwFallbackURL = Cvar_Get( "sv_wwwFallbackURL", "", CVAR_ARCHIVE );

	// fretn - note: redirecting of clients to other servers relies on this,
	// ET://someserver.com
	sv_fullmsg = Cvar_Get( "sv_fullmsg", "Server is full.", CVAR_ARCHIVE );

#ifdef USE_BANS
	sv_banFile = Cvar_Get("sv_banFile", "serverbans.dat", CVAR_ARCHIVE);
	Cvar_SetDescription( sv_banFile, "Name of the file that is used for storing the server bans" );
#endif

	sv_levelTimeReset = Cvar_Get( "sv_levelTimeReset", "0", CVAR_ARCHIVE_ND );

	Cvar_SetDescription( sv_levelTimeReset, "Whether to reset leveltime after new map loads\n"
		" Note: when enabled - fixes gfx for clients affected by \"frameloss\" bug,\n"
		" however it may be necessary to disable in case of troubles with custom mods similar to ettv" );

	sv_filter = Cvar_Get( "sv_filter", "filter.txt", CVAR_ARCHIVE );
	Cvar_SetDescription( sv_filter, "Cvar that point to filter file, if it is "" then filtering will be disabled" );

	sv_userinfoFloodProtect = Cvar_Get( "sv_userinfoFloodProtect", "1", CVAR_ARCHIVE_ND );
	Cvar_SetDescription( sv_userinfoFloodProtect, "Toggle server flood protection of userinfo changes to keep players from bringing the server down" );

	sv_filterCommands = Cvar_Get( "sv_filterCommands", "1", CVAR_ARCHIVE );
	Cvar_CheckRange( sv_filterCommands, "0", "2", CV_INTEGER );
	Cvar_SetDescription( sv_filterCommands, "Controls whether reliable commands are filtered for security with old game modules\n"
		" 0 - disabled\n"
		" 1 - filter newlines and carriage returns in reliable commands.\n"
		" 2 - also filter semicolons" );

	sv_paksOverflowMode = Cvar_Get( "sv_paksOverflowMode", "1", CVAR_ARCHIVE );
	Cvar_CheckRange( sv_paksOverflowMode, "1", "2", CV_INTEGER );
	Cvar_SetDescription( sv_paksOverflowMode, "Which style of overflow protection is used when systeminfo is too large to accomodate pak list\n"
		" 1 - refuse to set sv_paks and sv_pakNames (Quake3e+ETe style).\n"
		" 2 - set sv_paks and sv_pakNames to referenced if won't still overflow (ET:Legacy style)" );

#ifdef USE_BANS	
	// Load saved bans
	Cbuf_AddText("rehashbans\n");
#endif

	// track group cvar changes
	Cvar_SetGroup( sv_lanForceRate, CVG_SERVER );
	Cvar_SetGroup( sv_minRate, CVG_SERVER );
	Cvar_SetGroup( sv_maxRate, CVG_SERVER );
	Cvar_SetGroup( sv_fps, CVG_SERVER );

	// force initial check
	SV_TrackCvarChanges();

	SV_InitChallenger();
	svs.serverLoad = -1;
}


/*
==================
SV_FinalCommand

Used by SV_Shutdown to send a final message to all
connected clients before the server goes down.  The messages are sent immediately,
not just stuck on the outgoing message list, because the server is going
to totally exit after returning from this function.
==================
*/
void SV_FinalCommand( const char *cmd, qboolean disconnect ) {
	int i, j;
	client_t *cl;

	// send it twice, ignoring rate
	for ( j = 0 ; j < 2 ; j++ ) {
		for ( i = 0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++ ) {
			if ( cl->state >= CS_CONNECTED ) {
				// don't send a disconnect to a local client
				if ( cl->netchan.remoteAddress.type != NA_LOOPBACK ) {
					//%	SV_SendServerCommand( cl, "print \"%s\"", message );
					SV_SendServerCommand( cl, "%s", cmd );
					// ydnar: added this so map changes can use this functionality
					if ( disconnect ) {
						SV_SendServerCommand( cl, "disconnect" );
					}
				}
				// force a snapshot to be sent
				cl->lastSnapshotTime = svs.time - 9999; // generate a snapshot immediately
				//cl->state = CS_ZOMBIE; // skip delta generation
				SV_SendClientSnapshot( cl );
			}
		}
	}
}


/*
================
SV_Shutdown

Called when each game quits,
before Sys_Quit or Sys_Error
================
*/
void SV_Shutdown( const char *finalmsg ) {
	if ( !com_sv_running || !com_sv_running->integer ) {
		return;
	}

	Com_Printf( "----- Server Shutdown (%s) -----\n", finalmsg );

#ifdef USE_IPV6
	NET_LeaveMulticast6();
#endif

	if ( svs.clients && !com_errorEntered ) {
		SV_FinalCommand( va( "print \"%s\"", finalmsg ), qtrue );
	}

	SV_RemoveOperatorCommands();
	SV_MasterShutdown();
	SV_ShutdownGameProgs();
	SV_InitChallenger();

	// free current level
	SV_ClearServer();

	SV_FreeIP4DB();

	// free server static data
	if ( svs.clients ) {
		int index;

		for ( index = 0; index < sv_maxclients->integer; index++ )
			SV_FreeClient( &svs.clients[ index ] );
		
#ifdef USE_CLIENTS_ZONE
		Z_Free( svs.clients );
#else
		free( svs.clients );    // RF, avoid trying to allocate large chunk on a fragmented zone
#endif
	}
	Com_Memset( &svs, 0, sizeof( svs ) );
	sv.time = 0;
	svs.serverLoad = -1;

	Cvar_Set( "sv_running", "0" );

	// allow setting timescale 0 for demo playback
	Cvar_CheckRange( com_timescale, "0", "100", CV_FLOAT );

	Com_Printf( "---------------------------\n" );

#ifndef DEDICATED
	// disconnect any local clients
	if ( sv_killserver->integer != 2 )
		CL_Disconnect( qfalse );
#endif

	// clean some server cvars
	Cvar_Set( "sv_referencedPaks", "" );
	Cvar_Set( "sv_referencedPakNames", "" );
	Cvar_Set( "sv_mapChecksum", "" );
	Cvar_Set( "sv_serverid", "0" );
	Cvar_Set( "sv_paks", "" );
	Cvar_Set( "sv_pakNames", "" );
	Cvar_Set( "sv_currentPak", "0" );
	Cvar_Set( "sv_clientPaks", "0 0" );

	Sys_SetStatus( "Server is not running" );
}
