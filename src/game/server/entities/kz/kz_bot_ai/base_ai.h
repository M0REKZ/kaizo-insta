#ifndef GAME_SERVER_ENTITIES_KZ_BOT_AI_BASE_AI_H
#define GAME_SERVER_ENTITIES_KZ_BOT_AI_BASE_AI_H

#include <game/server/entities/character.h>
#include <game/server/player.h>

enum
{
	INVALID_AI = -1,
	KZBOT_AI = 0,
	POINTERBOT_AI,
};

class CBaseKZBotAI
{
public:

	CBaseKZBotAI(CCharacter * pChr);
	virtual ~CBaseKZBotAI();

	virtual void HandleInput(CNetObj_PlayerInput &Input);

	std::vector<SSwitchers> &Switchers() { return m_pGameWorld->m_Core.m_vSwitchers; }
	CGameWorld *GameWorld() { return m_pGameWorld; }
	CTuningParams *Tuning() { return GameWorld()->Tuning(); }
	CTuningParams *TuningList() { return GameWorld()->TuningList(); }
	CTuningParams *GetTuning(int i) { return GameWorld()->GetTuning(i); }
	class CConfig *Config() { return m_pGameWorld->Config(); }
	class CGameContext *GameServer() { return m_pGameServer; }
	class IServer *Server() { return m_pServer; }
	CCollision *Collision() { return m_pCCollision; }

	CCharacter * GetCharacter() { return m_pCharacter; }
	int GetAIType() { return m_AIType; }

protected:

	vec2* m_pPos = nullptr;
	CCharacterCore* m_pCore = nullptr;
	CPlayer* m_pPlayer = nullptr;
	int m_AIType = INVALID_AI;

private:

	CCharacter* m_pCharacter = nullptr;
	CCollision* m_pCCollision = nullptr;
	CGameWorld* m_pGameWorld = nullptr;
	CGameContext* m_pGameServer = nullptr;
	IServer* m_pServer = nullptr;

};

#endif
