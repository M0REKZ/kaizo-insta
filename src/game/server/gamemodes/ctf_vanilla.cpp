#include "ctf_vanilla.h"

#include <engine/server.h>
#include <engine/shared/config.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/entities/flag.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/version.h>

CGameControllerCTFVanilla::CGameControllerCTFVanilla(class CGameContext *pGameServer) :
    CGameControllerCTF(pGameServer)
{
    m_VanillaBehavior = true;

    m_pGameType = "CTF+";
}

CGameControllerCTFVanilla::~CGameControllerCTFVanilla() = default;

void CGameControllerCTFVanilla::OnCharacterSpawn(class CCharacter *pChr)
{
    OnCharacterConstruct(pChr);

    pChr->SetTeams(&Teams());
    Teams().OnCharacterSpawn(pChr->GetPlayer()->GetCid());

    // default health
    pChr->IncreaseHealth(10);

    pChr->ResetPickups();

    pChr->GiveWeapon(WEAPON_GUN, false, 10);
    pChr->GiveWeapon(WEAPON_HAMMER);
    pChr->SetActiveWeapon(WEAPON_GUN);
}

bool CGameControllerCTFVanilla::OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character)
{
    if(Character.m_IsGodmode)
    {
        Dmg = 0;
        return false;
    }
    // TODO: ddnet-insta cfg team damage
    // if(GameServer()->m_pController->IsFriendlyFire(Character.GetPlayer()->GetCid(), From) && !g_Config.m_SvTeamdamage)
    if(GameServer()->m_pController->IsFriendlyFire(Character.GetPlayer()->GetCid(), From))
        return false;

    if(g_Config.m_SvOnlyHookKills && From >= 0 && From <= MAX_CLIENTS)
    {
        CCharacter *pChr = GameServer()->m_apPlayers[From]->GetCharacter();
        if(!pChr || pChr->GetCore().HookedPlayer() != Character.GetPlayer()->GetCid())
            Dmg = 0;
    }

    if(WEAPON_LASER == Weapon)
    {
        Dmg = 5;
    }

    int damage = Dmg;

    if(From == Character.GetPlayer()->GetCid())
        damage = std::max(1, damage/2);
    
    Character.m_DamageTaken++;

    // create healthmod indicator
    if(Server()->Tick() < Character.m_DamageTakenTick+25)
    {
        // make sure that the damage indicators doesn't group together
        GameServer()->CreateDamageInd(Character.m_Pos, Character.m_DamageTaken*0.25f, Dmg);
    }
    else
    {
        Character.m_DamageTaken = 0;
        GameServer()->CreateDamageInd(Character.m_Pos, 0, damage);
    }
    
    if(damage)
    {
        if(Character.m_Armor)
        {
            if(damage > 1)
            {
                Character.m_Health--;
                damage--;
            }

            if(damage > Character.m_Armor)
            {
                damage -= Character.m_Armor;
                Character.m_Armor = 0;
            }
            else
            {
                Character.m_Armor -= damage;
                damage = 0;
            }
        }

        Character.m_Health -= damage;
    }

    Character.m_DamageTakenTick = Server()->Tick();

    if(From >= 0 && From != Character.GetPlayer()->GetCid() && GameServer()->m_apPlayers[From])
    {
        // do damage Hit sound
        CClientMask Mask = CClientMask().set(From);
        for(int i = 0; i < MAX_CLIENTS; i++)
        {
            if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->m_SpectatorId == From)
                Mask.set(i);
            
            // if(gaem)
        }
        GameServer()->CreateSound(GameServer()->m_apPlayers[From]->m_ViewPos, SOUND_HIT, Mask);
    }

    // check for death
    if(Character.m_Health <= 0)
    {
        Character.Die(From, Weapon);

        if(From >= 0 && From != Character.GetPlayer()->GetCid() && GameServer()->m_apPlayers[From])
        {
            CCharacter *pChr = GameServer()->m_apPlayers[From]->GetCharacter();
            if(pChr)
            {
                // set attacker's face to happy (taunt!)
                pChr->SetEmote(EMOTE_HAPPY, Server()->Tick() + Server()->TickSpeed());

                // refill nades
                int RefillNades = 0;
                if(g_Config.m_SvGrenadeAmmoRegenOnKill == 1)
                    RefillNades = 1;
                else if(g_Config.m_SvGrenadeAmmoRegenOnKill == 2)
                    RefillNades = g_Config.m_SvGrenadeAmmoRegenNum;
                if(RefillNades && g_Config.m_SvGrenadeAmmoRegen && Weapon == WEAPON_GRENADE)
                {
                    pChr->SetWeaponAmmo(WEAPON_GRENADE, minimum(pChr->GetCore().m_aWeapons[WEAPON_GRENADE].m_Ammo + RefillNades, g_Config.m_SvGrenadeAmmoRegenNum));
                }
            }

            // do damage Hit sound
            CClientMask Mask = CClientMask().set(From);
            for(int i = 0; i < MAX_CLIENTS; i++)
            {
                if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->m_SpectatorId == From)
                    Mask.set(i);
            }
            GameServer()->CreateSound(GameServer()->m_apPlayers[From]->m_ViewPos, SOUND_HIT, Mask);
        }
        return false;
    }

    if (damage > 2)
        GameServer()->CreateSound(Character.GetPos(), SOUND_PLAYER_PAIN_LONG);
    else
        GameServer()->CreateSound(Character.GetPos(), SOUND_PLAYER_PAIN_SHORT);

    if(damage)
    {
        Character.SetEmote(EMOTE_PAIN, Server()->Tick() + 500 * Server()->TickSpeed() / 1000);
    }

    return false;
}

int CGameControllerCTFVanilla::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
{
    if(WeaponId == WEAPON_SELF)
    {
        pVictim->GetPlayer()->m_RespawnTick = Server()->Tick() + Server()->TickSpeed() * 3.0f;
    }
    else
        pVictim->GetPlayer()->m_RespawnTick = Server()->Tick() + Server()->TickSpeed() * 0.5f;
    
    return CGameControllerCTF::OnCharacterDeath(pVictim, pKiller, WeaponId);
}
