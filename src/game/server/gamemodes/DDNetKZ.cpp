/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
/* Based on Race mod stuff and tweaked by GreYFoX@GTi and others to fit our DDRace needs. */
#include "DDNetKZ.h"

#include <engine/server.h>
#include <engine/shared/config.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/entities/flag.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/version.h>

#define GAME_TYPE_NAME "DDraceNetwork"
#define TEST_TYPE_NAME "TestDDraceNetwork"

CGameControllerDDNetKZ::CGameControllerDDNetKZ(class CGameContext *pGameServer) :
	CGameControllerPvp(pGameServer)
{
	// game
	m_AllowSkinChange = true;
	// m_pGameType = g_Config.m_SvTestingCommands ? TEST_TYPE_NAME : GAME_TYPE_NAME;
	m_GameFlags = protocol7::GAMEFLAG_RACE;

	m_pGameType = g_Config.m_SvTestingCommands ? "TestDDNetᵏᶻ" : "DDNetᵏᶻ";
	m_DefaultWeapon = WEAPON_HAMMER;
	m_IsInstagibKZ = false;
	m_IsVanillaGameType = false;
	m_AllowBangCommands = false;

	m_apFlagBalls[0] = 0;
	m_apFlagBalls[1] = 0;

	m_flagstand_temp_i_0 = 0;
	m_flagstand_temp_i_1 = 0;

	GameServer()->Tuning()->Set("gun_curvature", 0);
	GameServer()->Tuning()->Set("gun_speed", 1400);
	GameServer()->Tuning()->Set("shotgun_curvature", 0);
	GameServer()->Tuning()->Set("shotgun_speed", 500);
	GameServer()->Tuning()->Set("shotgun_speeddiff", 0);
}

CGameControllerDDNetKZ::~CGameControllerDDNetKZ() = default;

void CGameControllerDDNetKZ::Tick()
{
	CGameControllerPvp::Tick();

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		
		if(!GameServer()->m_apPlayers[i])
			continue;
	
		OnPlayerTick(GameServer()->m_apPlayers[i]);

		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetCharacter())
		{
			if(GameServer()->GetPlayerChar(i)->m_Health <= 0)
			{
				GameServer()->GetPlayerChar(i)->Die(i, WEAPON_WORLD);
			}
		}
	}

	for(int bi = 0; bi < 2; ++bi)
	{
		CFlagBall *b = m_apFlagBalls[bi];

		if(!b)
			continue;

		if(!(b->m_pCarrier))
		{ //	ball is carried by none
			//	small delay for throwing ball a bit away before fetching again
			if(b->m_AtStand || Server()->Tick() >= b->m_DropTick + Server()->TickSpeed() * 0.2f)
			{
				//	find all players that can grab the ball
				CCharacter *close_characters[MAX_CLIENTS];
				int max_num = GameServer()->m_World.FindEntities(b->m_Pos, b->ms_PhysSize, (CEntity **)close_characters, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
				int curr_num = 0;
				CFlag* f = m_apFlagBalls[bi]->GetOtherFlag();
				for(int i = 0; i < max_num; ++i)
				{
					if(!close_characters[i]->IsAlive() || close_characters[i]->GetPlayer()->GetTeam() == -1 || GameServer()->Collision()->IntersectLine(b->m_Pos, close_characters[i]->m_Pos, NULL, NULL) ||
						// don't take flag if already have one (might be useful with two flags)
						(f && f->m_pCarrier == close_characters[i]))
						continue;
					close_characters[curr_num++] = close_characters[i];
				}

				if(!curr_num)
					continue;

				b->Grab(close_characters[Server()->Tick() % curr_num]);
			}
		}
	}
}

