/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "character.h"
#include "laser.h"
#include "pickup.h"
#include "projectile.h"
#include "flag.h"

#include "ddnet_pvp/vanilla_pickup.h"

#include "kz/mine.h"
#include "kz/ball.h"
#include "kz/portal_projectile.h"
#include "kz/minigun_projectile.h"
#include "kz/blackhole.h"
#include "kz/kz_pickup.h"
#include "kz/portal.h"

#include <antibot/antibot_data.h>

#include <engine/antibot.h>
#include <engine/shared/config.h>

#include <game/generated/protocol.h>
#include <game/generated/server_data.h>
#include <game/mapitems.h>

#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/server/teams.h>
#include <game/kztiles.h>

#include <engine/server/server.h>

#include <stdio.h>

MACRO_ALLOC_POOL_ID_IMPL(CCharacter, MAX_CLIENTS)

// Character, "physical" player's part
CCharacter::CCharacter(CGameWorld *pWorld, CNetObj_PlayerInput LastInput) :
	CEntity(pWorld, CGameWorld::ENTTYPE_CHARACTER, vec2(0, 0), CCharacterCore::PhysicalSize())
{
	m_Health = 0;
	m_Armor = 0;
	m_TriggeredEvents7 = 0;
	m_StrongWeakId = 0;

	//+KZ
	m_InvisibleShieldId = Server()->SnapNewId();
	m_HasBallSnapId = Server()->SnapNewId();
	m_PortalKindId = Server()->SnapNewId();
	m_CursorId[0] = Server()->SnapNewId();
	m_CursorId[1] = Server()->SnapNewId();
		
	m_Input = LastInput;
	// never initialize both to zero
	m_Input.m_TargetX = 0;
	m_Input.m_TargetY = -1;

	m_LatestPrevPrevInput = m_LatestPrevInput = m_LatestInput = m_PrevInput = m_SavedInput = m_Input;

	m_LastTimeCp = -1;
	m_LastTimeCpBroadcasted = -1;
	for(float &CurrentTimeCp : m_aCurrentTimeCp)
	{
		CurrentTimeCp = 0.0f;
	}

	m_RollbackAttacker = -1;  //JSAURUS rollback
	m_RollbackAttackerWeapon = -1; //JSAURUS rollback
	m_RollbackDamageTick = 0; //JSAURUS rollback

	if(m_pPlayer)
		m_Core.m_PlayerRollback = m_pPlayer->m_Rollback; //JSAURUS rollback

	m_aCustomWeaponSnaps[KZ_WEAPON_TASER - KZ_CUSTOM_WEAPON_START] = WEAPON_LASER;
	m_aCustomWeaponSnaps[KZ_WEAPON_PORTAL_GUN - KZ_CUSTOM_WEAPON_START] = WEAPON_LASER;
	m_aCustomWeaponSnaps[KZ_WEAPON_MINIGUN - KZ_CUSTOM_WEAPON_START] = WEAPON_GRENADE;
	m_aCustomWeaponSnaps[KZ_WEAPON_BLACKHOLE - KZ_CUSTOM_WEAPON_START] = WEAPON_GRENADE;
	m_aCustomWeaponSnaps[KZ_WEAPON_CHARGE_HAMMER - KZ_CUSTOM_WEAPON_START] = WEAPON_HAMMER;

	m_aCustomWeaponMaxAmmo[KZ_WEAPON_TASER - KZ_CUSTOM_WEAPON_START] = 10;
	m_aCustomWeaponMaxAmmo[KZ_WEAPON_PORTAL_GUN - KZ_CUSTOM_WEAPON_START] = 10;
	m_aCustomWeaponMaxAmmo[KZ_WEAPON_MINIGUN - KZ_CUSTOM_WEAPON_START] = 1000;
	m_aCustomWeaponMaxAmmo[KZ_WEAPON_BLACKHOLE - KZ_CUSTOM_WEAPON_START] = 10;
	m_aCustomWeaponMaxAmmo[KZ_WEAPON_CHARGE_HAMMER - KZ_CUSTOM_WEAPON_START] = -1;

	m_aCustomWeaponAmmo[KZ_WEAPON_TASER - KZ_CUSTOM_WEAPON_START] = 0;
	m_aCustomWeaponAmmo[KZ_WEAPON_PORTAL_GUN - KZ_CUSTOM_WEAPON_START] = 0;
	m_aCustomWeaponAmmo[KZ_WEAPON_MINIGUN - KZ_CUSTOM_WEAPON_START] = 0;
	m_aCustomWeaponAmmo[KZ_WEAPON_BLACKHOLE - KZ_CUSTOM_WEAPON_START] = 0;
	m_aCustomWeaponAmmo[KZ_WEAPON_CHARGE_HAMMER - KZ_CUSTOM_WEAPON_START] = 0;

}

CCharacter::~CCharacter()
{
	if(m_InvisibleShieldId != -1)
	{
		Server()->SnapFreeId(m_InvisibleShieldId);
	}

	if(m_HasBallSnapId != -1)
	{
		Server()->SnapFreeId(m_HasBallSnapId);
	}

	if(m_PortalKindId != -1)
	{
		Server()->SnapFreeId(m_PortalKindId);
	}

	if(m_CursorId[0] != -1)
	{
		Server()->SnapFreeId(m_CursorId[0]);
	}

	if(m_CursorId[1] != -1)
	{
		Server()->SnapFreeId(m_CursorId[1]);
	}
}

void CCharacter::Reset()
{
	StopRecording();
	Destroy();
}

bool CCharacter::Spawn(CPlayer *pPlayer, vec2 Pos)
{
	if(m_pPlayer)
		m_Core.m_PlayerRollback = m_pPlayer->m_Rollback; //JSAURUS rollback
	m_Core.m_DeathTick = -1; //JSAURUS rollback
	m_EmoteStop = -1;
	m_LastAction = -1;
	m_LastNoAmmoSound = -1;
	m_LastWeapon = GameServer()->m_pController->GetDefaultWeapon(pPlayer);
	m_QueuedWeapon = -1;
	m_LastRefillJumps = false;
	m_LastPenalty = false;
	m_LastBonus = false;

	m_TeleGunTeleport = false;
	m_IsBlueTeleGunTeleport = false;

	m_pPlayer = pPlayer;
	m_Pos = Pos;

	for(int i = 0; i < POSITION_HISTORY; i++) // JSAURUS rollback
	{
		m_Core.m_Positions[i] = Pos;
	} // ---

	mem_zero(&m_LatestPrevPrevInput, sizeof(m_LatestPrevPrevInput));
	m_LatestPrevPrevInput.m_TargetY = -1;
	m_NumInputs = 0;
	m_SpawnTick = Server()->Tick();
	m_WeaponChangeTick = Server()->Tick();
	Antibot()->OnSpawn(m_pPlayer->GetCid());

	m_Core.Reset();
	m_Core.Init(&GameServer()->m_World.m_Core, Collision());
	m_Core.m_ActiveWeapon = GameServer()->m_pController->GetDefaultWeapon(pPlayer);
	m_Core.m_Pos = m_Pos;
	m_Core.m_Id = m_pPlayer->GetCid();
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCid()] = &m_Core;

	m_ReckoningTick = 0;
	m_SendCore = CCharacterCore();
	m_ReckoningCore = CCharacterCore();

	GameServer()->m_World.InsertEntity(this);
	m_Alive = true;

	GameServer()->m_pController->OnCharacterSpawn(this);

	m_RollbackHealth = m_Health;
	m_RollbackArmor = m_Armor;

	DDRaceInit();

	m_TuneZone = Collision()->IsTune(Collision()->GetMapIndex(Pos));
	m_TuneZoneOld = -1; // no zone leave msg on spawn
	m_NeededFaketuning = 0; // reset fake tunings on respawn and send the client
	SendZoneMsgs(); // we want a entermessage also on spawn
	GameServer()->SendTuningParams(m_pPlayer->GetCid(), m_TuneZone);

	TrySetRescue(RESCUEMODE_MANUAL);
	Server()->StartRecord(m_pPlayer->GetCid());

	int Team = GameServer()->m_aTeamMapping[m_pPlayer->GetCid()];

	if(Team != -1)
	{
		GameServer()->m_pController->Teams().SetForceCharacterTeam(m_pPlayer->GetCid(), Team);
		GameServer()->m_aTeamMapping[m_pPlayer->GetCid()] = -1;

		if(GameServer()->m_apSavedTeams[Team])
		{
			GameServer()->m_apSavedTeams[Team]->Load(GameServer(), Team, true, true);
			delete GameServer()->m_apSavedTeams[Team];
			GameServer()->m_apSavedTeams[Team] = nullptr;
		}

		if(GameServer()->m_apSavedTees[m_pPlayer->GetCid()])
		{
			GameServer()->m_apSavedTees[m_pPlayer->GetCid()]->Load(m_pPlayer->GetCharacter(), Team);
			delete GameServer()->m_apSavedTees[m_pPlayer->GetCid()];
			GameServer()->m_apSavedTees[m_pPlayer->GetCid()] = nullptr;
		}
	}

	return true;
}

void CCharacter::Destroy()
{
	if(m_HasBall) //+KZ
	{
		m_HasBall = false;
		CBall* ball = new CBall(&GameServer()->m_World, m_pPlayer->GetCid(), m_Pos, vec2(0,0));
		GameServer()->CreateSound(m_Pos, SOUND_GRENADE_FIRE, TeamMask());
		ball->GoToStartPos();
	}
	
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCid()] = nullptr;
	m_Alive = false;
	SetSolo(false);
}

void CCharacter::SetWeapon(int W)
{
	if(W == m_Core.m_ActiveWeapon)
		return;

	m_LastWeapon = m_Core.m_ActiveWeapon;
	m_QueuedWeapon = -1;
	m_Core.m_ActiveWeapon = W;
	GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SWITCH, TeamMask());

	if(m_Core.m_ActiveWeapon < 0 || m_Core.m_ActiveWeapon >= KZ_NUM_CUSTOM_WEAPONS)
		m_Core.m_ActiveWeapon = 0;

	//+KZ
	
	if(m_Core.m_ActiveWeapon == KZ_WEAPON_TASER)
	{
		GameServer()->SendBroadcast("Weapon: Taser",m_pPlayer->GetCid());
	}
	else if(m_Core.m_ActiveWeapon == KZ_WEAPON_PORTAL_GUN)
	{
		GameServer()->SendBroadcast("Weapon: Portal Gun",m_pPlayer->GetCid());
	}
	else if(m_Core.m_ActiveWeapon == KZ_WEAPON_MINIGUN)
	{
		GameServer()->SendBroadcast("Weapon: Minigun",m_pPlayer->GetCid());
	}
	else if(m_Core.m_ActiveWeapon == KZ_WEAPON_BLACKHOLE)
	{
		GameServer()->SendBroadcast("Weapon: Blackhole",m_pPlayer->GetCid());
		m_ShowCursor = true;
	}
	else if(m_Core.m_ActiveWeapon == KZ_WEAPON_CHARGE_HAMMER)
	{
		GameServer()->SendBroadcast("Weapon: Charge Hammer",m_pPlayer->GetCid());
	}

	if(m_Core.m_ActiveWeapon != KZ_WEAPON_BLACKHOLE)
	{
		m_ShowCursor = false;
	}
	
	
	//-----
}

void CCharacter::SetJetpack(bool Active)
{
	m_Core.m_Jetpack = Active;
}

void CCharacter::SetEndlessJump(bool Active)
{
	m_Core.m_EndlessJump = Active;
}

void CCharacter::SetJumps(int Jumps)
{
	m_Core.m_Jumps = Jumps;
}

void CCharacter::SetSolo(bool Solo)
{
	m_Core.m_Solo = Solo;
	Teams()->m_Core.SetSolo(m_pPlayer->GetCid(), Solo);
}

void CCharacter::SetSuper(bool Super)
{
	// Disable invincible mode before activating super mode. Both modes active at the same time wouldn't necessarily break anything but it's not useful.
	if(Super)
		SetInvincible(false);

	bool WasSuper = m_Core.m_Super;
	m_Core.m_Super = Super;
	if(Super && !WasSuper)
	{
		m_TeamBeforeSuper = Team();
		Teams()->SetCharacterTeam(GetPlayer()->GetCid(), TEAM_SUPER);
		m_DDRaceState = DDRACE_CHEAT;
	}
	else if(!Super && WasSuper)
	{
		Teams()->SetForceCharacterTeam(GetPlayer()->GetCid(), m_TeamBeforeSuper);
	}
}

void CCharacter::SetInvincible(bool Invincible)
{
	// Disable super mode before activating invincible mode. Both modes active at the same time wouldn't necessarily break anything but it's not useful.
	if(Invincible)
		SetSuper(false);

	m_Core.m_Invincible = Invincible;
	if(Invincible)
		UnFreeze();

	SetEndlessJump(Invincible);
}

void CCharacter::SetLiveFrozen(bool Active)
{
	m_Core.m_LiveFrozen = Active;
}

void CCharacter::SetDeepFrozen(bool Active)
{
	m_Core.m_DeepFrozen = Active;
}

bool CCharacter::IsGrounded()
{
	if(Collision()->CheckPoint(m_Pos.x + GetProximityRadius() / 2, m_Pos.y + GetProximityRadius() / 2 + 5))
		return true;
	if(Collision()->CheckPoint(m_Pos.x - GetProximityRadius() / 2, m_Pos.y + GetProximityRadius() / 2 + 5))
		return true;

	int MoveRestrictionsBelow = Collision()->GetMoveRestrictions(m_Pos + vec2(0, GetProximityRadius() / 2 + 4), 0.0f);
	return (MoveRestrictionsBelow & CANTMOVE_DOWN) != 0;
}

void CCharacter::HandleJetpack()
{
	vec2 Direction = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY));

	bool FullAuto = false;
	if(m_Core.m_ActiveWeapon == WEAPON_GRENADE || m_Core.m_ActiveWeapon == WEAPON_SHOTGUN || m_Core.m_ActiveWeapon == WEAPON_LASER)
		FullAuto = true;
	if(m_Core.m_Jetpack && m_Core.m_ActiveWeapon == WEAPON_GUN)
		FullAuto = true;

	// check if we gonna fire
	bool WillFire = false;
	if(CountInput(m_LatestPrevInput.m_Fire, m_LatestInput.m_Fire).m_Presses)
		WillFire = true;

	if(FullAuto && (m_LatestInput.m_Fire & 1) && m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_Ammo)
		WillFire = true;

	if(!WillFire)
		return;

	// check for ammo
	if(!m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_Ammo || m_FreezeTime)
	{
		return;
	}

	switch(m_Core.m_ActiveWeapon)
	{
	case WEAPON_GUN:
	{
		if(m_Core.m_Jetpack)
		{
			float Strength = GetTuning(m_TuneZone)->m_JetpackStrength;
			TakeDamage(Direction * -1.0f * (Strength / 100.0f / 6.11f), 0, m_pPlayer->GetCid(), m_Core.m_ActiveWeapon);
		}
	}
	}
}

