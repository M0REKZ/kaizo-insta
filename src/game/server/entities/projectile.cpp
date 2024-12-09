/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "projectile.h"
#include "character.h"
#include "flag.h" //+KZ

#include <engine/shared/config.h>

#include <game/generated/protocol.h>
#include <game/mapitems.h>

#include <game/server/gamecontext.h>
#include <game/server/gamemodes/DDRace.h>
#include <game/server/player.h>

CProjectile::CProjectile(
	CGameWorld *pGameWorld,
	int Type,
	int Owner,
	vec2 Pos,
	vec2 Dir,
	int Span,
	bool Freeze,
	bool Explosive,
	int SoundImpact,
	vec2 InitDir,
	int Layer,
	int Number) :
	CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{
	m_Type = Type;
	m_Pos = Pos;
	m_Direction = Dir;
	m_LifeSpan = Span;
	m_Owner = Owner;
	//m_Damage = Damage;
	m_SoundImpact = SoundImpact;
	m_StartTick = Server()->Tick();
	m_Explosive = Explosive;

	m_Layer = Layer;
	m_Number = Number;
	m_Freeze = Freeze;

	m_InitDir = InitDir;
	m_TuneZone = GameServer()->Collision()->IsTune(GameServer()->Collision()->GetMapIndex(m_Pos));

	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	m_BelongsToPracticeTeam = pOwnerChar && pOwnerChar->Teams()->IsPractice(pOwnerChar->Team());

	m_AffectedCharacters = CClientMask().set();

	if(m_Type == WEAPON_GRENADE)
	{
		m_AffectedCharacters = CClientMask().set().reset();

		if(Owner >= 0 && Owner <= MAX_CLIENTS)
		{
			for(int i = 0; i < MAX_CLIENTS; ++i)
			{
				if(GameServer()->m_apPlayers[i])
				{
					CPlayer *pPlayer = GameServer()->m_apPlayers[i];
					if(pPlayer->GetTeam() != TEAM_SPECTATORS &&
						pPlayer->GetCharacter() &&
						pPlayer->GetCharacter()->IsAlive() &&
						!NetworkClipped(Owner, pPlayer->GetCharacter()->GetPos()))
					{
						m_AffectedCharacters.set(i);
					}
				}
			}
		}
	}

	GameWorld()->InsertEntity(this);

	m_FirstTick = true;
}

void CProjectile::Reset()
{
	m_MarkedForDestroy = true;
}

vec2 CProjectile::GetPos(float Time)
{
	float Curvature = 0;
	float Speed = 0;

	switch(m_Type)
	{
	case WEAPON_GRENADE:
		if(!m_TuneZone)
		{
			Curvature = Tuning()->m_GrenadeCurvature;
			Speed = Tuning()->m_GrenadeSpeed;
		}
		else
		{
			Curvature = TuningList()[m_TuneZone].m_GrenadeCurvature;
			Speed = TuningList()[m_TuneZone].m_GrenadeSpeed;
		}

		break;

	case WEAPON_SHOTGUN:
		if(!m_TuneZone)
		{
            if(m_Owner < 0)
            {
                Curvature = Tuning()->m_FreezeBulletCurvature;
                Speed = Tuning()->m_FreezeBulletSpeed;
            }
            else
            {
                Curvature = Tuning()->m_ShotgunCurvature;
                Speed = Tuning()->m_ShotgunSpeed;
            }
		}
		else
		{
            if(m_Owner < 0)
            {
                Curvature = TuningList()[m_TuneZone].m_FreezeBulletCurvature;
                Speed = TuningList()[m_TuneZone].m_FreezeBulletSpeed;
            }
            else
            {
                Curvature = TuningList()[m_TuneZone].m_ShotgunCurvature;
                Speed = TuningList()[m_TuneZone].m_ShotgunSpeed;
            }
		}

		break;

	case WEAPON_GUN:
		if(!m_TuneZone)
		{
			Curvature = Tuning()->m_GunCurvature;
			Speed = Tuning()->m_GunSpeed;
		}
		else
		{
			Curvature = TuningList()[m_TuneZone].m_GunCurvature;
			Speed = TuningList()[m_TuneZone].m_GunSpeed;
		}
		break;
	}

	return CalcPos(m_Pos, m_Direction, Curvature, Speed, Time);
}

