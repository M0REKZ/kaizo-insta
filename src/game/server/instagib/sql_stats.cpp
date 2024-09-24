#include <base/system.h>
#include <cstdlib>
#include <engine/server/databases/connection.h>
#include <engine/server/databases/connection_pool.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/instagib/extra_columns.h>
#include <game/server/instagib/sql_stats_player.h>
#include <game/server/player.h>

#include "sql_stats.h"

class IDbConnection;

CInstaSqlResult::CInstaSqlResult()
{
	SetVariant(Variant::DIRECT);
	m_Stats.Reset();
	m_Rank = 0;
}

void CInstaSqlResult::SetVariant(Variant v)
{
	m_MessageKind = v;
	switch(v)
	{
	case RANK:
	case STATS:
	case DIRECT:
	case ALL:
		for(auto &aMessage : m_aaMessages)
			aMessage[0] = 0;
		break;
	case BROADCAST:
		m_aBroadcast[0] = 0;
		break;
	}
}

std::shared_ptr<CInstaSqlResult> CSqlStats::NewInstaSqlResult(int ClientId)
{
	CPlayer *pCurPlayer = GameServer()->m_apPlayers[ClientId];
	if(pCurPlayer->m_StatsQueryResult != nullptr) // TODO: send player a message: "too many requests"
		return nullptr;
	pCurPlayer->m_StatsQueryResult = std::make_shared<CInstaSqlResult>();
	return pCurPlayer->m_StatsQueryResult;
}

// this shares one ratelimit with ddnet based requests such as /rank, /times, /top5team and so on
bool CSqlStats::RateLimitPlayer(int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(pPlayer == 0)
		return true;
	if(pPlayer->m_LastSqlQuery + (int64_t)g_Config.m_SvSqlQueriesDelay * Server()->TickSpeed() >= Server()->Tick())
		return true;
	pPlayer->m_LastSqlQuery = Server()->Tick();
	return false;
}

void CSqlStats::ExecPlayerStatsThread(
	bool (*pFuncPtr)(IDbConnection *, const ISqlData *, char *pError, int ErrorSize),
	const char *pThreadName,
	int ClientId,
	const char *pName,
	const char *pTable)
{
	auto pResult = NewInstaSqlResult(ClientId);
	if(pResult == nullptr)
		return;
	auto Tmp = std::make_unique<CSqlPlayerStatsRequest>(pResult, g_Config.m_SvDebugStats);
	str_copy(Tmp->m_aName, pName, sizeof(Tmp->m_aName));
	str_copy(Tmp->m_aRequestingPlayer, Server()->ClientName(ClientId), sizeof(Tmp->m_aRequestingPlayer));
	str_copy(Tmp->m_aTable, pTable, sizeof(Tmp->m_aTable));

	m_pPool->Execute(pFuncPtr, std::move(Tmp), pThreadName);
}

void CSqlStats::ExecPlayerRankOrTopThread(
	bool (*pFuncPtr)(IDbConnection *, const ISqlData *, char *pError, int ErrorSize),
	const char *pThreadName,
	int ClientId,
	const char *pName,
	const char *pRankColumnDisplay,
	const char *pRankColumnSql,
	const char *pTable,
	const char *pOrderBy,
	int Offset)
{
	auto pResult = NewInstaSqlResult(ClientId);
	if(pResult == nullptr)
		return;
	auto Tmp = std::make_unique<CSqlPlayerStatsRequest>(pResult, g_Config.m_SvDebugStats);
	str_copy(Tmp->m_aName, pName, sizeof(Tmp->m_aName));
	str_copy(Tmp->m_aRequestingPlayer, Server()->ClientName(ClientId), sizeof(Tmp->m_aRequestingPlayer));
	str_copy(Tmp->m_aRankColumnDisplay, pRankColumnDisplay, sizeof(Tmp->m_aRankColumnDisplay));
	str_copy(Tmp->m_aRankColumnSql, pRankColumnSql, sizeof(Tmp->m_aRankColumnSql));
	str_copy(Tmp->m_aTable, pTable, sizeof(Tmp->m_aTable));
	str_copy(Tmp->m_aOrderBy, pOrderBy, sizeof(Tmp->m_aOrderBy));
	Tmp->m_Offset = Offset;

	m_pPool->Execute(pFuncPtr, std::move(Tmp), pThreadName);
}