void CCharacter::HandleNinja()
{
	if(m_Core.m_ActiveWeapon != WEAPON_NINJA)
		return;

	if((Server()->Tick() - m_Core.m_Ninja.m_ActivationTick) > (g_pData->m_Weapons.m_Ninja.m_Duration * Server()->TickSpeed() / 1000))
	{
		// time's up, return
		RemoveNinja();
		return;
	}

	int NinjaTime = m_Core.m_Ninja.m_ActivationTick + (g_pData->m_Weapons.m_Ninja.m_Duration * Server()->TickSpeed() / 1000) - Server()->Tick();

	if(NinjaTime % Server()->TickSpeed() == 0 && NinjaTime / Server()->TickSpeed() <= 5)
    {
		GameServer()->CreateDamageInd(m_Pos, 0, NinjaTime / Server()->TickSpeed(), TeamMask() & GameServer()->ClientsMaskExcludeClientVersionAndHigher(VERSION_DDNET_NEW_HUD));

	}

	GameServer()->m_pController->SetArmorProgress(this, NinjaTime);

	// force ninja Weapon
	SetWeapon(WEAPON_NINJA);

	m_Core.m_Ninja.m_CurrentMoveTime--;

	if(m_Core.m_Ninja.m_CurrentMoveTime == 0)
	{
		// reset velocity
		m_Core.m_Vel = m_Core.m_Ninja.m_ActivationDir * m_Core.m_Ninja.m_OldVelAmount;
	}

	if(m_Core.m_Ninja.m_CurrentMoveTime > 0)
	{
		// Set velocity
		m_Core.m_Vel = m_Core.m_Ninja.m_ActivationDir * g_pData->m_Weapons.m_Ninja.m_Velocity;
		vec2 OldPos = m_Pos;
		vec2 GroundElasticity = vec2(
			GetTuning(m_TuneZone)->m_GroundElasticityX,
			GetTuning(m_TuneZone)->m_GroundElasticityY);

		Collision()->MoveBox(&m_Core.m_Pos, &m_Core.m_Vel, vec2(GetProximityRadius(), GetProximityRadius()), GroundElasticity);

		// reset velocity so the client doesn't predict stuff
		ResetVelocity();

		// check if we Hit anything along the way
		{
			CEntity *apEnts[MAX_CLIENTS];
			float Radius = GetProximityRadius() * 2.0f;
			int Num = GameServer()->m_World.FindEntities(OldPos, Radius, apEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

			// check that we're not in solo part
			if(Teams()->m_Core.GetSolo(m_pPlayer->GetCid()))
				return;

			for(int i = 0; i < Num; ++i)
			{
				auto *pChr = static_cast<CCharacter *>(apEnts[i]);
				if(pChr == this)
					continue;

				// Don't hit players in other teams
				if(Team() != pChr->Team())
					continue;

				// Don't hit players in solo parts
				if(Teams()->m_Core.GetSolo(pChr->m_pPlayer->GetCid()))
					return;

				// make sure we haven't Hit this object before
				bool AlreadyHit = false;
				for(int j = 0; j < m_NumObjectsHit; j++)
				{
					if(m_apHitObjects[j] == pChr)
						AlreadyHit = true;
				}
				if(AlreadyHit)
					continue;

				// check so we are sufficiently close
				if(distance(pChr->m_Pos, m_Pos) > (GetProximityRadius() * 2.0f))
					continue;

				// Hit a player, give them damage and stuffs...
				GameServer()->CreateSound(pChr->m_Pos, SOUND_NINJA_HIT, TeamMask());
				// set his velocity to fast upward (for now)
				if(m_NumObjectsHit < 10)
					m_apHitObjects[m_NumObjectsHit++] = pChr;

				pChr->TakeDamage(vec2(0, -10.0f), g_pData->m_Weapons.m_Ninja.m_pBase->m_Damage, m_pPlayer->GetCid(), WEAPON_NINJA);
			}
		}

		return;
	}
}

void CCharacter::DoWeaponSwitch()
{
	// make sure we can switch
	if(m_ReloadTimer != 0 || m_QueuedWeapon == -1 || m_Core.m_aWeapons[WEAPON_NINJA].m_Got || (m_QueuedWeapon >= 0 && m_QueuedWeapon < NUM_WEAPONS ? !m_Core.m_aWeapons[m_QueuedWeapon].m_Got : ( m_QueuedWeapon >= KZ_CUSTOM_WEAPON_START && m_QueuedWeapon < KZ_NUM_CUSTOM_WEAPONS ? !m_aCustomWeaponGot[m_QueuedWeapon - KZ_CUSTOM_WEAPON_START] : false)))
		return;
	
	if(m_HasBall) //+KZ Ball
		return;

	// switch Weapon
	SetWeapon(m_QueuedWeapon);
}

void CCharacter::HandleWeaponSwitch()
{
	int WantedWeapon = m_Core.m_ActiveWeapon;
	if(m_QueuedWeapon != -1)
		WantedWeapon = m_QueuedWeapon;

	bool Anything = false;
	for(int i = 0; i < NUM_WEAPONS - 1; ++i)
		if(m_Core.m_aWeapons[i].m_Got)
			Anything = true;
	if(!Anything)
		return;
	// select Weapon
	int Next = CountInput(m_LatestPrevInput.m_NextWeapon, m_LatestInput.m_NextWeapon).m_Presses;
	int Prev = CountInput(m_LatestPrevInput.m_PrevWeapon, m_LatestInput.m_PrevWeapon).m_Presses;

	if(Next < 128) // make sure we only try sane stuff
	{
		while(Next) // Next Weapon selection
		{
			WantedWeapon = (WantedWeapon + 1) % KZ_NUM_CUSTOM_WEAPONS;
			if(WantedWeapon >= 0 && WantedWeapon < NUM_WEAPONS && m_Core.m_aWeapons[WantedWeapon].m_Got)
			{
					Next--;
			}
			else if(WantedWeapon >= KZ_CUSTOM_WEAPON_START && WantedWeapon < KZ_NUM_CUSTOM_WEAPONS && m_aCustomWeaponGot[WantedWeapon - KZ_CUSTOM_WEAPON_START])
			{
					Next--;
			}
		}
	}

	if(Prev < 128) // make sure we only try sane stuff
	{
		while(Prev) // Prev Weapon selection
		{
			WantedWeapon = (WantedWeapon - 1) < 0 ? KZ_NUM_CUSTOM_WEAPONS - 1 : WantedWeapon - 1;
			if(WantedWeapon >= 0 && WantedWeapon < NUM_WEAPONS && m_Core.m_aWeapons[WantedWeapon].m_Got)
			{
					Prev--;
			}
			else if(WantedWeapon >= KZ_CUSTOM_WEAPON_START && WantedWeapon < KZ_NUM_CUSTOM_WEAPONS && m_aCustomWeaponGot[WantedWeapon - KZ_CUSTOM_WEAPON_START])
			{
					Prev--;
			}
		}
	}

	// Direct Weapon selection
	if(m_LatestInput.m_WantedWeapon)
		WantedWeapon = m_Input.m_WantedWeapon - 1;

	// check for insane values
	if(WantedWeapon >= 0 && WantedWeapon < NUM_WEAPONS && WantedWeapon != m_Core.m_ActiveWeapon && m_Core.m_aWeapons[WantedWeapon].m_Got)
	{
		m_QueuedWeapon = WantedWeapon;
	}
	else if(WantedWeapon >= KZ_CUSTOM_WEAPON_START && WantedWeapon < KZ_NUM_CUSTOM_WEAPONS && WantedWeapon != m_Core.m_ActiveWeapon && m_aCustomWeaponGot[WantedWeapon - KZ_CUSTOM_WEAPON_START])
	{
		m_QueuedWeapon = WantedWeapon;
	}

	DoWeaponSwitch();
}

void CCharacter::FireWeapon()
{
	if(m_ReloadTimer != 0)
	{
		if(m_LatestInput.m_Fire & 1)
		{
			Antibot()->OnHammerFireReloading(m_pPlayer->GetCid());
		}
		return;
	}

	DoWeaponSwitch();
	vec2 MouseTarget = vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY);
	vec2 Direction = normalize(MouseTarget);

	bool FullAuto = false;
	if(m_Core.m_ActiveWeapon == WEAPON_GRENADE || m_Core.m_ActiveWeapon == WEAPON_SHOTGUN || m_Core.m_ActiveWeapon == WEAPON_LASER)
		FullAuto = true;
	if(m_Core.m_Jetpack && m_Core.m_ActiveWeapon == WEAPON_GUN)
		FullAuto = true;
	if(m_Core.m_ActiveWeapon == KZ_WEAPON_TASER || m_Core.m_ActiveWeapon == KZ_WEAPON_PORTAL_GUN || m_Core.m_ActiveWeapon == KZ_WEAPON_MINIGUN || m_Core.m_ActiveWeapon == KZ_WEAPON_CHARGE_HAMMER) //+KZ
		FullAuto = true;
	// allow firing directly after coming out of freeze or being unfrozen
	// by something
	if(m_FrozenLastTick)
		FullAuto = true;

	// don't fire hammer when player is deep and sv_deepfly is disabled
	if(!g_Config.m_SvDeepfly && m_Core.m_ActiveWeapon == WEAPON_HAMMER && m_Core.m_DeepFrozen)
		return;

	// check if we gonna fire
	bool WillFire = false;
	if(CountInput(m_LatestPrevInput.m_Fire, m_LatestInput.m_Fire).m_Presses)
		WillFire = true;

	if(FullAuto && (m_LatestInput.m_Fire & 1) && (m_Core.m_ActiveWeapon < NUM_WEAPONS ? m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_Ammo : m_aCustomWeaponAmmo[m_Core.m_ActiveWeapon - KZ_CUSTOM_WEAPON_START]))
		WillFire = true;

	if(!WillFire && m_Core.m_ActiveWeapon != KZ_WEAPON_CHARGE_HAMMER)
		return;

	if(m_FreezeTime)
	{
		// Timer stuff to avoid shrieking orchestra caused by unfreeze-plasma
		if(m_PainSoundTimer <= 0 && !(m_LatestPrevInput.m_Fire & 1))
		{
			m_PainSoundTimer = 1 * Server()->TickSpeed();
			GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_LONG, TeamMask()); // NOLINT(clang-analyzer-unix.Malloc)
		}
		return;
	}

	if(m_HasBall) //+KZ
	{
		m_HasBall = false;
		new CBall(&GameServer()->m_World, m_pPlayer->GetCid(), m_Pos, Direction);
		GameServer()->CreateSound(m_Pos, SOUND_GRENADE_FIRE, TeamMask());
		
		float FireDelay;
		GetTuning(m_TuneZone)->Get(38 + m_Core.m_ActiveWeapon, &FireDelay);
		m_ReloadTimer = FireDelay * Server()->TickSpeed() / 1000;
		
		SetWeapon(m_BallQueuedWeapon);
		m_BallQueuedWeapon = -1;
		return;
	}
	
	if(m_Core.m_Mounted)
		return;

	// check for ammo
    
	if(m_Core.m_ActiveWeapon >=0 && m_Core.m_ActiveWeapon < NUM_WEAPONS && !m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_Ammo)
		return;
	else if(m_Core.m_ActiveWeapon >= KZ_CUSTOM_WEAPON_START && m_Core.m_ActiveWeapon < KZ_NUM_CUSTOM_WEAPONS && !m_aCustomWeaponAmmo[m_Core.m_ActiveWeapon-KZ_CUSTOM_WEAPON_START])
		return;

	vec2 ProjStartPos = m_Pos + Direction * GetProximityRadius() * 0.75f;

	int InitialAmmo = GetWeaponAmmo(m_Core.m_ActiveWeapon);

	// ddnet-insta
	if(GameServer()->m_pController->OnFireWeapon(*this, m_Core.m_ActiveWeapon, Direction, MouseTarget, ProjStartPos) && m_Core.m_ActiveWeapon >=0 && m_Core.m_ActiveWeapon < NUM_WEAPONS)
		return;

	if(m_HasNoWeapon)
		return;

	switch(m_Core.m_ActiveWeapon)
	{
	case WEAPON_HAMMER:
	{
		
		//+KZ
		if(m_Mines)
		{
			new CMine(&GameServer()->m_World, m_Pos, m_pPlayer->GetCid(), true, false);
			m_Mines--;
		}
		
		// reset objects Hit
		m_NumObjectsHit = 0;
		GameServer()->CreateSound(m_Pos, SOUND_HAMMER_FIRE, TeamMask()); // NOLINT(clang-analyzer-unix.Malloc)

		Antibot()->OnHammerFire(m_pPlayer->GetCid());

		if(m_Core.m_HammerHitDisabled)
			break;

		CEntity *apEnts[MAX_CLIENTS];
		int Hits = 0;
		int Num = GameServer()->m_World.FindEntities(ProjStartPos, GetProximityRadius() * 0.5f, apEnts,
			MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

		for(int i = 0; i < Num; ++i)
		{
			auto *pTarget = static_cast<CCharacter *>(apEnts[i]);

			//if ((pTarget == this) || Collision()->IntersectLine(ProjStartPos, pTarget->m_Pos, NULL, NULL))
			if((pTarget == this || (pTarget->IsAlive() && !CanCollide(pTarget->GetPlayer()->GetCid()))))
				continue;

			// set his velocity to fast upward (for now)
			if(length(pTarget->m_Pos - ProjStartPos) > 0.0f)
				GameServer()->CreateHammerHit(pTarget->m_Pos - normalize(pTarget->m_Pos - ProjStartPos) * GetProximityRadius() * 0.5f, TeamMask());
			else
				GameServer()->CreateHammerHit(ProjStartPos, TeamMask());

			vec2 Dir;
			if(length(pTarget->m_Pos - m_Pos) > 0.0f)
				Dir = normalize(pTarget->m_Pos - m_Pos);
			else
				Dir = vec2(0.f, -1.f);

			float Strength = GetTuning(m_TuneZone)->m_HammerStrength;

			vec2 Temp = pTarget->m_Core.m_Vel + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f;
			Temp = ClampVel(pTarget->m_MoveRestrictions, Temp);
			Temp -= pTarget->m_Core.m_Vel;
			pTarget->TakeDamage((vec2(0.f, -1.0f) + Temp) * Strength, g_pData->m_Weapons.m_Hammer.m_pBase->m_Damage,
				m_pPlayer->GetCid(), m_Core.m_ActiveWeapon);
			pTarget->UnFreeze();

			if(m_FreezeHammer)
				pTarget->Freeze();

			Antibot()->OnHammerHit(m_pPlayer->GetCid(), pTarget->GetPlayer()->GetCid());

			Hits++;
		}

		// if we Hit anything, we have to wait for the reload
		if(Hits)
		{
			float FireDelay = GetTuning(m_TuneZone)->m_HammerHitFireDelay;
			m_ReloadTimer = FireDelay * Server()->TickSpeed() / 1000;
		}
	}
	break;

	case WEAPON_GUN:
	{
		if(!m_Core.m_Jetpack || !m_pPlayer->m_NinjaJetpack || m_Core.m_HasTelegunGun)
		{
			int Lifetime = (int)(Server()->TickSpeed() * GetTuning(m_TuneZone)->m_GunLifetime);

			new CProjectile(
				GameWorld(),
				WEAPON_GUN, //Type
				m_pPlayer->GetCid(), //Owner
				ProjStartPos, //Pos
				Direction, //Dir
				Lifetime, //Span
				false, //Freeze
				false, //Explosive
				-1, //SoundImpact
				MouseTarget //InitDir
			);

			GameServer()->CreateSound(m_Pos, SOUND_GUN_FIRE, TeamMask()); // NOLINT(clang-analyzer-unix.Malloc)
		}
	}
	break;

	case WEAPON_SHOTGUN:
	{
        if(g_Config.m_SvDDraceShotgun)
        {
			float LaserReach = GetTuning(m_TuneZone)->m_LaserReach;

			new CLaser(&GameServer()->m_World, m_Pos, Direction, LaserReach, m_pPlayer->GetCid(), WEAPON_SHOTGUN);
			GameServer()->CreateSound(m_Pos, SOUND_SHOTGUN_FIRE, TeamMask()); // NOLINT(clang-analyzer-unix.Malloc)
        }
        else
        {
            //Jsaurus......
            int ShotSpread = 2;

            for(int i = -ShotSpread; i <= ShotSpread; ++i)
            {
                float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
                float a = 0;
                if(Direction.x != 0 || Direction.y != 0)
                {
                    a = atanf(Direction.y/Direction.x);
                    if(Direction.x < 0)
                    a = a+pi;
                }
                a += Spreading[i+2];
                float v = 1-(absolute(i)/(float)ShotSpread);
                float Speed = mix((float)GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);
                // CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_SHOTGUN,
                //     m_pPlayer->GetCID(),
                //     ProjStartPos,
                //     vec2(cosf(a), sinf(a))*Speed,
                //     (int)(Server()->TickSpeed()*GameServer()->Tuning()->m_ShotgunLifetime),
                //     1, 0, 0, -1, WEAPON_SHOTGUN);

                new CProjectile(
                    GameWorld(),
                    WEAPON_SHOTGUN, //Type
                    m_pPlayer->GetCid(), //Owner
                    ProjStartPos, //Pos
                    direction(a)*Speed, //Dir
                    (int)(Server()->TickSpeed()*GameServer()->Tuning()->m_ShotgunLifetime), //Span
                    false, //Freeze
                    false, //Explosive
                    -1, //SoundImpact
                    vec2(cosf(a), sinf(a))*Speed // MouseTarget
                );
            }

            GameServer()->CreateSound(m_Pos, SOUND_SHOTGUN_FIRE);
        }
	}
	break;

	case WEAPON_GRENADE:
	{
		int Lifetime = (int)(Server()->TickSpeed() * GetTuning(m_TuneZone)->m_GrenadeLifetime);

		new CProjectile(
			GameWorld(),
			WEAPON_GRENADE, //Type
			m_pPlayer->GetCid(), //Owner
			ProjStartPos, //Pos
			Direction, //Dir
			Lifetime, //Span
			false, //Freeze
			true, //Explosive
			SOUND_GRENADE_EXPLODE, //SoundImpact
			MouseTarget // MouseTarget
		);

		GameServer()->CreateSound(m_Pos, SOUND_GRENADE_FIRE, TeamMask()); // NOLINT(clang-analyzer-unix.Malloc)
	}
	break;

	case WEAPON_LASER:
	{
		float LaserReach = GetTuning(m_TuneZone)->m_LaserReach;

		new CLaser(GameWorld(), m_Pos, Direction, LaserReach, m_pPlayer->GetCid(), WEAPON_LASER);
		GameServer()->CreateSound(m_Pos, SOUND_LASER_FIRE, TeamMask()); // NOLINT(clang-analyzer-unix.Malloc)
	}
	break;

	case WEAPON_NINJA:
	{
		// reset Hit objects
		m_NumObjectsHit = 0;

		m_Core.m_Ninja.m_ActivationDir = Direction;
		m_Core.m_Ninja.m_CurrentMoveTime = g_pData->m_Weapons.m_Ninja.m_Movetime * Server()->TickSpeed() / 1000;
		m_Core.m_Ninja.m_OldVelAmount = length(m_Core.m_Vel);

		GameServer()->CreateSound(m_Pos, SOUND_NINJA_FIRE, TeamMask()); // NOLINT(clang-analyzer-unix.Malloc)
	}
	break;

	//+KZ

	case KZ_WEAPON_TASER:
	{
		float LaserReach = GetTuning(m_TuneZone)->m_LaserReach;

		new CLaser(GameWorld(), m_Pos, Direction, LaserReach, m_pPlayer->GetCid(), KZ_WEAPON_TASER);
		GameServer()->CreateSound(m_Pos, SOUND_LASER_FIRE, TeamMask()); // NOLINT(clang-analyzer-unix.Malloc)
	}
	break;

	case KZ_WEAPON_PORTAL_GUN:
	{
		new CPortalProjectile(GameWorld(),m_pPlayer->GetCid(),m_Pos,Direction,m_BluePortal);
		GameServer()->CreateSound(m_Pos, SOUND_LASER_FIRE, TeamMask()); // NOLINT(clang-analyzer-unix.Malloc)
	}
	break;

	case KZ_WEAPON_MINIGUN:
	{
		new CMinigunProjectile(GameWorld(),m_pPlayer->GetCid(),m_Pos,Direction);
		GameServer()->CreateSound(m_Pos, SOUND_HOOK_LOOP, TeamMask()); // NOLINT(clang-analyzer-unix.Malloc)
	}
	break;

	case KZ_WEAPON_BLACKHOLE:
	{
		new CBlackHole(GameWorld(),vec2(m_Pos.x + m_Input.m_TargetX,m_Pos.y + m_Input.m_TargetY),m_pPlayer->GetCid());
		GameServer()->CreateSound(m_Pos, SOUND_HOOK_LOOP, TeamMask());

		if(InitialAmmo == GetWeaponAmmo(KZ_WEAPON_BLACKHOLE))
			m_aCustomWeaponAmmo[KZ_WEAPON_BLACKHOLE - KZ_CUSTOM_WEAPON_START]--;
	}
	break;
	case KZ_WEAPON_CHARGE_HAMMER:
	{
		if(!m_superhammer_charge_time && WillFire)
		{
			GameServer()->CreateSound(m_Pos + Direction*32*4, SOUND_GRENADE_EXPLODE);
			GameServer()->CreateExplosion(m_Pos + Direction*32*4, m_pPlayer->GetCid(), WEAPON_HAMMER, false, Team());
		}

		if (WillFire) {
		m_superhammer_charge_time++;
		if (m_superhammer_charge_time <= 60 && m_superhammer_charge_time % 15 == 0)
			GameServer()->CreateSound(m_Pos, SOUND_WEAPON_NOAMMO);
		} else {
			if (m_superhammer_charge_time > 15) {
				GameServer()->CreateSound(m_Pos + Direction*32*4, SOUND_GRENADE_EXPLODE);
				GameServer()->CreateExplosion(m_Pos + Direction*32*4, m_pPlayer->GetCid(), WEAPON_HAMMER, false,Team());
				if (m_superhammer_charge_time > 30)
					GameServer()->CreateExplosion(m_Pos + Direction*32*7, m_pPlayer->GetCid(), WEAPON_HAMMER, false,Team());
				if (m_superhammer_charge_time > 45)
					GameServer()->CreateExplosion(m_Pos + Direction*32*10, m_pPlayer->GetCid(), WEAPON_HAMMER, false,Team());
				if (m_superhammer_charge_time > 60)
					GameServer()->CreateExplosion(m_Pos + Direction*32*13, m_pPlayer->GetCid(), WEAPON_HAMMER, false,Team());
			}
			m_superhammer_charge_time = 0;
		}
	}
	//---------------
	}
	if(WillFire)
		m_AttackTick = Server()->Tick();

	if(!m_ReloadTimer && m_Core.m_ActiveWeapon < NUM_WEAPONS)
	{
		float FireDelay;
		GetTuning(m_TuneZone)->Get(38 + m_Core.m_ActiveWeapon, &FireDelay);
		m_ReloadTimer = FireDelay * Server()->TickSpeed() / 1000;
	}
	else if(m_Core.m_ActiveWeapon < KZ_NUM_CUSTOM_WEAPONS)
	{
		if(m_Core.m_ActiveWeapon == KZ_WEAPON_TASER || m_Core.m_ActiveWeapon == KZ_WEAPON_PORTAL_GUN)
		{
			m_ReloadTimer = GetTuning(m_TuneZone)->m_LaserFireDelay * Server()->TickSpeed() / 1000;
		}
		else if(m_Core.m_ActiveWeapon == KZ_WEAPON_MINIGUN)
		{
			m_ReloadTimer = 0.1 * Server()->TickSpeed();
		}
	}
}

void CCharacter::HandleWeapons()
{
	//ninja
	HandleNinja();
	HandleJetpack();

	if(m_PainSoundTimer > 0)
		m_PainSoundTimer--;

	// check reload timer
	if(m_ReloadTimer)
	{
		m_ReloadTimer--;
		return;
	}

	// fire Weapon, if wanted
	FireWeapon();

	//Ammo regen on Grenade
	int AmmoRegenTime = 0;
	int MaxAmmo = 0;
	if(m_Core.m_ActiveWeapon == WEAPON_GRENADE && g_Config.m_SvGrenadeAmmoRegen)
	{
		AmmoRegenTime = g_Config.m_SvGrenadeAmmoRegenTime;
		MaxAmmo = g_Config.m_SvGrenadeAmmoRegenNum;
	}
	else
	{
		// ammo regen
		AmmoRegenTime = g_pData->m_Weapons.m_aId[m_Core.m_ActiveWeapon].m_Ammoregentime;
		MaxAmmo = g_pData->m_Weapons.m_aId[m_Core.m_ActiveWeapon].m_Maxammo;
	}
	if(AmmoRegenTime && m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_Ammo >= 0)
	{
		// If equipped and not active, regen ammo?
		if(m_ReloadTimer <= 0)
		{
			if(m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_AmmoRegenStart < 0)
				m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_AmmoRegenStart = Server()->Tick();

			if((Server()->Tick() - m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_AmmoRegenStart) >= AmmoRegenTime * Server()->TickSpeed() / 1000)
			{
				// Add some ammo
				m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_Ammo = minimum(m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_Ammo + 1, MaxAmmo);
				m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_AmmoRegenStart = -1;
			}
		}
		else
		{
			m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_AmmoRegenStart = -1;
		}
	}
}

void CCharacter::GiveNinja()
{
	m_Core.m_Ninja.m_ActivationTick = Server()->Tick();
	m_Core.m_aWeapons[WEAPON_NINJA].m_Got = true;
	m_Core.m_aWeapons[WEAPON_NINJA].m_Ammo = -1;
	if(m_Core.m_ActiveWeapon != WEAPON_NINJA)
		m_LastWeapon = m_Core.m_ActiveWeapon;
	m_Core.m_ActiveWeapon = WEAPON_NINJA;

	if(!m_Core.m_aWeapons[WEAPON_NINJA].m_Got)
		GameServer()->CreateSound(m_Pos, SOUND_PICKUP_NINJA, TeamMask());
}

void CCharacter::RemoveNinja()
{
	m_Core.m_Ninja.m_ActivationDir = vec2(0, 0);
	m_Core.m_Ninja.m_ActivationTick = 0;
	m_Core.m_Ninja.m_CurrentMoveTime = 0;
	m_Core.m_Ninja.m_OldVelAmount = 0;
	m_Core.m_aWeapons[WEAPON_NINJA].m_Got = false;
	m_Core.m_aWeapons[WEAPON_NINJA].m_Ammo = 0;
	m_Core.m_ActiveWeapon = m_LastWeapon;

	SetWeapon(m_Core.m_ActiveWeapon);
}

void CCharacter::SetEmote(int Emote, int Tick)
{
	m_EmoteType = Emote;
	m_EmoteStop = Tick;
}

void CCharacter::OnPredictedInput(CNetObj_PlayerInput *pNewInput)
{
	// check for changes
	if(mem_comp(&m_SavedInput, pNewInput, sizeof(CNetObj_PlayerInput)) != 0)
		m_LastAction = Server()->Tick();

	// copy new input
	mem_copy(&m_Input, pNewInput, sizeof(m_Input));

	// it is not allowed to aim in the center
	if(m_Input.m_TargetX == 0 && m_Input.m_TargetY == 0)
		m_Input.m_TargetY = -1;

	mem_copy(&m_SavedInput, &m_Input, sizeof(m_SavedInput));
}

void CCharacter::OnDirectInput(CNetObj_PlayerInput *pNewInput)
{
	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
	mem_copy(&m_LatestInput, pNewInput, sizeof(m_LatestInput));
	m_NumInputs++;

	// it is not allowed to aim in the center
	if(m_LatestInput.m_TargetX == 0 && m_LatestInput.m_TargetY == 0)
		m_LatestInput.m_TargetY = -1;

	Antibot()->OnDirectInput(m_pPlayer->GetCid());

	if(m_NumInputs > 1 && m_pPlayer->GetTeam() != TEAM_SPECTATORS)
	{
		HandleWeaponSwitch();
		FireWeapon();
	}

	mem_copy(&m_LatestPrevPrevInput, &m_LatestPrevInput, sizeof(m_LatestInput));
	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
}

void CCharacter::ReleaseHook()
{
	m_Core.SetHookedPlayer(-1);
	m_Core.m_HookState = HOOK_RETRACTED;
	m_Core.m_TriggeredEvents |= COREEVENT_HOOK_RETRACT;
}

void CCharacter::ResetHook()
{
	ReleaseHook();
	m_Core.m_HookPos = m_Core.m_Pos;
}

void CCharacter::ResetInput()
{
	m_Input.m_Direction = 0;
	// simulate releasing the fire button
	if((m_Input.m_Fire & 1) != 0)
		m_Input.m_Fire++;
	m_Input.m_Fire &= INPUT_STATE_MASK;
	m_Input.m_Jump = 0;
	m_LatestPrevInput = m_LatestInput = m_Input;
}

void CCharacter::PreTick()
{
	if(m_StartTime > Server()->Tick())
	{
		// Prevent the player from getting a negative time
		// The main reason why this can happen is because of time penalty tiles
		// However, other reasons are hereby also excluded
		GameServer()->SendChatTarget(m_pPlayer->GetCid(), "You died of old age");
		Die(m_pPlayer->GetCid(), WEAPON_WORLD);
	}

	if(m_Paused)
		return;

	// set emote
	if(m_EmoteStop < Server()->Tick())
	{
		SetEmote(m_pPlayer->GetDefaultEmote(), -1);
	}

	DDRaceTick();

	Antibot()->OnCharacterTick(m_pPlayer->GetCid());

	m_Core.m_Input = m_Input;
	m_Core.Tick(true, !g_Config.m_SvNoWeakHook);
}

void CCharacter::Tick()
{

	if(m_pPlayer->m_PlayerFlags & PLAYERFLAG_AIM)
	{
		m_AimPressed = true;
	}
	else
	{
		m_AimPressed = false;
		m_Waitingforreleaseaim = false;
	}

	if(m_Core.m_ActiveWeapon == KZ_WEAPON_PORTAL_GUN && m_AimPressed && !m_Waitingforreleaseaim)
	{
		m_BluePortal = !m_BluePortal;
		m_Waitingforreleaseaim = true;
		if(m_BluePortal)
			GameServer()->SendBroadcast("Portal Gun: Blue Portal",m_pPlayer->GetCid());
		else
			GameServer()->SendBroadcast("Portal Gun: Orange Portal",m_pPlayer->GetCid());
	}

	if(m_ConfettiKZ && Server()->Tick() % 5 == 0)
	{
		GameServer()->CreateFinishEffect(m_Pos,TeamMask());
	}

	//m_Dying--;
	if(m_pPlayer->m_Rollback)
	{
		if(m_Dying != -1 && m_Dying < Server()->Tick() && (m_RollbackAttacker >= 0 && m_RollbackAttacker < MAX_CLIENTS))
		{
			m_DieNow = true;
		}
		
		if((m_RollbackAttacker >= 0 && m_RollbackAttacker < MAX_CLIENTS) && GameServer()->m_apPlayers[m_RollbackAttacker] && !(GameServer()->m_apPlayers[m_RollbackAttacker]->GetCharacter()))
		{
			m_DieNow = false;
			m_Dying = -1;
			m_Health = m_RollbackHealth;
			m_Armor = m_RollbackArmor;
		}

		if(m_DieNow && m_Dying != -1 && m_Dying < Server()->Tick() && m_Health <= 0 && m_RollbackAttacker != m_pPlayer->GetCid())
		{
			if(m_RollbackSendHitSound)
			{
				if(m_RollbackAttacker >= 0 && m_RollbackAttacker < MAX_CLIENTS)
				{
					// do damage Hit sound
					CClientMask Mask = CClientMask().set(m_RollbackAttacker);
					for(int i = 0; i < MAX_CLIENTS; i++)
					{
						if(!GameServer()->m_apPlayers[i])
							continue;

						if(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->m_SpectatorId == m_RollbackAttacker)
							Mask.set(i);
					}
					GameServer()->CreateSound(GameServer()->m_apPlayers[m_RollbackAttacker]->m_ViewPos, SOUND_HIT, Mask);
				}
			}

			Die(m_RollbackAttacker, m_RollbackAttackerWeapon);
			return;
		}
	}

	if(m_DropFlagBallTicks > 0) //+KZ
		m_DropFlagBallTicks--;

	if(g_Config.m_SvNoWeakHook)
	{
		if(m_Paused)
			return;

		m_Core.TickDeferred();
	}
	else
	{
		PreTick();
	}

	if(m_HasBall) //+KZ
	{
		if(m_BallReleaseTick > 0)
		{
			m_BallReleaseTick--;
		}
		else
		{
			vec2 MouseTarget = vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY);
			vec2 Direction = normalize(MouseTarget);
			m_HasBall = false;
			new CBall(&GameServer()->m_World, m_pPlayer->GetCid(), m_Pos, Direction);
			GameServer()->CreateSound(m_Pos, SOUND_GRENADE_FIRE, TeamMask());
			
			float FireDelay;
			GetTuning(m_TuneZone)->Get(38 + m_Core.m_ActiveWeapon, &FireDelay);
			m_ReloadTimer = FireDelay * Server()->TickSpeed() / 1000;
			
			SetWeapon(m_BallQueuedWeapon);
			m_BallQueuedWeapon = -1;
		}
	}

	//printf("%d\n",m_Water);
	if(m_Water || m_NoAir) //+KZ
	{
		if(m_AirTicks > 0)
		{
			m_AirTicks--;
		}
		else
		{
			if(m_AirDamageTick > 0)
			{
				m_AirDamageTick--;
			}
			else
			{
				DoKZDamage(vec2(0,0), 1, m_pPlayer->GetCid(), WEAPON_WORLD);
				m_AirDamageTick = Server()->TickSpeed();
			}
		}
	}
	else
	{
		m_AirTicks = 10 * Server()->TickSpeed();
	}

	if(!m_PrevInput.m_Hook && m_Input.m_Hook && !(m_Core.m_TriggeredEvents & COREEVENT_HOOK_ATTACH_PLAYER))
	{
		Antibot()->OnHookAttach(m_pPlayer->GetCid(), false);
	}

	// handle Weapons
	HandleWeapons();

	DDRacePostCoreTick();

	if(m_Core.m_TriggeredEvents & COREEVENT_HOOK_ATTACH_PLAYER)
	{
		const int HookedPlayer = m_Core.HookedPlayer();
		if(HookedPlayer != -1 && GameServer()->m_apPlayers[HookedPlayer]->GetTeam() != TEAM_SPECTATORS)
		{
			Antibot()->OnHookAttach(m_pPlayer->GetCid(), true);
		}
		if(g_Config.m_SvKillHook)
		{
			CCharacter *pChr = GameServer()->m_apPlayers[HookedPlayer]->GetCharacter();
			if(pChr)
				pChr->TakeDamage(vec2(0, 0), 10, m_pPlayer->GetCid(), WEAPON_GAME);
		}
	}

	// Previnput
	m_PrevInput = m_Input;

	m_PrevPos = m_Core.m_Pos;

	HandleFlagHookCatch(); //+KZ
}

void CCharacter::TickDeferred()
{
	// advance the dummy
	{
		CWorldCore TempWorld;
		m_ReckoningCore.Init(&TempWorld, Collision(), &Teams()->m_Core);
		m_ReckoningCore.m_Id = m_pPlayer->GetCid();
		m_ReckoningCore.Tick(false);
		m_ReckoningCore.Move();
		m_ReckoningCore.Quantize();
	}

	//lastsentcore
	vec2 StartPos = m_Core.m_Pos;
	vec2 StartVel = m_Core.m_Vel;
	bool StuckBefore = Collision()->TestBox(m_Core.m_Pos, CCharacterCore::PhysicalSizeVec2());

	m_Core.m_Id = m_pPlayer->GetCid();
	m_Core.Move();
	bool StuckAfterMove = Collision()->TestBox(m_Core.m_Pos, CCharacterCore::PhysicalSizeVec2());
	m_Core.Quantize();
	bool StuckAfterQuant = Collision()->TestBox(m_Core.m_Pos, CCharacterCore::PhysicalSizeVec2());
	m_Pos = m_Core.m_Pos;

	int position_index = Server()->Tick() % POSITION_HISTORY; //JSAURUS rollback
	m_Core.m_Positions[position_index] = m_Pos;
	if(m_pPlayer)
	{
		m_Core.m_LastAckedSnapshot++;
	} //------

	if(!StuckBefore && (StuckAfterMove || StuckAfterQuant))
	{
		// Hackish solution to get rid of strict-aliasing warning
		union
		{
			float f;
			unsigned u;
		} StartPosX, StartPosY, StartVelX, StartVelY;

		StartPosX.f = StartPos.x;
		StartPosY.f = StartPos.y;
		StartVelX.f = StartVel.x;
		StartVelY.f = StartVel.y;

		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "STUCK!!! %d %d %d %f %f %f %f %x %x %x %x",
			StuckBefore,
			StuckAfterMove,
			StuckAfterQuant,
			StartPos.x, StartPos.y,
			StartVel.x, StartVel.y,
			StartPosX.u, StartPosY.u,
			StartVelX.u, StartVelY.u);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	}

	{
		int Events = m_Core.m_TriggeredEvents;
		int CID = m_pPlayer->GetCid();

		// Some sounds are triggered client-side for the acting player (or for all players on Sixup)
		// so we need to avoid duplicating them
		CClientMask TeamMaskExceptSelfAndSixup = Teams()->TeamMask(Team(), CID, CID, CGameContext::FLAG_SIX);
		// Some are triggered client-side but only on Sixup
		CClientMask TeamMaskExceptSixup = Teams()->TeamMask(Team(), -1, CID, CGameContext::FLAG_SIX);

		if(Events & COREEVENT_GROUND_JUMP)
			GameServer()->CreateSound(m_Pos, SOUND_PLAYER_JUMP, TeamMaskExceptSelfAndSixup);

		if(Events & COREEVENT_HOOK_ATTACH_PLAYER)
			GameServer()->CreateSound(m_Pos, SOUND_HOOK_ATTACH_PLAYER, TeamMaskExceptSixup);

		if(Events & COREEVENT_HOOK_ATTACH_GROUND)
			GameServer()->CreateSound(m_Pos, SOUND_HOOK_ATTACH_GROUND, TeamMaskExceptSelfAndSixup);

		if(Events & COREEVENT_HOOK_HIT_NOHOOK)
			GameServer()->CreateSound(m_Pos, SOUND_HOOK_NOATTACH, TeamMaskExceptSelfAndSixup);

		if(Events & COREEVENT_GROUND_JUMP)
			m_TriggeredEvents7 |= protocol7::COREEVENTFLAG_GROUND_JUMP;
		if(Events & COREEVENT_AIR_JUMP)
			m_TriggeredEvents7 |= protocol7::COREEVENTFLAG_AIR_JUMP;
		if(Events & COREEVENT_HOOK_ATTACH_PLAYER)
			m_TriggeredEvents7 |= protocol7::COREEVENTFLAG_HOOK_ATTACH_PLAYER;
		if(Events & COREEVENT_HOOK_ATTACH_GROUND)
			m_TriggeredEvents7 |= protocol7::COREEVENTFLAG_HOOK_ATTACH_GROUND;
		if(Events & COREEVENT_HOOK_HIT_NOHOOK)
			m_TriggeredEvents7 |= protocol7::COREEVENTFLAG_HOOK_HIT_NOHOOK;
	}

	if(m_pPlayer->GetTeam() == TEAM_SPECTATORS)
	{
		m_Pos.x = m_Input.m_TargetX;
		m_Pos.y = m_Input.m_TargetY;
	}

	// update the m_SendCore if needed
	{
		CNetObj_Character Predicted;
		CNetObj_Character Current;
		mem_zero(&Predicted, sizeof(Predicted));
		mem_zero(&Current, sizeof(Current));
		m_ReckoningCore.Write(&Predicted);
		m_Core.Write(&Current);

		// only allow dead reckoning for a top of 3 seconds
		if(m_Core.m_HookedQuad.m_pQuad || m_Core.m_QuadCollided || m_Core.m_Reset || m_ReckoningTick + Server()->TickSpeed() * 3 < Server()->Tick() || mem_comp(&Predicted, &Current, sizeof(CNetObj_Character)) != 0)
		{
			m_ReckoningTick = Server()->Tick();
			m_SendCore = m_Core;
			m_ReckoningCore = m_Core;
			m_Core.m_Reset = false;
		}
	}
}

