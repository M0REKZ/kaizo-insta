#include <base/system.h>
#include <engine/shared/config.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/instagib/strhelpers.h>
#include <game/server/player.h>

#include <game/server/gamecontroller.h>

static bool AddSepPlayer(char *pBuf, int Size)
{
	int BufLen = str_length(pBuf);
	if(BufLen >= Size - 1)
		return false;

	//                | id  | team | name            | score  | kills  | deaths | ratio  | flag_grabs | flag_captures |
	//                | 128 | blue | ChillerDragon.* | 999999 | 999999 | 999999 | 300.5% | 999999     | 999999        |
	str_append(pBuf, "+-----+------+-----------------+--------+--------+--------+--------+------------+---------------+\n", Size - BufLen);
	return true;
}

static bool AddSep(char *pBuf, int Size, bool TeamPlay)
{
	int BufLen = str_length(pBuf);
	if(BufLen >= Size - 1)
		return false;

	//                | map             | gametype |
	//                | ctf5_solofng    | bolofng  |
	str_append(pBuf, "+-----------------+----------+", Size - BufLen);
	if(TeamPlay)
	{
		//                 red_score | blue_score |
		//                 999999    | 999999     |
		str_append(pBuf, "-----------+------------+", Size - BufLen);
	}
	str_append(pBuf, "\n", Size - BufLen);
	return true;
}

void IGameController::GetRoundEndStatsStrAsciiTable(char *pBuf, size_t SizeOfBuf)
{
	pBuf[0] = '\0';
	char aRow[2048];

	int ScoreRed = m_aTeamscore[TEAM_RED];
	int ScoreBlue = m_aTeamscore[TEAM_BLUE];

	if(!AddSep(pBuf, SizeOfBuf, IsTeamPlay()))
		return;
	str_append(pBuf, "| map             | gametype |", SizeOfBuf);
	if(IsTeamPlay())
		str_append(pBuf, " red_score | blue_score |", SizeOfBuf);
	str_append(pBuf, "\n", SizeOfBuf);
	if(!AddSep(pBuf, SizeOfBuf, IsTeamPlay()))
		return;
	str_format(aRow, sizeof(aRow), "| %-15s | %-8s |", g_Config.m_SvMap, m_pGameType);
	str_append(pBuf, aRow, SizeOfBuf);

	if(IsTeamPlay())
	{
		str_format(aRow, sizeof(aRow), " %-9d | %-10d |", ScoreRed, ScoreBlue);
		str_append(pBuf, aRow, SizeOfBuf);
	}

	str_append(pBuf, "\n", SizeOfBuf);
	if(!AddSep(pBuf, SizeOfBuf, IsTeamPlay()))
		return;
	str_append(pBuf, "\n", SizeOfBuf);

	if(!AddSepPlayer(pBuf, SizeOfBuf))
		return;
	str_append(pBuf, "| id  | team | name            | score  | kills  | deaths | ratio  | flag_grabs | flag_captures |\n", SizeOfBuf);
	if(!AddSepPlayer(pBuf, SizeOfBuf))
		return;

	for(const CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;
		if(pPlayer->GetTeam() < TEAM_RED)
			continue;
		if(pPlayer->GetTeam() > TEAM_BLUE)
			continue;

		char aRatio[16];
		str_format(aRatio, sizeof(aRatio), "%.1f%%", CalcKillDeathRatio(pPlayer->m_Kills, pPlayer->m_Deaths));
		str_format(
			aRow,
			sizeof(aRow),
			"| %-3d | %-4s | %-15s | %-6d | %-6d | %-6d | %-6s | %-10d | %-13d |\n",
			pPlayer->GetCid(),
			pPlayer->GetTeamStr(),
			Server()->ClientName(pPlayer->GetCid()),
			pPlayer->m_Score.value_or(0),
			pPlayer->m_Kills,
			pPlayer->m_Deaths,
			aRatio,
			pPlayer->m_Stats.m_FlagGrabs,
			pPlayer->m_Stats.m_FlagCaptures);

		str_append(pBuf, aRow, SizeOfBuf);
	}

	AddSepPlayer(pBuf, SizeOfBuf);
}