void CProjectile::Tick()
{
	int tick = m_StartTick; //JSAURUS rollback
	int origstart = m_StartTick;
	bool IsRollbackDamage = false;
	int RollbackDamageTick = 0;

	int Collide = 0;
	CCharacter *pTargetChr = 0;
	float Pt;
	float Ct;
	vec2 PrevPos;
	vec2 CurPos;
	vec2 ColPos;
	vec2 NewPos;

	CCharacter *pOwnerChar = 0;

	bool IsWeaponCollide = false;

	if(m_Owner >= 0)
		pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

	if(m_FirstTick && g_Config.m_SvRollback && m_Owner >= 0 && m_Owner < MAX_CLIENTS && GameServer()->m_apPlayers[m_Owner]->m_Rollback && GameServer()->m_apPlayers[m_Owner]->GetCharacter())
	{
		tick = GameServer()->m_apPlayers[m_Owner]->GetCharacter()->GetCore().m_LastAckedSnapshot;
		m_StartTick = tick;


		//int diff = origstart - tick;

		//Collide with wall and tee
		int CollideTick;
		for(CollideTick = m_StartTick; CollideTick <= origstart;CollideTick++)
		{
			Pt = (CollideTick - m_StartTick - 1) / (float)Server()->TickSpeed();
			Ct = (CollideTick - m_StartTick) / (float)Server()->TickSpeed();
			PrevPos = GetPos(Pt);
			CurPos = GetPos(Ct);
			Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &ColPos, &NewPos); //wall

			if(m_LifeSpan > -1)
				m_LifeSpan--;

			if(Collide)
				break;

			pTargetChr = GameServer()->m_World.IntersectCharacterTick(PrevPos, ColPos, m_Freeze ? 1.0f : 6.0f, ColPos, CollideTick, pOwnerChar, m_Owner); //tee

			if(pTargetChr)
			{
				IsWeaponCollide = false; //just explode >:(
				IsRollbackDamage = true;
				RollbackDamageTick = CollideTick;
				break;
			}
		}
	}
	else
	{

	Pt = (Server()->Tick() - m_StartTick - 1) / (float)Server()->TickSpeed();
	Ct = (Server()->Tick() - m_StartTick) / (float)Server()->TickSpeed();
	PrevPos = GetPos(Pt);
	CurPos = GetPos(Ct);
	//ColPos;
	//NewPos;
	if(!Collide)
		Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &ColPos, &NewPos);
	//CCharacter *pOwnerChar = 0;

	//if(m_Owner >= 0)
	//	pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

	//CCharacter *pTargetChr = 0;

	if(!pTargetChr && (pOwnerChar ? !pOwnerChar->GrenadeHitDisabled() : g_Config.m_SvHit))
		pTargetChr = GameServer()->m_World.IntersectCharacter(PrevPos, ColPos, m_Freeze ? 1.0f : 6.0f, ColPos, pOwnerChar, m_Owner);
	}

	//+KZ
	if(!Collide)
		Collide = HitFlag(PrevPos, CurPos);

	if(m_LifeSpan > -1)
		m_LifeSpan--;

	CClientMask TeamMask = CClientMask().set();
	//bool IsWeaponCollide = false;
	if(
		pOwnerChar &&
		pTargetChr &&
		pOwnerChar->IsAlive() &&
		pTargetChr->IsAlive() &&
		!pTargetChr->CanCollide(m_Owner))
	{
		IsWeaponCollide = true;
	}
	if(pOwnerChar && pOwnerChar->IsAlive())
	{
		TeamMask = pOwnerChar->TeamMask();
	}
	else if(m_Owner >= 0 && (m_Type != WEAPON_GRENADE || g_Config.m_SvDestroyBulletsOnDeath || m_BelongsToPracticeTeam))
	{
		m_MarkedForDestroy = true;
		return;
	}

	if(((pTargetChr && (pOwnerChar ? !pOwnerChar->GrenadeHitDisabled() : g_Config.m_SvHit || m_Owner == -1 || pTargetChr == pOwnerChar)) || Collide || GameLayerClipped(CurPos)) && !IsWeaponCollide)
	{
		if(m_Explosive /*??*/ && (!pTargetChr || (pTargetChr && (!m_Freeze || (m_Type == WEAPON_SHOTGUN && Collide)))))
		{
			int Number = 1;
			if(GameServer()->EmulateBug(BUG_GRENADE_DOUBLEEXPLOSION) && m_LifeSpan == -1)
			{
				Number = 2;
			}
			for(int i = 0; i < Number; i++)
			{
				if(IsRollbackDamage)
				{
					GameServer()->CreateExplosionTick(ColPos, m_Owner, m_Type, m_Owner == -1, (!pTargetChr ? -1 : pTargetChr->Team()), RollbackDamageTick, (m_Owner != -1) ? TeamMask : CClientMask().set(), m_AffectedCharacters);
				}
				else
				GameServer()->CreateExplosion(ColPos, m_Owner, m_Type, m_Owner == -1, (!pTargetChr ? -1 : pTargetChr->Team()),
					(m_Owner != -1) ? TeamMask : CClientMask().set(), m_AffectedCharacters);
				GameServer()->CreateSound(ColPos, m_SoundImpact,
					(m_Owner != -1) ? TeamMask : CClientMask().set());
			}
		}
		else if(m_Freeze)
		{
			CEntity *apEnts[MAX_CLIENTS];
			int Num = GameWorld()->FindEntities(CurPos, 1.0f, apEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
			for(int i = 0; i < Num; ++i)
			{
				auto *pChr = static_cast<CCharacter *>(apEnts[i]);
				if(pChr && (m_Layer != LAYER_SWITCH || (m_Layer == LAYER_SWITCH && m_Number > 0 && Switchers()[m_Number].m_aStatus[pChr->Team()])))
					pChr->Freeze();
			}
		}
		else if(pTargetChr)
			pTargetChr->TakeDamage(vec2(0, 0), 0, m_Owner, m_Type);

		if(pOwnerChar && !GameLayerClipped(ColPos) &&
			((m_Type == WEAPON_GRENADE && pOwnerChar->HasTelegunGrenade()) || (m_Type == WEAPON_GUN && pOwnerChar->HasTelegunGun())))
		{
			int MapIndex = GameServer()->Collision()->GetPureMapIndex(pTargetChr ? pTargetChr->m_Pos : ColPos);
			int TileFIndex = GameServer()->Collision()->GetFTileIndex(MapIndex);
			bool IsSwitchTeleGun = GameServer()->Collision()->GetSwitchType(MapIndex) == TILE_ALLOW_TELE_GUN;
			bool IsBlueSwitchTeleGun = GameServer()->Collision()->GetSwitchType(MapIndex) == TILE_ALLOW_BLUE_TELE_GUN;

			if(IsSwitchTeleGun || IsBlueSwitchTeleGun)
			{
				// Delay specifies which weapon the tile should work for.
				// Delay = 0 means all.
				int delay = GameServer()->Collision()->GetSwitchDelay(MapIndex);

				if(delay == 1 && m_Type != WEAPON_GUN)
					IsSwitchTeleGun = IsBlueSwitchTeleGun = false;
				if(delay == 2 && m_Type != WEAPON_GRENADE)
					IsSwitchTeleGun = IsBlueSwitchTeleGun = false;
				if(delay == 3 && m_Type != WEAPON_LASER)
					IsSwitchTeleGun = IsBlueSwitchTeleGun = false;
			}

			if(TileFIndex == TILE_ALLOW_TELE_GUN || TileFIndex == TILE_ALLOW_BLUE_TELE_GUN || IsSwitchTeleGun || IsBlueSwitchTeleGun || pTargetChr)
			{
				bool Found;
				vec2 PossiblePos;

				if(!Collide)
					Found = GetNearestAirPosPlayer(pTargetChr ? pTargetChr->m_Pos : ColPos, &PossiblePos);
				else
					Found = GetNearestAirPos(NewPos, CurPos, &PossiblePos);

				if(Found)
				{
					pOwnerChar->m_TeleGunPos = PossiblePos;
					pOwnerChar->m_TeleGunTeleport = true;
					pOwnerChar->m_IsBlueTeleGunTeleport = TileFIndex == TILE_ALLOW_BLUE_TELE_GUN || IsBlueSwitchTeleGun;
				}
			}
		}

		if(Collide && m_Bouncing != 0)
		{
			m_StartTick = Server()->Tick();
			m_Pos = NewPos + (-(m_Direction * 4));
			if(m_Bouncing == 1)
				m_Direction.x = -m_Direction.x;
			else if(m_Bouncing == 2)
				m_Direction.y = -m_Direction.y;
			if(absolute(m_Direction.x) < 1e-6f)
				m_Direction.x = 0;
			if(absolute(m_Direction.y) < 1e-6f)
				m_Direction.y = 0;
			m_Pos += m_Direction;
		}
		else if(m_Type == WEAPON_GUN || (!g_Config.m_SvDDraceShotgun ? (m_Type == WEAPON_SHOTGUN && m_Owner >= 0) : false))
		{
            if(!g_Config.m_SvDDraceShotgun) //JSAURUS
            {
                // GameServer()->CreateDamageInd(CurPos, -std::atan2(m_Direction.x, m_Direction.y), 10, (m_Owner != -1) ? TeamMask : CClientMask().set());
                if(pTargetChr)
                    pTargetChr->TakeDamage(vec2(0,0), 1, m_Owner, m_Type);
            }
            else
            {
                GameServer()->CreateDamageInd(CurPos, -std::atan2(m_Direction.x, m_Direction.y), 10, (m_Owner != -1) ? TeamMask : CClientMask().set());
            }
			m_MarkedForDestroy = true;
			return;
		}
		else
		{
			if(!m_Freeze)
			{
				m_MarkedForDestroy = true;
				return;
			}
		}
	}
	if(m_LifeSpan == -1)
	{
		if(m_Explosive)
		{
			if(m_Owner >= 0)
				pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

			TeamMask = CClientMask().set();
			if(pOwnerChar && pOwnerChar->IsAlive())
			{
				TeamMask = pOwnerChar->TeamMask();
			}

			GameServer()->CreateExplosion(ColPos, m_Owner, m_Type, m_Owner == -1, (!pOwnerChar ? -1 : pOwnerChar->Team()),
				(m_Owner != -1) ? TeamMask : CClientMask().set(), m_AffectedCharacters);
			GameServer()->CreateSound(ColPos, m_SoundImpact,
				(m_Owner != -1) ? TeamMask : CClientMask().set());
		}
		m_MarkedForDestroy = true;
		return;
	}

	int x = GameServer()->Collision()->GetIndex(PrevPos, CurPos);
	int z;
	if(g_Config.m_SvOldTeleportWeapons)
		z = GameServer()->Collision()->IsTeleport(x);
	else
		z = GameServer()->Collision()->IsTeleportWeapon(x);
	if(z && !GameServer()->Collision()->TeleOuts(z - 1).empty())
	{
		int TeleOut = GameServer()->m_World.m_Core.RandomOr0(GameServer()->Collision()->TeleOuts(z - 1).size());
		m_Pos = GameServer()->Collision()->TeleOuts(z - 1)[TeleOut];
		m_StartTick = Server()->Tick();
	}

	m_FirstTick = false;
}