void CCharacter::TickPaused()
{
	++m_AttackTick;
	++m_DamageTakenTick;
	++m_Core.m_Ninja.m_ActivationTick;
	++m_ReckoningTick;
	if(m_LastAction != -1)
		++m_LastAction;
	if(m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_AmmoRegenStart > -1)
		++m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_AmmoRegenStart;
	if(m_EmoteStop > -1)
		++m_EmoteStop;
}

bool CCharacter::IncreaseHealth(int Amount)
{
	if(m_Health >= 10)
		return false;
	m_Health = clamp(m_Health + Amount, 0, 10);
	return true;
}

bool CCharacter::IncreaseArmor(int Amount)
{
	if(m_Armor >= 10)
		return false;
	m_Armor = clamp(m_Armor + Amount, 0, 10);
	return true;
}

void CCharacter::StopRecording()
{
	if(Server()->IsRecording(m_pPlayer->GetCid()))
	{
		CPlayerData *pData = GameServer()->Score()->PlayerData(m_pPlayer->GetCid());

		if(pData->m_RecordStopTick - Server()->Tick() <= Server()->TickSpeed() && pData->m_RecordStopTick != -1)
			Server()->SaveDemo(m_pPlayer->GetCid(), pData->m_RecordFinishTime);
		else
			Server()->StopRecord(m_pPlayer->GetCid());

		pData->m_RecordStopTick = -1;
	}
}

void CCharacter::Die(int Killer, int Weapon, bool SendKillMsg, bool rollBack)
{
	if(rollBack && m_pPlayer->m_Rollback && Killer != m_pPlayer->GetCid() && Weapon == WEAPON_LASER && !m_DieNow)
	{
		m_Dying = Server()->Tick() + (m_pPlayer->m_Latency.m_Avg * Server()->TickSpeed())/1000;;
		return;
		
	}

	StopRecording();
	if(GameServer()->m_pController->m_IsInstagibKZ || GameServer()->m_pController->IsVanillaGameType())
		m_pPlayer->m_RespawnTick = Server()->Tick() + Server()->TickSpeed() / 2;
	else
		m_pPlayer->m_RespawnTick = Server()->Tick();
	int ModeSpecial = GameServer()->m_pController->OnCharacterDeath(this, (Killer < 0) ? nullptr : GameServer()->m_apPlayers[Killer], Weapon);

	char aBuf[512];
	if(Killer < 0 || !GameServer()->m_apPlayers[Killer])
	{
		str_format(aBuf, sizeof(aBuf), "kill killer='%d:%d:' victim='%d:%d:%s' weapon=%d special=%d",
			Killer, -1 - Killer,
			m_pPlayer->GetCid(), m_pPlayer->GetTeam(), Server()->ClientName(m_pPlayer->GetCid()), Weapon, ModeSpecial);
	}
	else
	{
		str_format(aBuf, sizeof(aBuf), "kill killer='%d:%d:%s' victim='%d:%d:%s' weapon=%d special=%d",
			Killer, GameServer()->m_apPlayers[Killer]->GetTeam(), Server()->ClientName(Killer),
			m_pPlayer->GetCid(), m_pPlayer->GetTeam(), Server()->ClientName(m_pPlayer->GetCid()), Weapon, ModeSpecial);
	}
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

	// send the kill message
	if(SendKillMsg && (Team() == TEAM_FLOCK || Teams()->TeamFlock(Team()) || Teams()->Count(Team()) == 1 || Teams()->GetTeamState(Team()) == CGameTeams::TEAMSTATE_OPEN || !Teams()->TeamLocked(Team())))
	{
		CNetMsg_Sv_KillMsg Msg;
		Msg.m_Killer = Killer;
		Msg.m_Victim = m_pPlayer->GetCid();
		Msg.m_Weapon = Weapon;
		Msg.m_ModeSpecial = ModeSpecial;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
	}

	// a nice sound
	GameServer()->CreateSound(m_Pos, SOUND_PLAYER_DIE, TeamMask());

	// this is to rate limit respawning to 3 secs
	m_pPlayer->m_PreviousDieTick = m_pPlayer->m_DieTick;
	m_pPlayer->m_DieTick = Server()->Tick();

	m_Alive = false;
	SetSolo(false);
	
	if(m_HasBall) //+KZ
	{
		m_HasBall = false;
		new CBall(&GameServer()->m_World, m_pPlayer->GetCid(), m_Pos, vec2(0,0));
		GameServer()->CreateSound(m_Pos, SOUND_GRENADE_FIRE, TeamMask());
	}

	GameServer()->m_World.RemoveEntity(this);
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCid()] = nullptr;
	GameServer()->CreateDeath(m_Pos, m_pPlayer->GetCid(), TeamMask());
	Teams()->OnCharacterDeath(GetPlayer()->GetCid(), Weapon);

	// Cancel swap requests
	for(auto &pPlayer : GameServer()->m_apPlayers)
	{
		if(pPlayer && pPlayer->m_SwapTargetsClientId == GetPlayer()->GetCid())
			pPlayer->m_SwapTargetsClientId = -1;
	}
	GetPlayer()->m_SwapTargetsClientId = -1;
	
	if(Weapon != WEAPON_GAME)
		GetPlayer()->m_RageQuitTick = Server()->Tick(); // For rage quit
}

bool CCharacter::TakeDamage(vec2 Force, int Dmg, int From, int Weapon)
{
	m_RollbackHealth = m_Health;
	m_RollbackArmor = m_Armor;
	m_RollbackAttacker = From;
	m_RollbackAttackerWeapon = Weapon;
	m_RollbackDamagePos = m_Pos;

	if(From < 0 || From >= MAX_CLIENTS) //+KZ
	{
		m_TakingNoOwnerDamage = true;
		From = m_pPlayer->GetCid();
		
		if(GameServer()->m_pController->m_IsInstagibKZ)
		{
			DoKZDamage(Force,Dmg,From,Weapon);
		}
	}

	if(GameServer()->m_pController->OnCharacterTakeDamage(Force, Dmg, From, Weapon, *this))
	{
		m_TakingNoOwnerDamage = false;
		return false;
	}

	if(Dmg)
	{
		SetEmote(EMOTE_PAIN, Server()->Tick() + 500 * Server()->TickSpeed() / 1000);
	}
	
	//+KZ
	if(m_HasBall && Weapon == WEAPON_HAMMER && From >= 0 && From < MAX_CLIENTS && GameServer()->GetPlayerChar(From))
	{
		m_HasBall = false;
		SetWeapon(m_BallQueuedWeapon);
		m_BallQueuedWeapon = -1;
		GameServer()->GetPlayerChar(From)->CatchBall();
	}

	if(m_Dying != -1 && Weapon == WEAPON_LASER) //only cancel death for laser
	{
		//m_RollbackAttacker = From;
		//m_RollbackAttackerWeapon = Weapon;
		//m_RollbackDamagePos = m_Pos;
	}
	else
	{
		m_RollbackHealth = m_Health;
		m_RollbackArmor = m_Armor;
		m_Dying = -1;
	}

	vec2 Temp = m_Core.m_Vel + Force;
	m_Core.m_Vel = ClampVel(m_MoveRestrictions, Temp);

	m_TakingNoOwnerDamage = false;
	return true;
}

//TODO: Move the emote stuff to a function
void CCharacter::SnapCharacter(int SnappingClient, int Id)
{
	int SnappingClientVersion = GameServer()->GetClientVersion(SnappingClient);
	CCharacterCore *pCore;
	int Tick, Emote = m_EmoteType, Weapon = m_Core.m_ActiveWeapon, AmmoCount = 0,
		  Health = 0, Armor = 0;
	if(!m_ReckoningTick || GameServer()->m_World.m_Paused)
	{
		Tick = 0;
		pCore = &m_Core;
	}
	else
	{
		Tick = m_ReckoningTick;
		pCore = &m_SendCore;
	}

	// change eyes and use ninja graphic if player is frozen
	if(m_Core.m_DeepFrozen || m_FreezeTime > 0 || m_Core.m_LiveFrozen)
	{
		if(Emote == EMOTE_NORMAL)
			Emote = (m_Core.m_DeepFrozen || m_Core.m_LiveFrozen) ? EMOTE_PAIN : EMOTE_BLINK;

		if((m_Core.m_DeepFrozen || m_FreezeTime > 0) && SnappingClientVersion < VERSION_DDNET_NEW_HUD)
			Weapon = WEAPON_NINJA;
	}

	// solo, collision, jetpack and ninjajetpack prediction
	if(m_pPlayer->GetCid() == SnappingClient)
	{
		int Faketuning = 0;
		if(m_pPlayer->GetClientVersion() < VERSION_DDNET_NEW_HUD || m_Core.m_Mounted || m_Water)
		{
			if(m_Core.m_Jetpack && Weapon != WEAPON_NINJA)
				Faketuning |= FAKETUNE_JETPACK;
			if(m_Core.m_Solo)
				Faketuning |= FAKETUNE_SOLO;
			if(m_Core.m_HammerHitDisabled)
				Faketuning |= FAKETUNE_NOHAMMER;
			if(m_Core.m_CollisionDisabled)
				Faketuning |= FAKETUNE_NOCOLL;
			if(m_Core.m_HookHitDisabled)
				Faketuning |= FAKETUNE_NOHOOK;
			if(!m_Core.m_EndlessJump && m_Core.m_Jumps == 0)
				Faketuning |= FAKETUNE_NOJUMP;
			if(m_Core.m_Mounted)
			{
				Faketuning |= FAKETUNE_NOHOOK;
				Faketuning |= FAKETUNE_NOCOLL;
				Faketuning |= FAKETUNE_NOJUMP;
				Faketuning |= FAKETUNE_NOHAMMER;
			}
		}
		if(Faketuning != m_NeededFaketuning || m_Water != m_SentWaterTune)
		{
			m_NeededFaketuning = Faketuning;
			m_SentWaterTune = m_Water;
			GameServer()->SendTuningParams(m_pPlayer->GetCid(), m_TuneZone); // update tunings
		}
	}

	// change eyes, use ninja graphic and set ammo count if player has ninjajetpack
	if(m_pPlayer->m_NinjaJetpack && m_Core.m_Jetpack && m_Core.m_ActiveWeapon == WEAPON_GUN && !m_Core.m_DeepFrozen && m_FreezeTime == 0 && !m_Core.m_HasTelegunGun)
	{
		if(Emote == EMOTE_NORMAL)
			Emote = EMOTE_HAPPY;
		Weapon = WEAPON_NINJA;
		AmmoCount = 10;
	}

	if(m_pPlayer->GetCid() == SnappingClient || SnappingClient == SERVER_DEMO_CLIENT ||
		(!g_Config.m_SvStrictSpectateMode && m_pPlayer->GetCid() == GameServer()->m_apPlayers[SnappingClient]->m_SpectatorId))
	{
		Health = m_Health;
		Armor = m_Armor;
		AmmoCount = (m_FreezeTime == 0) ? (m_Core.m_ActiveWeapon < NUM_WEAPONS ? m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_Ammo : m_aCustomWeaponAmmo[m_Core.m_ActiveWeapon - KZ_CUSTOM_WEAPON_START]) : 0;
	}
    if(!(((CServer*)Server())->m_aClients[m_pPlayer->GetCid()].m_KZBot))
	if(GetPlayer()->IsAfk() || GetPlayer()->IsPaused() || GetPlayer()->m_MenuAFK || Sitting())
	{
		if(m_FreezeTime > 0 || m_Core.m_DeepFrozen || m_Core.m_LiveFrozen)
			Emote = EMOTE_NORMAL;
		else
			Emote = EMOTE_BLINK;
	}

	if(Emote == EMOTE_NORMAL)
	{
		if(5 * Server()->TickSpeed() - ((Server()->Tick() - m_LastAction) % (5 * Server()->TickSpeed())) < 5)
			Emote = EMOTE_BLINK;
	}

	//+KZ
	if(GameServer()->m_pController->OnCharacterSnap(SnappingClient, Id))
		return;
	
	if(Weapon >= KZ_CUSTOM_WEAPON_START)
		m_SnapCustomWeapon = true;
	else
		m_SnapCustomWeapon = false;

	if(!Server()->IsSixup(SnappingClient))
	{
		CNetObj_Character *pCharacter = Server()->SnapNewItem<CNetObj_Character>(Id);
		if(!pCharacter)
			return;

		pCore->Write(pCharacter);

		pCharacter->m_Tick = Tick;
		pCharacter->m_Emote = Emote;

		if(pCharacter->m_HookedPlayer != -1)
		{
			if(!Server()->Translate(pCharacter->m_HookedPlayer, SnappingClient))
				pCharacter->m_HookedPlayer = -1;
		}

		pCharacter->m_AttackTick = m_AttackTick;
		pCharacter->m_Direction = m_Input.m_Direction;
		if(m_HasNoWeapon || m_HasFlagBall || m_DropFlagBallTicks > 0)
			pCharacter->m_Weapon = -1;
		else if(m_SnapCustomWeapon)
			pCharacter->m_Weapon = m_aCustomWeaponSnaps[m_Core.m_ActiveWeapon - KZ_CUSTOM_WEAPON_START];
		else
			pCharacter->m_Weapon = Weapon;
		pCharacter->m_AmmoCount = AmmoCount;
		pCharacter->m_Health = Health;
		pCharacter->m_Armor = Armor;
		pCharacter->m_PlayerFlags = GetPlayer()->m_PlayerFlags;
	}
	else
	{
		protocol7::CNetObj_Character *pCharacter = Server()->SnapNewItem<protocol7::CNetObj_Character>(Id);
		if(!pCharacter)
			return;

		pCore->Write(reinterpret_cast<CNetObj_CharacterCore *>(static_cast<protocol7::CNetObj_CharacterCore *>(pCharacter)));
		if(pCharacter->m_Angle > (int)(pi * 256.0f))
		{
			pCharacter->m_Angle -= (int)(2.0f * pi * 256.0f);
		}

		// m_HookTick can be negative when using the hook_duration tune, which 0.7 clients
		// will consider invalid. https://github.com/ddnet/ddnet/issues/3915
		pCharacter->m_HookTick = maximum(0, pCharacter->m_HookTick);

		pCharacter->m_Tick = Tick;
		pCharacter->m_Emote = Emote;
		pCharacter->m_AttackTick = m_AttackTick;
		pCharacter->m_Direction = m_Input.m_Direction;
		if(m_HasFlagBall || m_DropFlagBallTicks > 0)
			pCharacter->m_Weapon = -1;
		else if(m_SnapCustomWeapon)
			pCharacter->m_Weapon = m_aCustomWeaponSnaps[m_Core.m_ActiveWeapon - KZ_CUSTOM_WEAPON_START];
		else
			pCharacter->m_Weapon = Weapon;
		pCharacter->m_AmmoCount = AmmoCount;

		if(m_FreezeTime > 0 || m_Core.m_DeepFrozen)
			pCharacter->m_AmmoCount = m_Core.m_FreezeStart + g_Config.m_SvFreezeDelay * Server()->TickSpeed();
		else if(Weapon == WEAPON_NINJA)
			pCharacter->m_AmmoCount = m_Core.m_Ninja.m_ActivationTick + g_pData->m_Weapons.m_Ninja.m_Duration * Server()->TickSpeed() / 1000;

		pCharacter->m_Health = Health;
		pCharacter->m_Armor = Armor;
		pCharacter->m_TriggeredEvents = m_TriggeredEvents7;
	}
}

bool CCharacter::CanSnapCharacter(int SnappingClient)
{
	if(SnappingClient == SERVER_DEMO_CLIENT)
		return true;

	CCharacter *pSnapChar = GameServer()->GetPlayerChar(SnappingClient);
	CPlayer *pSnapPlayer = GameServer()->m_apPlayers[SnappingClient];

	if(pSnapPlayer->GetTeam() == TEAM_SPECTATORS || pSnapPlayer->IsPaused())
	{
		if(pSnapPlayer->m_SpectatorId != SPEC_FREEVIEW && !CanCollide(pSnapPlayer->m_SpectatorId) && (pSnapPlayer->m_ShowOthers == SHOW_OTHERS_OFF || (pSnapPlayer->m_ShowOthers == SHOW_OTHERS_ONLY_TEAM && !SameTeam(pSnapPlayer->m_SpectatorId))))
			return false;
		else if(pSnapPlayer->m_SpectatorId == SPEC_FREEVIEW && !CanCollide(SnappingClient) && pSnapPlayer->m_SpecTeam && !SameTeam(SnappingClient))
			return false;
	}
	else if(pSnapChar && !pSnapChar->m_Core.m_Super && !CanCollide(SnappingClient) && (pSnapPlayer->m_ShowOthers == SHOW_OTHERS_OFF || (pSnapPlayer->m_ShowOthers == SHOW_OTHERS_ONLY_TEAM && !SameTeam(SnappingClient))))
		return false;

	return true;
}

bool CCharacter::IsSnappingCharacterInView(int SnappingClientId)
{
	int Id = m_pPlayer->GetCid();

	// A player may not be clipped away if his hook or a hook attached to him is in the field of view
	bool PlayerAndHookNotInView = NetworkClippedLine(SnappingClientId, m_Pos, m_Core.m_HookPos);
	bool AttachedHookInView = false;
	if(PlayerAndHookNotInView)
	{
		for(const auto &AttachedPlayerId : m_Core.m_AttachedPlayers)
		{
			const CCharacter *pOtherPlayer = GameServer()->GetPlayerChar(AttachedPlayerId);
			if(pOtherPlayer && pOtherPlayer->m_Core.HookedPlayer() == Id)
			{
				if(!NetworkClippedLine(SnappingClientId, m_Pos, pOtherPlayer->m_Pos))
				{
					AttachedHookInView = true;
					break;
				}
			}
		}
	}
	if(PlayerAndHookNotInView && !AttachedHookInView)
	{
		return false;
	}
	return true;
}

void CCharacter::Snap(int SnappingClient)
{
	int Id = m_pPlayer->GetCid();
	int SnappingClientVersion = GameServer()->GetClientVersion(SnappingClient);
	bool Sixup = Server()->IsSixup(SnappingClient);

	if(m_Invisible)
	{
		if(Id != SnappingClient)
			return;
		
		vec2 postemp;
				
		postemp.x = m_Pos.x + 32*sin((float)Server()->Tick() / 25.0);
		postemp.y = m_Pos.y + 32*cos((float)Server()->Tick() / 25.0);
		
		//+KZ: indicator idea taken from catch16
		GameServer()->SnapPickup(CSnapContext(GameServer()->GetClientVersion(SnappingClient), Server()->IsSixup(SnappingClient)), m_InvisibleShieldId, postemp, POWERUP_ARMOR, 0, 0);
		
	}

	if(m_ShowCursor && Id == SnappingClient)
	{
		while(true)
		{
			vec2 postemp;
				
			postemp.x = m_Pos.x + m_Input.m_TargetX + 32*sin((float)(Server()->Tick() / 25.0) *2);
			postemp.y = m_Pos.y + m_Input.m_TargetY + 32*cos((float)(Server()->Tick() / 25.0) *2);

			CNetObj_Projectile *pProj = Server()->SnapNewItem<CNetObj_Projectile>(m_CursorId[0]);
			if(!pProj)
			{
				break;
			}
			pProj->m_X = postemp.x;
			pProj->m_Y = postemp.y;
			pProj->m_VelX = 0;
			pProj->m_VelY = 0;
			pProj->m_StartTick = Server()->Tick();
			pProj->m_Type = WEAPON_HAMMER;
			break;
		}
		while(true)
		{
			vec2 postemp;
				
			postemp.x = m_Pos.x + m_Input.m_TargetX + 32*sin(((float)(Server()->Tick() / 25.0) +200) *2);
			postemp.y = m_Pos.y + m_Input.m_TargetY + 32*cos(((float)(Server()->Tick() / 25.0) +200) *2);

			CNetObj_Projectile *pProj = Server()->SnapNewItem<CNetObj_Projectile>(m_CursorId[1]);
			if(!pProj)
			{
				break;
			}
			pProj->m_X = postemp.x;
			pProj->m_Y = postemp.y;
			pProj->m_VelX = 0;
			pProj->m_VelY = 0;
			pProj->m_StartTick = Server()->Tick();
			pProj->m_Type = WEAPON_HAMMER;
			break;
		}
	}

	while(m_HasBall)
	{
		vec2 postemp;
				
		postemp.x = m_Pos.x + 32*sin((float)(Server()->Tick() / 25.0) *2);
		postemp.y = m_Pos.y + 32*cos((float)(Server()->Tick() / 25.0) *2);

		CNetObj_Projectile *pProj = Server()->SnapNewItem<CNetObj_Projectile>(m_HasBallSnapId);
		if(!pProj)
		{
			break;
		}
		pProj->m_X = postemp.x;
		pProj->m_Y = postemp.y;
		pProj->m_VelX = 0;
		pProj->m_VelY = 0;
		pProj->m_StartTick = Server()->Tick();
		pProj->m_Type = WEAPON_HAMMER;
		break;
	}

	if(m_Core.m_ActiveWeapon == KZ_WEAPON_PORTAL_GUN || m_Core.m_ActiveWeapon == KZ_WEAPON_TASER)
	{
				vec2 postemp;
				
		postemp = m_Pos + (normalize(vec2(m_Input.m_TargetX,m_Input.m_TargetY)) * 82);

		GameServer()->SnapLaserObject(CSnapContext(SnappingClientVersion, Sixup),m_PortalKindId,postemp,postemp,Server()->Tick(),m_pPlayer->GetCid(),m_Core.m_ActiveWeapon == KZ_WEAPON_PORTAL_GUN ? (m_BluePortal ? LASERTYPE_RIFLE : LASERTYPE_SHOTGUN) : LASERTYPE_FREEZE);
	}
	
	if(!Server()->Translate(Id, SnappingClient))
		return;

	if(!CanSnapCharacter(SnappingClient))
	{
		return;
	}

	if(!IsSnappingCharacterInView(SnappingClient))
		return;

	SnapCharacter(SnappingClient, Id);

	CNetObj_DDNetCharacter *pDDNetCharacter = Server()->SnapNewItem<CNetObj_DDNetCharacter>(Id);
	if(!pDDNetCharacter)
		return;

	pDDNetCharacter->m_Flags = 0;
	if(m_Core.m_Solo)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_SOLO;
	if(m_Core.m_Super)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_SUPER;
	if(m_Core.m_Invincible || m_Sparkles)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_INVINCIBLE;
	if(m_Core.m_EndlessHook)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_ENDLESS_HOOK;
	if(m_Core.m_CollisionDisabled || !Tuning()->m_PlayerCollision)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_COLLISION_DISABLED;
	if(m_Core.m_HookHitDisabled || !Tuning()->m_PlayerHooking)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_HOOK_HIT_DISABLED;
	if(m_Core.m_EndlessJump || m_Water)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_ENDLESS_JUMP;
	if(m_Core.m_Jetpack)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_JETPACK;
	if(m_Core.m_HammerHitDisabled)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_HAMMER_HIT_DISABLED;
	if(m_Core.m_ShotgunHitDisabled)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_SHOTGUN_HIT_DISABLED;
	if(m_Core.m_GrenadeHitDisabled)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_GRENADE_HIT_DISABLED;
	if(m_Core.m_LaserHitDisabled)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_LASER_HIT_DISABLED;
	if(m_Core.m_HasTelegunGun)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_TELEGUN_GUN;
	if(m_Core.m_HasTelegunGrenade)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_TELEGUN_GRENADE;
	if(m_Core.m_HasTelegunLaser)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_TELEGUN_LASER;
	if(m_Core.m_aWeapons[WEAPON_HAMMER].m_Got && !m_HasBall && !m_HasFlagBall)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_HAMMER;
	if(m_Core.m_aWeapons[WEAPON_GUN].m_Got && !m_HasBall && !m_HasFlagBall)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_GUN;
	if(m_Core.m_aWeapons[WEAPON_SHOTGUN].m_Got && !m_HasBall && !m_HasFlagBall)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_SHOTGUN;
	if(m_Core.m_aWeapons[WEAPON_GRENADE].m_Got && !m_HasBall && !m_HasFlagBall)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_GRENADE;
	if(m_Core.m_aWeapons[WEAPON_LASER].m_Got && !m_HasBall && !m_HasFlagBall)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_LASER;
	if(m_Core.m_ActiveWeapon == WEAPON_NINJA && !m_HasBall && !m_HasFlagBall)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_NINJA;
	if(m_Core.m_LiveFrozen)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_MOVEMENTS_DISABLED;

	pDDNetCharacter->m_FreezeEnd = m_Core.m_DeepFrozen ? -1 : m_FreezeTime == 0 ? 0 : Server()->Tick() + m_FreezeTime;
	pDDNetCharacter->m_Jumps = m_Core.m_Jumps;
	pDDNetCharacter->m_TeleCheckpoint = m_TeleCheckpoint;
	pDDNetCharacter->m_StrongWeakId = m_StrongWeakId;

	// Display Information
	pDDNetCharacter->m_JumpedTotal = m_Core.m_JumpedTotal;
	pDDNetCharacter->m_NinjaActivationTick = m_Core.m_Ninja.m_ActivationTick;
	pDDNetCharacter->m_FreezeStart = m_Core.m_FreezeStart;
	if(m_Core.m_IsInFreeze)
	{
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_IN_FREEZE;
	}
	if(Teams()->IsPractice(Team()))
	{
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_PRACTICE_MODE;
	}
	if(Teams()->TeamLocked(Team()))
	{
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_LOCK_MODE;
	}
	if(Teams()->TeamFlock(Team()))
	{
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_TEAM0_MODE;
	}
	pDDNetCharacter->m_TargetX = m_Core.m_Input.m_TargetX;
	pDDNetCharacter->m_TargetY = m_Core.m_Input.m_TargetY;

	// ddnet-insta
	GameServer()->m_pController->SnapDDNetCharacter(SnappingClient, this, pDDNetCharacter);
}