CSqlStats::CSqlStats(CGameContext *pGameServer, CDbConnectionPool *pPool) :
	m_pPool(pPool),
	m_pGameServer(pGameServer),
	m_pServer(pGameServer->Server())
{
}

void CSqlStats::SetExtraColumns(CExtraColumns *pExtraColumns)
{
	m_pExtraColumns = pExtraColumns;
}

CSqlInstaData::~CSqlInstaData()
{
	if(m_DebugStats > 1)
		dbg_msg("sql-thread", "round stats request destructor called");

	if(m_pExtraColumns)
	{
		if(m_DebugStats > 1)
			dbg_msg("sql-thread", "free memory at %p", m_pExtraColumns);
		free(m_pExtraColumns);
		m_pExtraColumns = nullptr;
	}
}

void CSqlStats::ShowStats(int ClientId, const char *pName, const char *pTable)
{
	if(RateLimitPlayer(ClientId))
		return;
	ExecPlayerStatsThread(ShowStatsWorker, "show stats", ClientId, pName, pTable);
}

void CSqlStats::ShowRank(
	int ClientId,
	const char *pName,
	const char *pRankColumnDisplay,
	const char *pRankColumnSql,
	const char *pTable,
	const char *pOrderBy)
{
	if(RateLimitPlayer(ClientId))
		return;
	ExecPlayerRankOrTopThread(
		ShowRankWorker,
		"show rank",
		ClientId,
		pName,
		pRankColumnDisplay,
		pRankColumnSql,
		pTable,
		pOrderBy,
		0);
}

void CSqlStats::ShowTop(
	int ClientId,
	const char *pName,
	const char *pRankColumnDisplay,
	const char *pRankColumnSql,
	const char *pTable,
	const char *pOrderBy,
	int Offset)
{
	if(RateLimitPlayer(ClientId))
		return;
	ExecPlayerRankOrTopThread(
		ShowTopWorker,
		"show top",
		ClientId,
		pName,
		pRankColumnDisplay,
		pRankColumnSql,
		pTable,
		pOrderBy,
		Offset);
}

void CSqlStats::SaveRoundStats(const char *pName, const char *pTable, CSqlStatsPlayer *pStats)
{
	auto Tmp = std::make_unique<CSqlSaveRoundStatsRequest>(g_Config.m_SvDebugStats);

	Tmp->m_pExtraColumns = (CExtraColumns *)malloc(sizeof(CExtraColumns));
	mem_copy(Tmp->m_pExtraColumns, m_pExtraColumns, sizeof(CExtraColumns));
	if(g_Config.m_SvDebugStats > 1)
		dbg_msg("sql", "allocated memory at %p", Tmp->m_pExtraColumns);

	str_copy(Tmp->m_aName, pName);
	str_copy(Tmp->m_aTable, pTable);
	mem_copy(&Tmp->m_Stats, pStats, sizeof(Tmp->m_Stats));
	m_pPool->ExecuteWrite(SaveRoundStatsThread, std::move(Tmp), "save round stats");
}

