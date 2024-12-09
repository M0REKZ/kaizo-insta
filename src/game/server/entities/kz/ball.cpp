/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "ball.h"

#include <game/server/entities/character.h>

#include <game/generated/protocol.h>
#include <game/mapitems.h>
#include <game/teamscore.h>

#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include <engine/shared/config.h>
//dirty:
#include <game/server/gamecontroller.h>
#include <game/server/entities/ddnet_pvp/vanilla_projectile.h>
#include <game/server/entities/projectile.h>

#include <game/kztiles.h>

CBall::CBall(CGameWorld *pGameWorld, int Owner, vec2 Pos, vec2 Dir) :
CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{
	
	//m_Type = Type;
	m_Pos = Pos;
	m_Direction = Dir;
	m_Owner = Owner;
	m_SoundImpact = SOUND_HOOK_ATTACH_GROUND;
	
	m_StartTick = Server()->Tick();
	m_StartPos = m_Pos;
	
	if((Dir.x < 0?-Dir.x:Dir.x) > (Dir.y < 0?-Dir.y:Dir.y))
		m_FootPickupDistance = std::abs(Dir.x * (float)Server()->TickSpeed() * GameServer()->Tuning()->m_GrenadeSpeed / 4000.0);
	else
		m_FootPickupDistance = std::abs(Dir.y * (float)Server()->TickSpeed() * GameServer()->Tuning()->m_GrenadeSpeed / 4000.0);
	
	if((m_Owner < 0) || (m_Owner >= MAX_CLIENTS) || !(GameServer()->m_apPlayers[m_Owner]))
	{
		m_Owner = -1;
		m_Team = -1;
	}
	else
	{
		m_RespawnTick = g_Config.m_SvBallRespawn * Server()->TickSpeed();
		m_Team = GameServer()->m_apPlayers[m_Owner]->GetTeam();
	}
	
	GameWorld()->InsertEntity(this);
}