void CCharacter::PostSnap()
{
	m_TriggeredEvents7 = 0;
}

// DDRace

bool CCharacter::CanCollide(int ClientId)
{
	return Teams()->m_Core.CanCollide(GetPlayer()->GetCid(), ClientId);
}
bool CCharacter::SameTeam(int ClientId)
{
	return Teams()->m_Core.SameTeam(GetPlayer()->GetCid(), ClientId);
}

int CCharacter::Team()
{
	return Teams()->m_Core.Team(m_pPlayer->GetCid());
}

void CCharacter::FillAntibot(CAntibotCharacterData *pData)
{
	pData->m_Pos = m_Pos;
	pData->m_Vel = m_Core.m_Vel;
	pData->m_Angle = m_Core.m_Angle;
	pData->m_HookedPlayer = m_Core.HookedPlayer();
	pData->m_SpawnTick = m_SpawnTick;
	pData->m_WeaponChangeTick = m_WeaponChangeTick;
	pData->m_aLatestInputs[0].m_TargetX = m_LatestInput.m_TargetX;
	pData->m_aLatestInputs[0].m_TargetY = m_LatestInput.m_TargetY;
	pData->m_aLatestInputs[1].m_TargetX = m_LatestPrevInput.m_TargetX;
	pData->m_aLatestInputs[1].m_TargetY = m_LatestPrevInput.m_TargetY;
	pData->m_aLatestInputs[2].m_TargetX = m_LatestPrevPrevInput.m_TargetX;
	pData->m_aLatestInputs[2].m_TargetY = m_LatestPrevPrevInput.m_TargetY;
}

void CCharacter::HandleBroadcast()
{
	CPlayerData *pData = GameServer()->Score()->PlayerData(m_pPlayer->GetCid());

	if(m_DDRaceState == DDRACE_STARTED && m_pPlayer->GetClientVersion() == VERSION_VANILLA &&
		m_LastTimeCpBroadcasted != m_LastTimeCp && m_LastTimeCp > -1 &&
		m_TimeCpBroadcastEndTick > Server()->Tick() && pData->m_BestTime && pData->m_aBestTimeCp[m_LastTimeCp] != 0)
	{
		char aBroadcast[128];
		float Diff = m_aCurrentTimeCp[m_LastTimeCp] - pData->m_aBestTimeCp[m_LastTimeCp];
		str_format(aBroadcast, sizeof(aBroadcast), "Checkpoint | Diff : %+5.2f", Diff);
		GameServer()->SendBroadcast(aBroadcast, m_pPlayer->GetCid());
		m_LastTimeCpBroadcasted = m_LastTimeCp;
		m_LastBroadcast = Server()->Tick();
	}
	else if((m_pPlayer->m_TimerType == CPlayer::TIMERTYPE_BROADCAST || m_pPlayer->m_TimerType == CPlayer::TIMERTYPE_GAMETIMER_AND_BROADCAST) && m_DDRaceState == DDRACE_STARTED && m_LastBroadcast + Server()->TickSpeed() * g_Config.m_SvTimeInBroadcastInterval <= Server()->Tick())
	{
		char aBuf[32];
		int Time = (int64_t)100 * ((float)(Server()->Tick() - m_StartTime) / ((float)Server()->TickSpeed()));
		str_time(Time, TIME_HOURS, aBuf, sizeof(aBuf));
		GameServer()->SendBroadcast(aBuf, m_pPlayer->GetCid(), false);
		m_LastTimeCpBroadcasted = m_LastTimeCp;
		m_LastBroadcast = Server()->Tick();
	}
}

void CCharacter::HandleSkippableTiles(int Index)
{
	// handle death-tiles and leaving gamelayer
	if((Collision()->GetCollisionAt(m_Pos.x + GetProximityRadius() / 3.f, m_Pos.y - GetProximityRadius() / 3.f) == TILE_DEATH ||
		   Collision()->GetCollisionAt(m_Pos.x + GetProximityRadius() / 3.f, m_Pos.y + GetProximityRadius() / 3.f) == TILE_DEATH ||
		   Collision()->GetCollisionAt(m_Pos.x - GetProximityRadius() / 3.f, m_Pos.y - GetProximityRadius() / 3.f) == TILE_DEATH ||
		   Collision()->GetCollisionAt(m_Pos.x - GetProximityRadius() / 3.f, m_Pos.y + GetProximityRadius() / 3.f) == TILE_DEATH ||
		   Collision()->GetFrontCollisionAt(m_Pos.x + GetProximityRadius() / 3.f, m_Pos.y - GetProximityRadius() / 3.f) == TILE_DEATH ||
		   Collision()->GetFrontCollisionAt(m_Pos.x + GetProximityRadius() / 3.f, m_Pos.y + GetProximityRadius() / 3.f) == TILE_DEATH ||
		   Collision()->GetFrontCollisionAt(m_Pos.x - GetProximityRadius() / 3.f, m_Pos.y - GetProximityRadius() / 3.f) == TILE_DEATH ||
		   Collision()->GetFrontCollisionAt(m_Pos.x - GetProximityRadius() / 3.f, m_Pos.y + GetProximityRadius() / 3.f) == TILE_DEATH) &&
		!m_Core.m_Super && !m_Core.m_Invincible && !(Team() && Teams()->TeeFinished(m_pPlayer->GetCid())))
	{
		if(Teams()->IsPractice(Team()))
		{
			Freeze();
			// Rate limit death effects to once per second
			if(Server()->Tick() - m_pPlayer->m_DieTick >= Server()->TickSpeed())
			{
				m_pPlayer->m_DieTick = Server()->Tick();
				GameServer()->CreateSound(m_Pos, SOUND_PLAYER_DIE, TeamMask());
				GameServer()->CreateDeath(m_Pos, m_pPlayer->GetCid(), TeamMask());
			}
		}
		else
			Die(m_pPlayer->GetCid(), WEAPON_WORLD);
		return;
	}

	if(GameLayerClipped(m_Pos))
	{
		Die(m_pPlayer->GetCid(), WEAPON_WORLD);
		return;
	}

	if(Index < 0)
		return;

	// handle speedup tiles
	if(Collision()->IsSpeedup(Index))
	{
		vec2 Direction, TempVel = m_Core.m_Vel;
		int Force, MaxSpeed = 0;
		float TeeAngle, SpeederAngle, DiffAngle, SpeedLeft, TeeSpeed;
		Collision()->GetSpeedup(Index, &Direction, &Force, &MaxSpeed);
		if(Force == 255 && MaxSpeed)
		{
			m_Core.m_Vel = Direction * (MaxSpeed / 5);
		}
		else
		{
			if(MaxSpeed > 0 && MaxSpeed < 5)
				MaxSpeed = 5;
			if(MaxSpeed > 0)
			{
				if(Direction.x > 0.0000001f)
					SpeederAngle = -std::atan(Direction.y / Direction.x);
				else if(Direction.x < 0.0000001f)
					SpeederAngle = std::atan(Direction.y / Direction.x) + 2.0f * std::asin(1.0f);
				else if(Direction.y > 0.0000001f)
					SpeederAngle = std::asin(1.0f);
				else
					SpeederAngle = std::asin(-1.0f);

				if(SpeederAngle < 0)
					SpeederAngle = 4.0f * std::asin(1.0f) + SpeederAngle;

				if(TempVel.x > 0.0000001f)
					TeeAngle = -std::atan(TempVel.y / TempVel.x);
				else if(TempVel.x < 0.0000001f)
					TeeAngle = std::atan(TempVel.y / TempVel.x) + 2.0f * std::asin(1.0f);
				else if(TempVel.y > 0.0000001f)
					TeeAngle = std::asin(1.0f);
				else
					TeeAngle = std::asin(-1.0f);

				if(TeeAngle < 0)
					TeeAngle = 4.0f * std::asin(1.0f) + TeeAngle;

				TeeSpeed = std::sqrt(std::pow(TempVel.x, 2) + std::pow(TempVel.y, 2));

				DiffAngle = SpeederAngle - TeeAngle;
				SpeedLeft = MaxSpeed / 5.0f - std::cos(DiffAngle) * TeeSpeed;
				if(absolute((int)SpeedLeft) > Force && SpeedLeft > 0.0000001f)
					TempVel += Direction * Force;
				else if(absolute((int)SpeedLeft) > Force)
					TempVel += Direction * -Force;
				else
					TempVel += Direction * SpeedLeft;
			}
			else
				TempVel += Direction * Force;

			m_Core.m_Vel = ClampVel(m_MoveRestrictions, TempVel);
		}
	}
}

bool CCharacter::IsSwitchActiveCb(int Number, void *pUser)
{
	CCharacter *pThis = (CCharacter *)pUser;
	auto &aSwitchers = pThis->Switchers();
	return !aSwitchers.empty() && pThis->Team() != TEAM_SUPER && aSwitchers[Number].m_aStatus[pThis->Team()];
}

void CCharacter::SetTimeCheckpoint(int TimeCheckpoint)
{
	if(TimeCheckpoint > -1 && m_DDRaceState == DDRACE_STARTED && m_aCurrentTimeCp[TimeCheckpoint] == 0.0f && m_Time != 0.0f)
	{
		m_LastTimeCp = TimeCheckpoint;
		m_aCurrentTimeCp[m_LastTimeCp] = m_Time;
		m_TimeCpBroadcastEndTick = Server()->Tick() + Server()->TickSpeed() * 2;
		if(m_pPlayer->GetClientVersion() >= VERSION_DDRACE)
		{
			CPlayerData *pData = GameServer()->Score()->PlayerData(m_pPlayer->GetCid());
			if(pData->m_aBestTimeCp[m_LastTimeCp] != 0.0f)
			{
				CNetMsg_Sv_DDRaceTime Msg;
				Msg.m_Time = (int)(m_Time * 100.0f);
				Msg.m_Finish = 0;
				float Diff = (m_aCurrentTimeCp[m_LastTimeCp] - pData->m_aBestTimeCp[m_LastTimeCp]) * 100;
				Msg.m_Check = (int)Diff;
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, m_pPlayer->GetCid());
			}
		}
	}
}

void CCharacter::HandleTiles(int Index)
{
	int MapIndex = Index;
	//int PureMapIndex = Collision()->GetPureMapIndex(m_Pos);
	m_TileIndex = Collision()->GetTileIndex(MapIndex);
	m_TileFIndex = Collision()->GetFrontTileIndex(MapIndex);
	m_MoveRestrictions = Collision()->GetMoveRestrictions(IsSwitchActiveCb, this, m_Pos, 18.0f, MapIndex);
	if(Index < 0)
	{
		m_LastRefillJumps = false;
		m_LastPenalty = false;
		m_LastBonus = false;
		return;
	}
	SetTimeCheckpoint(Collision()->IsTimeCheckpoint(MapIndex));
	SetTimeCheckpoint(Collision()->IsFrontTimeCheckpoint(MapIndex));
	int TeleCheckpoint = Collision()->IsTeleCheckpoint(MapIndex);
	if(TeleCheckpoint)
		m_TeleCheckpoint = TeleCheckpoint;

	GameServer()->m_pController->HandleCharacterTiles(this, Index);
	if(!m_Alive)
		return;

	// freeze
	if(((m_TileIndex == TILE_FREEZE) || (m_TileFIndex == TILE_FREEZE)) && !m_Core.m_Super && !m_Core.m_Invincible && !m_Core.m_DeepFrozen)
	{
		Freeze();
	}
	else if(((m_TileIndex == TILE_UNFREEZE) || (m_TileFIndex == TILE_UNFREEZE)) && !m_Core.m_DeepFrozen)
		UnFreeze();

	// deep freeze
	if(((m_TileIndex == TILE_DFREEZE) || (m_TileFIndex == TILE_DFREEZE)) && !m_Core.m_Super && !m_Core.m_Invincible && !m_Core.m_DeepFrozen)
		m_Core.m_DeepFrozen = true;
	else if(((m_TileIndex == TILE_DUNFREEZE) || (m_TileFIndex == TILE_DUNFREEZE)) && !m_Core.m_Super && !m_Core.m_Invincible && m_Core.m_DeepFrozen)
		m_Core.m_DeepFrozen = false;

	// live freeze
	if(((m_TileIndex == TILE_LFREEZE) || (m_TileFIndex == TILE_LFREEZE)) && !m_Core.m_Super && !m_Core.m_Invincible)
	{
		m_Core.m_LiveFrozen = true;
	}
	else if(((m_TileIndex == TILE_LUNFREEZE) || (m_TileFIndex == TILE_LUNFREEZE)) && !m_Core.m_Super && !m_Core.m_Invincible)
	{
		m_Core.m_LiveFrozen = false;

		//+KZ for pprace compat
		if(g_Config.m_SvPortalMode == 2)
		{
			for(CPortalKZ* p = (CPortalKZ*)GameWorld()->FindFirst(CGameWorld::CUSTOM_ENTTYPE_PORTAL);p;p = (CPortalKZ*)p->TypeNext())
			{
				if(p->m_Owner == m_pPlayer->GetCid())
				{
					p->Reset();
					CPortalKZ* p2 = p->GetOtherPortal();
					if(p2)
					{
						p2->Reset();
					}
					return;
				}
			}
		}
	}

	// endless hook
	if(((m_TileIndex == TILE_EHOOK_ENABLE) || (m_TileFIndex == TILE_EHOOK_ENABLE)))
	{
		SetEndlessHook(true);
	}
	else if(((m_TileIndex == TILE_EHOOK_DISABLE) || (m_TileFIndex == TILE_EHOOK_DISABLE)))
	{
		SetEndlessHook(false);
	}

	// hit others
	if(((m_TileIndex == TILE_HIT_DISABLE) || (m_TileFIndex == TILE_HIT_DISABLE)) && (!m_Core.m_HammerHitDisabled || !m_Core.m_ShotgunHitDisabled || !m_Core.m_GrenadeHitDisabled || !m_Core.m_LaserHitDisabled))
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can't hit others");
		m_Core.m_HammerHitDisabled = true;
		m_Core.m_ShotgunHitDisabled = true;
		m_Core.m_GrenadeHitDisabled = true;
		m_Core.m_LaserHitDisabled = true;
	}
	else if(((m_TileIndex == TILE_HIT_ENABLE) || (m_TileFIndex == TILE_HIT_ENABLE)) && (m_Core.m_HammerHitDisabled || m_Core.m_ShotgunHitDisabled || m_Core.m_GrenadeHitDisabled || m_Core.m_LaserHitDisabled))
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can hit others");
		m_Core.m_ShotgunHitDisabled = false;
		m_Core.m_GrenadeHitDisabled = false;
		m_Core.m_HammerHitDisabled = false;
		m_Core.m_LaserHitDisabled = false;
	}

	// collide with others
	if(((m_TileIndex == TILE_NPC_DISABLE) || (m_TileFIndex == TILE_NPC_DISABLE)) && !m_Core.m_CollisionDisabled)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can't collide with others");
		m_Core.m_CollisionDisabled = true;
	}
	else if(((m_TileIndex == TILE_NPC_ENABLE) || (m_TileFIndex == TILE_NPC_ENABLE)) && m_Core.m_CollisionDisabled)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can collide with others");
		m_Core.m_CollisionDisabled = false;
	}

	// hook others
	if(((m_TileIndex == TILE_NPH_DISABLE) || (m_TileFIndex == TILE_NPH_DISABLE)) && !m_Core.m_HookHitDisabled)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can't hook others");
		m_Core.m_HookHitDisabled = true;
	}
	else if(((m_TileIndex == TILE_NPH_ENABLE) || (m_TileFIndex == TILE_NPH_ENABLE)) && m_Core.m_HookHitDisabled)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can hook others");
		m_Core.m_HookHitDisabled = false;
	}

	// unlimited air jumps
	if(((m_TileIndex == TILE_UNLIMITED_JUMPS_ENABLE) || (m_TileFIndex == TILE_UNLIMITED_JUMPS_ENABLE)) && !m_Core.m_EndlessJump)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You have unlimited air jumps");
		m_Core.m_EndlessJump = true;
	}
	else if(((m_TileIndex == TILE_UNLIMITED_JUMPS_DISABLE) || (m_TileFIndex == TILE_UNLIMITED_JUMPS_DISABLE)) && m_Core.m_EndlessJump)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You don't have unlimited air jumps");
		m_Core.m_EndlessJump = false;
	}

	// walljump
	if((m_TileIndex == TILE_WALLJUMP) || (m_TileFIndex == TILE_WALLJUMP))
	{
		if(m_Core.m_Vel.y > 0 && m_Core.m_Colliding && m_Core.m_LeftWall)
		{
			m_Core.m_LeftWall = false;
			m_Core.m_JumpedTotal = m_Core.m_Jumps >= 2 ? m_Core.m_Jumps - 2 : 0;
			m_Core.m_Jumped = 1;
		}
	}

	// jetpack gun
	if(((m_TileIndex == TILE_JETPACK_ENABLE) || (m_TileFIndex == TILE_JETPACK_ENABLE)) && !m_Core.m_Jetpack)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You have a jetpack gun");
		m_Core.m_Jetpack = true;
	}
	else if(((m_TileIndex == TILE_JETPACK_DISABLE) || (m_TileFIndex == TILE_JETPACK_DISABLE)) && m_Core.m_Jetpack)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You lost your jetpack gun");
		m_Core.m_Jetpack = false;
	}

	// refill jumps
	if(((m_TileIndex == TILE_REFILL_JUMPS) || (m_TileFIndex == TILE_REFILL_JUMPS)) && !m_LastRefillJumps)
	{
		m_Core.m_JumpedTotal = 0;
		m_Core.m_Jumped = 0;
		m_LastRefillJumps = true;
	}
	if((m_TileIndex != TILE_REFILL_JUMPS) && (m_TileFIndex != TILE_REFILL_JUMPS))
	{
		m_LastRefillJumps = false;
	}

	// Teleport gun
	if(((m_TileIndex == TILE_TELE_GUN_ENABLE) || (m_TileFIndex == TILE_TELE_GUN_ENABLE)) && !m_Core.m_HasTelegunGun)
	{
		m_Core.m_HasTelegunGun = true;

		GameServer()->SendChatTarget(GetPlayer()->GetCid(), "Teleport gun enabled");
	}
	else if(((m_TileIndex == TILE_TELE_GUN_DISABLE) || (m_TileFIndex == TILE_TELE_GUN_DISABLE)) && m_Core.m_HasTelegunGun)
	{
		m_Core.m_HasTelegunGun = false;

		GameServer()->SendChatTarget(GetPlayer()->GetCid(), "Teleport gun disabled");
	}

	if(((m_TileIndex == TILE_TELE_GRENADE_ENABLE) || (m_TileFIndex == TILE_TELE_GRENADE_ENABLE)) && !m_Core.m_HasTelegunGrenade)
	{
		m_Core.m_HasTelegunGrenade = true;

		GameServer()->SendChatTarget(GetPlayer()->GetCid(), "Teleport grenade enabled");
	}
	else if(((m_TileIndex == TILE_TELE_GRENADE_DISABLE) || (m_TileFIndex == TILE_TELE_GRENADE_DISABLE)) && m_Core.m_HasTelegunGrenade)
	{
		m_Core.m_HasTelegunGrenade = false;

		GameServer()->SendChatTarget(GetPlayer()->GetCid(), "Teleport grenade disabled");
	}

	if(((m_TileIndex == TILE_TELE_LASER_ENABLE) || (m_TileFIndex == TILE_TELE_LASER_ENABLE)) && !m_Core.m_HasTelegunLaser)
	{
		m_Core.m_HasTelegunLaser = true;

		GameServer()->SendChatTarget(GetPlayer()->GetCid(), "Teleport laser enabled");
	}
	else if(((m_TileIndex == TILE_TELE_LASER_DISABLE) || (m_TileFIndex == TILE_TELE_LASER_DISABLE)) && m_Core.m_HasTelegunLaser)
	{
		m_Core.m_HasTelegunLaser = false;

		GameServer()->SendChatTarget(GetPlayer()->GetCid(), "Teleport laser disabled");
	}

	// stopper
	if(m_Core.m_Vel.y > 0 && (m_MoveRestrictions & CANTMOVE_DOWN))
	{
		m_Core.m_Jumped = 0;
		m_Core.m_JumpedTotal = 0;
	}
	ApplyMoveRestrictions();

	// handle switch tiles
	if(Collision()->GetSwitchType(MapIndex) == TILE_SWITCHOPEN && Team() != TEAM_SUPER && Collision()->GetSwitchNumber(MapIndex) > 0)
	{
		Switchers()[Collision()->GetSwitchNumber(MapIndex)].m_aStatus[Team()] = true;
		Switchers()[Collision()->GetSwitchNumber(MapIndex)].m_aEndTick[Team()] = 0;
		Switchers()[Collision()->GetSwitchNumber(MapIndex)].m_aType[Team()] = TILE_SWITCHOPEN;
		Switchers()[Collision()->GetSwitchNumber(MapIndex)].m_aLastUpdateTick[Team()] = Server()->Tick();
	}
	else if(Collision()->GetSwitchType(MapIndex) == TILE_SWITCHTIMEDOPEN && Team() != TEAM_SUPER && Collision()->GetSwitchNumber(MapIndex) > 0)
	{
		Switchers()[Collision()->GetSwitchNumber(MapIndex)].m_aStatus[Team()] = true;
		Switchers()[Collision()->GetSwitchNumber(MapIndex)].m_aEndTick[Team()] = Server()->Tick() + 1 + Collision()->GetSwitchDelay(MapIndex) * Server()->TickSpeed();
		Switchers()[Collision()->GetSwitchNumber(MapIndex)].m_aType[Team()] = TILE_SWITCHTIMEDOPEN;
		Switchers()[Collision()->GetSwitchNumber(MapIndex)].m_aLastUpdateTick[Team()] = Server()->Tick();
	}
	else if(Collision()->GetSwitchType(MapIndex) == TILE_SWITCHTIMEDCLOSE && Team() != TEAM_SUPER && Collision()->GetSwitchNumber(MapIndex) > 0)
	{
		Switchers()[Collision()->GetSwitchNumber(MapIndex)].m_aStatus[Team()] = false;
		Switchers()[Collision()->GetSwitchNumber(MapIndex)].m_aEndTick[Team()] = Server()->Tick() + 1 + Collision()->GetSwitchDelay(MapIndex) * Server()->TickSpeed();
		Switchers()[Collision()->GetSwitchNumber(MapIndex)].m_aType[Team()] = TILE_SWITCHTIMEDCLOSE;
		Switchers()[Collision()->GetSwitchNumber(MapIndex)].m_aLastUpdateTick[Team()] = Server()->Tick();
	}
	else if(Collision()->GetSwitchType(MapIndex) == TILE_SWITCHCLOSE && Team() != TEAM_SUPER && Collision()->GetSwitchNumber(MapIndex) > 0)
	{
		Switchers()[Collision()->GetSwitchNumber(MapIndex)].m_aStatus[Team()] = false;
		Switchers()[Collision()->GetSwitchNumber(MapIndex)].m_aEndTick[Team()] = 0;
		Switchers()[Collision()->GetSwitchNumber(MapIndex)].m_aType[Team()] = TILE_SWITCHCLOSE;
		Switchers()[Collision()->GetSwitchNumber(MapIndex)].m_aLastUpdateTick[Team()] = Server()->Tick();
	}
	else if(Collision()->GetSwitchType(MapIndex) == TILE_FREEZE && Team() != TEAM_SUPER && !m_Core.m_Invincible)
	{
		if(Collision()->GetSwitchNumber(MapIndex) == 0 || Switchers()[Collision()->GetSwitchNumber(MapIndex)].m_aStatus[Team()])
		{
			Freeze(Collision()->GetSwitchDelay(MapIndex));
		}
	}
	else if(Collision()->GetSwitchType(MapIndex) == TILE_DFREEZE && Team() != TEAM_SUPER && !m_Core.m_Invincible)
	{
		if(Collision()->GetSwitchNumber(MapIndex) == 0 || Switchers()[Collision()->GetSwitchNumber(MapIndex)].m_aStatus[Team()])
			m_Core.m_DeepFrozen = true;
	}
	else if(Collision()->GetSwitchType(MapIndex) == TILE_DUNFREEZE && Team() != TEAM_SUPER && !m_Core.m_Invincible)
	{
		if(Collision()->GetSwitchNumber(MapIndex) == 0 || Switchers()[Collision()->GetSwitchNumber(MapIndex)].m_aStatus[Team()])
			m_Core.m_DeepFrozen = false;
	}
	else if(Collision()->GetSwitchType(MapIndex) == TILE_LFREEZE && Team() != TEAM_SUPER && !m_Core.m_Invincible)
	{
		if(Collision()->GetSwitchNumber(MapIndex) == 0 || Switchers()[Collision()->GetSwitchNumber(MapIndex)].m_aStatus[Team()])
		{
			m_Core.m_LiveFrozen = true;
		}
	}
	else if(Collision()->GetSwitchType(MapIndex) == TILE_LUNFREEZE && Team() != TEAM_SUPER && !m_Core.m_Invincible)
	{
		if(Collision()->GetSwitchNumber(MapIndex) == 0 || Switchers()[Collision()->GetSwitchNumber(MapIndex)].m_aStatus[Team()])
		{
			m_Core.m_LiveFrozen = false;
		}
	}
	else if(Collision()->GetSwitchType(MapIndex) == TILE_HIT_ENABLE && m_Core.m_HammerHitDisabled && Collision()->GetSwitchDelay(MapIndex) == WEAPON_HAMMER)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can hammer hit others");
		m_Core.m_HammerHitDisabled = false;
	}
	else if(Collision()->GetSwitchType(MapIndex) == TILE_HIT_DISABLE && !(m_Core.m_HammerHitDisabled) && Collision()->GetSwitchDelay(MapIndex) == WEAPON_HAMMER)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can't hammer hit others");
		m_Core.m_HammerHitDisabled = true;
	}
	else if(Collision()->GetSwitchType(MapIndex) == TILE_HIT_ENABLE && m_Core.m_ShotgunHitDisabled && Collision()->GetSwitchDelay(MapIndex) == WEAPON_SHOTGUN)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can shoot others with shotgun");
		m_Core.m_ShotgunHitDisabled = false;
	}
	else if(Collision()->GetSwitchType(MapIndex) == TILE_HIT_DISABLE && !(m_Core.m_ShotgunHitDisabled) && Collision()->GetSwitchDelay(MapIndex) == WEAPON_SHOTGUN)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can't shoot others with shotgun");
		m_Core.m_ShotgunHitDisabled = true;
	}
	else if(Collision()->GetSwitchType(MapIndex) == TILE_HIT_ENABLE && m_Core.m_GrenadeHitDisabled && Collision()->GetSwitchDelay(MapIndex) == WEAPON_GRENADE)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can shoot others with grenade");
		m_Core.m_GrenadeHitDisabled = false;
	}
	else if(Collision()->GetSwitchType(MapIndex) == TILE_HIT_DISABLE && !(m_Core.m_GrenadeHitDisabled) && Collision()->GetSwitchDelay(MapIndex) == WEAPON_GRENADE)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can't shoot others with grenade");
		m_Core.m_GrenadeHitDisabled = true;
	}
	else if(Collision()->GetSwitchType(MapIndex) == TILE_HIT_ENABLE && m_Core.m_LaserHitDisabled && Collision()->GetSwitchDelay(MapIndex) == WEAPON_LASER)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can shoot others with laser");
		m_Core.m_LaserHitDisabled = false;
	}
	else if(Collision()->GetSwitchType(MapIndex) == TILE_HIT_DISABLE && !(m_Core.m_LaserHitDisabled) && Collision()->GetSwitchDelay(MapIndex) == WEAPON_LASER)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can't shoot others with laser");
		m_Core.m_LaserHitDisabled = true;
	}
	else if(Collision()->GetSwitchType(MapIndex) == TILE_JUMP)
	{
		int NewJumps = Collision()->GetSwitchDelay(MapIndex);
		if(NewJumps == 255)
		{
			NewJumps = -1;
		}

		if(NewJumps != m_Core.m_Jumps)
		{
			char aBuf[256];
			if(NewJumps == -1)
				str_copy(aBuf, "You only have your ground jump now");
			else if(NewJumps == 1)
				str_format(aBuf, sizeof(aBuf), "You can jump %d time", NewJumps);
			else
				str_format(aBuf, sizeof(aBuf), "You can jump %d times", NewJumps);
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), aBuf);
			m_Core.m_Jumps = NewJumps;
		}
	}
	else if(Collision()->GetSwitchType(MapIndex) == TILE_ADD_TIME && !m_LastPenalty)
	{
		int min = Collision()->GetSwitchDelay(MapIndex);
		int sec = Collision()->GetSwitchNumber(MapIndex);
		int Team = Teams()->m_Core.Team(m_Core.m_Id);

		m_StartTime -= (min * 60 + sec) * Server()->TickSpeed();

		if((g_Config.m_SvTeam == SV_TEAM_FORCED_SOLO || (Team != TEAM_FLOCK && !Teams()->TeamFlock(Team))) && Team != TEAM_SUPER)
		{
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(Teams()->m_Core.Team(i) == Team && i != m_Core.m_Id && GameServer()->m_apPlayers[i])
				{
					CCharacter *pChar = GameServer()->m_apPlayers[i]->GetCharacter();

					if(pChar)
						pChar->m_StartTime = m_StartTime;
				}
			}
		}

		m_LastPenalty = true;
	}
	else if(Collision()->GetSwitchType(MapIndex) == TILE_SUBTRACT_TIME && !m_LastBonus)
	{
		int min = Collision()->GetSwitchDelay(MapIndex);
		int sec = Collision()->GetSwitchNumber(MapIndex);
		int Team = Teams()->m_Core.Team(m_Core.m_Id);

		m_StartTime += (min * 60 + sec) * Server()->TickSpeed();
		if(m_StartTime > Server()->Tick())
			m_StartTime = Server()->Tick();

		if((g_Config.m_SvTeam == SV_TEAM_FORCED_SOLO || (Team != TEAM_FLOCK && !Teams()->TeamFlock(Team))) && Team != TEAM_SUPER)
		{
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(Teams()->m_Core.Team(i) == Team && i != m_Core.m_Id && GameServer()->m_apPlayers[i])
				{
					CCharacter *pChar = GameServer()->m_apPlayers[i]->GetCharacter();

					if(pChar)
						pChar->m_StartTime = m_StartTime;
				}
			}
		}

		m_LastBonus = true;
	}

	if(Collision()->GetSwitchType(MapIndex) != TILE_ADD_TIME)
	{
		m_LastPenalty = false;
	}

	if(Collision()->GetSwitchType(MapIndex) != TILE_SUBTRACT_TIME)
	{
		m_LastBonus = false;
	}

	int z = Collision()->IsTeleport(MapIndex);
	if(!g_Config.m_SvOldTeleportHook && !g_Config.m_SvOldTeleportWeapons && z && !Collision()->TeleOuts(z - 1).empty())
	{
		if(m_Core.m_Super || m_Core.m_Invincible)
			return;
		int TeleOut = GameWorld()->m_Core.RandomOr0(Collision()->TeleOuts(z - 1).size());
		m_Core.m_Pos = Collision()->TeleOuts(z - 1)[TeleOut];
		if(!g_Config.m_SvTeleportHoldHook)
		{
			ResetHook();
		}
		if(g_Config.m_SvTeleportLoseWeapons)
			ResetPickups();
		return;
	}
	int evilz = Collision()->IsEvilTeleport(MapIndex);
	if(evilz && !Collision()->TeleOuts(evilz - 1).empty())
	{
		if(m_Core.m_Super || m_Core.m_Invincible)
			return;
		int TeleOut = GameWorld()->m_Core.RandomOr0(Collision()->TeleOuts(evilz - 1).size());
		m_Core.m_Pos = Collision()->TeleOuts(evilz - 1)[TeleOut];
		if(!g_Config.m_SvOldTeleportHook && !g_Config.m_SvOldTeleportWeapons)
		{
			m_Core.m_Vel = vec2(0, 0);

			if(!g_Config.m_SvTeleportHoldHook)
			{
				ResetHook();
				GameWorld()->ReleaseHooked(GetPlayer()->GetCid());
			}
			if(g_Config.m_SvTeleportLoseWeapons)
			{
				ResetPickups();
			}
		}
		return;
	}
	if(Collision()->IsCheckEvilTeleport(MapIndex))
	{
		if(m_Core.m_Super || m_Core.m_Invincible)
			return;
		// first check if there is a TeleCheckOut for the current recorded checkpoint, if not check previous checkpoints
		for(int k = m_TeleCheckpoint - 1; k >= 0; k--)
		{
			if(!Collision()->TeleCheckOuts(k).empty())
			{
				int TeleOut = GameWorld()->m_Core.RandomOr0(Collision()->TeleCheckOuts(k).size());
				m_Core.m_Pos = Collision()->TeleCheckOuts(k)[TeleOut];
				m_Core.m_Vel = vec2(0, 0);

				if(!g_Config.m_SvTeleportHoldHook)
				{
					ResetHook();
					GameWorld()->ReleaseHooked(GetPlayer()->GetCid());
				}

				return;
			}
		}
		// if no checkpointout have been found (or if there no recorded checkpoint), teleport to start
		vec2 SpawnPos;
		if(GameServer()->m_pController->CanSpawn(m_pPlayer->GetTeam(), &SpawnPos, GameServer()->GetDDRaceTeam(GetPlayer()->GetCid())))
		{
			m_Core.m_Pos = SpawnPos;
			m_Core.m_Vel = vec2(0, 0);

			if(!g_Config.m_SvTeleportHoldHook)
			{
				ResetHook();
				GameWorld()->ReleaseHooked(GetPlayer()->GetCid());
			}
		}
		return;
	}
	if(Collision()->IsCheckTeleport(MapIndex))
	{
		if(m_Core.m_Super || m_Core.m_Invincible)
			return;
		// first check if there is a TeleCheckOut for the current recorded checkpoint, if not check previous checkpoints
		for(int k = m_TeleCheckpoint - 1; k >= 0; k--)
		{
			if(!Collision()->TeleCheckOuts(k).empty())
			{
				int TeleOut = GameWorld()->m_Core.RandomOr0(Collision()->TeleCheckOuts(k).size());
				m_Core.m_Pos = Collision()->TeleCheckOuts(k)[TeleOut];

				if(!g_Config.m_SvTeleportHoldHook)
				{
					ResetHook();
				}

				return;
			}
		}
		// if no checkpointout have been found (or if there no recorded checkpoint), teleport to start
		vec2 SpawnPos;
		if(GameServer()->m_pController->CanSpawn(m_pPlayer->GetTeam(), &SpawnPos, GameServer()->GetDDRaceTeam(GetPlayer()->GetCid())))
		{
			m_Core.m_Pos = SpawnPos;

			if(!g_Config.m_SvTeleportHoldHook)
			{
				ResetHook();
			}
		}
		return;
	}
}

