#ifndef GAME_SERVER_ENTITIES_KZ_BOT_AI_KZ_AI_H
#define GAME_SERVER_ENTITIES_KZ_BOT_AI_KZ_AI_H

#include "base_ai.h"

class CKZBotAI : public CBaseKZBotAI
{
public:

	CKZBotAI(CCharacter * pChr);

	virtual void HandleInput(CNetObj_PlayerInput &Input) override;

	int m_TryingDirectionSmart = 0;
	bool m_TryingOppositeSmart = false;
	bool m_StopUntilTouchGround = false;
	int m_DontDoSmartTargetChase = 0;
	bool m_DoGrenadeJump = false;

};

#endif
