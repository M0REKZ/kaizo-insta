/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "flagball.h"

#include <game/generated/protocol.h>
#include <game/mapitems.h>
#include <game/teamscore.h>

#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include <engine/shared/config.h>
//dirty:
#include <game/server/gamecontroller.h>

#include <game/server/entities/character.h>
#include <game/kztiles.h>

CFlagBall::CFlagBall(CGameWorld *pGameWorld, int Team)
: CFlag(pGameWorld, Team)
{
	//m_StandPos = Pos;
	//m_Pos = Pos;
	//m_FlagSnapOffset = Server()->m_pController->m_FlagSnapTeamOffset;
	//Server()->m_pController->m_FlagSnapTeamOffset++;
}

void CFlagBall::Reset()
{
	CFlag::Reset();
//	m_LastCarrier = -1;
	m_DropTick = 0;
	//m_IdleTick = -1;
	//m_LastCarrierTeam = !m_Team;
}

void CFlagBall::Tick()
{
	if (!m_pCarrier && !m_AtStand)
	{
		if(Server()->Tick() > m_DropTick + Server()->TickSpeed() * 30)
		{
			Reset();
			GameServer()->m_pController->OnFlagReturn(this);
		}
		else
		{
			
			if (GameLayerClipped(m_Pos))
			{
				Reset();
				GameServer()->CreateSoundGlobal(SOUND_CTF_RETURN);
			}
			else
			{
				// do ball physics
				m_Vel.y += GameServer()->Tuning()->m_Gravity;
				if (m_Vel.x > -0.1f && m_Vel.x < 0.1f)
					m_Vel.x = 0.0f;
				else if (m_Vel.x < 0)
					m_Vel.x += 0.055f;
				else
					m_Vel.x -= 0.055f;

				//float vy = m_Vel.y;
				GameServer()->Collision()->MoveBox(&m_Pos, &m_Vel, vec2(ms_PhysSize, ms_PhysSize), vec2(0.5f,0.5f));
				/*if (m_Vel.x == 0 && (vy > 0) != (m_Vel.y > 0) && abs(m_Vel.y) < GameServer()->Tuning()->m_Gravity)
				{
					if (m_IdleTick == -1)
						m_IdleTick = Server()->Tick();
				}
				else
					m_IdleTick = -1;*/
			}
		}
		
	}

	HandleKZTiles();
}

void CFlagBall::TickDeferred()
{
}

void CFlagBall::Grab(class CCharacter *pChar)
{
	CFlag::Grab(pChar);
	pChar->m_HasFlagBall = true;
	m_Team = pChar->GetPlayer()->GetTeam();
}

void CFlagBall::Drop(vec2 Direction)
{
	if(m_pCarrier)
	{
		m_pCarrier->m_HasFlagBall = false;
		m_pLastCarrier = m_pCarrier;
		m_pCarrier = 0;
	}

	m_Vel = Direction * 20.7f;
	m_DropTick = Server()->Tick();
}

void CFlagBall::HandleKZTiles()
{
	CFlag::HandleKZTiles();

		if((Collision()->KZFound()) && GameServer()->m_pController->IsTeamplay())
	{
		
		int TileIndex = Collision()->GetKZTileIndex(m_Pos);
		
		if(TileIndex == TILE_BALL_REDGOAL || TileIndex == TILE_BALL_REDSLAM)
		{
			GameServer()->m_pController->m_aTeamscore[TEAM_BLUE]+= 100;
			Reset();
			GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE);
		}
		else if(TileIndex == TILE_BALL_BLUEGOAL || TileIndex == TILE_BALL_BLUESLAM)
		{
			GameServer()->m_pController->m_aTeamscore[TEAM_RED]+= 100;
			Reset();
			GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE);
		}
		else if(TileIndex == TILE_NO_BALL)
		{
			Reset();
			//GameServer()->CreateSound(CurPosition, m_SoundImpact);
		}
		else if(m_pLastCarrier && (TileIndex == TILE_BALL_NOTEAMGOAL || TileIndex == TILE_BALL_NOTEAMSLAM))
		{
			GameServer()->m_pController->m_aTeamscore[m_pLastCarrier->GetPlayer()->GetTeam()]+= 100;
			Reset();
			GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE);
		}
	}
}