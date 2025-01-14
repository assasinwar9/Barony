/*-------------------------------------------------------------------------------

	BARONY
	File: init_game.cpp
	Desc: contains game specific initialization code that shouldn't be
	seen in the editor.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "draw.hpp"
#include "files.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "interface/interface.hpp"
#include "messages.hpp"
#include "book.hpp"
#include "sound.hpp"
#include "shops.hpp"
#include "scores.hpp"
#include "magic/magic.hpp"
#include "monster.hpp"
#include "net.hpp"
#ifdef STEAMWORKS
#include <steam/steam_api.h>
#include "steam.hpp"
#endif
#include "menu.hpp"
#include "paths.hpp"
#include "player.hpp"
#include "cppfuncs.hpp"
#include "Directory.hpp"
#include "mod_tools.hpp"

/*-------------------------------------------------------------------------------

	initGame

	initializes certain game specific resources

-------------------------------------------------------------------------------*/

#define _LOADSTR1 language[746]
#define _LOADSTR2 language[747]
#define _LOADSTR3 language[748]
#define _LOADSTR4 language[749]

int initGame()
{
	int c, x;
	char name[32];
	File* fp;

	// setup some lists
	booksRead.first = NULL;
	booksRead.last = NULL;
	lobbyChatboxMessages.first = NULL;
	lobbyChatboxMessages.last = NULL;

	// steam stuff
#ifdef STEAMWORKS
	cpp_SteamServerWrapper_Instantiate(); //TODO: Remove these wrappers.
	cpp_SteamServerClientWrapper_Instantiate();

	cpp_SteamServerClientWrapper_OnP2PSessionRequest = &steam_OnP2PSessionRequest;
	//cpp_SteamServerClientWrapper_OnGameOverlayActivated = &steam_OnGameOverlayActivated;
	cpp_SteamServerClientWrapper_OnLobbyCreated = &steam_OnLobbyCreated;
	cpp_SteamServerClientWrapper_OnGameJoinRequested = &steam_OnGameJoinRequested;
	cpp_SteamServerClientWrapper_OnLobbyEntered = &steam_OnLobbyEntered;
	cpp_SteamServerClientWrapper_GameServerPingOnServerResponded = &steam_GameServerPingOnServerResponded;
	cpp_SteamServerClientWrapper_OnLobbyMatchListCallback = &steam_OnLobbyMatchListCallback;
	cpp_SteamServerClientWrapper_OnP2PSessionConnectFail = &steam_OnP2PSessionConnectFail;
	cpp_SteamServerClientWrapper_OnLobbyDataUpdate = &steam_OnLobbyDataUpdatedCallback;
 #ifdef USE_EOS
	cpp_SteamServerClientWrapper_OnRequestEncryptedAppTicket = &steam_OnRequestEncryptedAppTicket;
 #endif //USE_EOS
#endif

	// print a loading message
	drawClearBuffers();
	int w, h;
	getSizeOfText(ttf16, _LOADSTR1, &w, &h);
	ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, _LOADSTR1);

	GO_SwapBuffers(screen);

	initGameControllers();

	// load achievement images
	Directory achievementsDir("images/achievements");
	for (auto& item : achievementsDir.list)
	{
		std::string fullPath = achievementsDir.path + std::string("/") + item;
		char* name = const_cast<char*>(fullPath.c_str()); // <- evil
		achievementImages.emplace(std::make_pair(item, loadImage(name)));
	}

	// load model offsets
	printlog( "loading model offsets...\n");
	for ( c = 1; c < NUMMONSTERS; c++ )
	{
		// initialize all offsets to zero
		for ( x = 0; x < 20; x++ )
		{
			limbs[c][x][0] = 0;
			limbs[c][x][1] = 0;
			limbs[c][x][2] = 0;
		}

		// open file
		char filename[256];
		strcpy(filename, "models/creatures/");
		strcat(filename, monstertypename[c]);
		strcat(filename, "/limbs.txt");
		if ( (fp = openDataFile(filename, "r")) == NULL )
		{
			continue;
		}

		// read file
		int line;
		for ( line = 1; !fp->eof(); line++ )
		{
			char data[256];
			int limb = 20;
			int dummy;

			// read line from file
			fp->gets( data, 256 );

			// skip blank and comment lines
			if ( data[0] == '\n' || data[0] == '\r' || data[0] == '#' )
			{
				continue;
			}

			// process line
			if ( sscanf( data, "%d", &limb ) != 1 || limb >= 20 || limb < 0 )
			{
				printlog( "warning: syntax error in '%s':%d\n invalid limb index!\n", filename, line);
				continue;
			}
			if ( sscanf( data, "%d %f %f %f\n", &dummy, &limbs[c][limb][0], &limbs[c][limb][1], &limbs[c][limb][2] ) != 4 )
			{
				printlog( "warning: syntax error in '%s':%d\n invalid limb offsets!\n", filename, line);
				continue;
			}
		}

		// close file
		FileIO::close(fp);
	}

	// print a loading message
	drawClearBuffers();
	getSizeOfText(ttf16, _LOADSTR2, &w, &h);
	ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, _LOADSTR2);

	GO_SwapBuffers(screen);

	int newItems = 0;

	// load item types
	printlog( "loading items...\n");
	std::string itemsDirectory = PHYSFS_getRealDir("items/items.txt");
	itemsDirectory.append(PHYSFS_getDirSeparator()).append("items/items.txt");
	fp = openDataFile(itemsDirectory.c_str(), "r");
	for ( c = 0; !fp->eof(); ++c )
	{
		if ( c > SPELLBOOK_DETECT_FOOD )
		{
			newItems = c - SPELLBOOK_DETECT_FOOD - 1;
			items[c].name_identified = language[3500 + newItems * 2];
			items[c].name_unidentified = language[3501 + newItems * 2];
		}
		else if ( c > ARTIFACT_BOW )
		{
			newItems = c - ARTIFACT_BOW - 1;
			items[c].name_identified = language[2200 + newItems * 2];
			items[c].name_unidentified = language[2201 + newItems * 2];
		}
		else
		{
			items[c].name_identified = language[1545 + c * 2];
			items[c].name_unidentified = language[1546 + c * 2];
		}
		items[c].index = fp->geti();
		items[c].fpindex = fp->geti();
		items[c].variations = fp->geti();
		fp->gets2(name, 32);
		if ( !strcmp(name, "WEAPON") )
		{
			items[c].category = WEAPON;
		}
		else if ( !strcmp(name, "ARMOR") )
		{
			items[c].category = ARMOR;
		}
		else if ( !strcmp(name, "AMULET") )
		{
			items[c].category = AMULET;
		}
		else if ( !strcmp(name, "POTION") )
		{
			items[c].category = POTION;
		}
		else if ( !strcmp(name, "SCROLL") )
		{
			items[c].category = SCROLL;
		}
		else if ( !strcmp(name, "MAGICSTAFF") )
		{
			items[c].category = MAGICSTAFF;
		}
		else if ( !strcmp(name, "RING") )
		{
			items[c].category = RING;
		}
		else if ( !strcmp(name, "SPELLBOOK") )
		{
			items[c].category = SPELLBOOK;
		}
		else if ( !strcmp(name, "TOOL") )
		{
			items[c].category = TOOL;
		}
		else if ( !strcmp(name, "FOOD") )
		{
			items[c].category = FOOD;
		}
		else if ( !strcmp(name, "BOOK") )
		{
			items[c].category = BOOK;
		}
		else if ( !strcmp(name, "THROWN") )
		{
			items[c].category = THROWN;
		}
		else if ( !strcmp(name, "SPELL_CAT") )
		{
			items[c].category = SPELL_CAT;
		}
		else
		{
			items[c].category = GEM;
		}
		items[c].weight = fp->geti();
		items[c].value = fp->geti();
		items[c].images.first = NULL;
		items[c].images.last = NULL;
		while ( 1 )
		{
			string_t* string = (string_t*) malloc(sizeof(string_t));
			string->data = (char*) malloc(sizeof(char) * 64);
			string->lines = 1;

			node_t* node = list_AddNodeLast(&items[c].images);
			node->element = string;
			node->deconstructor = &stringDeconstructor;
			node->size = sizeof(string_t);
			string->node = node;

			x = 0;
			bool fileend = false;
			while ( (string->data[x] = fp->getc()) != '\n' )
			{
				if ( fp->eof() )
				{
					fileend = true;
					break;
				}
				x++;
			}
			if ( x == 0 || fileend )
			{
				list_RemoveNode(node);
				break;
			}
			string->data[x] = 0;
		}
	}
	for ( c = 0; c < NUMITEMS; c++ )
	{
		items[c].surfaces.first = NULL;
		items[c].surfaces.last = NULL;
		for ( x = 0; x < list_Size(&items[c].images); x++ )
		{
			SDL_Surface** surface = (SDL_Surface**) malloc(sizeof(SDL_Surface*));
			node_t* node = list_AddNodeLast(&items[c].surfaces);
			node->element = surface;
			node->deconstructor = &defaultDeconstructor;
			node->size = sizeof(SDL_Surface*);

			node_t* node2 = list_Node(&items[c].images, x);
			string_t* string = (string_t*)node2->element;
			std::string itemImgDir;
			if ( PHYSFS_getRealDir(string->data) != NULL )
			{
				itemImgDir = PHYSFS_getRealDir(string->data);
				itemImgDir.append(PHYSFS_getDirSeparator()).append(string->data);
			}
			else
			{
				itemImgDir = string->data;
			}
			char imgFileChar[256];
			strncpy(imgFileChar, itemImgDir.c_str(), 255);
			*surface = loadImage(imgFileChar);
		}
	}
	FileIO::close(fp);
	createBooks();
	setupSpells();

