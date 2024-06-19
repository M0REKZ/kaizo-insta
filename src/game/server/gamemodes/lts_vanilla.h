#ifndef GAME_SERVER_GAMEMODES_LTSVANILLA_H
#define GAME_SERVER_GAMEMODES_LTSVANILLA_H

#include "lms_vanilla.h"

class CGameControllerLTSVanilla : public CGameControllerLMSVanilla
{

protected:
    virtual bool DoWincheckMatch() override;
    virtual void SetAllUndead() override;
    //int m_RoundPauseTime = -1;

public:
	CGameControllerLTSVanilla(class CGameContext *pGameServer);
	~CGameControllerLTSVanilla();

	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
    virtual void OnPlayerConnect(class CPlayer *pPlayer) override;
	void Tick() override;
    //bool m_RoundActive = false;
    int m_aPlayerTeam[MAX_CLIENTS];
};
#endif // GAME_SERVER_GAMEMODES_LMS_H
