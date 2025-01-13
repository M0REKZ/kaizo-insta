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
	CGameControllerDDRace(pGameServer)
{
	// game
	m_AllowSkinChange = true;
	// m_pGameType = g_Config.m_SvTestingCommands ? TEST_TYPE_NAME : GAME_TYPE_NAME;
	m_GameFlags = protocol7::GAMEFLAG_RACE;

	m_pGameType = g_Config.m_SvTestingCommands ? "TestDDNetᵏᶻ" : "DDNetᵏᶻ";
	m_DefaultWeapon = WEAPON_HAMMER;
	m_IsInstagibKZ = false;
	m_IsVanillaGameType = false;

	m_apFlagBalls[0] = 0;
	m_apFlagBalls[1] = 0;

	m_flagstand_temp_i_0 = 0;
	m_flagstand_temp_i_1 = 0;
}

CGameControllerDDNetKZ::~CGameControllerDDNetKZ() = default;

void CGameControllerDDNetKZ::Tick()
{
	CGameControllerDDRace::Tick();

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
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
				for(int i = 0; i < max_num; ++i)
				{
					if(!close_characters[i]->IsAlive() || close_characters[i]->GetPlayer()->GetTeam() == -1 || GameServer()->Collision()->IntersectLine(b->m_Pos, close_characters[i]->m_Pos, NULL, NULL) ||
						// don't take flag if already have one (might be useful with two flags)
						m_apFlagBalls[bi ^ 1]->m_pCarrier == close_characters[i])
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

int CGameControllerDDNetKZ::SnapGameInfoExFlags2(int SnappingClient, int DDRaceFlags)
{
	return GAMEINFOFLAG2_HUD_AMMO | GAMEINFOFLAG2_HUD_HEALTH_ARMOR;
}

bool CGameControllerDDNetKZ::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
	CGameControllerDDRace::OnEntity(Index, x, y, Layer, Flags, Initial, Number);

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
	CGameControllerDDRace::Snap(SnappingClient);

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