bool CGameControllerDDNetKZ::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
	CGameControllerPvp::OnEntity(Index, x, y, Layer, Flags, Initial, Number);

	const vec2 Pos(x * 32.0f + 16.0f, y * 32.0f + 16.0f);
	int Team = -1;
	if(Index == ENTITY_FLAGSTAND_RED)
		Team = TEAM_RED;
	if(Index == ENTITY_FLAGSTAND_BLUE)
		Team = TEAM_BLUE;

	// twplus begin +KZ
	if(!(Team == -1 || m_apFlagBalls[Team]))
	{
		CFlagBall *F = new CFlagBall(&GameServer()->m_World, Team);
		// F->m_StandPos = Pos;
		F->m_Pos = Pos;
		m_apFlagBalls[Team] = F;
		GameServer()->m_World.InsertEntity(F);
	}

	if(Team == TEAM_RED && m_flagstand_temp_i_0 < 10)
	{
		// m_flagstands_0[m_flagstand_temp_i_0] = Pos;
		m_apFlagBalls[Team]->m_StandPositions[m_flagstand_temp_i_0] = Pos;
		m_flagstand_temp_i_0++;
		m_apFlagBalls[Team]->m_no_stands = m_flagstand_temp_i_0;
	}
	if(Team == TEAM_BLUE && m_flagstand_temp_i_1 < 10)
	{
		// m_flagstands_1[m_flagstand_temp_i_1] = Pos;
		m_apFlagBalls[Team]->m_StandPositions[m_flagstand_temp_i_1] = Pos;
		m_flagstand_temp_i_1++;
		m_apFlagBalls[Team]->m_no_stands = m_flagstand_temp_i_1;
	}
	if(Team == -1)
	{
		return false;
	}

	// twplus end +KZ

	/*
	if(Team == -1 || m_apFlags[Team])
		return false;

	CFlag *F = new CFlag(&GameServer()->m_World, Team);
	F->m_StandPos = Pos;
	F->m_Pos = Pos;
	m_apFlags[Team] = F;
	GameServer()->m_World.InsertEntity(F);
	 */
	return true;
}

void CGameControllerDDNetKZ::Snap(int SnappingClient)
{
	CGameControllerPvp::Snap(SnappingClient);

	bool invert = false;

	int FlagCarrierRed = FLAG_MISSING;
	if(m_apFlagBalls[TEAM_RED])
	{
		if(m_apFlagBalls[TEAM_RED]->m_AtStand)
			FlagCarrierRed = FLAG_ATSTAND;
		else if(m_apFlagBalls[TEAM_RED]->GetCarrier() && m_apFlagBalls[TEAM_RED]->GetCarrier()->GetPlayer())
			FlagCarrierRed = m_apFlagBalls[TEAM_RED]->GetCarrier()->GetPlayer()->GetCid();
		else
			FlagCarrierRed = FLAG_TAKEN;
	}

	int FlagCarrierBlue = FLAG_MISSING;
	if(m_apFlagBalls[TEAM_BLUE])
	{
		if(m_apFlagBalls[TEAM_BLUE]->m_AtStand)
			FlagCarrierBlue = FLAG_ATSTAND;
		else if(m_apFlagBalls[TEAM_BLUE]->GetCarrier() && m_apFlagBalls[TEAM_BLUE]->GetCarrier()->GetPlayer())
			FlagCarrierBlue = m_apFlagBalls[TEAM_BLUE]->GetCarrier()->GetPlayer()->GetCid();
		else
			FlagCarrierBlue = FLAG_TAKEN;
	}

	if((m_apFlagBalls[TEAM_RED] && FlagCarrierRed == SnappingClient && m_apFlagBalls[TEAM_RED]->m_Team != TEAM_RED) || (m_apFlagBalls[TEAM_BLUE] && FlagCarrierBlue == SnappingClient && m_apFlagBalls[TEAM_BLUE]->m_Team != TEAM_BLUE))
		invert = true;

	if((m_apFlagBalls[TEAM_BLUE] && m_apFlagBalls[TEAM_BLUE]->GetCarrier() && m_apFlagBalls[TEAM_BLUE]->GetCarrier()->GetPlayer()->GetTeam() == TEAM_RED) || (m_apFlagBalls[TEAM_RED] && m_apFlagBalls[TEAM_RED]->GetCarrier() && m_apFlagBalls[TEAM_RED]->GetCarrier()->GetPlayer()->GetTeam() == TEAM_BLUE))
		invert = true;

	if(m_apFlagBalls[TEAM_BLUE] && m_apFlagBalls[TEAM_BLUE]->GetCarrier() && m_apFlagBalls[TEAM_RED] && m_apFlagBalls[TEAM_RED]->GetCarrier() && m_apFlagBalls[TEAM_BLUE]->GetCarrier()->GetPlayer()->GetTeam() == m_apFlagBalls[TEAM_RED]->GetCarrier()->GetPlayer()->GetTeam())
	{
		if(m_apFlagBalls[TEAM_BLUE]->GetCarrier()->GetPlayer()->GetCid() == SnappingClient && m_apFlagBalls[TEAM_BLUE]->GetCarrier()->GetPlayer()->GetTeam() == TEAM_RED)
			invert = true;
		else if(m_apFlagBalls[TEAM_RED]->GetCarrier()->GetPlayer()->GetCid() == SnappingClient && m_apFlagBalls[TEAM_RED]->GetCarrier()->GetPlayer()->GetTeam() == TEAM_BLUE)
			invert = true;
		else
			invert = false;
	}

	if(Server()->IsSixup(SnappingClient))
	{
		protocol7::CNetObj_GameDataFlag *pGameDataObj = Server()->SnapNewItem<protocol7::CNetObj_GameDataFlag>(0);
		if(!pGameDataObj)
			return;

		if(invert)
		{
			pGameDataObj->m_FlagCarrierRed = FlagCarrierBlue;
			pGameDataObj->m_FlagCarrierBlue = FlagCarrierRed;
		}
		else
		{
			pGameDataObj->m_FlagCarrierRed = FlagCarrierRed;
			pGameDataObj->m_FlagCarrierBlue = FlagCarrierBlue;
		}
	}
	else
	{
		CNetObj_GameData *pGameDataObj = Server()->SnapNewItem<CNetObj_GameData>(0);
		if(!pGameDataObj)
			return;

		if(invert)
		{
			pGameDataObj->m_FlagCarrierRed = FlagCarrierBlue;
			pGameDataObj->m_FlagCarrierBlue = FlagCarrierRed;
		}
		else
		{
			pGameDataObj->m_FlagCarrierRed = FlagCarrierRed;
			pGameDataObj->m_FlagCarrierBlue = FlagCarrierBlue;
		}

		pGameDataObj->m_TeamscoreRed = m_aTeamscore[TEAM_RED];
		pGameDataObj->m_TeamscoreBlue = m_aTeamscore[TEAM_BLUE];
	}
}