void CProjectile::TickPaused()
{
	++m_StartTick;
}

void CProjectile::FillInfo(CNetObj_Projectile *pProj)
{
	pProj->m_X = (int)m_Pos.x;
	pProj->m_Y = (int)m_Pos.y;
	pProj->m_VelX = (int)(m_Direction.x * 100.0f);
	pProj->m_VelY = (int)(m_Direction.y * 100.0f);
	pProj->m_StartTick = m_StartTick;
	pProj->m_Type = m_Type;
}

void CProjectile::Snap(int SnappingClient)
{
	float Ct = (Server()->Tick() - m_StartTick) / (float)Server()->TickSpeed();

	if(NetworkClipped(SnappingClient, GetPos(Ct)))
		return;
    
    if(m_Owner < 0)
    {
        if(NetworkClipped(SnappingClient))
            return;

        int SnappingClientVersion = GameServer()->GetClientVersion(SnappingClient);
        
        vec2 CurPos = GetPos((Server()->Tick() - m_StartTick) / (float)Server()->TickSpeed());
        GameServer()->SnapLaserObject(CSnapContext(SnappingClientVersion), GetId(),
            CurPos, CurPos, m_StartTick , -1, LASERTYPE_SHOTGUN, 2, 0);
        return;
    }
    
    if(!g_Config.m_SvDDraceShotgun) //for instagib use
    {
        if(m_Type == WEAPON_SHOTGUN)
        {
                CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, GetId(), sizeof(CNetObj_Projectile)));
                if(pProj)
                    FillInfo(pProj);
                return;
        }
    }

	int SnappingClientVersion = GameServer()->GetClientVersion(SnappingClient);
	if(SnappingClientVersion < VERSION_DDNET_ENTITY_NETOBJS)
	{
		CCharacter *pSnapChar = GameServer()->GetPlayerChar(SnappingClient);
		int Tick = (Server()->Tick() % Server()->TickSpeed()) % ((m_Explosive) ? 6 : 20);
		if(pSnapChar && pSnapChar->IsAlive() && (m_Layer == LAYER_SWITCH && m_Number > 0 && !Switchers()[m_Number].m_aStatus[pSnapChar->Team()] && (!Tick)))
			return;
	}

	CCharacter *pOwnerChar = 0;
	CClientMask TeamMask = CClientMask().set();

	if(m_Owner >= 0)
		pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

	if(pOwnerChar && pOwnerChar->IsAlive())
		TeamMask = pOwnerChar->TeamMask();

	if(SnappingClient != SERVER_DEMO_CLIENT && m_Owner != -1 && !TeamMask.test(SnappingClient))
		return;

	CNetObj_DDRaceProjectile DDRaceProjectile;

	if(SnappingClientVersion >= VERSION_DDNET_ENTITY_NETOBJS)
	{
		CNetObj_DDNetProjectile *pDDNetProjectile = static_cast<CNetObj_DDNetProjectile *>(Server()->SnapNewItem(NETOBJTYPE_DDNETPROJECTILE, GetId(), sizeof(CNetObj_DDNetProjectile)));
		if(!pDDNetProjectile)
		{
			return;
		}
		FillExtraInfo(pDDNetProjectile);
	}
	else if(SnappingClientVersion >= VERSION_DDNET_ANTIPING_PROJECTILE && FillExtraInfoLegacy(&DDRaceProjectile))
	{
		int Type = SnappingClientVersion < VERSION_DDNET_MSG_LEGACY ? (int)NETOBJTYPE_PROJECTILE : NETOBJTYPE_DDRACEPROJECTILE;
		void *pProj = Server()->SnapNewItem(Type, GetId(), sizeof(DDRaceProjectile));
		if(!pProj)
		{
			return;
		}
		mem_copy(pProj, &DDRaceProjectile, sizeof(DDRaceProjectile));
	}
	else
	{
		CNetObj_Projectile *pProj = Server()->SnapNewItem<CNetObj_Projectile>(GetId());
		if(!pProj)
		{
			return;
		}
		FillInfo(pProj);
	}
}