#ifdef NINTENDO
	std::string maleNames, femaleNames;
	maleNames = BASE_DATA_DIR + std::string("/") + PLAYERNAMES_MALE_FILE;
	femaleNames = BASE_DATA_DIR + std::string("/") + PLAYERNAMES_FEMALE_FILE;
	randomPlayerNamesMale = getLinesFromDataFile(maleNames);
	randomPlayerNamesFemale = getLinesFromDataFile(femaleNames);
#else // NINTENDO
	randomPlayerNamesMale = getLinesFromDataFile(PLAYERNAMES_MALE_FILE);
	randomPlayerNamesFemale = getLinesFromDataFile(PLAYERNAMES_FEMALE_FILE);
#endif // !NINTENDO

	loadItemLists();

#if defined(USE_EOS) || defined(STEAMWORKS)
#else
 #ifdef NINTENDO
	#error "No DLC support on SWITCH yet :(" //TODO: Resolve this.
 #else // NINTENDO
	if ( PHYSFS_getRealDir("mythsandoutcasts.key") != NULL )
	{
		std::string serial = PHYSFS_getRealDir("mythsandoutcasts.key");
		serial.append(PHYSFS_getDirSeparator()).append("mythsandoutcasts.key");
		// open the serial file
		File* fp = nullptr;
		if ( (fp = FileIO::open(serial.c_str(), "rb")) != NULL )
		{
			char buf[64];
			size_t len = fp->read(&buf, sizeof(char), 32);
			buf[len] = '\0';
			serial = buf;
			// compute hash
			size_t DLCHash = serialHash(serial);
			if ( DLCHash == 144425 )
			{
				printlog("[LICENSE]: Myths and Outcasts DLC license key found.");
				enabledDLCPack1 = true;
			}
			else
			{
				printlog("[LICENSE]: DLC license key invalid.");
			}
			FileIO::close(fp);
		}
	}
	if ( PHYSFS_getRealDir("legendsandpariahs.key") != NULL ) //TODO: NX PORT: Update for the Switch?
	{
		std::string serial = PHYSFS_getRealDir("legendsandpariahs.key");
		serial.append(PHYSFS_getDirSeparator()).append("legendsandpariahs.key");
		// open the serial file
		File* fp = nullptr;
		if ( (fp = FileIO::open(serial.c_str(), "rb")) != NULL )
		{
			char buf[64];
			size_t len = fp->read(&buf, sizeof(char), 32);
			buf[len] = '\0';
			serial = buf;
			// compute hash
			size_t DLCHash = serialHash(serial);
			if ( DLCHash == 135398 )
			{
				printlog("[LICENSE]: Legends and Pariahs DLC license key found.");
				enabledDLCPack2 = true;
			}
			else
			{
				printlog("[LICENSE]: DLC license key invalid.");
			}
			FileIO::close(fp);
		}
	}
 #endif // !NINTENDO
