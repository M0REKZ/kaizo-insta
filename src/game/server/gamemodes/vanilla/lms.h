#ifndef GAME_SERVER_GAMEMODES_LMS_H
#define GAME_SERVER_GAMEMODES_LMS_H

#include "dm.h"

class CGameControllerLMS : public CGameControllerDM
{
protected:
    virtual bool DoWincheckRound() override;
    virtual void SetAllUndead();
    virtual void FakeEndRound();
    virtual void KillEveryone();
    int m_RoundPauseTime = -1;
public:
	CGameControllerLMS(class CGameContext *pGameServer);
	~CGameControllerLMS();

    virtual bool CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
    virtual void OnPlayerConnect(class CPlayer *pPlayer) override;
	void Tick() override;
    bool m_RoundActive = false;
};
#endif // GAME_SERVER_GAMEMODES_DM_H
