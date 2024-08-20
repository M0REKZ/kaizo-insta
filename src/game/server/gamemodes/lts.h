#ifndef GAME_SERVER_GAMEMODES_LTS_H
#define GAME_SERVER_GAMEMODES_LTS_H

#include "lms.h"

class CGameControllerLTS : public CGameControllerLMS
{

protected:
    virtual bool DoWincheckMatch() override;
    virtual void SetAllUndead() override;
    //int m_RoundPauseTime = -1;

public:
	CGameControllerLTS(class CGameContext *pGameServer);
	~CGameControllerLTS();

	void Tick() override;
	virtual void Snap(int SnappingClient) override;
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
    virtual void OnPlayerConnect(class CPlayer *pPlayer) override;
    //bool m_RoundActive = false;
    int m_aPlayerTeam[MAX_CLIENTS];
};
#endif // GAME_SERVER_GAMEMODES_LMS_H