#endif

	// print a loading message
	drawClearBuffers();
	getSizeOfText(ttf16, _LOADSTR3, &w, &h);
	ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, _LOADSTR3);

	GO_SwapBuffers(screen);

#ifdef USE_FMOD
	FMOD_ChannelGroup_SetVolume(music_group, musvolume / 128.f);
#elif defined USE_OPENAL
	OPENAL_ChannelGroup_SetVolume(music_group, musvolume / 128.f);
#endif
	removedEntities.first = NULL;
	removedEntities.last = NULL;
	safePacketsSent.first = NULL;
	safePacketsSent.last = NULL;
	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		safePacketsReceivedMap[c].clear();
	}
	topscores.first = NULL;
	topscores.last = NULL;
	topscoresMultiplayer.first = NULL;
	topscoresMultiplayer.last = NULL;
	messages.first = NULL;
	messages.last = NULL;
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		chestInv[i].first = NULL;
		chestInv[i].last = NULL;
		for ( c = 0; c < kNumChestItemsToDisplay; c++ )
		{
			invitemschest[i][c] = NULL;
		}
	}
	command_history.first = NULL;
	command_history.last = NULL;
	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		openedChest[c] = NULL;
	}
	mousex = xres / 2;
	mousey = yres / 2;

	// default player stats
	for (c = 0; c < MAXPLAYERS; c++)
	{
		players[c] = new Player(c, true);
		players[c]->init();
		// Stat set to 0 as monster type not needed, values will be filled with default, then overwritten by savegame or the charclass.cpp file
		stats[c] = new Stat(0);
		if (c > 0)
		{
			client_disconnected[c] = true;
		}
		players[c]->entity = nullptr;
		stats[c]->sex = static_cast<sex_t>(0);
		stats[c]->appearance = 0;
		strcpy(stats[c]->name, "");
		stats[c]->type = HUMAN;
		stats[c]->playerRace = RACE_HUMAN;
		stats[c]->FOLLOWERS.first = nullptr;
		stats[c]->FOLLOWERS.last = nullptr;
		stats[c]->inventory.first = nullptr;
		stats[c]->inventory.last = nullptr;
		stats[c]->clearStats();
		entitiesToDelete[c].first = nullptr;
		entitiesToDelete[c].last = nullptr;
		if (c == 0)
		{
			initClass(c);
		}
		GenericGUI[c].setPlayer(c);
		FollowerMenu[c].setPlayer(c);
		cameras[c].winx = 0;
		cameras[c].winy = 0;
		cameras[c].winw = xres;
		cameras[c].winh = yres;
		cast_animation[c].player = c;
	}

	// load music
