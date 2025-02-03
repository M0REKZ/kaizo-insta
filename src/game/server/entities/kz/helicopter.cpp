/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <game/generated/protocol.h>
#include <game/mapitems.h>
#include <game/teamscore.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/gamecontroller.h>

#include "helicopter.h"
#include "minigun_projectile.h"
#include <game/kztiles.h>

CHelicopter::CHelicopter(CGameWorld *pGameWorld, vec2 Pos, int Owner, int Layer) :
CVehicle(pGameWorld, Pos, Owner, Layer)
{
	m_ObjType = CGameWorld::CUSTOM_ENTTYPE_HELICOPTER;

	m_Vel = vec2(0,0);
	m_Size = vec2(64,64);
	m_MountOffset = vec2(0,0);
	m_pMounter = nullptr;
	m_MountDistance = 64.f;

	for(int i = 0;i<11;i++)
	{
		m_ExtraLaserIds[i] = Server()->SnapNewId();
	}

	GameWorld()->InsertEntity(this);

	if(m_Owner != -1 && !GameServer()->GetPlayerChar(m_Owner))
	{
		Reset();
	}
}

CHelicopter::~CHelicopter()
{
	for(int i = 0;i<11;i++)
	{
		Server()->SnapFreeId(m_ExtraLaserIds[i]);
	}
}

void CHelicopter::Tick()
{
	m_Vel.x = 0.f;
	m_Vel.y = Tuning()->m_Gravity/2;

	CVehicle::Tick();

	if(m_Vel.x > 0)
		m_Direction = true;
	else if(m_Vel.x < 0)
		m_Direction = false;
}