bool CSqlStats::ShowStatsWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize)
{
	const auto *pData = dynamic_cast<const CSqlPlayerStatsRequest *>(pGameData);
	auto *pResult = dynamic_cast<CInstaSqlResult *>(pGameData->m_pResult.get());

	char aBuf[4096];
	str_format(
		aBuf,
		sizeof(aBuf),
		"SELECT"
		" points, kills, deaths, spree,"
		" wins, losses, shots_fired, shots_hit %s "
		"FROM %s "
		"WHERE name = ?;",
		!pData->m_pExtraColumns ? "" : pData->m_pExtraColumns->SelectColumns(),
		pData->m_aTable);
	if(pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		dbg_msg("sql-thread", "prepare failed query: %s", aBuf);
		return true;
	}
	pSqlServer->BindString(1, pData->m_aName);
	pSqlServer->Print();

	bool End;
	if(pSqlServer->Step(&End, pError, ErrorSize))
	{
		dbg_msg("sql-thread", "step failed query: %s", aBuf);
		return true;
	}

	if(End)
	{
		pResult->m_MessageKind = CInstaSqlResult::DIRECT;
		str_format(pResult->m_aaMessages[0], sizeof(pResult->m_aaMessages[0]),
			"'%s' is unranked",
			pData->m_aName);
	}
	else
	{
		pResult->m_MessageKind = CInstaSqlResult::STATS;

		str_copy(pResult->m_Info.m_aRequestedPlayer, pData->m_aName, sizeof(pResult->m_Info.m_aRequestedPlayer));

		int Offset = 1;
		pResult->m_Stats.m_Points = pSqlServer->GetInt(Offset++);
		pResult->m_Stats.m_Kills = pSqlServer->GetInt(Offset++);
		pResult->m_Stats.m_Deaths = pSqlServer->GetInt(Offset++);
		pResult->m_Stats.m_BestSpree = pSqlServer->GetInt(Offset++);
		pResult->m_Stats.m_Wins = pSqlServer->GetInt(Offset++);
		pResult->m_Stats.m_Losses = pSqlServer->GetInt(Offset++);
		pResult->m_Stats.m_ShotsFired = pSqlServer->GetInt(Offset++);
		pResult->m_Stats.m_ShotsHit = pSqlServer->GetInt(Offset++);

		if(pData->m_DebugStats > 1)
		{
			dbg_msg("sql-thread", "loaded base stats:");
			pResult->m_Stats.Dump(pData->m_pExtraColumns, "sql-thread");
		}

		CSqlStatsPlayer EmptyStats;
		EmptyStats.Reset();

		// merge into empty stats instead of having a ReadStats() and a ReadAndMergeStats() method
		if(pData->m_pExtraColumns)
			pData->m_pExtraColumns->ReadAndMergeStats(&Offset, pSqlServer, &pResult->m_Stats, &EmptyStats);

		if(pData->m_DebugStats > 1)
		{
			dbg_msg("sql-thread", "loaded gametype specific stats:");
			pResult->m_Stats.Dump(pData->m_pExtraColumns, "sql-thread");
		}
	}
	return false;
}

bool CSqlStats::ShowRankWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize)
{
	const auto *pData = dynamic_cast<const CSqlPlayerStatsRequest *>(pGameData);
	auto *pResult = dynamic_cast<CInstaSqlResult *>(pGameData->m_pResult.get());

	char aBuf[4096];
	str_format(
		aBuf,
		sizeof(aBuf),
		"SELECT %s, rank " // column
		"FROM (SELECT name, %s, RANK() OVER (ORDER BY %s %s) rank FROM %s) sub_table " // column, column, table
		"WHERE name = ?;",
		pData->m_aRankColumnSql,
		pData->m_aRankColumnSql,
		pData->m_aRankColumnSql,
		pData->m_aOrderBy,
		pData->m_aTable);
	if(pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		dbg_msg("sql-thread", "prepare failed query: %s", aBuf);
		return true;
	}
	pSqlServer->BindString(1, pData->m_aName);
	pSqlServer->Print();

	bool End;
	if(pSqlServer->Step(&End, pError, ErrorSize))
	{
		dbg_msg("sql-thread", "step failed query: %s", aBuf);
		return true;
	}

	if(End)
	{
		pResult->m_MessageKind = CInstaSqlResult::DIRECT;
		str_format(pResult->m_aaMessages[0], sizeof(pResult->m_aaMessages[0]),
			"'%s' is unranked",
			pData->m_aName);
	}
	else
	{
		pResult->m_MessageKind = CInstaSqlResult::RANK;

		str_copy(pResult->m_Info.m_aRequestedPlayer, pData->m_aName, sizeof(pResult->m_Info.m_aRequestedPlayer));
		str_copy(pResult->m_aRankColumnDisplay, pData->m_aRankColumnDisplay, sizeof(pResult->m_aRankColumnDisplay));

		pResult->m_RankedScore = pSqlServer->GetInt(1);
		pResult->m_Rank = pSqlServer->GetInt(2);
	}
	return false;
}