#ifdef SOUND
#ifdef USE_OPENAL
#define FMOD_ChannelGroup_SetVolume OPENAL_ChannelGroup_SetVolume
#define fmod_system 0
#define FMOD_SOFTWARE 0
#define FMOD_System_CreateStream(A, B, C, D, E) OPENAL_CreateStreamSound(B, E)
#define FMOD_SOUND OPENAL_BUFFER
int fmod_result;
#endif

	FMOD_ChannelGroup_SetVolume(music_group, musvolume / 128.f);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/introduction.ogg", FMOD_SOFTWARE, NULL, &introductionmusic);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/intermission.ogg", FMOD_SOFTWARE, NULL, &intermissionmusic);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/minetown.ogg", FMOD_SOFTWARE, NULL, &minetownmusic);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/splash.ogg", FMOD_SOFTWARE, NULL, &splashmusic);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/library.ogg", FMOD_SOFTWARE, NULL, &librarymusic);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/shop.ogg", FMOD_SOFTWARE, NULL, &shopmusic);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/herxboss.ogg", FMOD_SOFTWARE, NULL, &herxmusic);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/temple.ogg", FMOD_SOFTWARE, NULL, &templemusic);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/endgame.ogg", FMOD_SOFTWARE, NULL, &endgamemusic);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/escape.ogg", FMOD_SOFTWARE, NULL, &escapemusic);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/devil.ogg", FMOD_SOFTWARE, NULL, &devilmusic);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/sanctum.ogg", FMOD_SOFTWARE, NULL, &sanctummusic);
	fmod_result = FMOD_System_CreateStream(fmod_system, "music/tutorial.ogg", FMOD_SOFTWARE, NULL, &tutorialmusic);
	if ( PHYSFS_getRealDir("music/gnomishmines.ogg") != NULL )
	{
		fmod_result = FMOD_System_CreateStream(fmod_system, "music/gnomishmines.ogg", FMOD_SOFTWARE, NULL, &gnomishminesmusic);
	}
	if ( PHYSFS_getRealDir("music/greatcastle.ogg") != NULL )
	{
		fmod_result = FMOD_System_CreateStream(fmod_system, "music/greatcastle.ogg", FMOD_SOFTWARE, NULL, &greatcastlemusic);
	}
	if ( PHYSFS_getRealDir("music/sokoban.ogg") != NULL )
	{
		fmod_result = FMOD_System_CreateStream(fmod_system, "music/sokoban.ogg", FMOD_SOFTWARE, NULL, &sokobanmusic);
	}
	if ( PHYSFS_getRealDir("music/caveslair.ogg") != NULL )
	{
		fmod_result = FMOD_System_CreateStream(fmod_system, "music/caveslair.ogg", FMOD_SOFTWARE, NULL, &caveslairmusic);
	}
	if ( PHYSFS_getRealDir("music/bramscastle.ogg") != NULL )
	{
		fmod_result = FMOD_System_CreateStream(fmod_system, "music/bramscastle.ogg", FMOD_SOFTWARE, NULL, &bramscastlemusic);
	}
	if ( PHYSFS_getRealDir("music/hamlet.ogg") != NULL )
	{
		fmod_result = FMOD_System_CreateStream(fmod_system, "music/hamlet.ogg", FMOD_SOFTWARE, NULL, &hamletmusic);
	}
	//fmod_result = FMOD_System_CreateStream(fmod_system, "music/story.ogg", FMOD_SOFTWARE, NULL, &storymusic);

	if ( NUMMINESMUSIC > 0 )
	{
		minesmusic = (FMOD_SOUND**) malloc(sizeof(FMOD_SOUND*)*NUMMINESMUSIC);
		for ( c = 0; c < NUMMINESMUSIC; c++ )
		{
			snprintf(tempstr, 1000, "music/mines%02d.ogg", c);
			fmod_result = FMOD_System_CreateStream(fmod_system, tempstr, FMOD_SOFTWARE, NULL, &minesmusic[c]);
		}
	}
	if ( NUMSWAMPMUSIC > 0 )
	{
		swampmusic = (FMOD_SOUND**) malloc(sizeof(FMOD_SOUND*)*NUMSWAMPMUSIC);
		for ( c = 0; c < NUMSWAMPMUSIC; c++ )
		{
			snprintf(tempstr, 1000, "music/swamp%02d.ogg", c);
			fmod_result = FMOD_System_CreateStream(fmod_system, tempstr, FMOD_SOFTWARE, NULL, &swampmusic[c]);
		}
	}
	if ( NUMLABYRINTHMUSIC > 0 )
	{
		labyrinthmusic = (FMOD_SOUND**) malloc(sizeof(FMOD_SOUND*)*NUMLABYRINTHMUSIC);
		for ( c = 0; c < NUMLABYRINTHMUSIC; c++ )
		{
			snprintf(tempstr, 1000, "music/labyrinth%02d.ogg", c);
			fmod_result = FMOD_System_CreateStream(fmod_system, tempstr, FMOD_SOFTWARE, NULL, &labyrinthmusic[c]);
		}
	}
	if ( NUMRUINSMUSIC > 0 )
	{
		ruinsmusic = (FMOD_SOUND**) malloc(sizeof(FMOD_SOUND*)*NUMRUINSMUSIC);
		for ( c = 0; c < NUMRUINSMUSIC; c++ )
		{
			snprintf(tempstr, 1000, "music/ruins%02d.ogg", c);
			fmod_result = FMOD_System_CreateStream(fmod_system, tempstr, FMOD_SOFTWARE, NULL, &ruinsmusic[c]);
		}
	}
	if ( NUMUNDERWORLDMUSIC > 0 )
	{
		underworldmusic = (FMOD_SOUND**) malloc(sizeof(FMOD_SOUND*)*NUMUNDERWORLDMUSIC);
		for ( c = 0; c < NUMUNDERWORLDMUSIC; c++ )
		{
			snprintf(tempstr, 1000, "music/underworld%02d.ogg", c);
			fmod_result = FMOD_System_CreateStream(fmod_system, tempstr, FMOD_SOFTWARE, NULL, &underworldmusic[c]);
		}
	}
	if ( NUMHELLMUSIC > 0 )
	{
		hellmusic = (FMOD_SOUND**) malloc(sizeof(FMOD_SOUND*)*NUMHELLMUSIC);
		for ( c = 0; c < NUMHELLMUSIC; c++ )
		{
			snprintf(tempstr, 1000, "music/hell%02d.ogg", c);
			fmod_result = FMOD_System_CreateStream(fmod_system, tempstr, FMOD_SOFTWARE, NULL, &hellmusic[c]);
		}
	}
	if ( NUMMINOTAURMUSIC > 0 )
	{
		minotaurmusic = (FMOD_SOUND**) malloc(sizeof(FMOD_SOUND*)*NUMMINOTAURMUSIC);
		for ( c = 0; c < NUMMINOTAURMUSIC; c++ )
		{
			snprintf(tempstr, 1000, "music/minotaur%02d.ogg", c);
			fmod_result = FMOD_System_CreateStream(fmod_system, tempstr, FMOD_SOFTWARE, NULL, &minotaurmusic[c]);
		}
	}
	if ( NUMCAVESMUSIC > 0 )
	{
		cavesmusic = (FMOD_SOUND**) malloc(sizeof(FMOD_SOUND*)*NUMCAVESMUSIC);
		for ( c = 0; c < NUMCAVESMUSIC; c++ )
		{
			snprintf(tempstr, 1000, "music/caves%02d.ogg", c);
			fmod_result = FMOD_System_CreateStream(fmod_system, tempstr, FMOD_SOFTWARE, NULL, &cavesmusic[c]);
		}
	}
	if ( NUMCITADELMUSIC > 0 )
	{
		citadelmusic = (FMOD_SOUND**)malloc(sizeof(FMOD_SOUND*)*NUMCITADELMUSIC);
		for ( c = 0; c < NUMCITADELMUSIC; c++ )
		{
			snprintf(tempstr, 1000, "music/citadel%02d.ogg", c);
			fmod_result = FMOD_System_CreateStream(fmod_system, tempstr, FMOD_SOFTWARE, NULL, &citadelmusic[c]);
		}
	}
	if ( NUMINTROMUSIC > 0 )
	{
		intromusic = (FMOD_SOUND**)malloc(sizeof(FMOD_SOUND*)*NUMINTROMUSIC);
		for ( c = 0; c < NUMINTROMUSIC; c++ )
		{
			if ( c == 0 )
			{
				strcpy(tempstr, "music/intro.ogg");
			}
			else
			{
				snprintf(tempstr, 1000, "music/intro%02d.ogg", c);
			}
			fmod_result = FMOD_System_CreateStream(fmod_system, tempstr, FMOD_SOFTWARE, NULL, &intromusic[c]);
		}
	}
