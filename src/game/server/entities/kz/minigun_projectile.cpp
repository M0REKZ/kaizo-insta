#include <game/server/gamecontext.h>
#include "fdd_stable_projectile.h"
#include "minigun_projectile.h"
#include <game/server/teams.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>

CMinigunProjectile::CMinigunProjectile(CGameWorld *pGameWorld, int Owner, vec2 Pos, vec2 Dir, bool HideOnSpec, bool OnlyShowOwner)
: CStableProjectile(pGameWorld,WEAPON_SHOTGUN,Owner,Pos,HideOnSpec,OnlyShowOwner)
{
	m_ObjType = CGameWorld::CUSTOM_ENTTYPE_MINIGUN_PROJECTILE;
	m_Dir = Dir;


	GameWorld()->InsertEntity(this);
}

void CMinigunProjectile::Tick()
{
	vec2 oldpos = m_Pos;
	m_Pos += m_Dir * 30;
	CStableProjectile::Tick();
	if(Collision()->CheckPoint(m_Pos))
	{
		GameServer()->CreateSound(m_Pos,SOUND_HOOK_NOATTACH,m_TeamMask);
		Reset();
	}

	CCharacter* pChr = nullptr;

	vec2 intersect;

	pChr = GameWorld()->IntersectCharacter(oldpos,m_Pos,0.f,intersect,GameServer()->GetPlayerChar(m_Owner),m_Owner);

	if(pChr)
	{
		pChr->TakeDamage(m_Dir * 2,1,m_Owner,WEAPON_GUN);
		GameServer()->CreateSound(m_Pos,SOUND_HOOK_ATTACH_PLAYER,m_TeamMask);
		Reset();
	}
}

void CMinigunProjectile::Snap(int SnappingClient)
{
	CStableProjectile::Snap(SnappingClient);
}

void CMinigunProjectile::Reset()
{
	CStableProjectile::Reset();
}
