#include <engine/shared/protocol.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontroller.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/version.h>

#include <game/server/gamecontext.h>

// implemented in ddracechat.cpp
// yes that is cursed
bool CheckClientId(int ClientId);

void CGameContext::ConStatsRound(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	if(!pSelf->m_pController->IsStatTrack())
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp",
			"! WARNING: not enough players connected to track stats");

	const char *pName = pResult->NumArguments() ? pResult->GetString(0) : pSelf->Server()->ClientName(pResult->m_ClientId);
	int TargetId = pSelf->m_pController->GetCidByName(pName);
	if(TargetId < 0 || TargetId >= MAX_CLIENTS)
		return;
	const CPlayer *pPlayer = pSelf->m_apPlayers[TargetId];

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "~~~ round stats for '%s'", pName);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", aBuf);
	str_format(aBuf, sizeof(aBuf), "~ Kills: %d", pPlayer->m_Stats.m_Kills);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", aBuf);
	str_format(aBuf, sizeof(aBuf), "~ Deaths: %d", pPlayer->m_Stats.m_Deaths);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", aBuf);
	str_format(aBuf, sizeof(aBuf), "~ Current killing spree: %d", pPlayer->Spree());
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", aBuf);
	str_format(aBuf, sizeof(aBuf), "~ Highest killing spree: %d", pPlayer->m_Stats.m_BestSpree);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", aBuf);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp",
		"~ see also /statsall");
}

void CGameContext::ConStatsAllTime(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	const char *pName = pResult->NumArguments() ? pResult->GetString(0) : pSelf->Server()->ClientName(pResult->m_ClientId);
	pSelf->m_pController->m_pSqlStats->ShowStats(pResult->m_ClientId, pName, pSelf->m_pController->StatsTable());
}

void CGameContext::ConRankKills(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	const char *pName = pResult->NumArguments() ? pResult->GetString(0) : pSelf->Server()->ClientName(pResult->m_ClientId);
	pSelf->m_pController->m_pSqlStats->ShowRank(pResult->m_ClientId, pName, "Kills", "kills", pSelf->m_pController->StatsTable(), "DESC");
}

void CGameContext::ConTopKills(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	const char *pName = pSelf->Server()->ClientName(pResult->m_ClientId);
	int Offset = pResult->NumArguments() ? pResult->GetInteger(0) : 1;
	pSelf->m_pController->m_pSqlStats->ShowTop(pResult->m_ClientId, pName, "Kills", "kills", pSelf->m_pController->StatsTable(), "DESC", Offset);
}

#define MACRO_ADD_COLUMN(name, sql_name, sql_type, bind_type, default, merge_method) ;
#define MACRO_RANK_COLUMN(name, sql_name, display_name, order_by) \
	void CGameContext::ConRank##name(IConsole::IResult *pResult, void *pUserData) \
	{ \
		CGameContext *pSelf = (CGameContext *)pUserData; \
		if(!CheckClientId(pResult->m_ClientId)) \
			return; \
\
		if(!pSelf->m_pController) \
			return; \
\
		const char *pName = pResult->NumArguments() ? pResult->GetString(0) : pSelf->Server()->ClientName(pResult->m_ClientId); \
		pSelf->m_pController->m_pSqlStats->ShowRank(pResult->m_ClientId, pName, display_name, #sql_name, pSelf->m_pController->StatsTable(), order_by); \
	}
#define MACRO_TOP_COLUMN(name, sql_name, display_name, order_by) \
	void CGameContext::ConTop##name(IConsole::IResult *pResult, void *pUserData) \
	{ \
		CGameContext *pSelf = (CGameContext *)pUserData; \
		if(!CheckClientId(pResult->m_ClientId)) \
			return; \
\
		if(!pSelf->m_pController) \
			return; \
\
		const char *pName = pSelf->Server()->ClientName(pResult->m_ClientId); \
		int Offset = pResult->NumArguments() ? pResult->GetInteger(0) : 1; \
		pSelf->m_pController->m_pSqlStats->ShowTop(pResult->m_ClientId, pName, display_name, #sql_name, pSelf->m_pController->StatsTable(), order_by, Offset); \
	}
#include <game/server/instagib/sql_colums_all.h>
#undef MACRO_ADD_COLUMN
#undef MACRO_RANK_COLUMN
#undef MACRO_TOP_COLUMN