bool CSqlStats::ShowTopWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize)
{
	const auto *pData = dynamic_cast<const CSqlPlayerStatsRequest *>(pGameData);
	auto *pResult = dynamic_cast<CInstaSqlResult *>(pGameData->m_pResult.get());
	pResult->m_MessageKind = CInstaSqlResult::DIRECT;

	auto *paMessages = pResult->m_aaMessages;

	int LimitStart = maximum(pData->m_Offset - 1, 0);

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf),
		"SELECT RANK() OVER (ORDER BY a.%s DESC) as ranking, %s, name "
		"FROM ("
		"  SELECT %s, name "
		"  FROM %s "
		"  ORDER BY %s DESC LIMIT ?"
		") as a "
		"ORDER BY ranking ASC, name ASC LIMIT ?, 5",
		pData->m_aRankColumnSql,
		pData->m_aRankColumnSql,
		pData->m_aRankColumnSql,
		pData->m_aTable,
		pData->m_aRankColumnSql);
	if(pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		dbg_msg("sql-thread", "prepare top failed query: %s", aBuf);
		return true;
	}
	pSqlServer->BindInt(1, LimitStart + 5);
	pSqlServer->BindInt(2, LimitStart);

	// show top points
	str_format(paMessages[0], sizeof(paMessages[0]), "-------- Top %s --------", pData->m_aRankColumnDisplay);
	bool End = false;
	int Line = 1;
	while(!pSqlServer->Step(&End, pError, ErrorSize) && !End)
	{
		int Rank = pSqlServer->GetInt(1);
		int Points = pSqlServer->GetInt(2);
		char aName[MAX_NAME_LENGTH];
		pSqlServer->GetString(3, aName, sizeof(aName));
		str_format(paMessages[Line], sizeof(paMessages[Line]),
			"%d. %s %s: %d", Rank, aName, pData->m_aRankColumnDisplay, Points);
		Line++;
	}
	if(!End)
	{
		return true;
	}
	str_copy(paMessages[Line], "-------------------------------", sizeof(paMessages[Line]));

	return false;
}