void CBall::Tick()
{
	if(m_Owner >= 0)
	{
		if(m_RespawnTick)
		{
			m_RespawnTick--;
		}
		else
		{
			GoToStartPos();
			m_RespawnTick = g_Config.m_SvBallRespawn * Server()->TickSpeed();
		}
	}
	
	float PreviousTick = (Server()->Tick()-m_StartTick-1)/(float)Server()->TickSpeed();
	float CurrentTick = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	float NextTick = (Server()->Tick()-m_StartTick+1)/(float)Server()->TickSpeed();
	
	//vec2 NextPosition = GetPos(NextTick);
	vec2 CurPosition = GetPos(CurrentTick);
	vec2 PrevPosition = GetPos(PreviousTick);
	vec2 CollisionPosition = vec2(0,0);
	vec2 FreePosition = vec2(0,0);
	
	CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	//CCharacter *TargetChr = GameServer()->m_World.IntersectCharacter(PrevPosition, CurPosition, 6.0f, CurPosition, OwnerChar);
	CCharacter *TChar;
	
	float TimeToCollision = -1.0f;
	for(float SearchTick1 = CurrentTick; SearchTick1 <= NextTick; SearchTick1 += (NextTick-CurrentTick)/30.0f)
	{
		vec2 TempPosition = GetPos(SearchTick1);
		if(GameServer()->Collision()->IsSolid((int) TempPosition.x, (int) TempPosition.y))
		{
			break;
		}
		TimeToCollision = SearchTick1;
	}
	if(TimeToCollision == -1.0f)
	{
		m_FootPickupDistance = 0;
		for(float SearchTick2 = CurrentTick; SearchTick2 > CurrentTick-1.0f; SearchTick2-=0.02f)
		{
			vec2 SearchPosition = GetPos(SearchTick2);
			if(!GameServer()->Collision()->IsSolid((int)SearchPosition.x, (int)SearchPosition.y))
			{
				TimeToCollision = SearchTick2;
				CollisionPosition = GetPos(SearchTick2+0.02f);
				FreePosition = GetPos(SearchTick2);
				break;
			}
		}
	}
	else
	{
		TimeToCollision += CurrentTick;
		CollisionPosition = GetPos(TimeToCollision+(NextTick-CurrentTick)/30.0f);
		FreePosition = GetPos(TimeToCollision);
	}
	if(TimeToCollision < NextTick-(NextTick-CurrentTick)/30.0f)
	{
		bool CollidedAtX = false;
		bool CollidedAtY = false;
		if(GameServer()->Collision()->IsSolid((int)FreePosition.x, (int)CollisionPosition.y))
		{
			CollidedAtY = true;
		}
		if(GameServer()->Collision()->IsSolid((int)CollisionPosition.x, (int)FreePosition.y))
		{
			CollidedAtX = true;
		}
		if(CollidedAtX)
		{
			m_Direction.x = -m_Direction.x/(50+100)*100;
			/*if (m_CollisionsByX >= 50)
			{
				Reset();
				GameServer()->CreateSound(CurPosition, m_SoundImpact);
				
			}
			m_CollisionsByX++;*/
		}
		else
		{
			m_Direction.x = m_Direction.x/(50+100)*100;
			//m_CollisionsByX = 0;
		}
		if(CollidedAtY)
		{
			m_Direction.y = -(m_Direction.y + 2*GameServer()->Tuning()->m_GrenadeCurvature/10000*GameServer()->Tuning()->m_GrenadeSpeed*(Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed())/(50+100)*100;
			/*if (m_CollisionByY >= 50)
			{
				Reset();
				GameServer()->CreateSound(CurPosition, m_SoundImpact);
				
			}
			m_CollisionByY = m_CollisionByY + 1;*/
		}
		else
		{
			m_Direction.y = (m_Direction.y + 2*GameServer()->Tuning()->m_GrenadeCurvature/10000*(Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeSpeed)/(50+100)*100;
			//m_CollisionByY = 0;
		}
		
		m_Pos = FreePosition;
		m_StartTick = Server()->Tick();
		m_FootPickupDistance = 0;
	}
	
	// ball hits death-tile or left the game layer, reset it
	if((GameServer()->Collision()->GetCollisionAt(m_Pos.x, m_Pos.y) == TILE_DEATH) || (GameServer()->Collision()->GetFrontCollisionAt(m_Pos.x, m_Pos.y) == TILE_DEATH) || GameLayerClipped(m_Pos))
	{
		GoToStartPos();
		GameServer()->CreateSound(CurPosition, m_SoundImpact);
	}
	
	if((Collision()->KZFound()) && GameServer()->m_pController->IsTeamplay())
	{
		
		int TileIndex = Collision()->GetKZTileIndex(CurPosition);
		
		if(TileIndex == TILE_BALL_REDGOAL || TileIndex == TILE_BALL_REDSLAM)
		{
			GameServer()->m_pController->m_aTeamscore[TEAM_BLUE]+= 100;
			GoToStartPos();
			GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE);
		}
		else if(TileIndex == TILE_BALL_BLUEGOAL || TileIndex == TILE_BALL_BLUESLAM)
		{
			GameServer()->m_pController->m_aTeamscore[TEAM_RED]+= 100;
			GoToStartPos();
			GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE);
		}
		else if(TileIndex == TILE_NO_BALL)
		{
			GoToStartPos();
			GameServer()->CreateSound(CurPosition, m_SoundImpact);
		}
		else if(m_Owner >=0 && (TileIndex == TILE_BALL_NOTEAMGOAL || TileIndex == TILE_BALL_NOTEAMSLAM))
		{
			GameServer()->m_pController->m_aTeamscore[m_Team]+= 100;
			GoToStartPos();
			GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE);
		}
	}
	
	if(m_FootPickupDistance == 0)
	{
		TChar = GameServer()->m_World.IntersectCharacter(PrevPosition, CurPosition, 6.0f, CurPosition, NULL);
	}
	else
	{
		m_FootPickupDistance--;
		TChar = GameServer()->m_World.IntersectCharacter(PrevPosition, CurPosition, 6.0f, CurPosition, OwnerChar);
	}
	if(TChar && !(TChar->m_HasBall))
	{
		//Char catch the ball
		TChar->CatchBall();
		GameServer()->CreateSound(CurPosition, m_SoundImpact);
		Reset();
	}
	
	
}


void CBall::Snap(int SnappingClient)
{
	
	if(NetworkClipped(SnappingClient))
		return;
	
	CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, GetId(), sizeof(CNetObj_Projectile)));
	if(!pProj)
		return;
	
	pProj->m_X = (int)m_Pos.x;
	pProj->m_Y = (int)m_Pos.y;
	pProj->m_VelX = (int)(m_Direction.x*100.0f);
	pProj->m_VelY = (int)(m_Direction.y*100.0f);
	pProj->m_StartTick = m_StartTick;
	pProj->m_Type = WEAPON_GRENADE;
}

void CBall::Reset()
{
	m_MarkedForDestroy = true;
}

vec2 CBall::GetPos(float Time)
{
	float Curvature = GameServer()->Tuning()->m_GrenadeCurvature;
	float Speed = GameServer()->Tuning()->m_GrenadeSpeed;

	return CalcPos(m_Pos, m_Direction, Curvature, Speed, Time);
}

void CBall::GoToStartPos()
{
	int n = rand() % GameServer()->m_pController->m_BallSpawnNum;
	m_Pos = GameServer()->m_pController->m_BallSpawnsKZ[n];
	m_Direction = vec2(0,0);
	m_Owner = -1;
	m_RespawnTick = g_Config.m_SvBallRespawn * Server()->TickSpeed();
	m_StartTick = Server()->Tick();

}
