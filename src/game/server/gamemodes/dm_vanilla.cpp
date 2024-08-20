#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "dm_vanilla.h"

CGameControllerDMVanilla::CGameControllerDMVanilla(class CGameContext *pGameServer) :
	CGameControllerDM(pGameServer)
{
    m_VanillaBehavior = true;
    
    m_GameFlags = 0;
    m_GameFlags_v7 = 0;

    m_pGameType = "DM+";
}

CGameControllerDMVanilla::~CGameControllerDMVanilla() = default;