void CCharacter::HandleQuads()
{
	CQuad *pQuad = nullptr;
	int StartNum = 0;
	int Number = 0;
	int Delay = 0;
	while(true)
	{
		StartNum = Collision()->GetQuadAt(m_Pos, &pQuad, StartNum);
		StartNum++;
		if(!pQuad)
			break;

		Number = pQuad->m_aColors[0].r;
		if(Number == 0)
			Number++;
		Delay = pQuad->m_aColors[0].g;

		//printf("QuadenvOffset: %d\n",pQuad->m_ColorEnvOffset);

		if(pQuad->m_ColorEnvOffset == TILE_FREEZE)
		{
			Freeze();
		}
		if(pQuad->m_ColorEnvOffset == TILE_DFREEZE)
		{
			m_Core.m_DeepFrozen = true;
		}
		if(pQuad->m_ColorEnvOffset == TILE_LFREEZE)
		{
			m_Core.m_LiveFrozen = true;
		}
		if(pQuad->m_ColorEnvOffset == TILE_DUNFREEZE)
		{
			m_Core.m_DeepFrozen = false;
		}
		if(pQuad->m_ColorEnvOffset == TILE_LUNFREEZE)
		{
			m_Core.m_LiveFrozen = false;
		}
		if(pQuad->m_ColorEnvOffset == TILE_UNFREEZE)
		{
			UnFreeze();
		}
		if(pQuad->m_ColorEnvOffset == TILE_DEATH)
		{
			Die(m_pPlayer->GetCid(), WEAPON_WORLD);
		}

		// endless hook
		if(pQuad->m_ColorEnvOffset == TILE_EHOOK_ENABLE)
		{
			SetEndlessHook(true);
		}
		else if(pQuad->m_ColorEnvOffset == TILE_EHOOK_DISABLE)
		{
			SetEndlessHook(false);
		}

		// hit others
		if((pQuad->m_ColorEnvOffset == TILE_HIT_DISABLE) && (!m_Core.m_HammerHitDisabled || !m_Core.m_ShotgunHitDisabled || !m_Core.m_GrenadeHitDisabled || !m_Core.m_LaserHitDisabled))
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can't hit others");
			m_Core.m_HammerHitDisabled = true;
			m_Core.m_ShotgunHitDisabled = true;
			m_Core.m_GrenadeHitDisabled = true;
			m_Core.m_LaserHitDisabled = true;
		}
		else if((pQuad->m_ColorEnvOffset == TILE_HIT_ENABLE) && (m_Core.m_HammerHitDisabled || m_Core.m_ShotgunHitDisabled || m_Core.m_GrenadeHitDisabled || m_Core.m_LaserHitDisabled))
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can hit others");
			m_Core.m_ShotgunHitDisabled = false;
			m_Core.m_GrenadeHitDisabled = false;
			m_Core.m_HammerHitDisabled = false;
			m_Core.m_LaserHitDisabled = false;
		}

		// collide with others
		if((pQuad->m_ColorEnvOffset == TILE_NPC_DISABLE) && !m_Core.m_CollisionDisabled)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can't collide with others");
			m_Core.m_CollisionDisabled = true;
		}
		else if((pQuad->m_ColorEnvOffset == TILE_NPC_ENABLE) && m_Core.m_CollisionDisabled)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can collide with others");
			m_Core.m_CollisionDisabled = false;
		}

		// hook others
		if((pQuad->m_ColorEnvOffset == TILE_NPH_DISABLE) && !m_Core.m_HookHitDisabled)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can't hook others");
			m_Core.m_HookHitDisabled = true;
		}
		else if((pQuad->m_ColorEnvOffset == TILE_NPH_ENABLE) && m_Core.m_HookHitDisabled)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can hook others");
			m_Core.m_HookHitDisabled = false;
		}

		// unlimited air jumps
		if((pQuad->m_ColorEnvOffset == TILE_UNLIMITED_JUMPS_ENABLE) && !m_Core.m_EndlessJump)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You have unlimited air jumps");
			m_Core.m_EndlessJump = true;
		}
		else if((pQuad->m_ColorEnvOffset == TILE_UNLIMITED_JUMPS_DISABLE) && m_Core.m_EndlessJump)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You don't have unlimited air jumps");
			m_Core.m_EndlessJump = false;
		}

		// walljump
		if(pQuad->m_ColorEnvOffset == TILE_WALLJUMP)
		{
			if(m_Core.m_Vel.y > 0 && m_Core.m_Colliding && m_Core.m_LeftWall)
			{
				m_Core.m_LeftWall = false;
				m_Core.m_JumpedTotal = m_Core.m_Jumps >= 2 ? m_Core.m_Jumps - 2 : 0;
				m_Core.m_Jumped = 1;
			}
		}

		// jetpack gun
		if(((pQuad->m_ColorEnvOffset == TILE_JETPACK_ENABLE)) && !m_Core.m_Jetpack)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You have a jetpack gun");
			m_Core.m_Jetpack = true;
		}
		else if(((pQuad->m_ColorEnvOffset == TILE_JETPACK_DISABLE)) && m_Core.m_Jetpack)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You lost your jetpack gun");
			m_Core.m_Jetpack = false;
		}

		// Teleport gun
		if((pQuad->m_ColorEnvOffset == TILE_TELE_GUN_ENABLE) && !m_Core.m_HasTelegunGun)
		{
			m_Core.m_HasTelegunGun = true;

			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "Teleport gun enabled");
		}
		else if((pQuad->m_ColorEnvOffset == TILE_TELE_GUN_DISABLE) && m_Core.m_HasTelegunGun)
		{
			m_Core.m_HasTelegunGun = false;

			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "Teleport gun disabled");
		}

		if((pQuad->m_ColorEnvOffset == TILE_TELE_GRENADE_ENABLE) && !m_Core.m_HasTelegunGrenade)
		{
			m_Core.m_HasTelegunGrenade = true;

			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "Teleport grenade enabled");
		}
		else if((pQuad->m_ColorEnvOffset == TILE_TELE_GRENADE_DISABLE) && m_Core.m_HasTelegunGrenade)
		{
			m_Core.m_HasTelegunGrenade = false;

			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "Teleport grenade disabled");
		}

		if((pQuad->m_ColorEnvOffset == TILE_TELE_LASER_ENABLE) && !m_Core.m_HasTelegunLaser)
		{
			m_Core.m_HasTelegunLaser = true;

			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "Teleport laser enabled");
		}
		else if((pQuad->m_ColorEnvOffset == TILE_TELE_LASER_DISABLE) && m_Core.m_HasTelegunLaser)
		{
			m_Core.m_HasTelegunLaser = false;

			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "Teleport laser disabled");
		}
		else if(pQuad->m_ColorEnvOffset == TILE_HIT_ENABLE && m_Core.m_HammerHitDisabled && Delay == WEAPON_HAMMER)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can hammer hit others");
			m_Core.m_HammerHitDisabled = false;
		}
		else if(pQuad->m_ColorEnvOffset == TILE_HIT_DISABLE && !(m_Core.m_HammerHitDisabled) && Delay == WEAPON_HAMMER)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can't hammer hit others");
			m_Core.m_HammerHitDisabled = true;
		}
		else if(pQuad->m_ColorEnvOffset == TILE_HIT_ENABLE && m_Core.m_ShotgunHitDisabled && Delay == WEAPON_SHOTGUN)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can shoot others with shotgun");
			m_Core.m_ShotgunHitDisabled = false;
		}
		else if(pQuad->m_ColorEnvOffset == TILE_HIT_DISABLE && !(m_Core.m_ShotgunHitDisabled) && Delay == WEAPON_SHOTGUN)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can't shoot others with shotgun");
			m_Core.m_ShotgunHitDisabled = true;
		}
		else if(pQuad->m_ColorEnvOffset == TILE_HIT_ENABLE && m_Core.m_GrenadeHitDisabled && Delay == WEAPON_GRENADE)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can shoot others with grenade");
			m_Core.m_GrenadeHitDisabled = false;
		}
		else if(pQuad->m_ColorEnvOffset == TILE_HIT_DISABLE && !(m_Core.m_GrenadeHitDisabled) && Delay == WEAPON_GRENADE)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can't shoot others with grenade");
			m_Core.m_GrenadeHitDisabled = true;
		}
		else if(pQuad->m_ColorEnvOffset == TILE_HIT_ENABLE && m_Core.m_LaserHitDisabled && Delay == WEAPON_LASER)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can shoot others with laser");
			m_Core.m_LaserHitDisabled = false;
		}
		else if(pQuad->m_ColorEnvOffset == TILE_HIT_DISABLE && !(m_Core.m_LaserHitDisabled) && Delay == WEAPON_LASER)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), "You can't shoot others with laser");
			m_Core.m_LaserHitDisabled = true;
		}
		else if(pQuad->m_ColorEnvOffset == TILE_JUMP)
		{
			int NewJumps = Delay;
			if(NewJumps == 255)
			{
				NewJumps = -1;
			}

			if(NewJumps != m_Core.m_Jumps)
			{
				char aBuf[256];
				if(NewJumps == -1)
					str_copy(aBuf, "You only have your ground jump now");
				else if(NewJumps == 1)
					str_format(aBuf, sizeof(aBuf), "You can jump %d time", NewJumps);
				else
					str_format(aBuf, sizeof(aBuf), "You can jump %d times", NewJumps);
				GameServer()->SendChatTarget(GetPlayer()->GetCid(), aBuf);
				m_Core.m_Jumps = NewJumps;
			}
		}

		// handle switch tiles
		if(pQuad->m_ColorEnvOffset == TILE_SWITCHOPEN && Team() != TEAM_SUPER && Number > 0)
		{
			Switchers()[Number].m_aStatus[Team()] = true;
			Switchers()[Number].m_aEndTick[Team()] = 0;
			Switchers()[Number].m_aType[Team()] = TILE_SWITCHOPEN;
			Switchers()[Number].m_aLastUpdateTick[Team()] = Server()->Tick();
		}
		else if(pQuad->m_ColorEnvOffset == TILE_SWITCHTIMEDOPEN && Team() != TEAM_SUPER && Number > 0)
		{
			Switchers()[Number].m_aStatus[Team()] = true;
			Switchers()[Number].m_aEndTick[Team()] = Server()->Tick() + 1 + Delay * Server()->TickSpeed();
			Switchers()[Number].m_aType[Team()] = TILE_SWITCHTIMEDOPEN;
			Switchers()[Number].m_aLastUpdateTick[Team()] = Server()->Tick();
		}
		else if(pQuad->m_ColorEnvOffset == TILE_SWITCHTIMEDCLOSE && Team() != TEAM_SUPER && Number > 0)
		{
			Switchers()[Number].m_aStatus[Team()] = false;
			Switchers()[Number].m_aEndTick[Team()] = Server()->Tick() + 1 + Delay * Server()->TickSpeed();
			Switchers()[Number].m_aType[Team()] = TILE_SWITCHTIMEDCLOSE;
			Switchers()[Number].m_aLastUpdateTick[Team()] = Server()->Tick();
		}
		else if(pQuad->m_ColorEnvOffset == TILE_SWITCHCLOSE && Team() != TEAM_SUPER && Number > 0)
		{
			Switchers()[Number].m_aStatus[Team()] = false;
			Switchers()[Number].m_aEndTick[Team()] = 0;
			Switchers()[Number].m_aType[Team()] = TILE_SWITCHCLOSE;
			Switchers()[Number].m_aLastUpdateTick[Team()] = Server()->Tick();
		}

		//Teleports
		if(!g_Config.m_SvOldTeleportHook && !g_Config.m_SvOldTeleportWeapons && pQuad->m_ColorEnvOffset == TILE_TELEIN && !Collision()->TeleOuts(Number - 1).empty())
		{
			if(!(m_Core.m_Super || m_Core.m_Invincible))
			{
				int TeleOut = GameWorld()->m_Core.RandomOr0(Collision()->TeleOuts(Number - 1).size());
				m_Core.m_Pos = Collision()->TeleOuts(Number - 1)[TeleOut];
				if(!g_Config.m_SvTeleportHoldHook)
				{
					ResetHook();
				}
				if(g_Config.m_SvTeleportLoseWeapons)
					ResetPickups();
			}
		}
		if(pQuad->m_ColorEnvOffset == TILE_TELEINEVIL && !Collision()->TeleOuts(Number - 1).empty())
		{
			if(!(m_Core.m_Super || m_Core.m_Invincible))
			{
				int TeleOut = GameWorld()->m_Core.RandomOr0(Collision()->TeleOuts(Number - 1).size());
				m_Core.m_Pos = Collision()->TeleOuts(Number - 1)[TeleOut];
				if(!g_Config.m_SvOldTeleportHook && !g_Config.m_SvOldTeleportWeapons)
				{
					m_Core.m_Vel = vec2(0, 0);

					if(!g_Config.m_SvTeleportHoldHook)
					{
						ResetHook();
						GameWorld()->ReleaseHooked(GetPlayer()->GetCid());
					}
					if(g_Config.m_SvTeleportLoseWeapons)
					{
						ResetPickups();
					}
				}
			}
		}
		if(pQuad->m_ColorEnvOffset == TILE_TELECHECKINEVIL)
		{
			if(!(m_Core.m_Super || m_Core.m_Invincible))
			{
				bool dontteletospawn = false;
				// first check if there is a TeleCheckOut for the current recorded checkpoint, if not check previous checkpoints
				for(int k = m_TeleCheckpoint - 1; k >= 0; k--)
				{
					if(!Collision()->TeleCheckOuts(k).empty())
					{
						int TeleOut = GameWorld()->m_Core.RandomOr0(Collision()->TeleCheckOuts(k).size());
						m_Core.m_Pos = Collision()->TeleCheckOuts(k)[TeleOut];
						m_Core.m_Vel = vec2(0, 0);

						if(!g_Config.m_SvTeleportHoldHook)
						{
							ResetHook();
							GameWorld()->ReleaseHooked(GetPlayer()->GetCid());
						}

						dontteletospawn = true;
						break;
					}
				}
				// if no checkpointout have been found (or if there no recorded checkpoint), teleport to start
				if(!dontteletospawn)
				{
					vec2 SpawnPos;
					if(GameServer()->m_pController->CanSpawn(m_pPlayer->GetTeam(), &SpawnPos, GameServer()->GetDDRaceTeam(GetPlayer()->GetCid())))
					{
						m_Core.m_Pos = SpawnPos;
						m_Core.m_Vel = vec2(0, 0);

						if(!g_Config.m_SvTeleportHoldHook)
						{
							ResetHook();
							GameWorld()->ReleaseHooked(GetPlayer()->GetCid());
						}
					}
				}
			}
		}
		if(pQuad->m_ColorEnvOffset == TILE_TELECHECKIN)
		{
			if(!(m_Core.m_Super || m_Core.m_Invincible))
			{
				bool dontteletospawn = false;
				// first check if there is a TeleCheckOut for the current recorded checkpoint, if not check previous checkpoints
				for(int k = m_TeleCheckpoint - 1; k >= 0; k--)
				{
					if(!Collision()->TeleCheckOuts(k).empty())
					{
						int TeleOut = GameWorld()->m_Core.RandomOr0(Collision()->TeleCheckOuts(k).size());
						m_Core.m_Pos = Collision()->TeleCheckOuts(k)[TeleOut];

						if(!g_Config.m_SvTeleportHoldHook)
						{
							ResetHook();
						}
						dontteletospawn = true;
						break;
					}
				}
				// if no checkpointout have been found (or if there no recorded checkpoint), teleport to start
				if(!dontteletospawn)
				{
					vec2 SpawnPos;
					if(GameServer()->m_pController->CanSpawn(m_pPlayer->GetTeam(), &SpawnPos, GameServer()->GetDDRaceTeam(GetPlayer()->GetCid())))
					{
						m_Core.m_Pos = SpawnPos;

						if(!g_Config.m_SvTeleportHoldHook)
						{
							ResetHook();
						}
					}
				}
			}
		}
		GameServer()->m_pController->HandleCharacterQuad(this, pQuad);
	}
}

