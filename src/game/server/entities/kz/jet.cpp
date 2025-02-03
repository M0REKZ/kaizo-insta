/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <game/generated/protocol.h>
#include <game/mapitems.h>
#include <game/teamscore.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/gamecontroller.h>

#include "jet.h"
#include "minigun_projectile.h"
#include <game/kztiles.h>

CJet::CJet(CGameWorld *pGameWorld, vec2 Pos, int Owner, int Layer) :
CVehicle(pGameWorld, Pos, Owner, Layer)
{
	m_ObjType = CGameWorld::CUSTOM_ENTTYPE_JET;

	m_Vel = vec2(0,0);
	m_Size = vec2(32,32);
	m_MountOffset = vec2(0,0);
	m_pMounter = nullptr;
	m_MountDistance = 64.f;

	for(int i = 0;i<3;i++)
	{
		m_ExtraLaserIds[i] = Server()->SnapNewId();
	}

	GameWorld()->InsertEntity(this);

	if(m_Owner != -1 && !GameServer()->GetPlayerChar(m_Owner))
	{
		Reset();
	}
}

CJet::~CJet()
{
	for(int i = 0;i<3;i++)
	{
		Server()->SnapFreeId(m_ExtraLaserIds[i]);
	}
}

void CJet::Tick()
{
	if(m_Explode && m_pMounter)
	{
		vec2 posfront;

		posfront.x = sin(m_Angle) * 32;
		posfront.y = cos(m_Angle) * 32;

		GameServer()->CreateExplosion(m_Pos,m_Owner,WEAPON_GRENADE,false,m_pMounter->Team());
		GameServer()->CreateSound(m_Pos,SOUND_GRENADE_EXPLODE);
		m_pMounter->DoKZDamage(posfront * -1,5,m_pMounter->GetPlayer()->GetCid(),WEAPON_GRENADE);

		if(m_Owner != -1)
			Reset();
		else
			m_Pos = m_StartPos;
		m_Angle = -1.57f;
		UnMount();
	}
	else
	{
		m_Explode = false;
	}

	if(m_ReloadTimer)
		m_ReloadTimer--;

	m_Vel.x = 0.f;
	m_Vel.y = Tuning()->m_Gravity/2;

	CVehicle::Tick();

	if(m_pMounter)
	{
		if(m_Vel.y > 15.f || m_Vel.y < -15.f || m_Vel.x > 15.f || m_Vel.x < -15.f)
		{
			vec2 posfront;

			posfront.x = sin(m_Angle) * 32;
			posfront.y = cos(m_Angle) * 32;
			posfront += m_Pos;

			m_Explode = Collision()->CheckPoint(posfront);
		}
	}

}

void CJet::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	int SnappingClientVersion = GameServer()->GetClientVersion(SnappingClient);
	bool Sixup = Server()->IsSixup(SnappingClient);

	//draw jet
	//GameServer()->SnapLaserObject(CSnapContext(SnappingClientVersion, Sixup),ID,TO,FROM,Server()->Tick(),m_Owner,LASERTYPE_DOOR);

	vec2 posfront;

	posfront.x = sin(m_Angle) * 64;
	posfront.y = cos(m_Angle) * 64;

	posfront += m_Pos;

	vec2 posback1,posback2;

	posback1.x = sin(m_Angle - 2.5f) * 64;
	posback1.y = cos(m_Angle - 2.5f) * 64;

	posback2.x = sin(m_Angle + 2.5f) * 64;
	posback2.y = cos(m_Angle + 2.5f) * 64;

	posback1 += m_Pos;
	posback2 += m_Pos;

	vec2 posbackcenter;

	posbackcenter.x = sin(m_Angle + 3.14159) * 45;
	posbackcenter.y = cos(m_Angle + 3.14159) * 45;

	posbackcenter += m_Pos;

	GameServer()->SnapLaserObject(CSnapContext(SnappingClientVersion, Sixup),GetId(),posfront,posback1,Server()->Tick(),m_Owner,LASERTYPE_DOOR);
	GameServer()->SnapLaserObject(CSnapContext(SnappingClientVersion, Sixup),m_ExtraLaserIds[0],posfront,posback2,Server()->Tick(),m_Owner,LASERTYPE_DOOR);
	GameServer()->SnapLaserObject(CSnapContext(SnappingClientVersion, Sixup),m_ExtraLaserIds[1],posback1,posbackcenter,Server()->Tick(),m_Owner,LASERTYPE_DOOR);
	GameServer()->SnapLaserObject(CSnapContext(SnappingClientVersion, Sixup),m_ExtraLaserIds[2],posback2,posbackcenter,Server()->Tick(),m_Owner,LASERTYPE_DOOR);
	
}

void CJet::HandleMounterInput(const CNetObj_PlayerInput* Input)
{
	m_Angle = atan2(Input->m_TargetX,Input->m_TargetY);

	if(Input->m_Hook)
	{
		m_Vel += normalize(vec2(Input->m_TargetX, Input->m_TargetY)) * 20;
	}

	if(Input->m_Fire & 1 && !m_ReloadTimer)
	{
		new CMinigunProjectile(GameWorld(),m_pMounter->GetPlayer()->GetCid(),m_Pos,normalize(vec2(Input->m_TargetX, Input->m_TargetY)));
		GameServer()->CreateSound(m_Pos, SOUND_HOOK_LOOP, m_pMounter->TeamMask());
		m_ReloadTimer = 0.1 * Server()->TickSpeed();
	}
}