bool CGameControllerDDNetKZ::OnFireWeapon(CCharacter &Character, int &Weapon, vec2 &Direction, vec2 &MouseTarget, vec2 &ProjStartPos)
{
	for(CFlagBall *pFlag : m_apFlagBalls)
	{
		if(!pFlag)
			continue;
			
		if(&Character == pFlag->m_pCarrier)
		{
			pFlag->Drop(Direction);
			Character.m_ReloadTimer = 1 * Server()->TickSpeed();
			Character.m_DropFlagBallTicks = Character.m_ReloadTimer;
			return true;
		}
	}
	return CGameControllerDDRace::OnFireWeapon(Character, Weapon, Direction, MouseTarget, ProjStartPos);
}

int CGameControllerDDNetKZ::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
{
	CGameControllerDDRace::OnCharacterDeath(pVictim, pKiller, WeaponId);
	int HadFlag = 0;

	// drop flags
	for(CFlagBall *pFlag : m_apFlagBalls)
	{
		if(pFlag && pKiller && pKiller->GetCharacter() && pFlag->GetCarrier() == pKiller->GetCharacter())
			HadFlag |= 2;
		if(pFlag && pFlag->GetCarrier() == pVictim)
		{
			GameServer()->CreateSoundGlobal(SOUND_CTF_DROP);
			GameServer()->SendGameMsg(protocol7::GAMEMSG_CTF_DROP, -1);
			pFlag->Drop();
			// https://github.com/ddnet-insta/ddnet-insta/issues/156
			pFlag->m_pLastCarrier = nullptr;
			
			HadFlag |= 1;
		}
		if(pFlag && pFlag->GetCarrier() == pVictim)
			pFlag->SetCarrier(0);
	}

	return HadFlag;
}

void CGameControllerDDNetKZ::OnPlayerDisconnect(CPlayer *pPlayer, const char *pReason)
{
	if(GameState() != IGS_END_ROUND)
		SaveStatsOnDisconnect(pPlayer);

	m_InvalidateConnectedIpsCache = true;
	
	int ClientId = pPlayer->GetCid();
	if(Server()->ClientIngame(ClientId))
	{
		char aBuf[512];
		if(pPlayer->m_RageQuitTick + Server()->TickSpeed() * 5 > Server()->Tick())
		{
			if(pReason && *pReason)
				str_format(aBuf, sizeof(aBuf), "'%s' rage quitted (%s)", Server()->ClientName(ClientId), pReason);
			else
				str_format(aBuf, sizeof(aBuf), "'%s' rage quitted", Server()->ClientName(ClientId));
		}
		else
		{
		if(pReason && *pReason)
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game (%s)", Server()->ClientName(ClientId), pReason);
		else
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game", Server()->ClientName(ClientId));
		}
		if(!g_Config.m_SvTournamentJoinMsgs || pPlayer->GetTeam() != TEAM_SPECTATORS)
			GameServer()->SendChat(-1, TEAM_ALL, aBuf, -1, CGameContext::FLAG_SIX);
		else if(g_Config.m_SvTournamentJoinMsgs == 2)
			SendChatSpectators(aBuf, CGameContext::FLAG_SIX);

		str_format(aBuf, sizeof(aBuf), "leave player='%d:%s'", ClientId, Server()->ClientName(ClientId));
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
	}

	// ddnet-insta
	if(pPlayer->GetTeam() != TEAM_SPECTATORS)
	{
		--m_aTeamSize[pPlayer->GetTeam()];
	}

	bool WasModerator = pPlayer->m_Moderating && Server()->ClientIngame(ClientId);

	//IGameController::OnPlayerDisconnect(pPlayer, pReason);

	if(!GameServer()->PlayerModerating() && WasModerator)
		GameServer()->SendChat(-1, TEAM_ALL, "Server kick/spec votes are no longer actively moderated.");

	if(g_Config.m_SvTeam != SV_TEAM_FORCED_SOLO)
		Teams().SetForceCharacterTeam(ClientId, TEAM_FLOCK);

	for(int Team = TEAM_FLOCK + 1; Team < TEAM_SUPER; Team++)
		if(Teams().IsInvited(Team, ClientId))
			Teams().SetClientInvited(Team, ClientId, false);
}

