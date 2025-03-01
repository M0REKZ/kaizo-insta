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
MACRO_CONFIG_INT(SvFlagLaserMomentum, sv_flag_laser_momentum, 0, -3000, 3000, CFGFLAG_SERVER, "If set, laser pushes flag with the force specified. (recommended value: 207)")
MACRO_CONFIG_INT(SvFlagProjectileMomentum, sv_flag_projectile_momentum, 0, -3000, 3000, CFGFLAG_SERVER, "If set, projectile pushes flag with the force specified. (recommended value: 207)")
MACRO_CONFIG_INT(SvFlagHookGrab, sv_flag_hook_grab, 0, 0, 1, CFGFLAG_SERVER, "Instantly grab flag when hooked")
MACRO_CONFIG_INT(SvRollback, sv_rollback, 0, 0, 1, CFGFLAG_SERVER, "Enables / Disables rollback support")
MACRO_CONFIG_INT(SvAntiAdbotPointer, sv_antiadbot_pointer, 1, 0, 3, CFGFLAG_SERVER, "Whether Pointer's AntiAdbot should be on")
MACRO_CONFIG_STR(SvChatDiscordWebhook, sv_chat_discord_webhook, 512, "", CFGFLAG_SERVER, "Where to send chat messages written by players")
MACRO_CONFIG_INT(SvBlackholeLife, sv_blackhole_life, 5, 0, 999, CFGFLAG_SERVER, "Blackhole lifetime")
MACRO_CONFIG_INT(SvDropWeapons, sv_drop_weapons, 0, 0, 1, CFGFLAG_SERVER, "Allow drop weapons")
MACRO_CONFIG_INT(SvMinesLife, sv_mines_life, 60, 0, 999, CFGFLAG_SERVER, "Mines lifetime")
MACRO_CONFIG_INT(SvPortalMode, sv_portal_mode, 0, 0, 2, CFGFLAG_SERVER | CFGFLAG_GAME, "Portal spawning behavior (0 = default, 1 = only on allow portal tile, 2 = pprace compatibility)")
MACRO_CONFIG_STR(SvRunWhenEmpty, sv_run_when_empty, 64, "", CFGFLAG_SERVER, "File to run when server is empty")
MACRO_CONFIG_INT(SvHammerPower, sv_hammer_power, 5, 1, 500, CFGFLAG_SERVER, "Hammer's power (For Hammer Power Powerup)")
MACRO_CONFIG_INT(SvAnticamperZoneTime, sv_anticamper_zone_time, 5, 5, 120, CFGFLAG_SERVER | CFGFLAG_GAME, "How long to wait till the player dies")
MACRO_CONFIG_INT(SvAnticamperZoneRange, sv_anticamper_zone_range, 600, 0, 1000, CFGFLAG_SERVER | CFGFLAG_GAME, "Distance how far away the player must move to escape anticamper")
MACRO_CONFIG_INT(SvSwapFlags, sv_swap_flags, 0, 0, 1, CFGFLAG_SERVER, "Swap flag positions")

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

//For Catch
MACRO_CONFIG_INT(SvCatchDamage, sv_catch_damage, 0, 0, 1, CFGFLAG_SERVER, "Enable damage in Catch64")
MACRO_CONFIG_INT(SvCatchWeapon, sv_catch_weapon, 0, -1, 5, CFGFLAG_SERVER, "Catch64 weapon")

//For Hide and Seek
MACRO_CONFIG_INT(SvHnSSeekerWeapon, sv_hns_seeker_weapon, 0, -1, 5, CFGFLAG_SERVER, "HidNSek Seeker weapon")
MACRO_CONFIG_INT(SvHnSHiderWeapon, sv_hns_hider_weapon, -1, -1, 5, CFGFLAG_SERVER, "HidNSek Hider weapon")
MACRO_CONFIG_INT(SvHnSSeekerAmount, sv_hns_seeker_amount, 1, 0, 63, CFGFLAG_SERVER, "Amount of Seekers")

MACRO_CONFIG_INT(SvKZBots, sv_kzbots, 0, 0, MAX_CLIENTS, CFGFLAG_SERVER, "Add KZ Bots")
MACRO_CONFIG_INT(SvKZBotsAI, sv_kzbots_ai, 0, 0, 99, CFGFLAG_SERVER, "KZ Bots AI (0 = +KZ AI, 1 = Pointer AI)")

#endif