#ifdef USE_OPENAL
#undef FMOD_ChannelGroup_SetVolume
#undef fmod_system
#undef FMOD_SOFTWARE
#undef FMOD_System_CreateStream
#undef FMOD_SOUND
#endif

#endif

	// print a loading message
	drawClearBuffers();
	getSizeOfText(ttf16, _LOADSTR4, &w, &h);
	ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, _LOADSTR4);

	GO_SwapBuffers(screen);

	// load extraneous game resources
	title_bmp = loadImage("images/system/title.png");
	logo_bmp = loadImage("images/system/logo.png");
	cursor_bmp = loadImage("images/system/cursor.png");
	cross_bmp = loadImage("images/system/cross.png");
	selected_cursor_bmp = loadImage("images/system/selectedcursor.png");
	controllerglyphs1_bmp = loadImage("images/system/glyphsheet_ns.png");
	skillIcons_bmp = loadImage("images/system/skillicons_sheet.png");

	loadAllScores(SCORESFILE);
	loadAllScores(SCORESFILE_MULTIPLAYER);
	gameModeManager.Tutorial.init();
	if (!loadInterfaceResources())
	{
		printlog("Failed to load interface resources.\n");
		return -1;
	}

	return 0;
}

/*-------------------------------------------------------------------------------

	deinitGame

	deinitializes certain game specific resources

-------------------------------------------------------------------------------*/

