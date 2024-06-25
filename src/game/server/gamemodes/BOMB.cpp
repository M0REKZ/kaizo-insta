#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "BOMB.h"

CGameControllerBOMB::CGameControllerBOMB(class CGameContext *pGameServer) :
	CGameControllerLMSVanilla(pGameServer)
{
    m_VanillaBehavior = true;

    m_GameFlags = 0;
    m_GameFlags_v7 = protocol7::GAMEFLAG_SURVIVAL;

    m_pGameType = "BOMB";
    
    m_BombTime = g_Config.m_SvBombTime * Server()->TickSpeed();
}

CGameControllerBOMB::~CGameControllerBOMB() = default;

void CGameControllerBOMB::Tick()
{

    CGameControllerLMSVanilla::Tick();
    
    SetSkins(); //a lot of ugly loops...
    
    if(m_RoundActive)
    {
        if(!BombAmount())
        {
            SetBombs();
            m_BombTime = g_Config.m_SvBombTime * Server()->TickSpeed();
        }
        
        m_BombTime--;

        if(!m_BombTime)
        {
            for(int i = 0; i < MAX_CLIENTS; ++i)
            {
                if(GameServer()->m_apPlayers[i])
                {
                    if(GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->m_IsBomb )
                    {
                        ExplodeBomb(GameServer()->m_apPlayers[i]);
                    }
                }
            }
            m_BombTime = g_Config.m_SvBombTime * Server()->TickSpeed();
        }
    }

}

void CGameControllerBOMB::ExplodeBomb(CPlayer* BombPlayer)
{
    vec2 BombPos = BombPlayer->GetCharacter()->m_Pos;
    
    GameServer()->CreateSound(BombPos, SOUND_GRENADE_EXPLODE);
    BombPlayer->KillCharacter();
    GameServer()->CreateExplosion(BombPos, BombPlayer->GetCid(), WEAPON_GAME, false, 0);
    BombPlayer->m_IsBomb = false;
    
    //if(BombPlayer->GetCharacter()) //If bomb didnt die by explosion damage
        
    
    char aBuf[128];
    str_format(aBuf, sizeof(aBuf), "'%s' eliminated!", Server()->ClientName(BombPlayer->GetCid()));
    GameServer()->SendChat(-1, protocol7::CHAT_ALL, aBuf, -1, CGameContext::CHAT_SIX);
}

void CGameControllerBOMB::Snap(int SnappingClient)
{
    CGameControllerInstagib::Snap(SnappingClient);
    
    if(!(m_BombTime % Server()->TickSpeed()))
    {
        if(GameServer()->m_apPlayers[SnappingClient] && (GameServer()->m_apPlayers[SnappingClient]->GetTeam() != TEAM_SPECTATORS && (!GameServer()->m_apPlayers[SnappingClient]->m_IsDead || (GameServer()->m_apPlayers[SnappingClient]->GetCharacter() && GameServer()->m_apPlayers[SnappingClient]->GetCharacter()->IsAlive())) && GameServer()->m_apPlayers[SnappingClient]->m_IsBomb))
        {
            GameServer()->CreateDamageInd(GameServer()->m_apPlayers[SnappingClient]->GetCharacter()->m_Pos, 0, m_BombTime / Server()->TickSpeed(), GameServer()->m_apPlayers[SnappingClient]->GetCharacter()->TeamMask());
            GameServer()->CreateSound(GameServer()->m_apPlayers[SnappingClient]->m_ViewPos, SOUND_HOOK_NOATTACH);
        }
    }
}

void CGameControllerBOMB::OnCharacterSpawn(class CCharacter *pChr)
{
    OnCharacterConstruct(pChr);

    pChr->SetTeams(&Teams());
    Teams().OnCharacterSpawn(pChr->GetPlayer()->GetCid());
    
    pChr->IncreaseHealth(10);
    pChr->IncreaseArmor(10);
    
    pChr->GiveWeapon(WEAPON_HAMMER);
}

bool CGameControllerBOMB::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
    return CGameControllerInstagib::OnEntity(Index, x, y, Layer, Flags, Initial, Number);
     
}

