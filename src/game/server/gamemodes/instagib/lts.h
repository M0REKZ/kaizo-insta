#ifndef GAME_SERVER_GAMEMODES_INSTAGIB_LTS_H
#define GAME_SERVER_GAMEMODES_INSTAGIB_LTS_H

#include "lms.h"

class CGameControllerInstaLTS : public CGameControllerInstaLMS
{

protected:
    virtual bool DoWincheckRound() override;
    virtual void SetAllUndead() override;
    //int m_RoundPauseTime = -1;

public:
	CGameControllerInstaLTS(class CGameContext *pGameServer);
	~CGameControllerInstaLTS();

	void Tick() override;
	virtual void Snap(int SnappingClient) override;
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
    virtual void OnPlayerConnect(class CPlayer *pPlayer) override;
    //bool m_RoundActive = false;
    int m_aPlayerTeam[MAX_CLIENTS];
};
#endif // GAME_SERVER_GAMEMODES_LMS_H