void deinitGame()
{
	int c, x;

	// send disconnect messages
	if ( multiplayer == CLIENT )
	{
		strcpy((char*)net_packet->data, "DISCONNECT");
		net_packet->data[10] = clientnum;
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 11;
		sendPacketSafe(net_sock, -1, net_packet, 0);
		printlog("disconnected from server.\n");
	}
	else if ( multiplayer == SERVER )
	{
		for ( x = 1; x < MAXPLAYERS; x++ )
		{
			if ( client_disconnected[x] == true )
			{
				continue;
			}
			strcpy((char*)net_packet->data, "DISCONNECT");
			net_packet->data[10] = clientnum;
			net_packet->address.host = net_clients[x - 1].host;
			net_packet->address.port = net_clients[x - 1].port;
			net_packet->len = 11;
			sendPacketSafe(net_sock, -1, net_packet, x - 1);

			stats[x]->freePlayerEquipment();
			client_disconnected[x] = true;
		}
	}

	// this short delay makes sure that the disconnect message gets out
	Uint32 timetoshutdown = SDL_GetTicks();
	while ( SDL_GetTicks() - timetoshutdown < 500 )
	{
		// handle network messages
		if ( multiplayer == CLIENT )
		{
			clientHandleMessages(fpsLimit);
		}
		else if ( multiplayer == SERVER )
		{
			serverHandleMessages(fpsLimit);
		}
		if ( !(SDL_GetTicks() % 25) && multiplayer )
		{
			int j = 0;
			node_t* node, *nextnode;
			for ( node = safePacketsSent.first; node != NULL; node = nextnode )
			{
				nextnode = node->next;

				packetsend_t* packet = (packetsend_t*)node->element;
				sendPacket(packet->sock, packet->channel, packet->packet, packet->hostnum);
				packet->tries++;
				if ( packet->tries >= MAXTRIES )
				{
					list_RemoveNode(node);
				}
				j++;
				if ( j >= MAXDELETES )
				{
					break;
				}
			}
		}
	}

	saveAllScores(SCORESFILE);
	saveAllScores(SCORESFILE_MULTIPLAYER);
	list_FreeAll(&topscores);
	list_FreeAll(&topscoresMultiplayer);
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		players[i]->messageZone.deleteAllNotificationMessages();
	}
	list_FreeAll(&removedEntities);
	if ( title_bmp != nullptr )
	{
		SDL_FreeSurface(title_bmp);
	}
	if ( logo_bmp != nullptr )
	{
		SDL_FreeSurface(logo_bmp);
	}
	if ( cursor_bmp != nullptr )
	{
		SDL_FreeSurface(cursor_bmp);
	}
	if ( cross_bmp != nullptr )
	{
		SDL_FreeSurface(cross_bmp);
	}
	if ( selected_cursor_bmp != nullptr )
	{
		SDL_FreeSurface(selected_cursor_bmp);
	}
	if ( controllerglyphs1_bmp != nullptr )
	{
		SDL_FreeSurface(controllerglyphs1_bmp);
	}
	if ( skillIcons_bmp != nullptr )
	{
		SDL_FreeSurface(skillIcons_bmp);
	}
	//if(sky_bmp!=NULL)
	//	SDL_FreeSurface(sky_bmp);
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		list_FreeAll(&chestInv[i]);
	}
	freeInterfaceResources();
	if ( books )
	{
		for ( c = 0; c < numbooks; c++ )
		{
			if ( books[c] )
			{
				if ( books[c]->name )
				{
					free(books[c]->name);
				}
				if ( books[c]->text )
				{
					free(books[c]->text);
				}
				if ( books[c]->bookgui_render_title )
				{
					free(books[c]->bookgui_render_title);
				}
				list_FreeAll(&books[c]->pages);
				free(books[c]);
			}
		}
		free(books);
	}
	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		players[c]->inventoryUI.appraisal.timer = 0;
		players[c]->inventoryUI.appraisal.current_item = 0;
		list_FreeAll(&stats[c]->inventory);
		list_FreeAll(&stats[c]->FOLLOWERS);
		if ( multiplayer == CLIENT )
		{
			if ( shopInv[c] )
			{
				list_FreeAll(shopInv[c]);
				free(shopInv[c]);
				shopInv[c] = NULL;
			}
		}
	}
	list_FreeAll(map.entities);
	if ( map.creatures )
	{
		list_FreeAll(map.creatures); //TODO: Need to do this?
	}
	if ( map.worldUI )
	{
		list_FreeAll(map.worldUI); //TODO: Need to do this?
	}
	list_FreeAll(&messages);
	if ( multiplayer == SINGLE )
	{
		list_FreeAll(&channeledSpells[0]);
	}
	else if ( multiplayer == CLIENT )
	{
		list_FreeAll(&channeledSpells[clientnum]);
	}
	else if ( multiplayer == SERVER )
	{
		for ( c = 0; c < MAXPLAYERS; ++c )
		{
			list_FreeAll(&channeledSpells[c]);
		}
	}

	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		list_FreeAll(&players[c]->magic.spellList);
	}
	list_FreeAll(&command_history);

	list_FreeAll(&safePacketsSent);
	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		safePacketsReceivedMap[c].clear();
	}