void CCharacter::HandleTuneLayer()
{
	m_TuneZoneOld = m_TuneZone;
	int CurrentIndex = Collision()->GetMapIndex(m_Pos);
	m_TuneZone = Collision()->IsTune(CurrentIndex);

	if(m_TuneZone)
		m_Core.m_Tuning = TuningList()[m_TuneZone]; // throw tunings from specific zone into gamecore
	else
		m_Core.m_Tuning = *Tuning();
	
	//+KZ
	if(m_Water && !m_TuneZone)
	{
		m_Core.m_Tuning.m_Gravity = 0.3f;
		m_Core.m_Tuning.m_GroundFriction = 0.9f;
		m_Core.m_Tuning.m_GroundControlSpeed = 6.0f;
		m_Core.m_Tuning.m_GroundJumpImpulse = 6.0f;
		m_Core.m_Tuning.m_AirFriction = 0.9f;
		m_Core.m_Tuning.m_AirControlSpeed = 6.0f;
		m_Core.m_Tuning.m_AirJumpImpulse = 6.0f;
		m_Core.m_Tuning.m_HookDragSpeed = 7.0f;
	}

	if(m_TuneZone != m_TuneZoneOld) // don't send tunigs all the time
	{
		// send zone msgs
		SendZoneMsgs();
	}
}

void CCharacter::SendZoneMsgs()
{
	// send zone leave msg
	// (m_TuneZoneOld >= 0: avoid zone leave msgs on spawn)
	if(m_TuneZoneOld >= 0 && GameServer()->m_aaZoneLeaveMsg[m_TuneZoneOld][0])
	{
		const char *pCur = GameServer()->m_aaZoneLeaveMsg[m_TuneZoneOld];
		const char *pPos;
		while((pPos = str_find(pCur, "\\n")))
		{
			char aBuf[256];
			str_copy(aBuf, pCur, pPos - pCur + 1);
			aBuf[pPos - pCur + 1] = '\0';
			pCur = pPos + 2;
			GameServer()->SendChatTarget(m_pPlayer->GetCid(), aBuf);
		}
		GameServer()->SendChatTarget(m_pPlayer->GetCid(), pCur);
	}
	// send zone enter msg
	if(GameServer()->m_aaZoneEnterMsg[m_TuneZone][0])
	{
		const char *pCur = GameServer()->m_aaZoneEnterMsg[m_TuneZone];
		const char *pPos;
		while((pPos = str_find(pCur, "\\n")))
		{
			char aBuf[256];
			str_copy(aBuf, pCur, pPos - pCur + 1);
			aBuf[pPos - pCur + 1] = '\0';
			pCur = pPos + 2;
			GameServer()->SendChatTarget(m_pPlayer->GetCid(), aBuf);
		}
		GameServer()->SendChatTarget(m_pPlayer->GetCid(), pCur);
	}
}

IAntibot *CCharacter::Antibot()
{
	return GameServer()->Antibot();
}

void CCharacter::SetTeams(CGameTeams *pTeams)
{
	m_pTeams = pTeams;
	m_Core.SetTeamsCore(&m_pTeams->m_Core);
}

bool CCharacter::TrySetRescue(int RescueMode)
{
	bool Set = false;
	if(g_Config.m_SvRescue || ((g_Config.m_SvTeam == SV_TEAM_FORCED_SOLO || Team() > TEAM_FLOCK) && Team() >= TEAM_FLOCK && Team() < TEAM_SUPER))
	{
		// check for nearby health pickups (also freeze)
		bool InHealthPickup = false;
		if(!m_Core.m_IsInFreeze)
		{
			CEntity *apEnts[9];
			int Num = GameWorld()->FindEntities(m_Pos, GetProximityRadius() + CPickup::ms_CollisionExtraSize, apEnts, std::size(apEnts), CGameWorld::ENTTYPE_PICKUP);
			for(int i = 0; i < Num; ++i)
			{
				CPickup *pPickup = static_cast<CPickup *>(apEnts[i]);
				if(pPickup->Type() == POWERUP_HEALTH)
				{
					// This uses a separate variable InHealthPickup instead of setting m_Core.m_IsInFreeze
					// as the latter causes freezebars to flicker when standing in the freeze range of a
					// health pickup. When the same code for client prediction is added, the freezebars
					// still flicker, but only when standing at the edge of the health pickup's freeze range.
					InHealthPickup = true;
					break;
				}
			}
		}

		if(!m_Core.m_IsInFreeze && IsGrounded() && !m_Core.m_DeepFrozen && !InHealthPickup)
		{
			ForceSetRescue(RescueMode);
			Set = true;
		}
	}

	return Set;
}

void CCharacter::ForceSetRescue(int RescueMode)
{
	m_RescueTee[RescueMode].Save(this);
	m_SetSavePos[RescueMode] = true;
}

void CCharacter::DDRaceTick()
{
	mem_copy(&m_Input, &m_SavedInput, sizeof(m_Input));
	GameServer()->m_pController->SetArmorProgress(this, m_FreezeTime);
	if(m_Input.m_Direction != 0 || m_Input.m_Jump != 0)
		m_LastMove = Server()->Tick();

	if(m_Core.m_LiveFrozen && !m_Core.m_Super && !m_Core.m_Invincible)
	{
		m_Input.m_Direction = 0;
		m_Input.m_Jump = 0;
		// Hook is possible in live freeze
	}
	if(m_FreezeTime > 0)
	{
		if(m_FreezeTime % Server()->TickSpeed() == Server()->TickSpeed() - 1)
		{
			// ddnet-insta added FreezeDamageIndicatorMask for fng
			GameServer()->CreateDamageInd(m_Pos, 0, (m_FreezeTime + 1) / Server()->TickSpeed(), GameServer()->m_pController->FreezeDamageIndicatorMask(this));
		}
		m_FreezeTime--;
		m_Input.m_Direction = 0;
		m_Input.m_Jump = 0;
		m_Input.m_Hook = 0;
		if(m_FreezeTime == 1)
			UnFreeze();
	}

	HandleTuneLayer(); // need this before coretick

	// check if the tee is in any type of freeze
	int Index = Collision()->GetPureMapIndex(m_Pos);
	const int aTiles[] = {
		Collision()->GetTileIndex(Index),
		Collision()->GetFrontTileIndex(Index),
		Collision()->GetSwitchType(Index)};
	m_Core.m_IsInFreeze = false;
	for(const int Tile : aTiles)
	{
		if(Tile == TILE_FREEZE || Tile == TILE_DFREEZE || Tile == TILE_LFREEZE || Tile == TILE_DEATH)
		{
			m_Core.m_IsInFreeze = true;
			break;
		}
	}
	m_Core.m_IsInFreeze |= (Collision()->GetCollisionAt(m_Pos.x + GetProximityRadius() / 3.f, m_Pos.y - GetProximityRadius() / 3.f) == TILE_DEATH ||
				Collision()->GetCollisionAt(m_Pos.x + GetProximityRadius() / 3.f, m_Pos.y + GetProximityRadius() / 3.f) == TILE_DEATH ||
				Collision()->GetCollisionAt(m_Pos.x - GetProximityRadius() / 3.f, m_Pos.y - GetProximityRadius() / 3.f) == TILE_DEATH ||
				Collision()->GetCollisionAt(m_Pos.x - GetProximityRadius() / 3.f, m_Pos.y + GetProximityRadius() / 3.f) == TILE_DEATH ||
				Collision()->GetFrontCollisionAt(m_Pos.x + GetProximityRadius() / 3.f, m_Pos.y - GetProximityRadius() / 3.f) == TILE_DEATH ||
				Collision()->GetFrontCollisionAt(m_Pos.x + GetProximityRadius() / 3.f, m_Pos.y + GetProximityRadius() / 3.f) == TILE_DEATH ||
				Collision()->GetFrontCollisionAt(m_Pos.x - GetProximityRadius() / 3.f, m_Pos.y - GetProximityRadius() / 3.f) == TILE_DEATH ||
				Collision()->GetFrontCollisionAt(m_Pos.x - GetProximityRadius() / 3.f, m_Pos.y + GetProximityRadius() / 3.f) == TILE_DEATH);

	// look for save position for rescue feature
	// always update auto rescue
	TrySetRescue(RESCUEMODE_AUTO);

	m_Core.m_Id = GetPlayer()->GetCid();
}

void CCharacter::DDRacePostCoreTick()
{
	m_Time = (float)(Server()->Tick() - m_StartTime) / ((float)Server()->TickSpeed());

	if(m_Core.m_EndlessHook || (m_Core.m_Super && g_Config.m_SvEndlessSuperHook))
		m_Core.m_HookTick = 0;

	m_FrozenLastTick = false;

	if(m_Core.m_DeepFrozen && !m_Core.m_Super && !m_Core.m_Invincible)
		Freeze();

	// following jump rules can be overridden by tiles, like Refill Jumps, Stopper and Wall Jump
	if(m_Core.m_Jumps == -1)
	{
		// The player has only one ground jump, so his feet are always dark
		m_Core.m_Jumped |= 2;
	}
	else if(m_Core.m_Jumps == 0)
	{
		// The player has no jumps at all, so his feet are always dark
		m_Core.m_Jumped |= 2;
	}
	else if(m_Core.m_Jumps == 1 && m_Core.m_Jumped > 0)
	{
		// If the player has only one jump, each jump is the last one
		m_Core.m_Jumped |= 2;
	}
	else if(m_Core.m_JumpedTotal < m_Core.m_Jumps - 1 && m_Core.m_Jumped > 1)
	{
		// The player has not yet used up all his jumps, so his feet remain light
		m_Core.m_Jumped = 1;
	}

	if((m_Core.m_Super || m_Core.m_EndlessJump) && m_Core.m_Jumped > 1)
	{
		// Super players and players with infinite jumps always have light feet
		m_Core.m_Jumped = 1;
	}

	int CurrentIndex = Collision()->GetMapIndex(m_Pos);
	HandleSkippableTiles(CurrentIndex);
	if(!m_Alive)
		return;

	// handle Anti-Skip tiles
	std::vector<int> vIndices = Collision()->GetMapIndices(m_PrevPos, m_Pos);
	if(!vIndices.empty())
	{
		for(int &Index : vIndices)
		{
			HandleTiles(Index);
			if(!m_Alive)
				return;
		}
	}
	else
	{
		HandleTiles(CurrentIndex);
		if(!m_Alive)
			return;
	}
	
	HandleKZTiles();

	HandleQuads();

	// teleport gun
	if(m_TeleGunTeleport)
	{
		GameServer()->CreateDeath(m_Pos, m_pPlayer->GetCid(), TeamMask());
		m_Core.m_Pos = m_TeleGunPos;
		if(!m_IsBlueTeleGunTeleport)
			m_Core.m_Vel = vec2(0, 0);
		GameServer()->CreateDeath(m_TeleGunPos, m_pPlayer->GetCid(), TeamMask());
		GameServer()->CreateSound(m_TeleGunPos, SOUND_WEAPON_SPAWN, TeamMask());
		m_TeleGunTeleport = false;
		m_IsBlueTeleGunTeleport = false;
	}

	HandleBroadcast();
}

bool CCharacter::Freeze(int Seconds)
{
	if(Seconds <= 0 || m_Core.m_Super || m_Core.m_Invincible || m_FreezeTime > Seconds * Server()->TickSpeed())
		return false;
	if(m_FreezeTime == 0 || m_Core.m_FreezeStart < Server()->Tick() - Server()->TickSpeed())
	{
		// m_Armor = 0; // ddnet-insta do not set m_Armor use SetArmorProgress instead
		GameServer()->m_pController->SetArmorProgressEmpty(this); // ddnet-insta
		m_FreezeTime = Seconds * Server()->TickSpeed();
		m_Core.m_FreezeStart = Server()->Tick();
		if(m_pPlayer && !m_FrozenKZ)
		{
			m_pPlayer->m_RageQuitTick = Server()->Tick();
			m_FrozenKZ = true;
		}
		return true;
	}
	return false;
}

bool CCharacter::Freeze()
{
	return Freeze(g_Config.m_SvFreezeDelay);
}

bool CCharacter::UnFreeze()
{
	if(m_FreezeTime > 0)
	{
		// m_Armor = 10; // ddnet-insta do not set m_Armor use SetArmorProgress instead
		GameServer()->m_pController->SetArmorProgressFull(this); // ddnet-insta
		if(m_Core.m_ActiveWeapon < NUM_WEAPONS ? !m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_Got : !m_aCustomWeaponGot[m_Core.m_ActiveWeapon - KZ_CUSTOM_WEAPON_START])
			m_Core.m_ActiveWeapon = WEAPON_GUN;
		m_FreezeTime = 0;
		m_Core.m_FreezeStart = 0;
		m_FrozenLastTick = true;
		if(m_pPlayer && m_FrozenKZ)
		{
			m_pPlayer->m_RageQuitTick = Server()->Tick();
			m_FrozenKZ = false;
		}
		return true;
	}
	return false;
}

void CCharacter::ResetJumps()
{
	m_Core.m_JumpedTotal = 0;
	m_Core.m_Jumped = 0;
}

void CCharacter::GiveWeapon(int Weapon, bool Remove, int Ammo)
{
	if(!Remove)
		m_HasNoWeapon = false;

	if(Weapon >= 0 && Weapon < NUM_WEAPONS)
	{
		if(Weapon == WEAPON_NINJA)
		{
			if(Remove)
				RemoveNinja();
			else
				GiveNinja();
			return;
		}

		if(Remove)
		{
			if(GetActiveWeapon() == Weapon)
				SetActiveWeapon(WEAPON_GUN);
		}
		else
		{
			m_Core.m_aWeapons[Weapon].m_Ammo = Ammo;
		}

		m_Core.m_aWeapons[Weapon].m_Got = !Remove;
	}
	else if(Weapon >= KZ_CUSTOM_WEAPON_START && Weapon < KZ_NUM_CUSTOM_WEAPONS)
	{
		m_aCustomWeaponGot[Weapon-KZ_CUSTOM_WEAPON_START] = !Remove;

		if(Remove)
		{
			if(GetActiveWeapon() == Weapon)
				SetActiveWeapon(WEAPON_GUN);
		}
		else
		{
			m_aCustomWeaponAmmo[Weapon-KZ_CUSTOM_WEAPON_START] = Ammo;
		}
	}

	int w = FindGotWeaponKZ();
	if(w != -1)
	{
		SetActiveWeapon(w);
	}
	else
	{
		SetActiveWeapon(WEAPON_GUN);
		m_HasNoWeapon = true;
	}
}

void CCharacter::GiveAllWeapons()
{
	for(int i = WEAPON_GUN; i < NUM_WEAPONS - 1; i++)
	{
		GiveWeapon(i);
	}
}

void CCharacter::ResetPickups()
{
	for(int i = WEAPON_SHOTGUN; i < NUM_WEAPONS - 1; i++)
	{
		m_Core.m_aWeapons[i].m_Got = false;
		if(m_Core.m_ActiveWeapon == i)
			m_Core.m_ActiveWeapon = WEAPON_GUN;
	}
}

void CCharacter::SetEndlessHook(bool Enable)
{
	if(m_Core.m_EndlessHook == Enable)
	{
		return;
	}
	GameServer()->SendChatTarget(GetPlayer()->GetCid(), Enable ? "Endless hook has been activated" : "Endless hook has been deactivated");

	m_Core.m_EndlessHook = Enable;
}

void CCharacter::Pause(bool Pause)
{
	m_Paused = Pause;
	if(Pause)
	{
		GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCid()] = nullptr;
		GameServer()->m_World.RemoveEntity(this);

		if(m_Core.HookedPlayer() != -1) // Keeping hook would allow cheats
		{
			ResetHook();
			GameWorld()->ReleaseHooked(GetPlayer()->GetCid());
		}
		m_PausedTick = Server()->Tick();
	}
	else
	{
		m_Core.m_Vel = vec2(0, 0);
		GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCid()] = &m_Core;
		GameServer()->m_World.InsertEntity(this);
		if(m_Core.m_FreezeStart > 0 && m_PausedTick >= 0)
		{
			m_Core.m_FreezeStart += Server()->Tick() - m_PausedTick;
		}
	}
}

void CCharacter::DDRaceInit()
{
	m_Paused = false;
	m_DDRaceState = DDRACE_NONE;
	m_PrevPos = m_Pos;
	for(bool &Set : m_SetSavePos)
		Set = false;
	m_LastBroadcast = 0;
	m_TeamBeforeSuper = 0;
	m_Core.m_Id = GetPlayer()->GetCid();
	m_TeleCheckpoint = 0;
	m_Core.m_EndlessHook = g_Config.m_SvEndlessDrag;
	if(g_Config.m_SvHit)
	{
		m_Core.m_HammerHitDisabled = false;
		m_Core.m_ShotgunHitDisabled = false;
		m_Core.m_GrenadeHitDisabled = false;
		m_Core.m_LaserHitDisabled = false;
	}
	else
	{
		m_Core.m_HammerHitDisabled = true;
		m_Core.m_ShotgunHitDisabled = true;
		m_Core.m_GrenadeHitDisabled = true;
		m_Core.m_LaserHitDisabled = true;
	}
	m_Core.m_Jumps = 2;
	m_FreezeHammer = false;

	int Team = Teams()->m_Core.Team(m_Core.m_Id);

	if(Teams()->TeamLocked(Team) && !Teams()->TeamFlock(Team))
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(Teams()->m_Core.Team(i) == Team && i != m_Core.m_Id && GameServer()->m_apPlayers[i])
			{
				CCharacter *pChar = GameServer()->m_apPlayers[i]->GetCharacter();

				if(pChar)
				{
					m_DDRaceState = pChar->m_DDRaceState;
					m_StartTime = pChar->m_StartTime;
				}
			}
		}
	}

	if(g_Config.m_SvTeam == SV_TEAM_MANDATORY && Team == TEAM_FLOCK)
	{
		GameServer()->SendStartWarning(GetPlayer()->GetCid(), "Please join a team before you start");
	}
}

void CCharacter::Rescue()
{
	if(m_SetSavePos[GetPlayer()->m_RescueMode] && !m_Core.m_Super && !m_Core.m_Invincible)
	{
		if(m_LastRescue + (int64_t)g_Config.m_SvRescueDelay * Server()->TickSpeed() > Server()->Tick() && !Teams()->IsPractice(Team()))
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "You have to wait %d seconds until you can rescue yourself", (int)((m_LastRescue + (int64_t)g_Config.m_SvRescueDelay * Server()->TickSpeed() - Server()->Tick()) / Server()->TickSpeed()));
			GameServer()->SendChatTarget(GetPlayer()->GetCid(), aBuf);
			return;
		}

		m_LastRescue = Server()->Tick();
		float StartTime = m_StartTime;
		m_RescueTee[GetPlayer()->m_RescueMode].Load(this, Team());
		// Don't load these from saved tee:
		m_Core.m_Vel = vec2(0, 0);
		m_Core.m_HookState = HOOK_IDLE;
		m_StartTime = StartTime;
		m_SavedInput.m_Direction = 0;
		m_SavedInput.m_Jump = 0;
		// simulate releasing the fire button
		if((m_SavedInput.m_Fire & 1) != 0)
			m_SavedInput.m_Fire++;
		m_SavedInput.m_Fire &= INPUT_STATE_MASK;
		m_SavedInput.m_Hook = 0;
		m_pPlayer->Pause(CPlayer::PAUSE_NONE, true);
	}
}

CClientMask CCharacter::TeamMask()
{
	return Teams()->TeamMask(Team(), -1, GetPlayer()->GetCid());
}

void CCharacter::SetPosition(const vec2 &Position)
{
	m_Core.m_Pos = Position;
}

void CCharacter::Move(vec2 RelPos)
{
	m_Core.m_Pos += RelPos;
}

void CCharacter::ResetVelocity()
{
	m_Core.m_Vel = vec2(0, 0);
}

void CCharacter::SetVelocity(vec2 NewVelocity)
{
	m_Core.m_Vel = ClampVel(m_MoveRestrictions, NewVelocity);
}

// The method is needed only to reproduce 'shotgun bug' ddnet#5258
// Use SetVelocity() instead.
void CCharacter::SetRawVelocity(vec2 NewVelocity)
{
	m_Core.m_Vel = NewVelocity;
}

void CCharacter::AddVelocity(vec2 Addition)
{
	SetVelocity(m_Core.m_Vel + Addition);
}

void CCharacter::ApplyMoveRestrictions()
{
	m_Core.m_Vel = ClampVel(m_MoveRestrictions, m_Core.m_Vel);
}

void CCharacter::SwapClients(int Client1, int Client2)
{
	const int HookedPlayer = m_Core.HookedPlayer();
	m_Core.SetHookedPlayer(HookedPlayer == Client1 ? Client2 : HookedPlayer == Client2 ? Client1 : HookedPlayer);
}

//+KZ

