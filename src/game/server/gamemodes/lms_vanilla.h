#ifndef GAME_SERVER_GAMEMODES_LMSVANILLA_H
#define GAME_SERVER_GAMEMODES_LMSVANILLA_H

#include "dm_vanilla.h"

class CGameControllerLMSVanilla : public CGameControllerDMVanilla
{
protected:
    virtual bool DoWincheckMatch() override;
    virtual void SetAllUndead();
    virtual void FakeEndRound();
    virtual void KillEveryone();
    int m_RoundPauseTime = -1;
public:
	CGameControllerLMSVanilla(class CGameContext *pGameServer);
	~CGameControllerLMSVanilla();

    virtual bool CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize) override;
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
    virtual void OnPlayerConnect(class CPlayer *pPlayer) override;
	void Tick() override;
    bool m_RoundActive = false;
};
#endif // GAME_SERVER_GAMEMODES_DM_H