#ifdef SOUND
#ifdef USE_OPENAL
#define FMOD_Channel_Stop OPENAL_Channel_Stop
#define FMOD_Sound_Release OPENAL_Sound_Release
#endif
	if ( !no_sound )
	{
		FMOD_Channel_Stop(music_channel);
		FMOD_Channel_Stop(music_channel2);
		FMOD_Sound_Release(introductionmusic);
		FMOD_Sound_Release(intermissionmusic);
		FMOD_Sound_Release(minetownmusic);
		FMOD_Sound_Release(splashmusic);
		FMOD_Sound_Release(librarymusic);
		FMOD_Sound_Release(shopmusic);
		FMOD_Sound_Release(herxmusic);
		FMOD_Sound_Release(templemusic);
		FMOD_Sound_Release(endgamemusic);
		FMOD_Sound_Release(escapemusic);
		FMOD_Sound_Release(devilmusic);
		FMOD_Sound_Release(sanctummusic);
		FMOD_Sound_Release(gnomishminesmusic);
		FMOD_Sound_Release(greatcastlemusic);
		FMOD_Sound_Release(sokobanmusic);
		FMOD_Sound_Release(caveslairmusic);
		FMOD_Sound_Release(bramscastlemusic);
		FMOD_Sound_Release(hamletmusic);
		FMOD_Sound_Release(tutorialmusic);
		for ( c = 0; c < NUMMINESMUSIC; c++ )
		{
			FMOD_Sound_Release(minesmusic[c]);
		}
		if ( minesmusic )
		{
			free(minesmusic);
		}
		for ( c = 0; c < NUMSWAMPMUSIC; c++ )
		{
			FMOD_Sound_Release(swampmusic[c]);
		}
		if ( swampmusic )
		{
			free(swampmusic);
		}
		for ( c = 0; c < NUMLABYRINTHMUSIC; c++ )
		{
			FMOD_Sound_Release(labyrinthmusic[c]);
		}
		if ( labyrinthmusic )
		{
			free(labyrinthmusic);
		}
		for ( c = 0; c < NUMRUINSMUSIC; c++ )
		{
			FMOD_Sound_Release(ruinsmusic[c]);
		}
		if ( ruinsmusic )
		{
			free(ruinsmusic);
		}
		for ( c = 0; c < NUMUNDERWORLDMUSIC; c++ )
		{
			FMOD_Sound_Release(underworldmusic[c]);
		}
		if ( underworldmusic )
		{
			free(underworldmusic);
		}
		for ( c = 0; c < NUMHELLMUSIC; c++ )
		{
			FMOD_Sound_Release(hellmusic[c]);
		}
		if ( hellmusic )
		{
			free(hellmusic);
		}
		for ( c = 0; c < NUMMINOTAURMUSIC; c++ )
		{
			FMOD_Sound_Release(minotaurmusic[c]);
		}
		if ( minotaurmusic )
		{
			free(minotaurmusic);
		}
		for ( c = 0; c < NUMCAVESMUSIC; c++ )
		{
			FMOD_Sound_Release(cavesmusic[c]);
		}
		if ( cavesmusic )
		{
			free(cavesmusic);
		}
		for ( c = 0; c < NUMCITADELMUSIC; c++ )
		{
			FMOD_Sound_Release(citadelmusic[c]);
		}
		if ( citadelmusic )
		{
			free(citadelmusic);
		}
		for ( c = 0; c < NUMINTROMUSIC; c++ )
		{
			FMOD_Sound_Release(intromusic[c]);
		}
		if ( intromusic )
		{
			free(intromusic);
		}
	}
