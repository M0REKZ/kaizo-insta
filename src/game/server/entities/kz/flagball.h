/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_FLAGBALL_H
#define GAME_SERVER_ENTITIES_FLAGBALL_H

#include <game/server/entities/flag.h>

class CFlagBall : public CFlag
{
public:
//	int m_LastCarrier;
	int m_LastCarrierTeam;
//	int m_IdleTick;

	CFlagBall(CGameWorld *pGameWorld, int Team);

	virtual void Reset() override;
	virtual void Tick() override;
	void TickDeferred() override;

	void Grab(class CCharacter *pChar);
	void Drop(vec2 Direction = vec2(0, 0));

	void HandleKZTiles();
};

#endif
