#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "fb.h"

CGameControllerInstaFB::CGameControllerInstaFB(class CGameContext *pGameServer) :
	CGameControllerInstagib(pGameServer)
{
	m_GameFlags = GAMEFLAG_TEAMS | GAMEFLAG_FLAGS;

	m_apFlagBalls[0] = 0;
	m_apFlagBalls[1] = 0;
	
	m_flagstand_temp_i_0 = 0;
	m_flagstand_temp_i_1 = 0;
}

CGameControllerInstaFB::~CGameControllerInstaFB() = default;

void CGameControllerInstaFB::Tick()
{
	CGameControllerInstagib::Tick();
	DoTeamScoreWincheck();

	for (int bi =  0; bi < 2; ++bi)
		{
			CFlagBall *b = m_apFlagBalls[bi];

			if (!b)
				continue;

			/*//	update position here and not in CBall::Tick because balls tend to tick before characters
			if (b->m_pCarryingCharacter)
				b->m_Pos = b->m_pCarryingCharacter->m_Pos;*/

			/*//	check/handle goal
			bool is_goal = false;
			for (int g = 0; g < m_NumGoals; g++)
			{
				if (distance(b->m_Pos, m_apGoalPos[g]) < g_Config.m_SvfbGoalsize)
				{
					is_goal = HandleGoal(b, g);
					break;
				}
			}
			if (is_goal)
				continue;*/

			if (b->m_pCarrier)
			{	//	ball is carried by player
				//	warn ball-carrying player once when he is near his own goal
				//CCharacter *ch = b->m_pCarryingCharacter;
				//CPlayer *p = ch->GetPlayer();
				//int t = p->GetTeam();
				/*if (g_Config.m_SvfbOwngoalWarn && !p->m_OwngoalWarned && t < m_NumGoals && distance(ch->m_Pos, m_apGoalPos[t]) < g_Config.m_SvfbGoalsize * 5)
				{
					GameServer()->SendBroadcast(
						"Attention! This is your team's goal\n"
						"you are approaching. You probably\n"
						"want to go to the other side!",
						p->m_ClientID);

					// play the server-chat sound
					GameServer()->CreateSound(ch->m_Pos, SOUND_CHAT_SERVER, CmaskOne(p->m_ClientID));
					p->m_OwngoalWarned = true;
				}*/
			}
			else
			{	//	ball is carried by none
				//	small delay for throwing ball a bit away before fetching again
				if (b->m_AtStand || Server()->Tick() >= b->m_DropTick + Server()->TickSpeed() * 0.2f)
				{
					//	find all players that can grab the ball
					CCharacter *close_characters[MAX_CLIENTS];
					int max_num = GameServer()->m_World.FindEntities(b->m_Pos, b->ms_PhysSize, (CEntity**)close_characters, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
					int curr_num = 0;
					for (int i = 0; i < max_num; ++i)
					{
						if (!close_characters[i]->IsAlive() || close_characters[i]->GetPlayer()->GetTeam() == -1 || GameServer()->Collision()->IntersectLine(b->m_Pos, close_characters[i]->m_Pos, NULL, NULL) ||
							// don't take flag if already have one (might be useful with two flags)
							m_apFlagBalls[bi ^ 1]->m_pCarrier == close_characters[i]) 
							continue;
						close_characters[curr_num++] = close_characters[i];
					}
					
					if (!curr_num)
						continue;

					b->Grab(close_characters[Server()->Tick() % curr_num]);
				}
			}
		}
}

bool CGameControllerInstaFB::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
	CGameControllerInstagib::OnEntity(Index, x, y, Layer, Flags, Initial, Number);

	const vec2 Pos(x * 32.0f + 16.0f, y * 32.0f + 16.0f);
	int Team = -1;
	if(Index == ENTITY_FLAGSTAND_RED)
		Team = TEAM_RED;
	if(Index == ENTITY_FLAGSTAND_BLUE)
		Team = TEAM_BLUE;
	
	//twplus begin +KZ
	if(!(Team == -1 || m_apFlagBalls[Team]))
	{
		CFlagBall *F = new CFlagBall(&GameServer()->m_World, Team);
		//F->m_StandPos = Pos;
		F->m_Pos = Pos;
		m_apFlagBalls[Team] = F;
		GameServer()->m_World.InsertEntity(F);
	}
	
	if (Team == TEAM_RED && m_flagstand_temp_i_0 < 10) {
		//m_flagstands_0[m_flagstand_temp_i_0] = Pos;
		m_apFlagBalls[Team]->m_StandPositions[m_flagstand_temp_i_0] = Pos;
		m_flagstand_temp_i_0++;
		m_apFlagBalls[Team]->m_no_stands = m_flagstand_temp_i_0;
	}
	if (Team == TEAM_BLUE && m_flagstand_temp_i_1 < 10) {
		//m_flagstands_1[m_flagstand_temp_i_1] = Pos;
		m_apFlagBalls[Team]->m_StandPositions[m_flagstand_temp_i_1] = Pos;
		m_flagstand_temp_i_1++;
		m_apFlagBalls[Team]->m_no_stands = m_flagstand_temp_i_1;
	}
	if (Team == -1)
	{
		return false;
	}
	
	//twplus end +KZ
	
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

void CGameControllerInstaFB::Snap(int SnappingClient)
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

void CGameControllerInstaFB::DoTeamScoreWincheck()
{
	if(m_GameOverTick == -1 && !m_Warmup)
	{
		// check score win condition
		if((g_Config.m_SvScorelimit > 0 && (m_aTeamscore[TEAM_RED] >= g_Config.m_SvScorelimit || m_aTeamscore[TEAM_BLUE] >= g_Config.m_SvScorelimit)) ||
			(g_Config.m_SvTimelimit > 0 && (Server()->Tick()-m_RoundStartTick) >= g_Config.m_SvTimelimit*Server()->TickSpeed()*60))
		{
			if(m_aTeamscore[TEAM_RED] != m_aTeamscore[TEAM_BLUE])
				EndRound();
			else
				m_SuddenDeath = 1;
		}
	}
}

int CGameControllerInstaFB::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
{
	CGameControllerPvp::OnCharacterDeath(pVictim, pKiller, WeaponId);
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

			if(pKiller && pKiller->GetTeam() != pVictim->GetPlayer()->GetTeam())
			{
				pKiller->IncrementScore();
				if(IsStatTrack())
					pKiller->m_Stats.m_FlaggerKills++;
			}

			HadFlag |= 1;
		}
		if(pFlag && pFlag->GetCarrier() == pVictim)
			pFlag->SetCarrier(0);
	}

	return HadFlag;
}

bool CGameControllerInstaFB::OnFireWeapon(CCharacter &Character, int &Weapon, vec2 &Direction, vec2 &MouseTarget, vec2 &ProjStartPos)
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
	return CGameControllerPvp::OnFireWeapon(Character, Weapon, Direction, MouseTarget, ProjStartPos);
}