void CCharacter::HandleKZTiles()
{
	if(!(Collision()->KZFound()))
		return;
	
	int TileIndex = Collision()->GetKZTileIndex(m_Pos);
	bool ApplyRest = false;
	
	
	if(TileIndex == KZ_TILE_ADMIN)
	{
		if(Server()->GetAuthedState(m_pPlayer->GetCid()) == AUTHED_NO)
		{
			Die(m_pPlayer->GetCid(), WEAPON_WORLD);
			GameServer()->SendChatTarget(m_pPlayer->GetCid(), "Only Admins allowed");
		}
	}
	if(TileIndex == KZ_TILE_NOAIR || TileIndex == KZ_TILE_WATER)
	{
		m_NoAir = true;
	}
	else if(m_NoAir)
	{
		m_NoAir = false;
	}
	
	if(TileIndex == KZ_TILE_WATER || TileIndex == KZ_TILE_FLY)
	{
		m_Core.m_JumpedTotal = 0;
		m_Core.m_Jumped = 0;
	}
	
	if(TileIndex == KZ_TILE_WATER && !m_Water)
	{
		m_Water = true;
	}
	else if(TileIndex != KZ_TILE_WATER && m_Water)
	{
		m_Water = false;
	}
	
	if(TileIndex == KZ_TILE_INVISIBLE && !m_Invisible)
	{
		m_Invisible = true;
	}
	else if(TileIndex != KZ_TILE_INVISIBLE && m_Invisible)
	{
		m_Invisible = false;
	}

	if(TileIndex == KZ_TILE_INVINCIBLE && !m_Core.m_Invincible)
	{
		GameServer()->SendChatTarget(m_pPlayer->GetCid(), "You are invincible");
		SetInvincible(true);
	}
	else if(TileIndex == KZ_TILE_NO_INVINCIBLE && m_Core.m_Invincible)
	{
		GameServer()->SendChatTarget(m_pPlayer->GetCid(), "Now you are not invincible, sad...");
		SetInvincible(false);
	}

	if(TileIndex == KZ_TILE_RAINBOW_ON && !m_Rainbow)
	{
		GameServer()->SendChatTarget(m_pPlayer->GetCid(), "You got rainbow");
		Rainbow(true);
	}
	else if(TileIndex == KZ_TILE_RAINBOW_OFF && m_Rainbow)
	{
		GameServer()->SendChatTarget(m_pPlayer->GetCid(), "You lost rainbow");
		Rainbow(false);
	}

	if(TileIndex == KZ_TILE_GODMODE && !m_IsGodmode)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "'%s' got godmode!", Server()->ClientName(m_pPlayer->GetCid()));
		GameServer()->SendChat(-1, TEAM_ALL, aBuf);

		m_IsGodmode = true;
	}
	else if(TileIndex == KZ_TILE_NO_GODMODE && m_IsGodmode)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "'%s' lost godmode...", Server()->ClientName(m_pPlayer->GetCid()));
		GameServer()->SendChat(-1, TEAM_ALL, aBuf);

		m_IsGodmode = false;
	}

	if(TileIndex == KZ_TILE_SPARKLES_ON && !m_Sparkles)
	{
		GameServer()->SendChatTarget(m_pPlayer->GetCid(), "You got sparkles");
		m_Sparkles = true;
	}
	else if(TileIndex == KZ_TILE_SPARKLES_OFF && m_Sparkles)
	{
		GameServer()->SendChatTarget(m_pPlayer->GetCid(), "You lost sparkles");
		m_Sparkles = false;
	}

	if(TileIndex == KZ_TILE_CONFETTI_ON && !m_ConfettiKZ)
	{
		GameServer()->SendChatTarget(m_pPlayer->GetCid(), "You got confetti");
		m_ConfettiKZ = true;
	}
	else if(TileIndex == KZ_TILE_CONFETTI_OFF && m_ConfettiKZ)
	{
		GameServer()->SendChatTarget(m_pPlayer->GetCid(), "You lost confetti");
		m_ConfettiKZ = false;
	}

	if(TileIndex == KZ_TILE_SIT && !m_Sit)
	{
		m_Sit = true;
	}
	else if(TileIndex != KZ_TILE_SIT && m_Sit)
	{
		m_Sit = false;
	}

	//+KZ for pprace compat
	if(TileIndex == KZ_TILE_RESET_PORTAL && !m_PortalReset)
	{
		for(CPortalKZ* p = (CPortalKZ*)GameWorld()->FindFirst(CGameWorld::CUSTOM_ENTTYPE_PORTAL);p;p = (CPortalKZ*)p->TypeNext())
		{
			if(p->m_Owner == m_pPlayer->GetCid())
			{
				p->Reset();
				CPortalKZ* p2 = p->GetOtherPortal();
				if(p2)
				{
					p2->Reset();
				}
				return;
			}
		}
		m_PortalReset = true;
	}
	else if(TileIndex != KZ_TILE_RESET_PORTAL && m_PortalReset)
	{
		m_PortalReset = false;
	}

	int NewJumps = m_Core.m_Jumps;

	if(TileIndex == KZ_TILE_PLUS_JUMP && !m_insidetilejump)
	{
		NewJumps++;
		m_insidetilejump = true;
	}
	else if(TileIndex == KZ_TILE_MINUS_JUMP && !m_insidetilejump)
	{
		if(NewJumps > 0)
			NewJumps--;
		m_insidetilejump = true;
	}
	else if(m_insidetilejump && TileIndex != KZ_TILE_PLUS_JUMP && TileIndex != KZ_TILE_MINUS_JUMP)
	{
		m_insidetilejump = false;
	}

	if(NewJumps != m_Core.m_Jumps)
	{
		char aBuf[256];
		if(NewJumps == 0)
			str_copy(aBuf, "Now you can't jump");
		else if(NewJumps == 1)
			str_format(aBuf, sizeof(aBuf), "You can jump %d time", NewJumps);
		else
			str_format(aBuf, sizeof(aBuf), "You can jump %d times", NewJumps);
		GameServer()->SendChatTarget(GetPlayer()->GetCid(), aBuf);
		m_Core.m_Jumps = NewJumps;
	}
	
	
	if(Collision()->GetKZTileIndex(m_Pos.x - 15 , m_Pos.y) == KZ_TILE_5_DAMAGE)
	{
		DoKZDamage(vec2(15,0), 5, m_pPlayer->GetCid(), WEAPON_WORLD);
	}
	if(Collision()->GetKZTileIndex(m_Pos.x + 15 , m_Pos.y) == KZ_TILE_5_DAMAGE)
	{
		DoKZDamage(vec2(-15,0), 5, m_pPlayer->GetCid(), WEAPON_WORLD);
	}
	if(Collision()->GetKZTileIndex(m_Pos.x , m_Pos.y - 15) == KZ_TILE_5_DAMAGE)
	{
		DoKZDamage(vec2(0,15), 5, m_pPlayer->GetCid(), WEAPON_WORLD);
	}
	if(Collision()->GetKZTileIndex(m_Pos.x, m_Pos.y + 15 ) == KZ_TILE_5_DAMAGE)
	{
		DoKZDamage(vec2(0,-15), 5, m_pPlayer->GetCid(), WEAPON_WORLD);
	}

	if(Collision()->GetKZTileIndex(m_Pos.x - 15 , m_Pos.y) == KZ_TILE_1_DAMAGE)
	{
		DoKZDamage(vec2(15,0), 1, m_pPlayer->GetCid(), WEAPON_WORLD);
	}
	if(Collision()->GetKZTileIndex(m_Pos.x + 15 , m_Pos.y) == KZ_TILE_1_DAMAGE)
	{
		DoKZDamage(vec2(-15,0), 1, m_pPlayer->GetCid(), WEAPON_WORLD);
	}
	if(Collision()->GetKZTileIndex(m_Pos.x , m_Pos.y - 15) == KZ_TILE_1_DAMAGE)
	{
		DoKZDamage(vec2(0,15), 1, m_pPlayer->GetCid(), WEAPON_WORLD);
	}
	if(Collision()->GetKZTileIndex(m_Pos.x, m_Pos.y + 15 ) == KZ_TILE_1_DAMAGE)
	{
		DoKZDamage(vec2(0,-15), 1, m_pPlayer->GetCid(), WEAPON_WORLD);
	}
	
	
	bool found = false;
	if(Collision()->GetKZTileIndex(m_Pos.x + GetProximityRadius() / 3.f, m_Pos.y - GetProximityRadius() / 3.f) == KZ_TILE_SLOWDEATH)
	{
		found = true;
	}
	if(Collision()->GetKZTileIndex(m_Pos.x + GetProximityRadius() / 3.f, m_Pos.y + GetProximityRadius() / 3.f) == KZ_TILE_SLOWDEATH)
	{
		found = true;
	}
	if(Collision()->GetKZTileIndex(m_Pos.x - GetProximityRadius() / 3.f, m_Pos.y - GetProximityRadius() / 3.f) == KZ_TILE_SLOWDEATH)
	{
		found = true;
	}
	if(Collision()->GetKZTileIndex(m_Pos.x - GetProximityRadius() / 3.f, m_Pos.y + GetProximityRadius() / 3.f) == KZ_TILE_SLOWDEATH)
	{
		found = true;
	}
	
	if(found)
	{
		m_slowDeathTick--;
		if (m_slowDeathTick < 0) {
			DoKZDamage(vec2(0,0), 1, m_pPlayer->GetCid(), WEAPON_WORLD);
			m_slowDeathTick = 10;
		}
	}
	
	
	
	found = false;
	if(Collision()->GetKZTileIndex(m_Pos.x + GetProximityRadius() / 3.f, m_Pos.y - GetProximityRadius() / 3.f) == KZ_TILE_HEALTHZONE)
	{
		found = true;
	}
	if(Collision()->GetKZTileIndex(m_Pos.x + GetProximityRadius() / 3.f, m_Pos.y + GetProximityRadius() / 3.f) == KZ_TILE_HEALTHZONE)
	{
		found = true;
	}
	if(Collision()->GetKZTileIndex(m_Pos.x - GetProximityRadius() / 3.f, m_Pos.y - GetProximityRadius() / 3.f) == KZ_TILE_HEALTHZONE)
	{
		found = true;
	}
	if(Collision()->GetKZTileIndex(m_Pos.x - GetProximityRadius() / 3.f, m_Pos.y + GetProximityRadius() / 3.f) == KZ_TILE_HEALTHZONE)
	{
		found = true;
	}
	
	if(found)
	{
		m_healthArmorZoneTick--;
		if (m_healthArmorZoneTick < 0) {
			if (m_Health < 10) {
				m_Health = m_Health + 1;
				GameServer()->CreateSound(m_Pos, SOUND_PICKUP_HEALTH);
			}
			m_EmoteType = EMOTE_HAPPY;
			m_EmoteStop = Server()->Tick() + 500 * Server()->TickSpeed() / 1000;
			m_healthArmorZoneTick = 10;
		}
	}
	
	
	
	found = false;
	if(Collision()->GetKZTileIndex(m_Pos.x + GetProximityRadius() / 3.f, m_Pos.y - GetProximityRadius() / 3.f) == KZ_TILE_ARMORZONE)
	{
		found = true;
	}
	if(Collision()->GetKZTileIndex(m_Pos.x + GetProximityRadius() / 3.f, m_Pos.y + GetProximityRadius() / 3.f) == KZ_TILE_ARMORZONE)
	{
		found = true;
	}
	if(Collision()->GetKZTileIndex(m_Pos.x - GetProximityRadius() / 3.f, m_Pos.y - GetProximityRadius() / 3.f) == KZ_TILE_ARMORZONE)
	{
		found = true;
	}
	if(Collision()->GetKZTileIndex(m_Pos.x - GetProximityRadius() / 3.f, m_Pos.y + GetProximityRadius() / 3.f) == KZ_TILE_ARMORZONE)
	{
		found = true;
	}
	
	if(found)
	{
		m_healthArmorZoneTick--;
		if (m_healthArmorZoneTick < 0) {
			if (m_Armor < 10) {
				m_Armor = m_Armor + 1;
				GameServer()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR);
			}
			m_EmoteType = EMOTE_HAPPY;
			m_EmoteStop = Server()->Tick() + 500 * Server()->TickSpeed() / 1000;
			m_healthArmorZoneTick = 10;
		}
	}
	
	
	if(m_HasBall && TileIndex == KZ_TILE_NO_BALL)
	{
		CBall *ball = new CBall(&GameServer()->m_World, m_pPlayer->GetCid(), m_Pos, vec2(0,0));
		ball->GoToStartPos();
		GameServer()->CreateSound(m_Pos, SOUND_GRENADE_FIRE, TeamMask());
		m_HasBall = false;
		SetWeapon(m_BallQueuedWeapon);
		m_BallQueuedWeapon = -1;
	}
	
	if(TileIndex == KZ_TILE_TEE_KILL)
	{
		if(m_HasBall)
		{
			//GameServer()->m_pController->m_aTeamscore[TEAM_BLUE]+= 100;
			CBall *ball = new CBall(&GameServer()->m_World, m_pPlayer->GetCid(), m_Pos, vec2(0,0));
			ball->GoToStartPos();
			GameServer()->CreateSound(m_Pos, SOUND_GRENADE_FIRE, TeamMask());
			m_HasBall = false;
		}
		Die(m_pPlayer->GetCid(), WEAPON_WORLD);
	}
	
	if(GameServer()->m_pController->IsTeamPlay())
	{
		
		if(TileIndex == KZ_TILE_BALL_REDSLAM)
		{
			if(m_HasBall)
			{
				GameServer()->m_pController->m_aTeamscore[TEAM_BLUE]+= 125;
				CBall *ball = new CBall(&GameServer()->m_World, m_pPlayer->GetCid(), m_Pos, vec2(0,0));
				ball->GoToStartPos();
				GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE);
				m_HasBall = false;
			}
			Die(m_pPlayer->GetCid(), WEAPON_WORLD);
		}
		else if(TileIndex == KZ_TILE_BALL_BLUESLAM)
		{
			if(m_HasBall)
			{
				GameServer()->m_pController->m_aTeamscore[TEAM_RED]+= 125;
				CBall *ball = new CBall(&GameServer()->m_World, m_pPlayer->GetCid(), m_Pos, vec2(0,0));
				ball->GoToStartPos();
				GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE);
				m_HasBall = false;
			}
			Die(m_pPlayer->GetCid(), WEAPON_WORLD);
		}
		else if(TileIndex == KZ_TILE_BALL_NOTEAMSLAM)
		{
			if(m_HasBall)
			{
				GameServer()->m_pController->m_aTeamscore[m_pPlayer->GetTeam()]+= 125;
				CBall *ball = new CBall(&GameServer()->m_World, m_pPlayer->GetCid(), m_Pos, vec2(0,0));
				ball->GoToStartPos();
				GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE);
				m_HasBall = false;
			}
			Die(m_pPlayer->GetCid(), WEAPON_WORLD);
		}
		
		if(m_pPlayer->GetTeam() == TEAM_BLUE && TileIndex == KZ_TILE_TEAMRED_DEATH)
		{
			Die(m_pPlayer->GetCid(), WEAPON_WORLD);
		}
		else if(m_pPlayer->GetTeam() == TEAM_RED && TileIndex == KZ_TILE_TEAMBLUE_DEATH)
		{
			Die(m_pPlayer->GetCid(), WEAPON_WORLD);
		}
		
		if(m_pPlayer->GetTeam() == TEAM_BLUE)
		{
			if(Collision()->GetKZTileIndex(m_Pos.x - 15 , m_Pos.y) == KZ_TILE_TEAMRED)
			{
				m_MoveRestrictions |= CANTMOVE_LEFT;
				ApplyRest = true;
			}
			if(Collision()->GetKZTileIndex(m_Pos.x + 15 , m_Pos.y) == KZ_TILE_TEAMRED)
			{
				m_MoveRestrictions |= CANTMOVE_RIGHT;
				ApplyRest = true;
			}
			if(Collision()->GetKZTileIndex(m_Pos.x , m_Pos.y - 15) == KZ_TILE_TEAMRED)
			{
				m_MoveRestrictions |= CANTMOVE_UP;
				ApplyRest = true;
			}
			if(Collision()->GetKZTileIndex(m_Pos.x, m_Pos.y + 15 ) == KZ_TILE_TEAMRED)
			{
				m_MoveRestrictions |= CANTMOVE_DOWN;
				m_Core.m_Jumped = 0;
				m_Core.m_JumpedTotal = 0;
				ApplyRest = true;
			}
		}
		else if(m_pPlayer->GetTeam() == TEAM_RED)
		{
			if(Collision()->GetKZTileIndex(m_Pos.x - 15 , m_Pos.y) == KZ_TILE_TEAMBLUE)
			{
				m_MoveRestrictions |= CANTMOVE_LEFT;
				ApplyRest = true;
			}
			if(Collision()->GetKZTileIndex(m_Pos.x + 15 , m_Pos.y) == KZ_TILE_TEAMBLUE)
			{
				m_MoveRestrictions |= CANTMOVE_RIGHT;
				ApplyRest = true;
			}
			if(Collision()->GetKZTileIndex(m_Pos.x , m_Pos.y - 15) == KZ_TILE_TEAMBLUE)
			{
				m_MoveRestrictions |= CANTMOVE_UP;
				ApplyRest = true;
			}
			if(Collision()->GetKZTileIndex(m_Pos.x, m_Pos.y + 15 ) == KZ_TILE_TEAMBLUE)
			{
				m_MoveRestrictions |= CANTMOVE_DOWN;
				m_Core.m_Jumped = 0;
				m_Core.m_JumpedTotal = 0;
				ApplyRest = true;
			}
		}
	}
	
	if(ApplyRest)
	{
		ApplyMoveRestrictions();
	}
	
}

void CCharacter::DoKZDamage(vec2 Force, int Dmg, int From, int Weapon)
{
	
	if(From < 0 || From >= MAX_CLIENTS) //+KZ
		From = m_pPlayer->GetCid();
	if(!m_IsGodmode)
	{
		if(m_Armor > 0)
		{
			int temp = m_Armor;
			m_Armor -= Dmg;
			Dmg -= temp;
			if(Dmg < 0)
				Dmg = 0;
			m_Health -= Dmg;
		}
		else
		{
			m_Health -= Dmg;
		}
		
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_SHORT);
		
		if(Dmg)
		{
			SetEmote(EMOTE_PAIN, Server()->Tick() + 500 * Server()->TickSpeed() / 1000);
		}
	}

	vec2 Temp = m_Core.m_Vel + Force;
	m_Core.m_Vel = ClampVel(m_MoveRestrictions, Temp);
	GameServer()->CreateDamageInd(m_Pos, 0, Dmg);
	
	if(!m_IsGodmode)
	{
		if(m_Health <= 0)
		{
			Die(From, Weapon);
		}
	}
}

void CCharacter::CatchBall()
{
	m_BallReleaseTick = Server()->TickSpeed() * 5;
	m_BallQueuedWeapon = m_Core.m_ActiveWeapon;
	m_HasBall = true;
	SetWeapon(WEAPON_GRENADE);
}

void CCharacter::HandleFlagHookCatch()
{
	if(!g_Config.m_SvFlagHookGrab)
		return;

	if(m_Core.m_HookState == HOOK_FLYING)
	{
	 	for (CFlag *flag = (CFlag*)GameWorld()->FindFirst(CGameWorld::ENTTYPE_FLAG); flag; flag = (CFlag *)flag->TypeNext())
 		{
			if(!(flag->CanHookGrabKZ(this)))
				continue;

			if(distance(m_Core.m_HookPos,flag->m_Pos) < 60.f)
				flag->Grab(this);
		}
	}
}

void CCharacter::HandleKZBot(CNetObj_PlayerInput &Input)
{
	if(!(GameServer()->CountPlayersKZ()))
		return;
	
	switch(g_Config.m_SvKZBotsAI)
	{
		case 0: //+KZ's IA
			DoKZBotAI(Input);
			break;
		case 1: //Pointer's harder IA
			DoPointerBotAI(Input);
			break;
	}
	
	
}