#ifdef USE_OPENAL
#undef FMOD_Channel_Stop
#undef FMOD_Sound_Release
#endif
#endif

	// free items
	printlog( "freeing item data...\n");
	for ( c = 0; c < NUMITEMS; c++ )
	{
		list_FreeAll(&items[c].images);
		node_t* node, *nextnode;
		for ( node = items[c].surfaces.first; node != NULL; node = nextnode )
		{
			nextnode = node->next;
			SDL_Surface** surface = (SDL_Surface**)node->element;
			if ( surface )
				if ( *surface )
				{
					SDL_FreeSurface(*surface);
				}
		}
		list_FreeAll(&items[c].surfaces);
	}

	freeSpells();

	// pathmaps
	if ( pathMapGrounded )
	{
		free(pathMapGrounded);
	}
	pathMapGrounded = NULL;
	if ( pathMapFlying )
	{
		free(pathMapFlying);
	}
	pathMapFlying = NULL;

	// clear steam achievement list
	list_FreeAll(&booksRead);

	// clear lobby chatbox data
	list_FreeAll(&lobbyChatboxMessages);

	// steam stuff
#ifdef STEAMWORKS
	cpp_SteamServerWrapper_Destroy();
	cpp_SteamServerClientWrapper_Destroy();
	if ( currentLobby )
	{
		SteamMatchmaking()->LeaveLobby(*static_cast<CSteamID*>(currentLobby));
		cpp_Free_CSteamID(currentLobby); //TODO: Remove these bodges.
		currentLobby = NULL;
	}
	if ( lobbyToConnectTo )
	{
		cpp_Free_CSteamID(lobbyToConnectTo);
		lobbyToConnectTo = NULL;
	}
	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		if ( steamIDRemote[c] )
		{
			cpp_Free_CSteamID(steamIDRemote[c]);
			steamIDRemote[c] = NULL;
		}
	}
	for ( c = 0; c < MAX_STEAM_LOBBIES; c++ )
	{
		if ( lobbyIDs[c] )
		{
			cpp_Free_CSteamID(lobbyIDs[c]);
			lobbyIDs[c] = NULL;
		}
	}
#endif
#if defined USE_EOS
	if ( EOS.CurrentLobbyData.currentLobbyIsValid() )
	{
		EOS.leaveLobby();

		Uint32 shutdownTicks = SDL_GetTicks();
		while ( EOS.CurrentLobbyData.bAwaitingLeaveCallback )
		{
#ifdef APPLE
			SDL_Event event;
			while ( SDL_PollEvent(&event) != 0 )
			{
				//Makes Mac work because Apple had to do it different.
			}
#endif
			EOS_Platform_Tick(EOS.PlatformHandle);
			SDL_Delay(50);
			if ( SDL_GetTicks() - shutdownTicks >= 3000 )
			{
				break;
			}
		}
	}
	EOS.AccountManager.deinit();
	EOS.shutdown();
#endif

	//Close game controller
	/*if (game_controller)
	{
		SDL_GameControllerClose(game_controller);
		game_controller = nullptr;
	}*/
	/*if (game_controller)
	{
		delete game_controller;
	}*/

	IRCHandler.disconnect();

	if ( shoparea )
	{
		free(shoparea);
	}

	for (int i = 0; i < MAXPLAYERS; ++i)
	{
		delete players[i];
		players[i] = nullptr;
	}
}