bool CSqlStats::SaveRoundStatsThread(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize)
{
	if(w != Write::NORMAL)
		return false;

	const CSqlSaveRoundStatsRequest *pData = dynamic_cast<const CSqlSaveRoundStatsRequest *>(pGameData);
	if(pData->m_DebugStats > 1)
	{
		dbg_msg("sql-thread", "writing stats of player '%s'", pData->m_aName);
		dbg_msg("sql-thread", "extra columns %p", pData->m_pExtraColumns);
	}

	char aBuf[4096];
	str_format(
		aBuf,
		sizeof(aBuf),
		"SELECT"
		" name, points, kills, deaths, spree,"
		" wins, losses, shots_fired, shots_hit %s "
		"FROM %s "
		"WHERE name = ?;",
		!pData->m_pExtraColumns ? "" : pData->m_pExtraColumns->SelectColumns(),
		pData->m_aTable);
	if(pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		dbg_msg("sql-thread", "prepare failed query: %s", aBuf);
		return true;
	}
	pSqlServer->BindString(1, pData->m_aName);
	pSqlServer->Print();

	if(pData->m_DebugStats > 1)
		dbg_msg("sql-thread", "select query: %s", aBuf);

	bool End;
	if(pSqlServer->Step(&End, pError, ErrorSize))
	{
		dbg_msg("sql-thread", "step failed");
		return true;
	}
	if(End)
	{
		if(pData->m_DebugStats > 1)
			dbg_msg("sql-thread", "inserting new record ...");
		// the _backup table is not used yet
		// because we guard the write mode at the top
		// to avoid complexity for now
		// but at some point we could write stats to a fallback database
		// and then merge them later
		str_format(
			aBuf,
			sizeof(aBuf),
			"%s INTO %s%s(" // INSERT INTO
			" name,"
			" points, kills, deaths, spree,"
			" wins, losses,"
			" shots_fired, shots_hit  %s"
			") VALUES ("
			" ?,"
			" ?, ?, ?, ?,"
			" ?, ?,"
			" ?, ? %s"
			");",
			pSqlServer->InsertIgnore(),
			pData->m_aTable,
			w == Write::NORMAL ? "" : "_backup",
			!pData->m_pExtraColumns ? "" : pData->m_pExtraColumns->InsertColumns(),
			!pData->m_pExtraColumns ? "" : pData->m_pExtraColumns->InsertValues());

		if(pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		{
			dbg_msg("sql-thread", "prepare insert failed query=%s", aBuf);
			return true;
		}

		if(pData->m_DebugStats > 1)
			dbg_msg("sql-thread", "inserted query: %s", aBuf);

		int Offset = 1;
		pSqlServer->BindString(Offset++, pData->m_aName);
		pSqlServer->BindInt(Offset++, pData->m_Stats.m_Points);
		pSqlServer->BindInt(Offset++, pData->m_Stats.m_Kills);
		pSqlServer->BindInt(Offset++, pData->m_Stats.m_Deaths);
		pSqlServer->BindInt(Offset++, pData->m_Stats.m_BestSpree);
		pSqlServer->BindInt(Offset++, pData->m_Stats.m_Wins);
		pSqlServer->BindInt(Offset++, pData->m_Stats.m_Losses);
		pSqlServer->BindInt(Offset++, pData->m_Stats.m_ShotsFired);
		pSqlServer->BindInt(Offset++, pData->m_Stats.m_ShotsHit);

		if(pData->m_DebugStats > 1)
			dbg_msg("sql-thread", "pre extra offset %d", Offset);

		if(pData->m_pExtraColumns)
			pData->m_pExtraColumns->InsertBindings(&Offset, pSqlServer, &pData->m_Stats);

		if(pData->m_DebugStats > 1)
			dbg_msg("sql-thread", "final offset %d", Offset);

		pSqlServer->Print();

		int NumInserted;
		if(pSqlServer->ExecuteUpdate(&NumInserted, pError, ErrorSize))
		{
			return true;
		}
	}
	else
	{
		if(pData->m_DebugStats > 1)
			dbg_msg("sql-thread", "updating existing record ...");

		CSqlStatsPlayer MergeStats;
		// 1 is name that we don't really need for now
		int Offset = 2;
		MergeStats.m_Points = pSqlServer->GetInt(Offset++);
		MergeStats.m_Kills = pSqlServer->GetInt(Offset++);
		MergeStats.m_Deaths = pSqlServer->GetInt(Offset++);
		MergeStats.m_BestSpree = pSqlServer->GetInt(Offset++);
		MergeStats.m_Wins = pSqlServer->GetInt(Offset++);
		MergeStats.m_Losses = pSqlServer->GetInt(Offset++);
		MergeStats.m_ShotsFired = pSqlServer->GetInt(Offset++);
		MergeStats.m_ShotsHit = pSqlServer->GetInt(Offset++);

		if(pData->m_DebugStats > 1)
		{
			dbg_msg("sql-thread", "loaded stats:");
			MergeStats.Dump(pData->m_pExtraColumns, "sql-thread");
		}

		MergeStats.Merge(&pData->m_Stats);
		if(pData->m_pExtraColumns)
			pData->m_pExtraColumns->ReadAndMergeStats(&Offset, pSqlServer, &MergeStats, &pData->m_Stats);

		if(pData->m_DebugStats > 1)
		{
			dbg_msg("sql-thread", "merged stats:");
			MergeStats.Dump(pData->m_pExtraColumns, "sql-thread");
		}

		str_format(
			aBuf,
			sizeof(aBuf),
			"UPDATE %s%s "
			"SET"
			" points = ?, kills = ?, deaths = ?, spree = ?,"
			" wins = ?, losses = ?,"
			" shots_fired = ?, shots_hit = ? %s"
			"WHERE name = ?;",
			pData->m_aTable,
			w == Write::NORMAL ? "" : "_backup",
			!pData->m_pExtraColumns ? "" : pData->m_pExtraColumns->UpdateColumns());

		if(pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		{
			dbg_msg("sql-thread", "prepare update failed query=%s", aBuf);
			return true;
		}

		Offset = 1;
		pSqlServer->BindInt(Offset++, MergeStats.m_Points);
		pSqlServer->BindInt(Offset++, MergeStats.m_Kills);
		pSqlServer->BindInt(Offset++, MergeStats.m_Deaths);
		pSqlServer->BindInt(Offset++, MergeStats.m_BestSpree);
		pSqlServer->BindInt(Offset++, MergeStats.m_Wins);
		pSqlServer->BindInt(Offset++, MergeStats.m_Losses);
		pSqlServer->BindInt(Offset++, MergeStats.m_ShotsFired);
		pSqlServer->BindInt(Offset++, MergeStats.m_ShotsHit);

		if(pData->m_pExtraColumns)
			pData->m_pExtraColumns->UpdateBindings(&Offset, pSqlServer, &MergeStats);

		pSqlServer->BindString(Offset++, pData->m_aName);

		pSqlServer->Print();

		int NumUpdated;
		if(pSqlServer->ExecuteUpdate(&NumUpdated, pError, ErrorSize))
		{
			return true;
		}

		if(NumUpdated == 0 && pData->m_Stats.HasValues())
		{
			dbg_msg("sql-thread", "update failed no rows changed but got the following stats:");
			pData->m_Stats.Dump(pData->m_pExtraColumns, "sql-thread");
			return true;
		}
		else if(NumUpdated > 1)
		{
			dbg_msg("sql-thread", "affected %d rows when trying to update stats of one player!", NumUpdated);
			dbg_assert(false, "FATAL ERROR: your database is probably corrupted! Time to restore the backup.");
			return true;
		}
	}
	return false;
}

void CSqlStats::CreateTable(const char *pName)
{
	auto Tmp = std::make_unique<CSqlCreateTableRequest>();
	str_copy(Tmp->m_aName, pName);
	Tmp->m_aColumns[0] = '\0';
	if(m_pExtraColumns)
		str_copy(Tmp->m_aColumns, m_pExtraColumns->CreateTable());
	m_pPool->ExecuteWrite(CreateTableThread, std::move(Tmp), "create table");
}

bool CSqlStats::CreateTableThread(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize)
{
	if(w == Write::NORMAL_FAILED)
	{
		if(!MysqlAvailable())
		{
			dbg_msg("sql-thread", "failed to create table! Make sure to compile with MySQL support if you want to use stats");
			return true;
		}

		dbg_assert(false, "CreateTableThread failed to write");
		return true;
	}
	const CSqlCreateTableRequest *pData = dynamic_cast<const CSqlCreateTableRequest *>(pGameData);

	// autoincrement not recommended by sqlite3
	// also its hard to be portable accross mysql and sqlite3
	// ddnet also uses any kind of unicode playername in the points update query

	char aBuf[4096];
	str_format(aBuf, sizeof(aBuf),
		"CREATE TABLE IF NOT EXISTS %s%s("
		"name        VARCHAR(%d)   COLLATE %s  NOT NULL,"
		"first_seen  TIMESTAMP     NOT NULL DEFAULT CURRENT_TIMESTAMP, "
		"points      INTEGER       DEFAULT 0,"
		"kills       INTEGER       DEFAULT 0,"
		"deaths      INTEGER       DEFAULT 0,"
		"spree       INTEGER       DEFAULT 0,"
		"wins        INTEGER       DEFAULT 0,"
		"losses      INTEGER       DEFAULT 0,"
		"shots_fired INTEGER       DEFAULT 0,"
		"shots_hit   INTEGER       DEFAULT 0,"
		"%s"
		"PRIMARY KEY (name)"
		");",
		pData->m_aName,
		w == Write::NORMAL ? "" : "_backup",
		MAX_NAME_LENGTH_SQL,
		pSqlServer->BinaryCollate(),
		pData->m_aColumns);

	if(pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		return true;
	}
	pSqlServer->Print();
	int NumInserted;
	return pSqlServer->ExecuteUpdate(&NumInserted, pError, ErrorSize);
}
