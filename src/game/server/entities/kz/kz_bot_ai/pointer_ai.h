#ifndef GAME_SERVER_ENTITIES_KZ_BOT_AI_POINTER_AI_H
#define GAME_SERVER_ENTITIES_KZ_BOT_AI_POINTER_AI_H

#include "base_ai.h"

class CPointerBotAI : public CBaseKZBotAI
{
public:

	CPointerBotAI(CCharacter * pChr);

	virtual void HandleInput(CNetObj_PlayerInput &Input) override;

	int m_botAggroPointer = -1;
	int m_ticksSinceFirePointer = 0;
	int m_botDirectionPointer = 1;
};

#endif