void CCharacter::DoKZBotAI(CNetObj_PlayerInput &Input)
{
	//Spaghetti yummy
	
	CCharacter *pClosestChar = nullptr;
	CVanillaPickup *pClosestPickup = nullptr;
	CFlag *pEnemyFlag = nullptr;
	CFlag *pTeamFlag = nullptr;
	bool targetisup = false;
	bool dontjump = false;
	bool butjumpifwall = false;
	bool jumpifgoingtofall = false;
	
	vec2 TargetPos = vec2(0,0);
	bool TargetPosSet = false;
	bool DoSmartTargetChase = false;
	
	Input.m_Fire = false;
	
	//if(str_find_nocase(GameServer()->m_pController->m_pGameType, "CTF"))
	{
		CFlag *p = (CFlag *)GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_FLAG);
		for(; p; p = (CFlag *)p->TypeNext())
		{
			
			if(p->GetTeam() == m_pPlayer->GetTeam())
			{
				pTeamFlag = p;
				continue;
			}
			
			pEnemyFlag = p;
		}
	}
	
	{
		float ClosestRange = 100000.0f;
		CVanillaPickup *pClosest = 0;
		
		CVanillaPickup *p = (CVanillaPickup *)GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_PICKUP);
		for(; p; p = (CVanillaPickup *)p->TypeNext())
		{
			if(Collision()->FastIntersectLine(m_Pos,p->m_Pos,nullptr,nullptr))
				continue;
			
			if((p->Type() == POWERUP_HEALTH && m_Health >= 10) || (p->Type() == POWERUP_ARMOR && m_Armor >= 10) || (p->Subtype() == WEAPON_SHOTGUN && m_Core.m_aWeapons[WEAPON_SHOTGUN].m_Ammo) || (p->Subtype() == WEAPON_LASER && m_Core.m_aWeapons[WEAPON_LASER].m_Ammo) || (p->Subtype() == WEAPON_GRENADE && m_Core.m_aWeapons[WEAPON_GRENADE].m_Ammo)  || (p->Type() == POWERUP_NINJA && m_Core.m_aWeapons[WEAPON_NINJA].m_Got))
				continue;
			
			if(p->GetSpawnTick() > 0)
			{
				//printf("%d",((CVanillaPickup*)p)->GetSpawnTick());
				continue;
			}
			
			
			float Len = distance(m_Pos, p->m_Pos);
			if(Len < p->GetProximityRadius() + 100000.0f)
			{
				if(Len < ClosestRange)
				{
					ClosestRange = Len;
					pClosest = p;
				}
			}
		}
		
		pClosestPickup = pClosest;
	}
	
	{
		float ClosestRange = 100000.0f;
		CCharacter *pClosest = 0;
		
		CCharacter *p = (CCharacter *)GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER);
		for(; p; p = (CCharacter *)p->TypeNext())
		{
			if(p == this)
				continue;
			
			if(GameServer()->m_pController->IsTeamPlay() && p->GetPlayer()->GetTeam() == m_pPlayer->GetTeam())
				continue;
			
			if(str_find_nocase(GameServer()->m_pController->m_pGameType, "freeze") && p->GetCore().m_DeepFrozen)
				continue;

			if(str_find_nocase(GameServer()->m_pController->m_pGameType, "catch64") && p->GetPlayer()->m_CatchColor == m_pPlayer->m_CatchColor)
				continue;
			
			
			float Len = distance(m_Pos, p->m_Pos);
			if(Len < p->GetProximityRadius() + 100000.0f)
			{
				if(Len < ClosestRange)
				{
					ClosestRange = Len;
					pClosest = p;
				}
			}
		}
		
		pClosestChar = pClosest;
	}
	
	if(pEnemyFlag)
	{
		if(pTeamFlag && pEnemyFlag->m_pCarrier == this)
		{
			//Input.m_Direction = pTeamFlag->m_Pos.x > m_Pos.x ? 1 : -1;
			TargetPos = pTeamFlag->m_Pos;
			TargetPosSet = true;
			DoSmartTargetChase = true;
		}
		else
		{
			//Input.m_Direction = pEnemyFlag->m_Pos.x > m_Pos.x ? 1 : -1;
			TargetPos = pEnemyFlag->m_Pos;
			TargetPosSet = true;
			DoSmartTargetChase = true;
		}
	}
	
	if(pClosestPickup && !pEnemyFlag && !pTeamFlag)
	{
		//Input.m_Direction = pClosestPickup->m_Pos.x > m_Pos.x ? 1 : -1;
		TargetPos = pClosestPickup->m_Pos;
		TargetPosSet = true;
		
		/*if((pClosestPickup->m_Pos.y + pClosestPickup->GetProximityRadius() * 2.f) < m_Pos.y && !(Collision()->GetCollisionAt(m_Pos.x , m_Pos.y - GetProximityRadius() / 3.f) == TILE_DEATH))
		{
			targetisup = true;
		}*/
	}
	
	if(pClosestChar)
	{
		Input.m_TargetX = pClosestChar->m_Pos.x - m_Pos.x; // aim
		Input.m_TargetY = pClosestChar->m_Pos.y - m_Pos.y;
		
		if(!pClosestPickup && !pEnemyFlag  && !pTeamFlag)
		{
			//Input.m_Direction = pClosestChar->m_Pos.x > m_Pos.x ? 1 : -1;
			TargetPos = pClosestChar->m_Pos;
			TargetPosSet = true;
			DoSmartTargetChase = true;
		}
		
		//printf("%.5f %.5f \n",distance(m_Pos, pClosestChar->m_Pos),30.0f);
		
		if(m_Core.m_aWeapons[WEAPON_NINJA].m_Got)
		{
			//SetWeapon(WEAPON_LASER);
		}
		else if(m_Core.m_aWeapons[WEAPON_LASER].m_Got && m_Core.m_aWeapons[WEAPON_LASER].m_Ammo && distance(m_Pos, pClosestChar->m_Pos) < GameServer()->Tuning()->m_LaserReach)
		{
			SetWeapon(WEAPON_LASER);
			
			//aim laserbounce
		}
		else if(m_Core.m_aWeapons[WEAPON_SHOTGUN].m_Got && m_Core.m_aWeapons[WEAPON_SHOTGUN].m_Ammo && distance(m_Pos, pClosestChar->m_Pos) < GameServer()->Tuning()->m_ShotgunLifetime * 3000.0f)
		{
			SetWeapon(WEAPON_SHOTGUN);
		}
		else if(m_Core.m_aWeapons[WEAPON_HAMMER].m_Got && distance(m_Pos, pClosestChar->m_Pos) < 40.0f)
		{
			SetWeapon(WEAPON_HAMMER);
		}
		else if(m_Core.m_aWeapons[WEAPON_GUN].m_Got)
		{
			SetWeapon(WEAPON_GUN);
		}
		
		/*if(pClosestChar->m_Pos.y < m_Pos.y && !(Collision()->GetCollisionAt(m_Pos.x , m_Pos.y - GetProximityRadius() / 3.f) == TILE_DEATH))
		{
			targetisup = true;
		}*/
		
		if((m_Core.m_ActiveWeapon == WEAPON_LASER ? (!Collision()->FastIntersectLine(m_Pos,pClosestChar->m_Pos,nullptr,nullptr) && distance(m_Pos, pClosestChar->m_Pos) < GameServer()->Tuning()->m_LaserReach) : !Collision()->FastIntersectLine(m_Pos,pClosestChar->m_Pos,nullptr,nullptr)) || m_Core.m_aWeapons[WEAPON_NINJA].m_Got)
		{
			if(!m_LatestInput.m_Fire)
				Input.m_Fire = true;
			else
				Input.m_Fire = false;
			
			if(Server()->Tick() % Server()->TickSpeed() == 0)
			{
				Input.m_Hook = false;
			}
			else if(distance(m_Pos, pClosestChar->m_Pos) < GameServer()->Tuning()->m_HookLength)
			{
				Input.m_Hook = true;
			}
		}
		
		if(Collision()->FastIntersectLine(m_Pos,pClosestChar->m_Pos,nullptr,nullptr) && m_Core.m_ActiveWeapon == WEAPON_LASER)
		{
			bool fire = false;
			
			float halfy = (pClosestChar->m_Pos.y - m_Pos.y) / 2;
			float halfx = (pClosestChar->m_Pos.x - m_Pos.x) / 2;
			
			vec2 BouncePos_Right,BouncePos_Left,BouncePos_Up,BouncePos_Down;
			bool Bounced_Right = false,Bounced_Left = false,Bounced_Up = false,Bounced_Down = false;
			
			vec2 At, tempvar;
			
			//try y first
			if(Collision()->FastIntersectLine(vec2(m_Pos.x + GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_Pos.y),vec2(m_Pos.x,halfy + m_Pos.y),nullptr,&tempvar))
			{
				Collision()->UnIntersectLineKZ(vec2(m_Pos.x + GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_Pos.y),vec2(m_Pos.x,halfy + m_Pos.y),&BouncePos_Right,nullptr);
				if(!Collision()->IntersectLine(m_Pos,BouncePos_Right,nullptr,nullptr) && tempvar.x > BouncePos_Right.x)
					Bounced_Right = true;
			}
			if(Collision()->FastIntersectLine(vec2(m_Pos.x - GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_Pos.y),vec2(m_Pos.x,halfy + m_Pos.y),nullptr,&tempvar))
			{
				Collision()->UnIntersectLineKZ(vec2(m_Pos.x - GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_Pos.y),vec2(m_Pos.x,halfy + m_Pos.y),&BouncePos_Left,nullptr);
				if(!Collision()->IntersectLine(m_Pos,BouncePos_Left,nullptr,nullptr) && tempvar.x < BouncePos_Left.x)
					Bounced_Left = true;
			}
			//try x now
			if(Collision()->FastIntersectLine(vec2(halfx + m_Pos.x,m_Pos.y + GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_Pos.x,m_Pos.y),nullptr,&tempvar))
			{
				Collision()->UnIntersectLineKZ(vec2(halfx + m_Pos.x,m_Pos.y + GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_Pos.x,m_Pos.y),&BouncePos_Down,nullptr);
				if(!Collision()->IntersectLine(m_Pos,BouncePos_Down,nullptr,nullptr) && tempvar.y > BouncePos_Down.y)
					Bounced_Down = true;
			}
			if(Collision()->FastIntersectLine(vec2(halfx + m_Pos.x,m_Pos.y - GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_Pos.x,m_Pos.y),nullptr,&BouncePos_Up))
			{
				Collision()->UnIntersectLineKZ(vec2(halfx + m_Pos.x,m_Pos.y - GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_Pos.x,m_Pos.y),&BouncePos_Up,nullptr);
				if(!Collision()->IntersectLine(m_Pos,BouncePos_Up,nullptr,nullptr) && tempvar.y < BouncePos_Up.y)
					Bounced_Up = true;
			}
			
			//try y
			if(Bounced_Right && (BouncePos_Right.y > (halfy + m_Pos.y) - 5.f && BouncePos_Right.y < (halfy + m_Pos.y) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Right,pClosestChar->m_Pos,nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Right, vec2(m_Pos.x,m_Pos.y + (BouncePos_Right.y - m_Pos.y)*2), 0.f, At, this))
			{
				Input.m_TargetX = BouncePos_Right.x - m_Pos.x; // aim
				Input.m_TargetY = BouncePos_Right.y - m_Pos.y;
				fire = true;
			}
			else if(Bounced_Left && (BouncePos_Left.y > (halfy + m_Pos.y) - 5.f && BouncePos_Left.y < (halfy + m_Pos.y) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Left,pClosestChar->m_Pos,nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Left, vec2(m_Pos.x,m_Pos.y + (BouncePos_Left.y - m_Pos.y)*2), 0.f, At, this))
			{
				Input.m_TargetX = BouncePos_Left.x - m_Pos.x; // aim
				Input.m_TargetY = BouncePos_Left.y - m_Pos.y;
				fire = true;
			}
				
			//try x
			if(Bounced_Up && (BouncePos_Up.x > (halfx + m_Pos.x) - 5.f && BouncePos_Up.x < (halfx + m_Pos.x) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Up,pClosestChar->m_Pos,nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Up, vec2(m_Pos.x + (BouncePos_Up.x - m_Pos.x)*2,m_Pos.y), 0.f, At, this))
			{
				Input.m_TargetX = BouncePos_Up.x - m_Pos.x; // aim
				Input.m_TargetY = BouncePos_Up.y - m_Pos.y;
				fire = true;
			}
			else if(Bounced_Down && (BouncePos_Down.x > (halfx + m_Pos.x) - 5.f && BouncePos_Down.x < (halfx + m_Pos.x) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Down,pClosestChar->m_Pos,nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Down, vec2(m_Pos.x + (BouncePos_Down.x - m_Pos.x)*2,m_Pos.y), 0.f, At, this))
			{
				Input.m_TargetX = BouncePos_Down.x - m_Pos.x; // aim
				Input.m_TargetY = BouncePos_Down.y - m_Pos.y;
				fire = true;
			}
					
			if(!fire)
			{
				//try again but moving point
				{
					float tempfloat =0.0f;
					
					tempfloat = halfy; //save halfy
					
					//move point
					halfx = halfx - m_Pos.x > 0 ? halfx + (tempfloat/2 + tempfloat/halfx) : halfx - (tempfloat/2 + tempfloat/halfx);
					if(halfy - m_Pos.y > 0)
					{
						halfy += (halfx/2 + halfx/halfy);
						if(halfy - m_Pos.y < 0)
							halfy += (halfy - m_Pos.y)*2;
					}
					else if(halfy - m_Pos.y < 0)
					{
						halfy -= (halfx/2 + halfx/halfy);
						if(halfy - m_Pos.y > 0)
							halfy -= (halfy - m_Pos.y)*2;
					}
					
					if(halfx - m_Pos.x > 0)
					{
						halfx += (tempfloat/2 + tempfloat/halfx);
						if(halfx - m_Pos.x < 0)
						{
							halfx += (halfx - m_Pos.x)*2;
						}
					}
					else if(halfx - m_Pos.x < 0)
					{
						halfx -= (tempfloat/2 + tempfloat/halfx);
						if(halfx - m_Pos.x > 0)
						{
							halfx -= (halfx - m_Pos.x)*2;
						}
					}
				}
				
				//vec2 BouncePos_Right,BouncePos_Left,BouncePos_Up,BouncePos_Down;
				Bounced_Right = false,Bounced_Left = false,Bounced_Up = false,Bounced_Down = false;
				
				//vec2 At, tempvar;
				
				//try y first
				if(Collision()->FastIntersectLine(vec2(m_Pos.x + GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_Pos.y),vec2(m_Pos.x,halfy + m_Pos.y),nullptr,&tempvar))
				{
					Collision()->UnIntersectLineKZ(vec2(m_Pos.x + GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_Pos.y),vec2(m_Pos.x,halfy + m_Pos.y),&BouncePos_Right,nullptr);
					if(!Collision()->IntersectLine(m_Pos,BouncePos_Right,nullptr,nullptr) && tempvar.x > BouncePos_Right.x)
						Bounced_Right = true;
				}
				if(Collision()->FastIntersectLine(vec2(m_Pos.x - GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_Pos.y),vec2(m_Pos.x,halfy + m_Pos.y),nullptr,&tempvar))
				{
					Collision()->UnIntersectLineKZ(vec2(m_Pos.x - GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_Pos.y),vec2(m_Pos.x,halfy + m_Pos.y),&BouncePos_Left,nullptr);
					if(!Collision()->IntersectLine(m_Pos,BouncePos_Left,nullptr,nullptr) && tempvar.x < BouncePos_Left.x)
						Bounced_Left = true;
				}
				//try x now
				if(Collision()->FastIntersectLine(vec2(halfx + m_Pos.x,m_Pos.y + GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_Pos.x,m_Pos.y),nullptr,&tempvar))
				{
					Collision()->UnIntersectLineKZ(vec2(halfx + m_Pos.x,m_Pos.y + GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_Pos.x,m_Pos.y),&BouncePos_Down,nullptr);
					if(!Collision()->IntersectLine(m_Pos,BouncePos_Down,nullptr,nullptr) && tempvar.y > BouncePos_Down.y)
						Bounced_Down = true;
				}
				if(Collision()->FastIntersectLine(vec2(halfx + m_Pos.x,m_Pos.y - GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_Pos.x,m_Pos.y),nullptr,&BouncePos_Up))
				{
					Collision()->UnIntersectLineKZ(vec2(halfx + m_Pos.x,m_Pos.y - GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_Pos.x,m_Pos.y),&BouncePos_Up,nullptr);
					if(!Collision()->IntersectLine(m_Pos,BouncePos_Up,nullptr,nullptr) && tempvar.y < BouncePos_Up.y)
						Bounced_Up = true;
				}
				
				//try y
				if(Bounced_Right && (BouncePos_Right.y > (halfy + m_Pos.y) - 5.f && BouncePos_Right.y < (halfy + m_Pos.y) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Right,pClosestChar->m_Pos,nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Right, vec2(m_Pos.x,m_Pos.y + (BouncePos_Right.y - m_Pos.y)*2), 0.f, At, this))
				{
					Input.m_TargetX = BouncePos_Right.x - m_Pos.x; // aim
					Input.m_TargetY = BouncePos_Right.y - m_Pos.y;
					fire = true;
				}
				else if(Bounced_Left && (BouncePos_Left.y > (halfy + m_Pos.y) - 5.f && BouncePos_Left.y < (halfy + m_Pos.y) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Left,pClosestChar->m_Pos,nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Left, vec2(m_Pos.x,m_Pos.y + (BouncePos_Left.y - m_Pos.y)*2), 0.f, At, this))
				{
					Input.m_TargetX = BouncePos_Left.x - m_Pos.x; // aim
					Input.m_TargetY = BouncePos_Left.y - m_Pos.y;
					fire = true;
				}
					
				//try x
				if(Bounced_Up && (BouncePos_Up.x > (halfx + m_Pos.x) - 5.f && BouncePos_Up.x < (halfx + m_Pos.x) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Up,pClosestChar->m_Pos,nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Up, vec2(m_Pos.x + (BouncePos_Up.x - m_Pos.x)*2,m_Pos.y), 0.f, At, this))
				{
					Input.m_TargetX = BouncePos_Up.x - m_Pos.x; // aim
					Input.m_TargetY = BouncePos_Up.y - m_Pos.y;
					fire = true;
				}
				else if(Bounced_Down && (BouncePos_Down.x > (halfx + m_Pos.x) - 5.f && BouncePos_Down.x < (halfx + m_Pos.x) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Down,pClosestChar->m_Pos,nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Down, vec2(m_Pos.x + (BouncePos_Down.x - m_Pos.x)*2,m_Pos.y), 0.f, At, this))
				{
					Input.m_TargetX = BouncePos_Down.x - m_Pos.x; // aim
					Input.m_TargetY = BouncePos_Down.y - m_Pos.y;
					fire = true;
				}
			}
				
			
			if(fire)
			{
				Input.m_Fire = true;
			}
				
			
			
		}
	}
	
	
	if(TargetPosSet && !m_StopUntilTouchGround)
	{
		if(DoSmartTargetChase && !m_DontDoSmartTargetChase)
		{
			if(m_TryingDirectionSmart)
			{
				Input.m_Direction = m_TryingDirectionSmart;
				if((TargetPos.y + 56.0f) < m_Pos.y && !(Collision()->GetCollisionAt(m_Pos.x , m_Pos.y - GetProximityRadius() / 3.f) == TILE_DEATH))
				{
					targetisup = true;
				}
				else
				{
					dontjump = true;
					butjumpifwall = true;
				}
				if(!(Collision()->FastIntersectLine(m_Pos,m_Pos + vec2(0.f,1000.f),nullptr,nullptr)))
				{
					//danger no floor, remove directionsmart
					m_TryingOppositeSmart = m_TryingDirectionSmart = 0;
				}
			}
			else if(((TargetPos.x - m_Pos.x < 0 ? (TargetPos.y - m_Pos.y < 0 ? TargetPos.x - m_Pos.x > TargetPos.y - m_Pos.y : (TargetPos.x - m_Pos.x)*-1 < TargetPos.y - m_Pos.y) : (TargetPos.y - m_Pos.y < 0 ? TargetPos.x - m_Pos.x < (TargetPos.y - m_Pos.y)*-1 : TargetPos.x - m_Pos.x < TargetPos.y - m_Pos.y)) && TargetPos.x > m_Pos.x - 500.0f && TargetPos.x < m_Pos.x + 500.0f))
			{
				//is up/down
				
				bool left = false,right = false;
				
				if(TargetPos.y < m_Pos.y)
				{
					left = Collision()->FastIntersectLine(m_Pos,m_Pos + vec2(-400.f,-400.f),nullptr,nullptr);
					//middle = Collision()->IntersectLine(m_Pos,m_Pos + vec2(0,-350.f),nullptr,nullptr);
					right = Collision()->FastIntersectLine(m_Pos,m_Pos + vec2(400.f,-400.f),nullptr,nullptr);
					
					if(left && right && !m_TryingDirectionSmart && !Collision()->FastIntersectLine(m_Pos,m_Pos + vec2(0,-350.f),nullptr,nullptr))//middle
					{
						Input.m_Direction = TargetPos.x > m_Pos.x ? 1 : -1;
						targetisup = true;
					}
					else if(!m_TryingDirectionSmart && !left)//left
					{
						Input.m_Direction = -1;
						targetisup = true;
					}
					else if(!m_TryingDirectionSmart && !right)//right
					{
						Input.m_Direction = 1;
						targetisup = true;
					}
					else
					{
						//bool leftside = false,rightside = false;
						//vec2 leftcol,rightcol;
						
						
						if(!m_TryingDirectionSmart)
						{
							m_TryingDirectionSmart = TargetPos.x > m_Pos.x ? 1 : -1;
							/*leftside = Collision()->FastIntersectLine(m_Pos,m_Pos + vec2(-250.f,0),nullptr,&leftcol);
							rightside = Collision()->FastIntersectLine(m_Pos,m_Pos + vec2(250.f,0),nullptr,&rightcol);
							if(!leftside && !rightside)
							{
								m_TryingDirectionSmart = TargetPos.x > m_Pos.x ? 1 : -1;
							}
							else
							{
								if(!leftside)
								{
									m_TryingDirectionSmart = -1;
								}
								else if(!rightside)
								{
									m_TryingDirectionSmart = 1;
								}
								else
								{
									float d1,d2;
									d1 = distance(m_Pos,leftcol);
									d2 = distance(m_Pos,rightcol);
									
									if(d1 > d2)
									{
										m_TryingDirectionSmart = 1;
									}
									else
									{
										m_TryingDirectionSmart = -1;
									}
								}
							}*/
						}
						else
						{
							Input.m_Direction = m_TryingDirectionSmart;
						}
					}
				}
				else
				{
					left = Collision()->FastIntersectLine(m_Pos + vec2(-100.f,0),m_Pos + vec2(-100.f,-100.f),nullptr,nullptr);
					//middle = Collision()->IntersectLine(m_Pos + vec2(0,0),m_Pos + vec2(0,-100.f),nullptr,nullptr);
					right = Collision()->FastIntersectLine(m_Pos + vec2(100.f,0),m_Pos + vec2(100.f,-100.f),nullptr,nullptr);
					
					if(left && right && !m_TryingDirectionSmart && !Collision()->FastIntersectLine(m_Pos + vec2(0,0),m_Pos + vec2(0,-100.f),nullptr,nullptr))//middle
					{
						m_TryingDirectionSmart = TargetPos.x > m_Pos.x ? 1 : -1;
					}
					else if(!left && !m_TryingDirectionSmart)
					{
						m_TryingDirectionSmart = -1;
						dontjump = true;
						butjumpifwall = true;
					}
					else if(!right && !m_TryingDirectionSmart)
					{
						m_TryingDirectionSmart = 1;
						dontjump = true;
						butjumpifwall = true;
					}
					else
					{
						bool leftside = false,rightside = false;
						vec2 leftcol,rightcol;
						
						
						if(!m_TryingDirectionSmart)
						{
							if(!(leftside = Collision()->FastIntersectLine(m_Pos,m_Pos + vec2(-1000.f,0),nullptr,&leftcol)))
							{
								m_TryingDirectionSmart = -1;
							}
							else if(!(rightside = Collision()->FastIntersectLine(m_Pos,m_Pos + vec2(1000.f,0),nullptr,&rightcol)))
							{
								m_TryingDirectionSmart = 1;
							}
							else
							{
								float d1,d2;
								d1 = distance(m_Pos,leftcol);
								d2 = distance(m_Pos,rightcol);
								
								if(d1 > d2)
								{
									m_TryingDirectionSmart = 1;
								}
								else
								{
									m_TryingDirectionSmart = -1;
								}
							}
						}
						else
						{
							Input.m_Direction = m_TryingDirectionSmart;
						}
					}
				}
			}
			else
			{
				//is still away
				
				//bool up = false,middle = false,down = false;
				
				if(TargetPos.x > m_Pos.x)
				{
					//up = Collision()->IntersectLine(m_Pos,m_Pos + vec2(100.f,-100.f),nullptr,nullptr) || Collision()->IntersectLine(m_Pos + vec2(0,-100.f),m_Pos + vec2(50.f,-150.f),nullptr,nullptr);
					//middle = Collision()->IntersectLine(m_Pos,m_Pos + vec2(150.f,0),nullptr,nullptr);
					//down = Collision()->IntersectLine(m_Pos,m_Pos + vec2(100.f,100.f),nullptr,nullptr)  || Collision()->IntersectLine(m_Pos + vec2(0,100.f),m_Pos + vec2(50.f,150.f),nullptr,nullptr);
					
					if(!Collision()->FastIntersectLine(m_Pos,m_Pos + vec2(100.f,0),nullptr,nullptr))//middle
					{
						Input.m_Direction = 1;
						dontjump = true;
						jumpifgoingtofall = true;
						butjumpifwall = true;
					}
					else if(!(Collision()->FastIntersectLine(m_Pos,m_Pos + vec2(50.f,50.f),nullptr,nullptr))  || !(Collision()->IntersectLine(m_Pos + vec2(0,100.f),m_Pos + vec2(50.f,150.f),nullptr,nullptr)))//down
					{
						Input.m_Direction = 1;
						jumpifgoingtofall = true;
						dontjump = true;
						butjumpifwall = true;
					}
					else if(!(Collision()->FastIntersectLine(m_Pos,m_Pos + vec2(50.f,-50.f),nullptr,nullptr)) || !(Collision()->IntersectLine(m_Pos + vec2(0,-100.f),m_Pos + vec2(50.f,-150.f),nullptr,nullptr)))//up
					{
						Input.m_Direction = 1;
						targetisup = true;
					}
					else
					{
						if(!m_TryingDirectionSmart)
						{
							bool upside = false, downside = false;
							downside = Collision()->FastIntersectLine(m_Pos + vec2(GetProximityRadius()/2,0),m_Pos + vec2(0,300.f),nullptr,nullptr);
							upside = Collision()->FastIntersectLine(m_Pos + vec2(GetProximityRadius()/2,0),m_Pos + vec2(0,-150.f),nullptr,nullptr);
							if(!upside)
							{
								targetisup = true;
							}
							else if(downside)
							{
								m_TryingDirectionSmart = -1;
								targetisup = true;
							}
							
						}
					}
				}
				else
				{
					//up = Collision()->IntersectLine(m_Pos,m_Pos + vec2(-100.f,-100.f),nullptr,nullptr) || Collision()->IntersectLine(m_Pos + vec2(0,-100.f),m_Pos + vec2(-50.f,-150.f),nullptr,nullptr);
					//middle = Collision()->IntersectLine(m_Pos,m_Pos + vec2(-150.f,0),nullptr,nullptr);
					//down = Collision()->IntersectLine(m_Pos,m_Pos + vec2(-100.f,100.f),nullptr,nullptr)  || Collision()->IntersectLine(m_Pos + vec2(0,100.f),m_Pos + vec2(-50.f,150.f),nullptr,nullptr);
					
					if(!Collision()->FastIntersectLine(m_Pos,m_Pos + vec2(-100.f,0),nullptr,nullptr))//middle
					{
						Input.m_Direction = -1;
						jumpifgoingtofall = true;
						dontjump = true;
						butjumpifwall = true;
					}
					else if(!(Collision()->FastIntersectLine(m_Pos,m_Pos + vec2(-50.f,100.f),nullptr,nullptr))  || !(Collision()->FastIntersectLine(m_Pos + vec2(0,100.f),m_Pos + vec2(-50.f,150.f),nullptr,nullptr)))//down
					{
						Input.m_Direction = -1;
						jumpifgoingtofall = true;
						dontjump = true;
						butjumpifwall = true;
					}
					else if(!(Collision()->FastIntersectLine(m_Pos,m_Pos + vec2(-50.f,-100.f),nullptr,nullptr)) || !(Collision()->FastIntersectLine(m_Pos + vec2(0,-100.f),m_Pos + vec2(-50.f,-150.f),nullptr,nullptr)))//up
					{
						Input.m_Direction = -1;
						targetisup = true;
					}
					else
					{
						if(!m_TryingDirectionSmart)
						{
							bool upside = false, downside = false;
							downside = Collision()->FastIntersectLine(m_Pos + vec2(GetProximityRadius()/-2,0),m_Pos + vec2(0,300.f),nullptr,nullptr);
							upside = Collision()->FastIntersectLine(m_Pos + vec2(GetProximityRadius()/-2,0),m_Pos + vec2(0,-150.f),nullptr,nullptr);
							if(!upside)
							{
								targetisup = true;
							}
							else if(upside && downside)
							{
								m_TryingDirectionSmart = 1;
								targetisup = true;
							}
							
						}
					}
				}
			}
			
		}
		else
		{
			Input.m_Direction = TargetPos.x > m_Pos.x ? 1 : -1;
			
			if((TargetPos.y + 56.0f) < m_Pos.y && !(Collision()->GetCollisionAt(m_Pos.x , m_Pos.y - GetProximityRadius() / 3.f) == TILE_DEATH))
			{
				targetisup = true;
			}
			else
			{
				dontjump = true;
				butjumpifwall = true;
			}
			m_DontDoSmartTargetChase--;
		}
	}
	
	if(m_TryingDirectionSmart && !Collision()->FastIntersectLine(m_Pos,TargetPos,nullptr,nullptr))
	{
		m_TryingOppositeSmart = m_TryingDirectionSmart = 0;
		m_StopUntilTouchGround = true;
	}
	
	if(m_StopUntilTouchGround && IsGrounded())
	{
		m_StopUntilTouchGround = false;
		m_DontDoSmartTargetChase = Server()->TickSpeed() * 2;
	}
	if(m_Core.m_Colliding && IsGrounded())
	{
		if(!m_TryingOppositeSmart)
		{
			m_TryingDirectionSmart *= -1;
			m_TryingOppositeSmart = true;
		}
		else
		{
			m_TryingDirectionSmart = 0;
			m_TryingOppositeSmart = false;
		}
	}
	
	
	//HELP
	if((jumpifgoingtofall ? !(Collision()->FastIntersectLine(m_Pos,m_Pos + vec2(0.f,1000.f),nullptr,nullptr)) : false) || (butjumpifwall ? m_Core.m_Colliding : false) || (!dontjump && ((Collision()->GetCollisionAt(m_Pos.x , m_Pos.y + GetProximityRadius() / 3.f) == TILE_DEATH) || !Collision()->FastIntersectLine(m_Pos,m_Pos + vec2(0.f,1000.f),nullptr,nullptr) || m_Core.m_Colliding || (((Collision()->CheckPoint(m_Pos.x + GetProximityRadius() / 2, m_Pos.y + GetProximityRadius() / 2 + 5)) && !(Collision()->CheckPoint(m_Pos.x - GetProximityRadius() / 2, m_Pos.y + GetProximityRadius() / 2 + 5)))) || ((!(Collision()->CheckPoint(m_Pos.x + GetProximityRadius() / 2, m_Pos.y + GetProximityRadius() / 2 + 5)) && (Collision()->CheckPoint(m_Pos.x - GetProximityRadius() / 2, m_Pos.y + GetProximityRadius() / 2 + 5)))) || targetisup)))
	{
		
		if(IsGrounded() || (m_Core.m_Jumps > 0 && m_Core.m_Vel.y > 0))
			Input.m_Jump = true;
		else
			Input.m_Jump = false;
	}
}

void CCharacter::DoPointerBotAI(CNetObj_PlayerInput &Input)
{
	
	Input.m_Direction = 0;
	Input.m_TargetX = (rand() % 128) - 64; // look randomly
	Input.m_TargetY = (rand() % 128) - 64;
	Input.m_Jump = false;
	Input.m_Fire = true;
	if (!GameServer()->m_pController->m_IsInstagibKZ) // make non-automatic weapons work, but still have ammo reload work in instagib
		Input.m_Fire = Server()->Tick() % (2) == 1;
	Input.m_Hook = false;
	Input.m_PlayerFlags = PLAYERFLAG_PLAYING;
	Input.m_WantedWeapon = WEAPON_GUN+1;
	if (GameServer()->m_pController->m_IsInstagibKZ)
	{
		if(GameServer()->m_pController->m_pGameType[0] == 'i')
			Input.m_WantedWeapon = WEAPON_LASER+1;
		else if (GameServer()->m_pController->m_pGameType[0] == 'g')
			Input.m_WantedWeapon = WEAPON_GRENADE+1;
	}
	Input.m_NextWeapon = WEAPON_GUN+1;
	Input.m_PrevWeapon = WEAPON_GUN+1;
	// move, and occasionally jump
	if (rand() % (Server()->TickSpeed()*2) == 1)
		m_botDirectionPointer = -m_botDirectionPointer;
	Input.m_Direction = m_botDirectionPointer;
	if (rand() % (Server()->TickSpeed()*2) == 1)
		Input.m_Jump = true;
	
	
	if (GameServer()->Server()->Tick() % (Server()->TickSpeed()) == 1)
	{
		// get a new aggro
		float smallestDistance = 850.0;
		m_botAggroPointer = -1;
		
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (i != m_pPlayer->GetCid() && GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetCharacter()) {
				vec2 pos = GameServer()->m_apPlayers[i]->GetCharacter()->m_Pos;
				float d = sqrt((pos.x - m_Pos.x)*(pos.x - m_Pos.x) + (1.35)*(pos.y - m_Pos.y)*(pos.y - m_Pos.y));
				// vertical distance is multiplied by a factor, since screens are larger horizontally
				if (d < smallestDistance && !(GameServer()->m_pController->IsTeamPlay() && GameServer()->m_apPlayers[i]->GetTeam() == m_pPlayer->GetTeam())) {
					smallestDistance = d;
					m_botAggroPointer = i;
				}
			}
		}
	}
	if (m_botAggroPointer == -1)
	{
		m_ticksSinceFirePointer = 0; // reset
		Input.m_Fire = false; // do not shoot by default
	}
	else
	{
		m_ticksSinceFirePointer++;
		if (GameServer()->m_pController->m_pGameType[0] == 'g')
		{
			if (m_ticksSinceFirePointer > 50)
			{
				m_ticksSinceFirePointer = 0; // reset
				Input.m_Fire = true; // fire
			}
			else
				Input.m_Fire = false; // do not shoot by default
		}
		else if(GameServer()->m_pController->m_IsInstagibKZ)
		{
			m_ticksSinceFirePointer = 0; // reset
			Input.m_Fire = true; // fire
		}
		else
		{
			if (m_ticksSinceFirePointer > 10)
			{ // 10 = 0.2s
				m_ticksSinceFirePointer = 0; // reset
				Input.m_Fire = true; // fire
			} else
				Input.m_Fire = false; // do not shoot by default
			Input.m_Fire = GameServer()->Server()->Tick() % (2) == 1;
		}
		
		// aim
		if (GameServer()->m_apPlayers[m_botAggroPointer] && GameServer()->m_apPlayers[m_botAggroPointer]->GetCharacter()) {
			vec2 pos = GameServer()->m_apPlayers[m_botAggroPointer]->GetCharacter()->m_Pos;
			//float d = sqrt((pos.x - m_Pos.x)*(pos.x - m_Pos.x) + (pos.y - m_Pos.y)*(pos.y - m_Pos.y)); unused
			Input.m_TargetX = pos.x - m_Pos.x; // aim
			Input.m_TargetY = pos.y - m_Pos.y;
			if (GameServer()->m_pController->m_pGameType[0] == 'g') // grenade curve correction, somewhat
				Input.m_TargetY = Input.m_TargetY + (-abs(Input.m_TargetX)*0.3);
		}
	}
	
	m_Core.m_aWeapons[WEAPON_GUN].m_Ammo = 10;
}

 //JSAURUS rollback
bool CCharacter::TakeDamage(vec2 Force, int Dmg, int From, int Weapon, int tick)
{
	m_Core.m_DeathTick = Server()->Tick() + (m_pPlayer->m_Latency.m_Avg * Server()->TickSpeed())/1000;

	//m_KillTick = tick;
	m_RollbackAttacker = From;
	m_RollbackAttackerWeapon = Weapon;
	m_RollbackDamagePos = m_Pos;

	if(m_RollbackAttacker >= 0 && m_RollbackAttacker < MAX_CLIENTS)
	{
		if(GameServer()->m_apPlayers[m_RollbackAttacker] && GameServer()->m_apPlayers[m_RollbackAttacker]->GetCharacter())
			m_RollbackDamagePos = m_Core.m_Positions[(GameServer()->m_apPlayers[m_RollbackAttacker]->GetCharacter()->GetCore().m_LastAckedSnapshot-1) % POSITION_HISTORY];
	}

	if(m_RollbackAttacker >= 0 && m_RollbackAttacker < MAX_CLIENTS && m_RollbackAttacker != m_pPlayer->GetCid() && GameServer()->m_apPlayers[m_RollbackAttacker])
	{
		//GameServer()->CreateSound(GameServer()->m_apPlayers[m_RollbackAttacker]->m_ViewPos, SOUND_HIT, TeamMask());
		bool a = TakeDamage(Force, Dmg, m_RollbackAttacker, m_RollbackAttackerWeapon);
		return a;
	}
	if(m_RollbackAttacker < 0 || m_Core.m_Id == From || !m_pPlayer || !m_pPlayer->m_Rollback || !g_Config.m_SvRollback)
	{
		bool a = TakeDamage(Force, Dmg, From, Weapon);
		return a;
	}

	return false;
}

bool CCharacter::DropWeapon(int Weapon)
{
	if(GetWeaponGot(Weapon))
	{
		CKZPickup* pickup = new CKZPickup(GameWorld(),POWERUP_WEAPON,Weapon);
		pickup->m_Pos = m_Pos;
		pickup->m_Dropped = true;
		pickup->m_DropTick = Server()->Tick();
		pickup->m_Vel = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY)) * 10;
		pickup->m_ThisTeamOnly = Team();
		pickup->m_Ammo = GetWeaponAmmo(Weapon);

		GameServer()->CreateSound(m_Pos,SOUND_WEAPON_SWITCH,TeamMask());

		GiveWeapon(Weapon,true,0);

		return true;
	}
	return false;
}

int CCharacter::FindGotWeaponKZ()
{
	bool got;
	for(int i=WEAPON_HAMMER;i<KZ_NUM_CUSTOM_WEAPONS;i++)
	{
		got = GetWeaponGot(i);
		if(got)
		{
			return i;
		}
	}
	return -1;
}