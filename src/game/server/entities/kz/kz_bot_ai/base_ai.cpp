#include "base_ai.h"

CBaseKZBotAI::CBaseKZBotAI(CCharacter * pChr)
{
	m_pCharacter = pChr;
	m_pCore = (CCharacterCore *)pChr->Core();
	m_pPlayer = pChr->GetPlayer();

	m_pCCollision = pChr->Collision();
	m_pGameWorld = pChr->GameWorld();
	m_pGameServer = pChr->GameServer();
	m_pServer = pChr->Server();
	
	m_pPos = &pChr->m_Pos;
}

CBaseKZBotAI::~CBaseKZBotAI()
{
}

void CBaseKZBotAI::HandleInput(CNetObj_PlayerInput &Input)
{
}
