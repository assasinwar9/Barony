/*-------------------------------------------------------------------------------

	BARONY
	File: actchest.cpp
	Desc: implements all chest related code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "interface/interface.hpp"
#include "items.hpp"
#include "sound.hpp"
#include "net.hpp"
#include "player.hpp"

/*
 * Chest theme ideas:
"random"
"empty / tiny amount of worthless garbage"
"food"
"treasures, jewelry, gems"
"weapons / armor"
"tools"
"spellbooks / scrolls"
"magicstaffs"
"potions"
 */

//chest->children->first is the chest's inventory.

void actChest(Entity* my)
{
	if ( !my )
	{
		return;
	}

	my->actChest();
}

void Entity::actChest()
{
	chestAmbience--;
	if ( chestAmbience <= 0 )
	{
		chestAmbience = TICKS_PER_SECOND * 30;
		playSoundEntityLocal(this, 149, 32);
	}

	if ( ticks == 1 )
	{
		this->createWorldUITooltip();
	}

	if ( multiplayer == CLIENT )
	{
		return;
	}

	int i;

	if (!chestInit)
	{
		chestInit = 1;
		chestHealth = 90 + rand() % 20;
		chestMaxHealth = chestHealth;
		chestOldHealth = chestHealth;
		chestPreventLockpickCapstoneExploit = 1;
		chestLockpickHealth = 40;
		int roll = 0;

		if ( chestLocked == -1 )
		{
			roll = rand() % 10;
			if ( roll == 0 )   // 10% chance //TODO: This should be weighted, depending on chest type.
			{
				chestLocked = 1;
				chestPreventLockpickCapstoneExploit = 0;
			}
			else
			{
				chestLocked = 0;
			}
			//messagePlayer(0, "Chest rolled: %d, locked: %d", roll, chestLocked); //debug print
		}
		else  if ( chestLocked >= 0 )
		{
			roll = rand() % 100;
			if ( roll < chestLocked )
			{
				chestLocked = 1;
				chestPreventLockpickCapstoneExploit = 0;
			}
			else
			{
				chestLocked = 0;
			}

			//messagePlayer(0, "Chest rolled: %d, locked: %d", roll, chestLocked); //debug print
		}

		node_t* node = NULL;
		node = list_AddNodeFirst(&children);
		node->element = malloc(sizeof(list_t)); //Allocate memory for the inventory list.
		node->deconstructor = &listDeconstructor;
		list_t* inventory = (list_t*) node->element;
		inventory->first = NULL;
		inventory->last = NULL;

		int itemcount = 0;

		int chesttype = 0;

		if (chestType > 0) //If chest spawned by editor sprite, manually set the chest content category. Otherwise this value should be 0 (random).
		{ 
			chesttype = chestType; //Value between 0 and 7.
		}
		else 
		{
			if (strcmp(map.name, "The Mystic Library")) 
			{
				chesttype = rand() % 8;
				if ( chesttype == 1 )
				{
					if ( currentlevel > 10 )
					{
						// re-roll the garbage chest.
						while ( chesttype == 1 )
						{
							chesttype = rand() % 8;
						}
					}
					else
					{
						// re-roll the garbage chest 50% chance
						if ( rand() % 2 == 0 )
						{
							chesttype = rand() % 8;
						}
					}
				}
			}
			else 
			{
				chesttype = 6; // magic chest			
			}
		}

		int minimumQuality = 0;
		if ( currentlevel >= 32 )
		{
			minimumQuality = 10;
		}
		else if ( currentlevel >= 18 )
		{
			minimumQuality = 5;
		}

		if ( chestHasVampireBook )
		{
			newItem(SPELLBOOK_VAMPIRIC_AURA, EXCELLENT, 0, 1, rand(), true, inventory);
		}

		switch (chesttype)   //Note that all of this needs to be properly balanced over time.
		{
			//TODO: Make all applicable item additions work on a category based search?
			case 0:
				//Completely random.
				itemcount = (rand() % 5) + 1;
				for (i = 0; i < itemcount; ++i)
				{
					//And add the current entity to it.
					//int itemnum = rand() % NUMITEMS;
					//while (itemnum == SPELL_ITEM || (items[itemnum].level == -1) || items[itemnum].level > currentlevel + 5 )
					//{
					//	//messagePlayer(0, "Skipping item %d, level %d", itemnum, items[itemnum].level);
					//	itemnum = rand() % NUMITEMS;    //Keep trying until you don't get a spell or invalid item.
					//}
					//newItem(static_cast<ItemType>(itemnum), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
					int cat = rand() % (NUMCATEGORIES - 1); // exclude spell_cat
					Item* currentItem = newItem(itemLevelCurve(static_cast<Category>(cat), 0, currentlevel + 5), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
					if ( currentItem )
					{
						if ( currentItem->type >= BRONZE_TOMAHAWK && currentItem->type <= CRYSTAL_SHURIKEN )
						{
							// thrown weapons always fixed status. (tomahawk = decrepit, shuriken = excellent)
							currentItem->status = std::min(static_cast<Status>(DECREPIT + (currentItem->type - BRONZE_TOMAHAWK)), EXCELLENT);
						}
					}
				}
				break;
			case 1:
				//Garbage chest
				if (rand() % 2)
				{
					//Empty.
				}
				else
				{
					//Some worthless garbage. Like a rock. //TODO: Sometimes spawn item 139, worthless piece of glass. Maybe go a step further and have a random amount of items, say 1 - 5, and they can be either rock or the worthless piece of glass or any other garbage.
					itemcount = (rand() % 3) + 1;
					int itemStatus = WORN + rand() % 3;
					for ( i = 0; i < itemcount; ++i )
					{
						newItem(GEM_ROCK, static_cast<Status>(itemStatus), 0, 1, rand(), false, inventory);
					}
				}
				break;
			case 2:
				//Food.
				//Items 152 - 158 are all food.
				itemcount = (rand() % 5) + 1;
				for (i = 0; i < itemcount; ++i)
				{
					//newItem(static_cast<ItemType>(FOOD_BREAD + (rand() % 7)), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
					newItem(itemLevelCurve(FOOD, 0, currentlevel + 5), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
				}
				break;
			case 3:
				//Treasures, jewelry, gems 'n stuff.
				itemcount = (rand() % 5) + 1;
				for (i = 0; i < itemcount; ++i)
				{
					if ( rand() % 4 )
					{
						newItem(static_cast<ItemType>(GEM_GARNET + rand() % 15), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
					}
					else
					{
						newItem(GEM_GLASS, static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
					}
				}
				//Random chance to spawn a ring or an amulet or some other jewelry.
				if (rand() % 2)
				{
					if (rand() % 2)
					{
						//Spawn a ring.
						//newItem(static_cast<ItemType>(RING_ADORNMENT + rand() % 12), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						newItem(itemLevelCurve(RING, 0, currentlevel + 5), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
					}
					else
					{
						//Spawn an amulet.
						//newItem(static_cast<ItemType>(AMULET_SEXCHANGE + rand() % 6), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						newItem(itemLevelCurve(AMULET, 0, currentlevel + 5), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
					}
				}
				break;
			case 4:
				//Weapons, armor, stuff.
				//Further break this down into either spawning only weapon(s), only armor(s), or a combo, like a set.

				switch (rand() % 3)   //TODO: Note, switch to rand()%4 if/when case 3 is implemented.
				{
					case 0:
						//Only a weapon. Items 0 - 16.
					{
						//int item = rand() % 18;
						////Since the weapons are not a continuous set, check to see if the weapon is part of the continuous set. If it is not, move on to the next block. In this case, there's only one weapon that is not part of the continous set: the crossbow.
						//if (item < 16)
						//	//Almost every weapon.
						//{
						//	newItem(static_cast<ItemType>(rand() % 17), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						//}
						//else
						//	//Crossbow.
						//{
						//	newItem(CROSSBOW, static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						//}
						newItem(itemLevelCurve(WEAPON, minimumQuality, currentlevel + 5), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
					}
					break;
					case 1:
						//Only a piece of armor.
					{
						/*
						 * 0 - 1 are the steel shields, items 17 and 18.
						 * 2 - 5 are the gauntlets, items 20 - 23.
						 * 6 - 15 are the boots & shirts (as in, breastplates and all variants), items 28 - 37.
						 * 16 - 19 are the hats & helmets, items 40 - 43
						 */
						//int item = rand() % 15;
						//if (item <= 1)
						//	//Steel shields. Items 17 & 18.
						//{
						//	newItem(static_cast<ItemType>(17 + rand() % 2), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						//}
						//else if (item <= 5)
						//	//Gauntlets. Items 20 - 23.
						//{
						//	if ( rand() % 3 > 0 )
						//	{
						//		newItem(static_cast<ItemType>(20 + rand() % 4), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						//	}
						//	else
						//	{
						//		// new gauntlets
						//		newItem(static_cast<ItemType>(BRASS_KNUCKLES + rand() % 3), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						//	}
						//}
						//else if (item <= 10)
						//	//Hats & helmets. Items 40 - 43.
						//{
						//	newItem(static_cast<ItemType>(40 + rand() % 4), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						//}
						//else if (item <= 15)
						//	//Boots & shirts. Items 28 - 37.
						//{
						//	newItem(static_cast<ItemType>(28 + rand() % 10), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						//}
						newItem(itemLevelCurve(ARMOR, minimumQuality, currentlevel + 5), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
					}
					break;
					case 2:
						//A weapon and an armor, chance of thrown.
					{
						//int item = rand() % 18;
						////Since the weapons are not a continuous set, check to see if the weapon is part of the continuous set. If it is not, move on to the next block. In this case, there's only one weapon that is not part of the continous set: the crossbow.
						//if (item < 16)
						//	//Almost every weapon.
						//{
						//	newItem(static_cast<ItemType>(rand() % 17), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						//}
						//else
						//	//Crossbow.
						//{
						//	newItem(static_cast<ItemType>(19), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						//}

						///*
						// * 0 - 1 are the steel shields, items 17 and 18.
						// * 2 - 5 are the gauntlets, items 20 - 23.
						// * 6 - 15 are the boots & shirts (as in, breastplates and all variants), items 28 - 37.
						// * 16 - 19 are the hats & helmets, items 40 - 43
						// */
						//item = rand() % 20;
						//if (item <= 1)
						//	//Steel shields. Items 17 & 18.
						//{
						//	newItem(static_cast<ItemType>(17 + rand() % 2), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						//}
						//else if (item <= 5)
						//	//Gauntlets. Items 20 - 23.
						//{
						//	if ( rand() % 3 > 0 )
						//	{
						//		newItem(static_cast<ItemType>(20 + rand() % 4), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						//	}
						//	else
						//	{
						//		// new gauntlets
						//		newItem(static_cast<ItemType>(BRASS_KNUCKLES + rand() % 3), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						//	}
						//}
						//else if (item <= 10)
						//	//Hats & helmets. Items 40 - 43.
						//{
						//	newItem(static_cast<ItemType>(40 + rand() % 4), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						//}
						//else if (item <= 15)
						//	//Boots & shirts. Items 28 - 37.
						//{
						//	newItem(static_cast<ItemType>(28 + rand() % 10), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						//}

						newItem(itemLevelCurve(WEAPON, minimumQuality, currentlevel + 5), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						newItem(itemLevelCurve(ARMOR, minimumQuality, currentlevel + 5), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						
						// try for thrown items.
						itemcount = 0 + rand() % 2;
						for ( i = 0; i < itemcount; ++i )
						{
							Item* thrown = newItem(itemLevelCurve(THROWN, minimumQuality, currentlevel), WORN, 0, 3 + rand() % 3, rand(), false, inventory);
							if ( thrown )
							{
								if ( thrown->type >= BRONZE_TOMAHAWK && thrown->type <= CRYSTAL_SHURIKEN )
								{
									// thrown weapons always fixed status. (tomahawk = decrepit, shuriken = excellent)
									thrown->status = std::min(static_cast<Status>(DECREPIT + (thrown->type - BRONZE_TOMAHAWK)), EXCELLENT);
								}
							}
						}
					}
					break;
					case 3:
						//TODO: Rarer. Getting a full set of armor + a weapon.
						break;
				}
				break;
			case 5:
			{
				//Tools.
				Status durability = static_cast<Status>(WORN + rand() % 3);
				switch ( rand() % 3 )
				{
					case 0:
						itemcount = rand() % 3;
						for ( i = 0; i < itemcount; ++i )
						{
							newItem(TOOL_BEARTRAP, durability, 0, 1 + rand() % 3, rand(), false, inventory);
						}
						// fall through
					case 1:
						itemcount = 1 + rand() % 2;
						for (i = 0; i < itemcount; ++i)
						{
							newItem(static_cast<ItemType>(TOOL_PICKAXE + rand() % 12), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						}
						if ( rand() % 20 == 0 )
						{
							newItem(CLOAK_BACKPACK, durability, 0, 1, rand(), false, inventory);
						}
						if ( rand() % 20 == 0 )
						{
							newItem(TOOL_TINKERING_KIT, DECREPIT, 0, 1, rand(), false, inventory);
							newItem(TOOL_METAL_SCRAP, DECREPIT, 0, 10 + rand() % 11, 0, true, inventory);
							newItem(TOOL_MAGIC_SCRAP, DECREPIT, 0, 10 + rand() % 11, 0, true, inventory);
						}
						break;
					case 2:
						itemcount = 1 + rand() % 2;
						for ( i = 0; i < itemcount; ++i )
						{
							Item* thrown = newItem(itemLevelCurve(THROWN, minimumQuality, currentlevel), WORN, 0, 3 + rand() % 3, rand(), false, inventory);
							if ( thrown )
							{
								if ( thrown->type >= BRONZE_TOMAHAWK && thrown->type <= CRYSTAL_SHURIKEN )
								{
									// thrown weapons always fixed status. (tomahawk = decrepit, shuriken = excellent)
									thrown->status = std::min(static_cast<Status>(DECREPIT + (thrown->type - BRONZE_TOMAHAWK)), EXCELLENT);
								}
							}
						}
						break;
					default:
						break;
				}
				break;
			}
			case 6:
				//Magic chest.
				//So first choose what kind of magic chest it is.
			{
				/*
				 * Types:
				 * * Scroll chest. Has some scrolls in it ( 3 - 5).
				 * * Book chest. Basically a small library. 1-3 books.
				 * * Staff chest. Staff or 2.
				 * * Wizard's chest, which will contain 1-2 scrolls, a magic book, a staff, and either a wizard/magician/whatever implement of some sort or a piece of armor.
				 */
				int magic_type = rand() % 4;

				switch (magic_type)
				{
					case 0:
						//Have 3-5 scrolls.
						itemcount = 3 + (rand() % 3);
						for (i = 0; i < itemcount; ++i)
						{
							//newItem(static_cast<ItemType>(SCROLL_IDENTIFY + rand() % 12), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
							newItem(itemLevelCurve(SCROLL, 0, currentlevel + 5), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						}
						if ( rand() % 10 == 0 )
						{
							if ( rand() % 5 == 0 )
							{
								newItem(ENCHANTED_FEATHER, EXCELLENT, 0, 1, ENCHANTED_FEATHER_MAX_DURABILITY - 1, false, inventory);
							}
							else
							{
								newItem(ENCHANTED_FEATHER, SERVICABLE, 0, 1, (3 * (ENCHANTED_FEATHER_MAX_DURABILITY - 1)) / 4, false, inventory);
							}
							if ( rand() % 2 == 0 )
							{
								newItem(SCROLL_BLANK, static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 3, rand(), false, inventory);
							}
						}
						break;
					case 1:
						//Have 1-3 books.
						itemcount = 1 + (rand() % 3);
						for (i = 0; i < itemcount; ++i)
						{
							//newItem(static_cast<ItemType>(SPELLBOOK_FORCEBOLT + rand() % 22), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
							newItem(itemLevelCurve(SPELLBOOK, 0, currentlevel + 6), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						}
						break;
					case 2:
						//A staff.
						//newItem(static_cast<ItemType>(MAGICSTAFF_LIGHT + rand() % 10), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						newItem(itemLevelCurve(MAGICSTAFF, 0, currentlevel + 5), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						break;
					case 3:
						//So spawn several items at once. A wizard's chest!

						//First the scrolls (1 - 2).
						itemcount = 1 + rand() % 2;
						for (i = 0; i < itemcount; ++i)
						{
							//newItem(static_cast<ItemType>(SCROLL_IDENTIFY + rand() % 12), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
							newItem(itemLevelCurve(SCROLL, 0, currentlevel + 5), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						}

						//newItem(static_cast<ItemType>(SPELLBOOK_FORCEBOLT + rand() % 22), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						newItem(itemLevelCurve(SPELLBOOK, 0, currentlevel + 6), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						//newItem(static_cast<ItemType>(MAGICSTAFF_LIGHT + rand() % 10), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						newItem(itemLevelCurve(MAGICSTAFF, 0, currentlevel + 5), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
						switch (rand() % 7)
						{
							case 0:
								//A cloak. Item 24.
								newItem(CLOAK, static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
								break;
							case 1:
								//A cloak of magic resistance. Item 25.
								newItem(CLOAK_MAGICREFLECTION, static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
								break;
							case 2:
								//A cloak of invisibility. Item 26.
								newItem(CLOAK_INVISIBILITY, static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
								break;
							case 3:
								//A cloak of protection. Item 27.
								newItem(CLOAK_PROTECTION, static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
								break;
							case 4:
								//A phyregian's hat/fez hat. Item 38.
								if ( rand() % 5 == 0 )
								{
									newItem(HAT_FEZ, static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
								}
								else
								{
									newItem(HAT_PHRYGIAN, static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
								}
								break;
							case 5:
								//A wizard's hat. Item 39.
								newItem(HAT_WIZARD, static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
								break;
							case 6:
								newItem(ENCHANTED_FEATHER, EXCELLENT, 0, 1, ENCHANTED_FEATHER_MAX_DURABILITY - 1, false, inventory);
								if ( rand() % 2 == 0 )
								{
									newItem(SCROLL_BLANK, static_cast<Status>(WORN + rand() % 3), 0, 1 + rand() % 3, rand(), false, inventory);
								}
								break;
						}
						break;
				}
			}
			break;
			case 7:
				//Potions.
				//Items 50 - 64 are potions.
				itemcount = (rand() % 3) + 1;
				for (i = 0; i < itemcount; ++i)
				{
					//newItem(static_cast<ItemType>(POTION_WATER + (rand() % 15)), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
					newItem(itemLevelCurve(POTION, 0, currentlevel + 7), static_cast<Status>(WORN + rand() % 3), 0, 1, rand(), false, inventory);
				}
				if ( rand() % 2 == 0 )
				{
					newItem(TOOL_ALEMBIC, static_cast<Status>(WORN + rand() % 3), -1 + rand() % 3, 1, rand(), false, inventory);
					newItem(POTION_EMPTY, SERVICABLE, 0, 2 + rand() % 3, 0, true, inventory);
				}
				if ( rand() % 4 == 0 )
				{
					newItem(TOOL_ALEMBIC, static_cast<Status>(WORN + rand() % 3), -1 + rand() % 3, 1, rand(), false, inventory);
					newItem(POTION_EMPTY, SERVICABLE, 0, 0 + rand() % 3, 0, true, inventory);
				}
				break;
			case 8:
				break;
			default:
				//Default case. Should never be reached.
				newItem(static_cast<ItemType>(0), BROKEN, 0, 1, rand(), false, inventory);
				printlog("warning: default cause in chest init theme type reached. This should never happen.");
				break;
		}
	}

	list_t* inventory = static_cast<list_t* >(children.first->element);
	node_t* node = NULL;
	Item* item = NULL;

	chestOldHealth = chestHealth;

	if ( chestHealth <= 0 )
	{
		// the chest busts open, drops some items randomly, then destroys itself.
		node_t* nextnode;
		for ( node = inventory->first; node != NULL; node = nextnode )
		{
			nextnode = node->next;
			item = (Item*)node->element;
			if ( rand() % 2 == 0 )
			{
				dropItemMonster(item, this, NULL);
			}
		}

		// wood chunk particles
		int c;
		for ( c = 0; c < 10; c++ )
		{
			Entity* entity = spawnGib(this);
			entity->flags[INVISIBLE] = false;
			entity->sprite = 187; // Splinter.vox
			entity->x = floor(x / 16) * 16 + 8;
			entity->y = floor(y / 16) * 16 + 8;
			entity->z = -7 + rand() % 14;
			entity->yaw = (rand() % 360) * PI / 180.0;
			entity->pitch = (rand() % 360) * PI / 180.0;
			entity->roll = (rand() % 360) * PI / 180.0;
			entity->vel_x = cos(entity->yaw) * (0.5 + (rand() % 100) / 100.f);
			entity->vel_y = sin(entity->yaw) * (0.5 + (rand() % 100) / 100.f);
			entity->vel_z = -.25;
			entity->fskill[3] = 0.04;
			serverSpawnGibForClient(entity);
		}
		playSoundEntity(this, 177, 64);

		if ( chestStatus == 1 )
		{
			messagePlayer(chestOpener, language[671]); // "The chest is smashed into pieces!" only notify if chest is currently open.
		}

		this->closeChest();

		// remove chest entities
		Entity* parentEntity = uidToEntity(parent);
		if ( parentEntity )
		{
			list_RemoveNode(parentEntity->mynode);    // remove lid
		}
		list_RemoveNode(mynode); // remove me
		return;
	}
	else
	{
		if ( multiplayer != CLIENT && chestHasVampireBook )
		{
			node = inventory->first;
			if ( node )
			{
				item = (Item*)node->element;
				if ( item )
				{
					if ( item->type == SPELLBOOK_VAMPIRIC_AURA )
					{
						spawnAmbientParticles(40, 600, 20 + rand() % 30, 0.5, true);
					}
					else
					{
						chestHasVampireBook = 0;
						serverUpdateEntitySkill(this, 11);
					}
				}
			}
		}
		if ( chestHasVampireBook )
		{
			spawnAmbientParticles(40, 600, 20 + rand() % 30, 0.5, true);
		}
	}

	if ( chestStatus == 1 )
	{
		if ( players[chestOpener] && players[chestOpener]->entity )
		{
			unsigned int distance = sqrt(pow(x - players[chestOpener]->entity->x, 2) + pow(y - players[chestOpener]->entity->y, 2));
			if (distance > TOUCHRANGE)
			{
				closeChest();
			}
		}
		else
		{
			closeChest();
		}
	}

	//Using the chest (TODO: Monsters using it?).
	int chestclicked = -1;
	for (i = 0; i < MAXPLAYERS; ++i)
	{
		if ( (i == 0 && selectedEntity[0] == this) || (client_selected[i] == this) || (splitscreen && selectedEntity[i] == this) )
		{
			if (inrange[i])
			{
				chestclicked = i;
			}
		}
	}
	if ( chestLidClicked )
	{
		chestclicked = chestLidClicked - 1;
		chestLidClicked = 0;
	}
	if ( chestclicked >= 0 )
	{
		if ( !chestLocked && !openedChest[chestclicked] )
		{
			if ( !chestStatus )
			{
				messagePlayer(chestclicked, language[459]);
				openedChest[chestclicked] = this;

				chestOpener = chestclicked;
				if ( players[chestclicked]->isLocalPlayer() ) // i.e host opened the chest, close GUIs
				{
					players[chestclicked]->closeAllGUIs(DONT_CHANGE_SHOOTMODE, CLOSEGUI_DONT_CLOSE_CHEST);
				}
				if ( !players[chestclicked]->isLocalPlayer() && multiplayer == SERVER)
				{
					//Send all of the items to the client.
					strcpy((char*)net_packet->data, "CHST");  //Chest.
					SDLNet_Write32((Uint32)getUID(), &net_packet->data[4]); //Give the client the UID.
					net_packet->address.host = net_clients[chestclicked - 1].host;
					net_packet->address.port = net_clients[chestclicked - 1].port;
					net_packet->len = 8;
					sendPacketSafe(net_sock, -1, net_packet, chestclicked - 1);
					for (node = inventory->first; node != NULL; node = node->next)
					{
						item = (Item*) node->element;
						strcpy((char*)net_packet->data, "CITM");  //Chest item.
						SDLNet_Write32((Uint32)item->type, &net_packet->data[4]);
						SDLNet_Write32((Uint32)item->status, &net_packet->data[8]);
						SDLNet_Write32((Uint32)item->beatitude, &net_packet->data[12]);
						SDLNet_Write32((Uint32)item->count, &net_packet->data[16]);
						SDLNet_Write32((Uint32)item->appearance, &net_packet->data[20]);
						net_packet->data[24] = item->identified;
						net_packet->address.host = net_clients[chestclicked - 1].host;
						net_packet->address.port = net_clients[chestclicked - 1].port;
						net_packet->len = 25;
						sendPacketSafe(net_sock, -1, net_packet, chestclicked - 1);
					}
				}
				else
				{
					players[chestclicked]->openStatusScreen(GUI_MODE_INVENTORY, INVENTORY_MODE_ITEM); // Reset the GUI to the inventory.
					if ( numItemsInChest(chestclicked) > 0 )   //Warp mouse to first item in chest only if there are any items!
					{
						selectedChestSlot[chestclicked] = 0;
						warpMouseToSelectedChestSlot(chestclicked);
					}
					else
					{
						selectedChestSlot[chestclicked] = -1;
						warpMouseToSelectedInventorySlot(chestclicked); //Because setting shootmode to false tends to start the mouse in the middle of the screen. Which is not nice.
					}
				}
				chestStatus = 1; //Toggle chest open/closed.
			}
			else
			{
				messagePlayer(chestclicked, language[460]);
				if ( !players[chestOpener]->isLocalPlayer() )
				{
					strcpy((char*)net_packet->data, "CCLS");  //Chest close.
					net_packet->address.host = net_clients[chestOpener - 1].host;
					net_packet->address.port = net_clients[chestOpener - 1].port;
					net_packet->len = 4;
					sendPacketSafe(net_sock, -1, net_packet, chestOpener - 1);
				}
				else
				{
					chestitemscroll[chestclicked] = 0;
				}
				if (chestOpener != chestclicked)
				{
					messagePlayer(chestOpener, language[461]);
				}
				closeChestServer();
			}
		}
		else if ( chestLocked )
		{
			messagePlayer(chestclicked, language[462]);
			playSoundEntity(this, 152, 64);
		}
	}
}

void actChestLid(Entity* my)
{
	int i;

	Entity* parent = uidToEntity(my->parent);
	if ( !parent )
	{
		list_RemoveNode(my->mynode);
		return;
	}

	if ( multiplayer != CLIENT )
	{
		my->skill[1] = parent->skill[1];
		if ( multiplayer == SERVER )
		{
			if ( my->skill[3] != my->skill[1] )
			{
				my->skill[3] = my->skill[1];
				serverUpdateEntitySkill(my, 1);
			}
		}

		for (i = 0; i < MAXPLAYERS; ++i)
		{
			if ( (i == 0 && selectedEntity[0] == my) || (client_selected[i] == my) || (splitscreen && selectedEntity[i] == my) )
			{
				if (inrange[i])
				{
					parent->skill[6] = i + 1;
				}
			}
		}
	}

	if ( my->skill[1] )
	{
		// chest is open
		if ( !my->skill[0] )
		{
			my->skill[0] = 1;
			if ( multiplayer != CLIENT )
			{
				playSoundEntity(my, 21, 64);
			}
			my->fskill[0] = 0.25;
		}
		if ( my->pitch > -PI / 2 )
		{
			my->pitch -= my->fskill[0];
			my->fskill[0] -= 0.02;
			if ( my->pitch <= -PI / 2 )
			{
				my->pitch = -PI / 2;
				my->fskill[0] = 0;
			}
		}
	}
	else
	{
		// chest is closed
		if ( my->skill[0] )
		{
			my->skill[0] = 0;
			if ( multiplayer != CLIENT )
			{
				playSoundEntity(my, 22, 64);
			}
			my->fskill[0] = 0.025;
		}
		if ( my->pitch < 0 )
		{
			my->pitch += my->fskill[0];
			my->fskill[0] += 0.025;
			if ( my->pitch >= 0 )
			{
				my->pitch = 0;
				my->fskill[0] = 0;
			}
		}
	}
}

int getChestOpenerFromEntity(const Entity& chest)
{
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( openedChest[i] == &chest )
		{
			return i;
		}
	}
	return clientnum;
}

void Entity::closeChest()
{
	int player = getChestOpenerFromEntity(*this);

	if ( players[player]->isLocalPlayer() && multiplayer == CLIENT)
	{
		//If client, tell server the chest got closed.
		if (openedChest[player] != NULL)
		{
			//Message server.
			if ( chestHealth > 0 )
			{
				messagePlayer(player, language[460]);
			}
			
			strcpy( (char*)net_packet->data, "CCLS" );
			net_packet->data[4] = player;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 5;
			sendPacketSafe(net_sock, -1, net_packet, 0);

			closeChestClientside(player);
			return;
		}
	}

	if (chestStatus)
	{
		chestStatus = 0;

		if ( chestHealth > 0 )
		{
			messagePlayer(player, language[460]);
		}

		openedChest[chestOpener] = nullptr;
		if ( !players[chestOpener]->isLocalPlayer() && multiplayer == SERVER)
		{
			//Tell the client that the chest got closed.
			strcpy((char*)net_packet->data, "CCLS");  //Chest close.
			net_packet->address.host = net_clients[chestOpener - 1].host;
			net_packet->address.port = net_clients[chestOpener - 1].port;
			net_packet->len = 4;
			sendPacketSafe(net_sock, -1, net_packet, chestOpener - 1);
		}
		else
		{
			if ( players[chestOpener]->isLocalPlayer() )
			{
				for ( int c = 0; c < kNumChestItemsToDisplay; ++c )
				{
					invitemschest[chestOpener][c] = nullptr;
				}
			}
			chestitemscroll[chestOpener] = 0;
			//Reset chest-gamepad related stuff here.
			selectedChestSlot[chestOpener] = -1;
		}
	}
}

void Entity::closeChestServer()
{
	if (chestStatus)
	{
		chestStatus = 0;
		openedChest[chestOpener] = NULL;
		if ( players[chestOpener]->isLocalPlayer() )
		{
			for ( int c = 0; c < kNumChestItemsToDisplay; ++c )
			{
				invitemschest[chestOpener][c] = nullptr;
			}
		}
	}
}

void Entity::addItemToChest(Item* item)
{
	if (!item)
	{
		return;
	}
	int player = getChestOpenerFromEntity(*this);
	if ( players[player]->isLocalPlayer() && multiplayer == CLIENT )
	{
		//Tell the server.
		strcpy( (char*)net_packet->data, "CITM" );
		net_packet->data[4] = player;
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		SDLNet_Write32((Uint32)item->type, &net_packet->data[5]);
		SDLNet_Write32((Uint32)item->status, &net_packet->data[9]);
		SDLNet_Write32((Uint32)item->beatitude, &net_packet->data[13]);
		SDLNet_Write32((Uint32)item->count, &net_packet->data[17]);
		SDLNet_Write32((Uint32)item->appearance, &net_packet->data[21]);
		net_packet->data[25] = item->identified;
		net_packet->len = 26;
		sendPacketSafe(net_sock, -1, net_packet, 0);

		addItemToChestClientside(player, item);
		return;
	}

	Item* item2 = NULL;

	//Add the item to the chest's inventory.
	list_t* inventory = static_cast<list_t* >(children.first->element);

	node_t* t_node = NULL;
	//If item's already in the chest, add it to a pre-existing stack.
	for (t_node = inventory->first; t_node != NULL; t_node = t_node->next)
	{
		item2 = (Item*) t_node->element;
		if (!itemCompare(item, item2, false))
		{
			item2->count += item->count;
			return;
		}
	}

	item->node = list_AddNodeFirst(inventory);
	item->node->element = item;
	item->node->deconstructor = &defaultDeconstructor;

	if ( !players[chestOpener]->isLocalPlayer() && multiplayer == SERVER )
	{
		strcpy((char*)net_packet->data, "CITM");
		SDLNet_Write32((Uint32)item->type, &net_packet->data[4]);
		SDLNet_Write32((Uint32)item->status, &net_packet->data[8]);
		SDLNet_Write32((Uint32)item->beatitude, &net_packet->data[12]);
		SDLNet_Write32((Uint32)item->count, &net_packet->data[16]);
		SDLNet_Write32((Uint32)item->appearance, &net_packet->data[20]);
		net_packet->data[24] = item->identified;
		net_packet->address.host = net_clients[chestOpener - 1].host;
		net_packet->address.port = net_clients[chestOpener - 1].port;
		net_packet->len = 25;
		sendPacketSafe(net_sock, -1, net_packet, chestOpener - 1);
	}
}

void Entity::addItemToChestFromInventory(int player, Item* item, bool all)
{
	if (!item || !players[player] || !players[player]->entity)
	{
		return;
	}

	if (itemCategory(item) == SPELL_CAT)
	{
		return;
	}

	bool isEquipped = itemIsEquipped(item, player);

	if ( isEquipped )
	{
		if ( !item->canUnequip(stats[player]) )
		{
			if ( shouldInvertEquipmentBeatitude(stats[player]) && item->beatitude > 0 )
			{
				messagePlayer(player, language[3218]);
			}
			else
			{
				messagePlayer(player, language[1087]);
			}
			item->identified = true;
			return;
		}
	}
	playSoundPlayer(player, 47 + rand() % 3, 64);

	Item* newitem = NULL;
	if ( (newitem = (Item*) malloc(sizeof(Item))) == NULL)
	{
		printlog( "failed to allocate memory for new item!\n" );
		return; //Error or something.
	}
	newitem->node = NULL;
	newitem->count = 1;
	newitem->type = item->type;
	newitem->status = item->status;
	newitem->beatitude = item->beatitude;
	newitem->appearance = item->appearance;
	newitem->identified = item->identified;

	Item** slot = itemSlot(stats[player], item);
	if ( multiplayer == CLIENT )
	{
		// tell the server to unequip.
		if ( slot != nullptr )
		{
			if ( slot == &stats[player]->weapon )
			{
				playerTryEquipItemAndUpdateServer(player, item, false);
			}
			else if ( slot == &stats[player]->shield && itemCategory(newitem) == SPELLBOOK )
			{
				playerTryEquipItemAndUpdateServer(player, item, false);
			}
			else
			{
				if ( slot == &stats[player]->helmet )
				{
					clientUnequipSlotAndUpdateServer(player, EQUIP_ITEM_SLOT_HELM, item);
				}
				else if ( slot == &stats[player]->breastplate )
				{
					clientUnequipSlotAndUpdateServer(player, EQUIP_ITEM_SLOT_BREASTPLATE, item);
				}
				else if ( slot == &stats[player]->gloves )
				{
					clientUnequipSlotAndUpdateServer(player, EQUIP_ITEM_SLOT_GLOVES, item);
				}
				else if ( slot == &stats[player]->shoes )
				{
					clientUnequipSlotAndUpdateServer(player, EQUIP_ITEM_SLOT_BOOTS, item);
				}
				else if ( slot == &stats[player]->shield )
				{
					clientUnequipSlotAndUpdateServer(player, EQUIP_ITEM_SLOT_SHIELD, item);
				}
				else if ( slot == &stats[player]->cloak )
				{
					clientUnequipSlotAndUpdateServer(player, EQUIP_ITEM_SLOT_CLOAK, item);
				}
				else if ( slot == &stats[player]->amulet )
				{
					clientUnequipSlotAndUpdateServer(player, EQUIP_ITEM_SLOT_AMULET, item);
				}
				else if ( slot == &stats[player]->ring )
				{
					clientUnequipSlotAndUpdateServer(player, EQUIP_ITEM_SLOT_RING, item);
				}
				else if ( slot == &stats[player]->mask )
				{
					clientUnequipSlotAndUpdateServer(player, EQUIP_ITEM_SLOT_MASK, item);
				}
			}
		}
	}

	// unequip the item
	if ( item->count <= 1 || all)
	{
		if ( slot != NULL )
		{
			*slot = NULL;
		}
	}
	if ( item->node != NULL )
	{
		if ( item->node->list == &stats[player]->inventory )
		{
			if (!all)
			{
				item->count--;
				if ( item->count <= 0 )
				{
					list_RemoveNode(item->node);
				}
			}
			else
			{
				newitem->count = item->count;
				list_RemoveNode(item->node);
			}
		}
	}
	else
	{
		item->count--;
		if ( item->count <= 0 )
		{
			free(item);
		}
	}

	messagePlayer(player, language[463], newitem->getName());
	addItemToChest(newitem);

	return; //Do not execute the rest of this function.
}

Item* Entity::getItemFromChest(Item* item, bool all, bool getInfoOnly)
{
	/*
	 * getInfoOnly just returns a copy of the item at the slot, it does not actually grab the item.
	 * Note that the returned memory will need to be freed.
	 */
	
	Item* newitem = NULL;

	if ( item == NULL )
	{
		return NULL;
	}

	int player = getChestOpenerFromEntity(*this);

	if ( players[player]->isLocalPlayer() && multiplayer == CLIENT )
	{
		if (!item || !item->node)
		{
			return NULL;
		}

		if ( (newitem = (Item*) malloc(sizeof(Item))) == NULL)
		{
			printlog( "failed to allocate memory for new item!\n" );
			return NULL; //Error or something.
		}
		newitem->node = NULL;
		newitem->count = 1;
		newitem->type = item->type;
		newitem->status = item->status;
		newitem->beatitude = item->beatitude;
		newitem->appearance = item->appearance;
		newitem->identified = item->identified;

		//Tell the server.
		if ( !getInfoOnly )
		{
			strcpy( (char*)net_packet->data, "RCIT" );  //Have the server remove the item from the chest).
			net_packet->data[4] = player;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			SDLNet_Write32((Uint32)item->type, &net_packet->data[5]);
			SDLNet_Write32((Uint32)item->status, &net_packet->data[9]);
			SDLNet_Write32((Uint32)item->beatitude, &net_packet->data[13]);
			int count = 1;
			if (all)
			{
				count = item->count;
			}
			SDLNet_Write32((Uint32)count, &net_packet->data[17]);
			SDLNet_Write32((Uint32)item->appearance, &net_packet->data[21]);
			net_packet->data[25] = item->identified;
			net_packet->len = 26;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
	}
	else
	{
		if ( !item )
		{
			return NULL;
		}
		if ( !item->node )
		{
			return NULL;
		}
		if ( item->node->list != children.first->element )
		{
			return NULL;
		}

		if ( (newitem = (Item*) malloc(sizeof(Item))) == NULL)
		{
			printlog( "failed to allocate memory for new item!\n" );
			return NULL; //Error or something.
		}
		newitem->node = NULL;
		newitem->count = 1;
		newitem->type = item->type;
		newitem->status = item->status;
		newitem->beatitude = item->beatitude;
		newitem->appearance = item->appearance;
		newitem->identified = item->identified;
	}

	if (!all)
	{
		//Grab only one item from the chest.
		newitem->count = 1;
		if (!getInfoOnly )
		{
			item->count -= 1;
			if ( item->count <= 0 )
			{
				list_RemoveNode(item->node);
			}
		}
	}
	else
	{
		//Grab all items from the chest.
		newitem->count = item->count;
		if ( !getInfoOnly )
		{
			list_RemoveNode(item->node);
		}
	}

	return newitem;
}

void closeChestClientside(const int player)
{
	if ( player < 0 )
	{
		return;
	}
	if (!openedChest[player])
	{
		return;
	}

	if ( multiplayer == CLIENT && players[player]->isLocalPlayer() )
	{
		//Only called for the client.

		list_FreeAll(&chestInv[player]);

		openedChest[player] = NULL;

		chestitemscroll[player] = 0;

		for ( int c = 0; c < kNumChestItemsToDisplay; ++c )
		{
			invitemschest[player][c] = nullptr;
		}

		//Reset chest-gamepad related stuff here.
		selectedChestSlot[player] = -1;
	}
}

void addItemToChestClientside(const int player, Item* item)
{
	if (openedChest[player])
	{
		//messagePlayer(player, "Recieved item.");

		//If there's an open chests, add an item to it.
		//TODO: Add item to the chest.

		Item* item2 = NULL;
		node_t* node = NULL;

		for (node = chestInv[player].first; node != NULL; node = node->next)
		{
			item2 = (Item*) node->element;
			if (!itemCompare(item, item2, false))
			{
				item2->count += item->count;
				return;
			}
		}

		item->node = list_AddNodeFirst(&chestInv[player]);
		item->node->element = item;
		item->node->deconstructor = &defaultDeconstructor;
	}
	//TODO: Else: Ruh-roh, error!
}



void Entity::addItemToChestServer(Item* item)
{
	if (!item)
	{
		return;
	}

	Item* item2 = NULL;
	node_t* t_node = NULL;

	//Add the item to the chest's inventory.
	list_t* inventory = static_cast<list_t* >(children.first->element);

	if (!inventory)
	{
		return;
	}

	//If item's already in the chest, add it to a pre-existing stack.
	for (t_node = inventory->first; t_node != NULL; t_node = t_node->next)
	{
		item2 = (Item*) t_node->element;
		if (!itemCompare(item, item2, false))
		{
			item2->count += item->count;
			return;
		}
	}

	item->node = list_AddNodeFirst(inventory);
	item->node->element = item;
	item->node->deconstructor = &defaultDeconstructor;
}

void Entity::removeItemFromChestServer(Item* item, int count)
{
	if (!item)
	{
		return;
	}

	Item* item2 = NULL;
	node_t* t_node = NULL;

	list_t* inventory = static_cast<list_t* >(children.first->element);
	if (!inventory)
	{
		return;
	}

	for (t_node = inventory->first; t_node != NULL; t_node = t_node->next)
	{
		item2 = (Item*) t_node->element;
		if (!item2  || !item2->node || item2->node->list != children.first->element)
		{
			return;
		}
		if (!itemCompare(item, item2, false))
		{
			if (count < item2->count)
			{
				//Grab only one item from the chest.
				int oldcount = item2->count;
				item2->count = oldcount - count;
				if ( item2->count <= 0 )
				{
					list_RemoveNode(item2->node);
				}
			}
			else
			{
				//Grab all items from the chest.
				list_RemoveNode(item2->node);
			}
			return;
		}
	}
}

void Entity::unlockChest()
{
	chestLocked = 0;
	chestPreventLockpickCapstoneExploit = 1;
}

void Entity::lockChest()
{
	chestLocked = 1;
}

void Entity::chestHandleDamageMagic(int damage, Entity &magicProjectile, Entity *caster)
{
	chestHealth -= damage; //Decrease chest health.
	if ( caster )
	{
		if ( caster->behavior == &actPlayer )
		{
			if ( chestHealth <= 0 )
			{
				if ( magicProjectile.behavior == &actBomb )
				{
					messagePlayer(caster->skill[2], language[3617], items[magicProjectile.skill[21]].name_identified, language[675]);
				}
				else
				{
					messagePlayer(caster->skill[2], language[2520]);
				}
			}
			else
			{
				if ( magicProjectile.behavior == &actBomb )
				{
					messagePlayer(caster->skill[2], language[3618], items[magicProjectile.skill[21]].name_identified, language[675]);
				}
				else
				{
					messagePlayer(caster->skill[2], language[378], language[675]);
				}
			}
		}
		updateEnemyBar(caster, this, language[675], chestHealth, chestMaxHealth);
	}
	playSoundEntity(this, 28, 128);
}