void CProjectile::SwapClients(int Client1, int Client2)
{
	m_Owner = m_Owner == Client1 ? Client2 : m_Owner == Client2 ? Client1 : m_Owner;
}

// DDRace

void CProjectile::SetBouncing(int Value)
{
	m_Bouncing = Value;
}

bool CProjectile::FillExtraInfoLegacy(CNetObj_DDRaceProjectile *pProj)
{
	const int MaxPos = 0x7fffffff / 100;
	if(absolute((int)m_Pos.y) + 1 >= MaxPos || absolute((int)m_Pos.x) + 1 >= MaxPos)
	{
		//If the modified data would be too large to fit in an integer, send normal data instead
		return false;
	}
	//Send additional/modified info, by modifying the fields of the netobj
	float Angle = -std::atan2(m_Direction.x, m_Direction.y);

	int Data = 0;
	Data |= (absolute(m_Owner) & 255) << 0;
	if(m_Owner < 0)
		Data |= LEGACYPROJECTILEFLAG_NO_OWNER;
	//This bit tells the client to use the extra info
	Data |= LEGACYPROJECTILEFLAG_IS_DDNET;
	// LEGACYPROJECTILEFLAG_BOUNCE_HORIZONTAL, LEGACYPROJECTILEFLAG_BOUNCE_VERTICAL
	Data |= (m_Bouncing & 3) << 10;
	if(m_Explosive)
		Data |= LEGACYPROJECTILEFLAG_EXPLOSIVE;
	if(m_Freeze)
		Data |= LEGACYPROJECTILEFLAG_FREEZE;

	pProj->m_X = (int)(m_Pos.x * 100.0f);
	pProj->m_Y = (int)(m_Pos.y * 100.0f);
	pProj->m_Angle = (int)(Angle * 1000000.0f);
	pProj->m_Data = Data;
	pProj->m_StartTick = m_StartTick;
	pProj->m_Type = m_Type;
	return true;
}