bool CGameControllerBOMB::OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character)
{
    Dmg = 0; //TODO: maybe i should add an option for bomb with damage
    CGameControllerLMSVanilla::OnCharacterTakeDamage(Force, Dmg, From, Weapon, Character);
    if(GameServer()->m_apPlayers[From] && GameServer()->m_apPlayers[From]->m_IsBomb)
        TransferBomb(GameServer()->m_apPlayers[From], Character.GetPlayer());
    return false;
}

int CGameControllerBOMB::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
{
    pVictim->GetPlayer()->m_IsBomb = false;
    pVictim->GetPlayer()->SetTeamRaw(TEAM_SPECTATORS);
    pVictim->GetPlayer()->m_IsDead = true;
    //if(!BombAmount())
        //SetBombs();
	return false;
}


int CGameControllerBOMB::BombAmount()
{
    int amount = 0;
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(GameServer()->m_apPlayers[i])
        {
            if(GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS && (!GameServer()->m_apPlayers[i]->m_IsDead || (GameServer()->m_apPlayers[i]->GetCharacter() && GameServer()->m_apPlayers[i]->GetCharacter()->IsAlive())) && GameServer()->m_apPlayers[i]->m_IsBomb)
            {
                amount++;
            }
        }
    }
    return amount;
}

int CGameControllerBOMB::CharAmount()
{

    int amount = 0;
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(GameServer()->m_apPlayers[i])
        {
            if(GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS && (!GameServer()->m_apPlayers[i]->m_IsDead || (GameServer()->m_apPlayers[i]->GetCharacter() && GameServer()->m_apPlayers[i]->GetCharacter()->IsAlive())))
            {
                amount++;
            }
        }
    }
    return amount;
}

void CGameControllerBOMB::TransferBomb(CPlayer* From, CPlayer* To)
{
    To->m_IsBomb = From->m_IsBomb;
    From->m_IsBomb = false;
}

void CGameControllerBOMB::SetBombs()
{
    int bombneed = CharAmount() / g_Config.m_SvBombAmount;
    
    if(!bombneed && CharAmount() <= 1)
        return;
    else
        bombneed = 1;
    
    
    
    int rnd = 0;
    while(bombneed)
    {
        rnd = rand() % MAX_CLIENTS;
        if(GameServer()->m_apPlayers[rnd])
        {
            if(GameServer()->m_apPlayers[rnd]->GetTeam() != TEAM_SPECTATORS && (!GameServer()->m_apPlayers[rnd]->m_IsDead || (GameServer()->m_apPlayers[rnd]->GetCharacter() && GameServer()->m_apPlayers[rnd]->GetCharacter()->IsAlive())))
            {
                GameServer()->m_apPlayers[rnd]->m_IsBomb = true;
                bombneed--;
            }
        }
    }
    
    SetSkins();
}

void CGameControllerBOMB::SetSkins()
{
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(GameServer()->m_apPlayers[i])
        {
            if(GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS && (!GameServer()->m_apPlayers[i]->m_IsDead || (GameServer()->m_apPlayers[i]->GetCharacter() && GameServer()->m_apPlayers[i]->GetCharacter()->IsAlive())))
            {
                if(GameServer()->m_apPlayers[i]->m_IsBomb)
                {
                    str_copy(GameServer()->m_apPlayers[i]->m_TeeInfos.m_aSkinName, "bomb", sizeof(GameServer()->m_apPlayers[i]->m_TeeInfos.m_aSkinName));
                    GameServer()->m_apPlayers[i]->m_TeeInfos.m_UseCustomColor = 0;
                }
                else
                {
                    str_copy(GameServer()->m_apPlayers[i]->m_TeeInfos.m_aSkinName, "cammostripes", sizeof(GameServer()->m_apPlayers[i]->m_TeeInfos.m_aSkinName));
                    GameServer()->m_apPlayers[i]->m_TeeInfos.m_UseCustomColor = 1;
                    GameServer()->m_apPlayers[i]->m_TeeInfos.m_ColorBody = 16777215;
                    GameServer()->m_apPlayers[i]->m_TeeInfos.m_ColorFeet = 16777215;
                }
            }
        }
    }
}
