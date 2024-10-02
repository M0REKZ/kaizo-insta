#include <base/system.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontroller.h>
#include <game/server/gamemodes/base_pvp/base_pvp.h>
#include <game/server/player.h>

#include <game/server/gamecontext.h>

void CGameContext::RegisterInstagibCommands()
{
	Console()->Chain("sv_scorelimit", ConchainGameinfoUpdate, this);
	Console()->Chain("sv_timelimit", ConchainGameinfoUpdate, this);
	Console()->Chain("sv_grenade_ammo_regen", ConchainResetInstasettingTees, this);
	Console()->Chain("sv_spawn_weapons", ConchainSpawnWeapons, this);
	Console()->Chain("sv_tournament_chat_smart", ConchainSmartChat, this);
	Console()->Chain("sv_tournament_chat", ConchainTournamentChat, this);
	Console()->Chain("sv_zcatch_colors", ConchainZcatchColors, this);
	Console()->Chain("sv_spectator_votes", ConchainSpectatorVotes, this);
	Console()->Chain("sv_spectator_votes_sixup", ConchainSpectatorVotes, this);

	Console()->Register("rank", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConRankCmdlist, this, "Lists available rank commands");
	Console()->Register("top5", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConTopCmdlist, this, "Lists available top commands");
	Console()->Register("top", "", CFGFLAG_CHAT | CFGFLAG_SERVER, ConTopCmdlist, this, "Lists available top commands");

	Console()->Register("stats", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConStatsRound, this, "Shows the current round stats of player name (your stats by default)");

	Console()->Register("statsall", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConStatsAllTime, this, "Shows the all time stats of player name (your stats by default)");
	Console()->Register("stats_all", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConStatsAllTime, this, "Shows the all time stats of player name (your stats by default)");

	Console()->Register("points", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaRankPoints, this, "Shows the all time points rank of player name (your stats by default)");
	Console()->Register("rank_points", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaRankPoints, this, "Shows the all time points rank of player name (your stats by default)");

	Console()->Register("rank_kills", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConRankKills, this, "Shows the all time kills rank of player name (your stats by default)");
	Console()->Register("top5kills", "?i[rank to start with]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConTopKills, this, "Shows the all time best ranks by kills");

	// TODO: what about rank flag times with stat track on vs off? how does the user choose which to show
	//       i think the best is to not let the user choose but show all at once
	//       like ddnet does show regional and global rankings together
	//       the flag ranks could show ranks for the current gametype and for all gametypes and for stat track off/on
	Console()->Register("rank_flags", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConRankFastcaps, this, "Shows the all time flag time rank of player name (your stats by default)");
	Console()->Register("top5flags", "?i[rank to start with]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConTopFastcaps, this, "Shows the all time best ranks by flag time");
	Console()->Register("rank_caps", "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConRankFlagCaptures, this, "Shows the all time flag capture rank of player name (your stats by default)");

#define MACRO_ADD_COLUMN(name, sql_name, sql_type, bind_type, default, merge_method) ;
#define MACRO_RANK_COLUMN(name, sql_name, display_name, order_by) \
	Console()->Register("rank_" #sql_name, "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaRank##name, this, "Shows the all time " #sql_name " rank of player name (your stats by default)");
#define MACRO_TOP_COLUMN(name, sql_name, display_name, order_by) \
	Console()->Register("top5" #sql_name, "?i[rank to start with]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaTop##name, this, "Shows the all time best ranks by " #sql_name);
#include <game/server/instagib/sql_colums_all.h>
#undef MACRO_ADD_COLUMN
#undef MACRO_RANK_COLUMN
#undef MACRO_TOP_COLUMN

#define CONSOLE_COMMAND(name, params, flags, callback, userdata, help) Console()->Register(name, params, flags, callback, userdata, help);
#include <game/server/instagib/rcon_commands.h>
#undef CONSOLE_COMMAND

	// generate callbacks to trigger insta settings update for all instagib configs
	// when one of the insta configs is changed
	// we update the checkboxes [x] in the vote menu
#define MACRO_CONFIG_INT(Name, ScriptName, Def, Min, Max, Flags, Desc) \
	Console()->Chain(#ScriptName, ConchainInstaSettingsUpdate, this);
#define MACRO_CONFIG_COL(Name, ScriptName, Def, Flags, Desc) // only int checkboxes for now
#define MACRO_CONFIG_STR(Name, ScriptName, Len, Def, Flags, Desc) // only int checkboxes for now
#include <engine/shared/variables_insta.h>
#undef MACRO_CONFIG_INT
#undef MACRO_CONFIG_COL
#undef MACRO_CONFIG_STR
	Console()->Chain("sv_gametype", ConchainInstaSettingsUpdate, this);
}

void CGameContext::ConchainInstaSettingsUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->UpdateVoteCheckboxes();
	pSelf->RefreshVotes();
}

void CGameContext::ConchainGameinfoUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
	{
		CGameContext *pSelf = (CGameContext *)pUserData;
		if(pSelf->m_pController)
			pSelf->m_pController->CheckGameInfo();
	}
}

void CGameContext::ConchainResetInstasettingTees(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(pResult->NumArguments())
	{
		for(auto *pPlayer : pSelf->m_apPlayers)
		{
			if(!pPlayer)
				continue;
			CCharacter *pChr = pPlayer->GetCharacter();
			if(!pChr)
				continue;
			pChr->ResetInstaSettings();
		}
	}
}

void CGameContext::ConchainSpawnWeapons(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
	{
		CGameContext *pSelf = (CGameContext *)pUserData;
		if(pSelf->m_pController)
			pSelf->m_pController->UpdateSpawnWeapons();
	}
}

void CGameContext::ConchainSmartChat(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);

	if(!pResult->NumArguments())
		return;

	CGameContext *pSelf = (CGameContext *)pUserData;
	char aBuf[512];
	str_format(
		aBuf,
		sizeof(aBuf),
		"Warning: sv_tournament_chat is currently set to %d you might want to update that too.",
		pSelf->Config()->m_SvTournamentChat);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ddnet-insta", aBuf);
}

void CGameContext::ConchainTournamentChat(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);

	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->Config()->m_SvTournamentChatSmart)
		return;

	pSelf->Console()->Print(
		IConsole::OUTPUT_LEVEL_STANDARD,
		"ddnet-insta",
		"Warning: this variable will be set automatically on round end because sv_tournament_chat_smart is active.");
}

void CGameContext::ConchainZcatchColors(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);

	CGameContext *pSelf = (CGameContext *)pUserData;
	if(pSelf->m_pController)
		pSelf->m_pController->OnUpdateZcatchColorConfig();
}

void CGameContext::ConchainSpectatorVotes(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);

	CGameContext *pSelf = (CGameContext *)pUserData;
	if(pSelf->m_pController)
		pSelf->m_pController->OnUpdateSpectatorVotesConfig();
}