void CProjectile::FillExtraInfo(CNetObj_DDNetProjectile *pProj)
{
	int Flags = 0;
	if(m_Bouncing & 1)
	{
		Flags |= PROJECTILEFLAG_BOUNCE_HORIZONTAL;
	}
	if(m_Bouncing & 2)
	{
		Flags |= PROJECTILEFLAG_BOUNCE_VERTICAL;
	}
	if(m_Explosive)
	{
		Flags |= PROJECTILEFLAG_EXPLOSIVE;
	}
	if(m_Freeze)
	{
		Flags |= PROJECTILEFLAG_FREEZE;
	}

	if(m_Owner < 0)
	{
		pProj->m_VelX = round_to_int(m_Direction.x * 1e6f);
		pProj->m_VelY = round_to_int(m_Direction.y * 1e6f);
	}
	else
	{
		pProj->m_VelX = round_to_int(m_InitDir.x);
		pProj->m_VelY = round_to_int(m_InitDir.y);
		Flags |= PROJECTILEFLAG_NORMALIZE_VEL;
	}

	pProj->m_X = round_to_int(m_Pos.x * 100.0f);
	pProj->m_Y = round_to_int(m_Pos.y * 100.0f);
	pProj->m_Type = m_Type;
	pProj->m_StartTick = m_StartTick;
	pProj->m_Owner = m_Owner;
	pProj->m_Flags = Flags;
	pProj->m_SwitchNumber = m_Number;
	pProj->m_TuneZone = m_TuneZone;
}


//+KZ
int CProjectile::HitFlag(vec2 From, vec2 To)
{
	if (g_Config.m_SvFlagProjectileMomentum == 0)
		return 0;

	int Collided = 0;
	
	vec2 outpos;
 	for (CFlag *flag = (CFlag*)GameWorld()->FindFirst(CGameWorld::ENTTYPE_FLAG); flag; flag = (CFlag *)flag->TypeNext())
 	{
		if(flag->m_pCarrier)
			continue;
		closest_point_on_line(From, To, flag->m_Pos, outpos);
 		if (distance(flag->m_Pos, outpos) < 40.f)
 		{
 			flag->m_Vel += normalize(To - From) * g_Config.m_SvFlagProjectileMomentum * 0.1f;
 			flag->m_DropTick = Server()->Tick();
			if(m_Owner >= 0 && m_Owner < MAX_CLIENTS)
 				flag->m_pLastCarrier = GameServer()->GetPlayerChar(m_Owner);
			else
				flag->m_pLastCarrier = nullptr;
			flag->m_AtStand = false;
			Collided = TILE_SOLID;
 		}
 	};

	return Collided;
}