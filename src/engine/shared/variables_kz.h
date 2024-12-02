// kaizo-insta config variables
#ifndef ENGINE_SHARED_VARIABLES_KZ_H
#define ENGINE_SHARED_VARIABLES_KZ_H
#undef ENGINE_SHARED_VARIABLES_KZ_H // this file will be included several times

#ifndef MACRO_CONFIG_INT
#error "The config macros must be defined"
#define MACRO_CONFIG_INT(Name, ScriptName, Def, Min, Max, Save, Desc) ;
#define MACRO_CONFIG_COL(Name, ScriptName, Def, Save, Desc) ;
#define MACRO_CONFIG_STR(Name, ScriptName, Len, Def, Save, Desc) ;
#endif

//+KZ

MACRO_CONFIG_INT(SvPlasmaKills, sv_plasma_kills, 0, 0, 100, CFGFLAG_SERVER | CFGFLAG_GAME, "Plasma turret damage for player")
MACRO_CONFIG_INT(SvLightKills, sv_light_kills, 0, 0, 100, CFGFLAG_SERVER | CFGFLAG_GAME, "Rotating Laser damage for player")
MACRO_CONFIG_INT(SvDDraceShotgun, sv_ddrace_shotgun, 0, 0, 1, CFGFLAG_SERVER, "DDrace shotgun (0 = vanilla, 1 = ddrace)")
MACRO_CONFIG_INT(SvForceLaserType, sv_force_laser_type, 0, 0, 2, CFGFLAG_SERVER, "Laser behavior, overriding sv_oldlaser and tunes but no tune zones (0 = default, 1 = ddrace 2 = vanilla)")
MACRO_CONFIG_INT(SvEnableDDraceHUD, sv_ddrace_hud, 0, 0, 1, CFGFLAG_SERVER, "Toggle to enable ddrace HUD")
MACRO_CONFIG_INT(SvLaserJump, sv_laser_jump, 0, 0, 1, CFGFLAG_SERVER, "Create an explosion on first rifle bounce, allowing laser jumps.")

//For Vanilla
MACRO_CONFIG_INT(SvSpawnPickupWeapons, sv_spawn_pickup_weapons, 1, 0, 1, CFGFLAG_SERVER, "enable or disable weapons spawning (does not work on instagib)")
MACRO_CONFIG_INT(SvSpawnPickups, sv_spawn_pickups, 1, 0, 1, CFGFLAG_SERVER, "enable or disable heart and armor spawning (does not work on instagib)")

//For BOMB
MACRO_CONFIG_INT(SvBombTime, sv_bomb_time, 15, 0, 1, CFGFLAG_SERVER, "Time in seconds for bomb to explode")
MACRO_CONFIG_INT(SvBombAmount, sv_bomb_amount, 6, 0, 64, CFGFLAG_SERVER, "1 bomb for each specified amount of players (spawns at least 1 if there is less players)")
MACRO_CONFIG_INT(SvBombDamage, sv_bomb_damage, 0, 0, 1, CFGFLAG_SERVER, "Enable damage in BOMB")
MACRO_CONFIG_INT(SvBombWeapon, sv_bomb_weapon, 0, -1, 5, CFGFLAG_SERVER, "BOMB weapon")

//For iFreeze
MACRO_CONFIG_INT(SvFreezeAutomeltTime, sv_freeze_automelt_time, 30, 10, 120, CFGFLAG_SERVER, "Time until the player respawns automatically when he's frozen")
MACRO_CONFIG_INT(SvFreezeMeltRange, sv_freeze_melt_range, 100, 10, 1000, CFGFLAG_SERVER, "Maximum range to melt a player")
MACRO_CONFIG_INT(SvFreezeMeltTime, sv_freeze_melt_time, 1200, 500, 5000, CFGFLAG_SERVER, "Time (in ms) the player must stand next to a player to melt him")
MACRO_CONFIG_INT(SvFreezeMeltRespawn, sv_freeze_melt_respawn, 1, 0, 1, CFGFLAG_SERVER, "If a player respawns after he was being melted")

//For Ball
MACRO_CONFIG_INT(SvBallRespawn, sv_ball_respawn, 10, 1, 1000, CFGFLAG_SERVER, "Seconds for ball to go back to ball spawn")

MACRO_CONFIG_INT(SvKZBots, sv_kzbots, 0, 0, MAX_CLIENTS, CFGFLAG_SERVER, "Add KZ Bots")
MACRO_CONFIG_INT(SvKZBotsAI, sv_kzbots_ai, 0, 0, 99, CFGFLAG_SERVER, "KZ Bots AI (0 = +KZ AI, 1 = Pointer AI)")

#endif