int CGameControllerDDNetKZ::SnapGameInfoExFlags(int SnappingClient, int DDRaceFlags)
{
	int Flags =
		//GAMEINFOFLAG_PREDICT_VANILLA | // ddnet-insta
		//GAMEINFOFLAG_ENTITIES_VANILLA | // ddnet-insta
		GAMEINFOFLAG_BUG_VANILLA_BOUNCE | // ddnet-insta
		//GAMEINFOFLAG_GAMETYPE_VANILLA | // ddnet-insta
		GAMEINFOFLAG_TIMESCORE |
		//GAMEINFOFLAG_GAMETYPE_RACE |
		//GAMEINFOFLAG_GAMETYPE_DDRACE |
		GAMEINFOFLAG_GAMETYPE_DDNET |
		GAMEINFOFLAG_UNLIMITED_AMMO |
		GAMEINFOFLAG_RACE_RECORD_MESSAGE |
		GAMEINFOFLAG_ALLOW_EYE_WHEEL |
		GAMEINFOFLAG_ALLOW_HOOK_COLL |
		GAMEINFOFLAG_ALLOW_ZOOM |
		GAMEINFOFLAG_BUG_DDRACE_GHOST |
		/* GAMEINFOFLAG_BUG_DDRACE_INPUT | */ // https://github.com/ddnet-insta/ddnet-insta/issues/161
		//GAMEINFOFLAG_PREDICT_DDRACE |
		GAMEINFOFLAG_PREDICT_DDRACE_TILES |
		GAMEINFOFLAG_ENTITIES_DDNET |
		GAMEINFOFLAG_ENTITIES_DDRACE |
		GAMEINFOFLAG_ENTITIES_RACE |
		GAMEINFOFLAG_RACE;
	//if(!g_Config.m_SvAllowZoom) //allow zoom always in ddrace -> +KZ
	//	Flags &= ~(GAMEINFOFLAG_ALLOW_ZOOM);

	// ddnet clients do not predict sv_old_laser correctly
	// https://github.com/ddnet/ddnet/issues/7589
	if(g_Config.m_SvOldLaser)
		Flags &= ~(GAMEINFOFLAG_PREDICT_DDRACE);

	return Flags;
}

void CGameControllerDDNetKZ::OnCharacterSpawn(class CCharacter *pChr)
{
	pChr->SetTeams(&Teams());
	Teams().OnCharacterSpawn(pChr->GetPlayer()->GetCid());

	// default health
	pChr->IncreaseHealth(10);

	// give default weapons
	pChr->GiveWeapon(WEAPON_HAMMER);
	pChr->GiveWeapon(WEAPON_GUN);

	pChr->SetActiveWeapon(WEAPON_HAMMER);
}

void CGameControllerDDNetKZ::InitPlayer(CPlayer *pPlayer)
{
	pPlayer->m_Spree = 0;
	pPlayer->m_UntrackedSpree = 0;
	pPlayer->ResetStats();
	pPlayer->m_SavedStats.Reset();

	pPlayer->m_IsReadyToPlay = !GameServer()->m_pController->IsPlayerReadyMode();
	pPlayer->m_DeadSpecMode = false;
	pPlayer->m_GameStateBroadcast = false;
	//pPlayer->m_Score = 0; // ddnet-insta
	//pPlayer->m_DisplayScore = GameServer()->m_DisplayScore;
	pPlayer->m_JoinTime = time_get();

	RoundInitPlayer(pPlayer);
}