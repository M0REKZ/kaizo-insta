#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "tdm_vanilla.h"

CGameControllerTDMVanilla::CGameControllerTDMVanilla(class CGameContext *pGameServer) :
	CGameControllerTDM(pGameServer)
{
    m_VanillaBehavior = true;
    
    m_GameFlags = GAMEFLAG_TEAMS;

    m_GameFlags_v7 = protocol7::GAMEFLAG_TEAMS;

    m_pGameType = "TDM+";
}

CGameControllerTDMVanilla::~CGameControllerTDMVanilla() = default;