void CHelicopter::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	int SnappingClientVersion = GameServer()->GetClientVersion(SnappingClient);
	bool Sixup = Server()->IsSixup(SnappingClient);

	//draw helicopter
	//GameServer()->SnapLaserObject(CSnapContext(SnappingClientVersion, Sixup),ID,TO,FROM,Server()->Tick(),m_Owner,LASERTYPE_DOOR);
	GameServer()->SnapLaserObject(CSnapContext(SnappingClientVersion, Sixup),GetId(),vec2(m_Pos.x - m_Size.x/2,m_Pos.y + m_Size.y/2),vec2(m_Pos.x + m_Size.x/2,m_Pos.y + m_Size.y/2),Server()->Tick(),m_Owner,LASERTYPE_DOOR);
	GameServer()->SnapLaserObject(CSnapContext(SnappingClientVersion, Sixup),m_ExtraLaserIds[0],vec2(m_Pos.x - m_Size.x/2,m_Pos.y + m_Size.y/2),vec2(m_Pos.x - (m_Size.x/2 + 16),m_Pos.y),Server()->Tick(),m_Owner,LASERTYPE_DOOR);
	GameServer()->SnapLaserObject(CSnapContext(SnappingClientVersion, Sixup),m_ExtraLaserIds[1],vec2(m_Pos.x + m_Size.x/2,m_Pos.y + m_Size.y/2),vec2(m_Pos.x + (m_Size.x/2 + 16),m_Pos.y),Server()->Tick(),m_Owner,LASERTYPE_DOOR);

	GameServer()->SnapLaserObject(CSnapContext(SnappingClientVersion, Sixup),m_ExtraLaserIds[2],vec2(m_Pos.x - m_Size.x/2,m_Pos.y - m_Size.y/2),vec2(m_Pos.x + m_Size.x/2,m_Pos.y - m_Size.y/2),Server()->Tick(),m_Owner,LASERTYPE_DOOR);
	GameServer()->SnapLaserObject(CSnapContext(SnappingClientVersion, Sixup),m_ExtraLaserIds[3],vec2(m_Pos.x - m_Size.x/2,m_Pos.y - m_Size.y/2),vec2(m_Pos.x - (m_Size.x/2 + 16),m_Pos.y),Server()->Tick(),m_Owner,LASERTYPE_DOOR);
	GameServer()->SnapLaserObject(CSnapContext(SnappingClientVersion, Sixup),m_ExtraLaserIds[4],vec2(m_Pos.x + m_Size.x/2,m_Pos.y - m_Size.y/2),vec2(m_Pos.x + (m_Size.x/2 + 16),m_Pos.y),Server()->Tick(),m_Owner,LASERTYPE_DOOR);

	GameServer()->SnapLaserObject(CSnapContext(SnappingClientVersion, Sixup),m_ExtraLaserIds[5],vec2(m_Pos.x,m_Pos.y - (m_Size.y/2 + 20)),vec2(m_Pos.x,m_Pos.y - m_Size.y/2),Server()->Tick(),m_Owner,LASERTYPE_DOOR);
	GameServer()->SnapLaserObject(CSnapContext(SnappingClientVersion, Sixup),m_ExtraLaserIds[6],vec2(m_Pos.x,m_Pos.y - (m_Size.y/2 + 20)),vec2(m_Pos.x - (m_Size.x/2 + 16),m_Pos.y - (m_Size.y/2 + 20)),Server()->Tick(),m_Owner,LASERTYPE_DOOR);
	GameServer()->SnapLaserObject(CSnapContext(SnappingClientVersion, Sixup),m_ExtraLaserIds[7],vec2(m_Pos.x,m_Pos.y - (m_Size.y/2 + 20)),vec2(m_Pos.x + (m_Size.x/2 + 16),m_Pos.y - (m_Size.y/2 + 20)),Server()->Tick(),m_Owner,LASERTYPE_DOOR);

	vec2 postemp;
	vec2 postemp2;
					
	postemp.x = 32*sin((float)Server()->Tick() / 25.0);
	postemp.y = 32*cos((float)Server()->Tick() / 25.0);

	postemp2.x = 32*sin(((float)Server()->Tick() / 25.0) + 3.14159);
	postemp2.y = 32*cos(((float)Server()->Tick() / 25.0) + 3.14159);

	if(m_Direction)
	{
		GameServer()->SnapLaserObject(CSnapContext(SnappingClientVersion, Sixup),m_ExtraLaserIds[8],vec2(m_Pos.x - (m_Size.x/2 + 16),m_Pos.y),vec2(m_Pos.x - (m_Size.x/2 + 50),m_Pos.y),Server()->Tick(),m_Owner,LASERTYPE_DOOR);
		GameServer()->SnapLaserObject(CSnapContext(SnappingClientVersion, Sixup),m_ExtraLaserIds[9],vec2(m_Pos.x - (m_Size.x/2 + 50),m_Pos.y),vec2(m_Pos.x - (m_Size.x/2 + 50),m_Pos.y)+postemp,Server()->Tick(),m_Owner,LASERTYPE_DOOR);
		GameServer()->SnapLaserObject(CSnapContext(SnappingClientVersion, Sixup),m_ExtraLaserIds[10],vec2(m_Pos.x - (m_Size.x/2 + 50),m_Pos.y),vec2(m_Pos.x - (m_Size.x/2 + 50),m_Pos.y)+postemp2,Server()->Tick(),m_Owner,LASERTYPE_DOOR);
	}
	else
	{
		GameServer()->SnapLaserObject(CSnapContext(SnappingClientVersion, Sixup),m_ExtraLaserIds[8],vec2(m_Pos.x + (m_Size.x/2 + 16),m_Pos.y),vec2(m_Pos.x + (m_Size.x/2 + 50),m_Pos.y),Server()->Tick(),m_Owner,LASERTYPE_DOOR);
		GameServer()->SnapLaserObject(CSnapContext(SnappingClientVersion, Sixup),m_ExtraLaserIds[9],vec2(m_Pos.x + (m_Size.x/2 + 50),m_Pos.y),vec2(m_Pos.x + (m_Size.x/2 + 50),m_Pos.y)+postemp,Server()->Tick(),m_Owner,LASERTYPE_DOOR);
		GameServer()->SnapLaserObject(CSnapContext(SnappingClientVersion, Sixup),m_ExtraLaserIds[10],vec2(m_Pos.x + (m_Size.x/2 + 50),m_Pos.y),vec2(m_Pos.x + (m_Size.x/2 + 50),m_Pos.y)+postemp2,Server()->Tick(),m_Owner,LASERTYPE_DOOR);
	}
}

void CHelicopter::HandleMounterInput(const CNetObj_PlayerInput* Input)
{
	if(Input->m_Jump)
	{
		m_Vel.y = -5.f;
	}
	if(Input->m_Direction > 0)
	{
		m_Vel.x = 6.f;
	}
	else if(Input->m_Direction < 0)
	{
		m_Vel.x = -6.f;
	}
	else
	{
		m_Vel.x = 0.f;
	}

	if(Input->m_Hook)
	{
		m_Vel.y = 5.f;
	}

	if(Input->m_Fire)
	{
		new CMinigunProjectile(GameWorld(),m_pMounter->GetPlayer()->GetCid(),m_Pos,normalize(vec2(Input->m_TargetX, Input->m_TargetY)));
		GameServer()->CreateSound(m_Pos, SOUND_HOOK_LOOP, m_pMounter->TeamMask());
	}
}
