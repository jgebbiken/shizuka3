#include "stdafx.h"
#include "DungeonBrowserManager.h"
#include "DBManager.h"
#include "QID.h"
#include "Peer.h"
#include "ClientManager.h"
#include "Cache.h"
#include <stdint.h>

extern BOOL g_test_server;
bool g_test_server1 = true;

CBrowserTeam::CBrowserTeam(DWORD dwLeaderID)
{
	m_dwLeaderID = dwLeaderID;
	m_Member_Map.clear();
}

CBrowserTeam::~CBrowserTeam()
{
	for (itertype(m_Member_Map) it = m_Member_Map.begin(); it != m_Member_Map.end(); ++it)
		delete (*it).second;
	m_Member_Map.clear();
}


void CBrowserTeam::ForwardPacket(BYTE bHeader, const void* c_pData, DWORD dwSize)
{
	for (itertype(m_set_ForwardPeer) it = m_set_ForwardPeer.begin(); it != m_set_ForwardPeer.end(); ++it)
	{
		(*it)->EncodeHeader(bHeader, 0, sizeof(DWORD)+dwSize);
		(*it)->EncodeDWORD(GetBrowserID());
		(*it)->Encode(c_pData, dwSize);
	}
}

void CBrowserTeam::LoadMember(SQLMsg* pMsg)
{
	int iNumRows = pMsg->Get()->uiNumRows;
	sys_log(0, "CDungeonBrowserManager::LoadMember: %d",iNumRows);

	if (iNumRows)
	{
		for (int i = 0; i < iNumRows; ++i)
		{
			TMemberBrowser* tMem = new TMemberBrowser;
			MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
			int col = 0;
			str_to_number(tMem->dwPlayerID, row[col++]);
			strlcpy(tMem->strName, row[col++], sizeof(tMem->strName));
			str_to_number(tMem->bState, row[col++]);
			str_to_number(tMem->bLevel, row[col++]);
			str_to_number(tMem->bRace, row[col++]);
		

			if (g_test_server1)
				sys_log(0, "CBrowserTeam::LoadMember %u: LoadPlayer :%u",GetBrowserID(),tMem->dwPlayerID);

			m_Member_Map[tMem->dwPlayerID] = tMem;
			
			sys_log(0, "CBrowserTeam::LoadPlayer Check ID  :%u",m_Member_Map[tMem->dwPlayerID]->dwPlayerID);
		}
		
	}
}

void CBrowserTeam::LoadMember(CPeer* pkPeer, DWORD dwHandle)
{
	if (m_set_ForwardPeer.find(pkPeer) != m_set_ForwardPeer.end())
	{
		sys_err("already loaded items for channel %u %s", pkPeer->GetChannel(), pkPeer->GetHost());

		return;
	}
	pkPeer->EncodeHeader(HEADER_DG_BROWSER_TEAM_LOADMEMBER, dwHandle, sizeof(DWORD) + sizeof(WORD) + sizeof(TMemberBrowser) * m_Member_Map.size());
	pkPeer->EncodeDWORD(GetBrowserID());
	pkPeer->EncodeWORD(m_Member_Map.size());	
	
	for (TMemberBrowserMap::iterator it = m_Member_Map.begin(); it != m_Member_Map.end(); ++it)
	{
		pkPeer->Encode((it)->second, sizeof(TMemberBrowser));
		sys_log(0, "CBrowserTeam::Encode : %d",it->second->dwPlayerID);
	}

	AddPeer(pkPeer);

}


void CBrowserTeam::AddMember(DWORD dwPlayerID, int iLevel, const char * strName, BYTE bRace)
{
	sys_log(0, "CDungeonBrowserManager::AddMember:%d",dwPlayerID);
	TMemberBrowserMap::iterator it = m_Member_Map.find(dwPlayerID);
	if (it == m_Member_Map.end())
	{
		//if the PlayerID doesnt exist
		TMemberBrowser* TMemberBrowserInfo = new TMemberBrowser;

		TMemberBrowserInfo->dwPlayerID = dwPlayerID;
		strlcpy(TMemberBrowserInfo->strName, strName, sizeof(TMemberBrowserInfo->strName));
		TMemberBrowserInfo->bState = 1;
		TMemberBrowserInfo->bLevel = iLevel;
		TMemberBrowserInfo->bRace = bRace;
		m_Member_Map.insert(std::pair<DWORD, TMemberBrowser*>(TMemberBrowserInfo->dwPlayerID, TMemberBrowserInfo));

		char szDeleteQuery[256];
		snprintf(szDeleteQuery, sizeof(szDeleteQuery), "DELETE FROM player.browser_team_member WHERE player_id = %u", TMemberBrowserInfo->dwPlayerID);
		CDBManager::Instance().AsyncQuery(szDeleteQuery);

		char szQuery[256];
		snprintf(szQuery, sizeof(szQuery), "INSERT INTO browser_team_member (leader_id,player_id,name,state,level,race) VALUES (%u,%u,%s,%u,%u)", m_dwLeaderID, TMemberBrowserInfo->dwPlayerID, TMemberBrowserInfo->strName, TMemberBrowserInfo->bState, TMemberBrowserInfo->bRace);
		CDBManager::Instance().AsyncQuery(szQuery);


		TPacketBrowserTeamAdd* pTeamD;
		pTeamD->dwLeaderPID = GetBrowserID();
		pTeamD->dwPID = dwPlayerID;
		strlcpy(pTeamD->strName, strName, sizeof(pTeamD->strName));
		pTeamD->bLevel = iLevel;
		pTeamD->bRace = bRace;
		

		ForwardPacket(HEADER_DG_BROWSER_TEAM_ADD, pTeamD, sizeof(TPacketBrowserTeamAdd));
	}
	else
		sys_err("player already in the browser team : %u", dwPlayerID);
}

bool CBrowserTeam::FindPlayer(DWORD dwPlayerID)
{
	TMemberBrowserMap::iterator it = m_Member_Map.find(dwPlayerID);
	if (it == m_Member_Map.end())
	{
		return false;
	}

	return true;

}

void CBrowserTeam::RemoveMember(DWORD dwPlayerID)
{
	TMemberBrowserMap::iterator it = m_Member_Map.find(dwPlayerID);
	if (it == m_Member_Map.end())
	{
		sys_err("cannot find this player in the browser team : %u", dwPlayerID);
		return;
	}
	m_Member_Map.erase(it);
	delete (*it).second;
	
	char szDeleteQuery[256];
	snprintf(szDeleteQuery, sizeof(szDeleteQuery), "DELETE FROM player.browser_team_member WHERE player_id = %u", dwPlayerID);
	CDBManager::Instance().AsyncQuery(szDeleteQuery);

	TPacketBrowserTeamRemove* pTeamD;
	pTeamD->dwPID = dwPlayerID;
	pTeamD->dwLeaderPID = GetBrowserID();

	ForwardPacket(HEADER_DG_BROWSER_TEAM_REMOVE, pTeamD, sizeof(TPacketBrowserTeamRemove));
}

void CBrowserTeam::UpdateState(DWORD dwPlayerID, BYTE bState)
{
	sys_log(0, "CDungeonBrowserManager::UpdateState:1");
	TMemberBrowserMap::iterator it = m_Member_Map.find(dwPlayerID);
	if (it == m_Member_Map.end())
	{
		sys_err("cannot find this player in the browser team : %u", dwPlayerID);
		return;
	}
	it->second->bState = bState;
	TPacketBrowserTeamUpdate* pTeamD;
	pTeamD->dwPID = dwPlayerID;
	pTeamD->dwLeaderPID = GetBrowserID();
	pTeamD->bState = bState;
	sys_log(0, "CDungeonBrowserManager::UpdateState:6");
	ForwardPacket(HEADER_DG_BROWSER_TEAM_UPDATE, pTeamD, sizeof(TPacketBrowserTeamUpdate));
}

void CBrowserTeam::AddPeer(CPeer* pkPeer)
{
	if (m_set_ForwardPeer.find(pkPeer) == m_set_ForwardPeer.end())
		m_set_ForwardPeer.insert(pkPeer);
}


//Manager

CDungeonBrowserManager::CDungeonBrowserManager()
{
	m_dwLastFlushBrowserTeam = time(0);
}
CDungeonBrowserManager::~CDungeonBrowserManager()
{
	for (itertype(m_map_BrowserTeams) it = m_map_BrowserTeams.begin(); it != m_map_BrowserTeams.end(); ++it)
		delete (*it).second;
	m_map_BrowserTeams.clear();
}

void CDungeonBrowserManager::Initialize()
{
	std::auto_ptr<SQLMsg> pMsg(CDBManager::Instance().DirectQuery("SELECT leader_id, game_index FROM dungeon_browser"));
	uint32_t uiNumRows = pMsg->Get()->uiAffectedRows;

	if (uiNumRows)
	{
		for (int i = 0; i < uiNumRows; i++)
		{
			MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
			int col = 0;

			DWORD dwleaderID;
			str_to_number(dwleaderID, row[col++]);

			BYTE bGameIndex;
			str_to_number(bGameIndex, row[col++]);

			CBrowserTeam* pBrowserTeam = new CBrowserTeam(dwleaderID);
			pBrowserTeam->SetGameIndex(bGameIndex);
			m_map_BrowserTeams.insert(std::pair<DWORD, CBrowserTeam*>(dwleaderID, pBrowserTeam));

			char szItemQuery[512];
			snprintf(szItemQuery, sizeof(szItemQuery), "SELECT player_id, name, state, level, race FROM player.browser_team_member WHERE leader_id = %u", dwleaderID);
			CDBManager::Instance().ReturnQuery(szItemQuery, QID_BROWSER_MEMBER_LOAD, 0, new DungeonBrowserHandleInfo(dwleaderID));
			
			// log
			//if (g_test_server1)
			sys_log(0, "CDungeonBrowserManager::Initialize: Load Memberlist: %u", dwleaderID);


		}
	}
	
	// Dungeon max member
	
	std::auto_ptr<SQLMsg> pMsg1(CDBManager::Instance().DirectQuery("SELECT GameIndex, max_member FROM dungeons"));
	uint32_t uiNumRows1 = pMsg1->Get()->uiAffectedRows;

	if (uiNumRows1)
	{
		for (int i = 0; i < uiNumRows1; i++)
		{
			MYSQL_ROW row = mysql_fetch_row(pMsg1->Get()->pSQLResult);
			int col = 0;

			DWORD dwGameIndex;
			str_to_number(dwGameIndex, row[col++]);

			DWORD dwMaxMember;
			str_to_number(dwMaxMember, row[col++]);

			m_map_MaxMember.insert(std::pair<DWORD, DWORD>(dwGameIndex, dwMaxMember));

		}
	}
	
}

void CDungeonBrowserManager::QueryResult(CPeer* pkPeer, SQLMsg* pMsg, int iQIDNum)
{
	std::auto_ptr<DungeonBrowserHandleInfo> pInfo((DungeonBrowserHandleInfo*)((CQueryInfo*)pMsg->pvUserData)->pvData);
	CBrowserTeam* pDungeonTeam = GetDungeonTeam(pInfo->dwLeaderID);
	if (!pDungeonTeam)
	{
		sys_err("team of the dungeon browser does not exist with this id : %u", pInfo->dwLeaderID);
		return;
	}

	switch (iQIDNum)
	{
		case QID_BROWSER_MEMBER_LOAD:
			pDungeonTeam->LoadMember(pMsg);
			break;
		default:
			sys_err("unknown qid browser: %u", iQIDNum);
			break;
	}
}



void CDungeonBrowserManager::Destroy()
{
	return;
}

CBrowserTeam* CDungeonBrowserManager::GetDungeonTeam(DWORD dwLeaderID)
{
	itertype(m_map_BrowserTeams) it = m_map_BrowserTeams.find(dwLeaderID);
	if (it == m_map_BrowserTeams.end())
		return NULL;
	sys_log(0, "CDungeonBrowserManager::GetDungeonTeam: %u", it->second->GetBrowserID());
	return it->second;

}

void CDungeonBrowserManager::DestroyDungeonTeam(DWORD dwLeaderID)
{
	char szDeleteQuery[256];
	snprintf(szDeleteQuery, sizeof(szDeleteQuery), "DELETE FROM player.dungeon_browser WHERE leader_id = %u", dwLeaderID);
	CDBManager::Instance().AsyncQuery(szDeleteQuery);

	char szDeleteQuery1[256];
	snprintf(szDeleteQuery1, sizeof(szDeleteQuery1), "DELETE FROM player.browser_team_member WHERE leader_id = %u", dwLeaderID);
	CDBManager::Instance().AsyncQuery(szDeleteQuery1);

	m_map_BrowserTeams.erase(dwLeaderID);
}

void CDungeonBrowserManager::ProcessPacket(CPeer* pkPeer, DWORD dwHandle, BYTE bHeader, const void* c_pData)
{
	if (g_test_server1)
		sys_log(0, "CDungeonBrowserManager::ProcessPacket: %u", bHeader);

	switch (bHeader)
	{
		case HEADER_GD_BROWSER_TEAM_ADD:
			{
				const TPacketBrowserTeamAdd* pDat = (TPacketBrowserTeamAdd*)c_pData;
				CBrowserTeam* pBrowserTeam = GetDungeonTeam(pDat->dwLeaderPID);
				if (pBrowserTeam)
				{
					bool alreayin = false;
					for (itertype(m_map_BrowserTeams) it = m_map_BrowserTeams.begin(); it != m_map_BrowserTeams.end(); ++it){
						if (it->second->FindPlayer(pDat->dwPID))
							alreayin = true;
					}
					if (!alreayin && pBrowserTeam->GetMaxMember() > pBrowserTeam->GetActualMember())
						pBrowserTeam->AddMember(pDat->dwPID, pDat->bLevel, pDat->strName, pDat->bRace);
				}
			}
			break;

		case HEADER_GD_BROWSER_TEAM_LOAD:
			{
				DWORD dwLeaderID = *(DWORD*)c_pData;
				CBrowserTeam* pBrowserTeam = GetDungeonTeam(dwLeaderID);

				if (pBrowserTeam)
				{
					pBrowserTeam->LoadMember(pkPeer);
				}
			}
			break;

		case HEADER_GD_BROWSER_TEAM_REMOVE:
			{
				const TPacketBrowserTeamRemove* pDat = (TPacketBrowserTeamRemove*)c_pData;
				CBrowserTeam* pBrowserTeam = GetDungeonTeam(pDat->dwLeaderPID);
				if (pBrowserTeam)
				{
					pBrowserTeam->RemoveMember(pDat->dwPID);
				}
			}
			break;
		case HEADER_GD_BROWSER_TEAM_DESTROY:
			{
				const TPacketBrowserTeamDestroy* pDat = (TPacketBrowserTeamDestroy*)c_pData;
				CBrowserTeam* pBrowserTeam = GetDungeonTeam(pDat->dwLeaderPID);
				if (pBrowserTeam)
				{
					TPacketBrowserTeamDestroy* tTeamD;
					tTeamD->dwLeaderPID = pDat->dwLeaderPID;
					pBrowserTeam->ForwardPacket(HEADER_DG_BROWSER_TEAM_DESTROY, tTeamD, sizeof(TPacketBrowserTeamDestroy));
					DestroyDungeonTeam(pDat->dwLeaderPID);
				}							   
			}
			break;
		case HEADER_GD_BROWSER_TEAM_CREATE:
		{
			const TPacketBrowserTeamCreate* pDat = (TPacketBrowserTeamCreate*)c_pData;
			CBrowserTeam* pBrowserTeam = GetDungeonTeam(pDat->dwLeaderPID);
			bool alreayin = false;
					for (itertype(m_map_BrowserTeams) it = m_map_BrowserTeams.begin(); it != m_map_BrowserTeams.end(); ++it){
						if (it->second->FindPlayer(pDat->dwLeaderPID))
							alreayin = true;
					}
			if (!pBrowserTeam && !alreayin)
			{
				if (g_test_server1)
					sys_log(0, "CreateBrowserTeam %u", pDat->dwLeaderPID);
				
				itertype(m_map_MaxMember) itMember = m_map_MaxMember.find(pDat->bGameIndex);
				if (itMember == m_map_MaxMember.end())
				{
					sys_err("cannot find this gameindex in the browser : %u", pDat->bGameIndex);
					return;
				}		

				pBrowserTeam = new CBrowserTeam(pDat->dwLeaderPID);
				m_map_BrowserTeams.insert(std::pair<DWORD, CBrowserTeam*>(pDat->dwLeaderPID, pBrowserTeam));

				char szQuery[256];
				snprintf(szQuery, sizeof(szQuery), "INSERT INTO dungeon_browser (leader_id,game_index) VALUES (%u,%u)", pDat->dwLeaderPID, pDat->bGameIndex);
				CDBManager::Instance().AsyncQuery(szQuery);

				TPacketBrowserTeamCreate* pTeamD;
				pTeamD->dwLeaderPID = pDat->dwLeaderPID;
				pTeamD->bLevel = pDat->bLevel;
				pTeamD->bRace = pDat->bRace;
				strlcpy(pTeamD->strName, pDat->strName, sizeof(pTeamD->strName));
				pTeamD->bGameIndex = pDat->bGameIndex;		
				

				pBrowserTeam->ForwardPacket(HEADER_DG_BROWSER_TEAM_CREATE, pTeamD, sizeof(TPacketBrowserTeamCreate));

				pBrowserTeam->AddMember(pDat->dwLeaderPID, pDat->bLevel, pDat->strName, pDat->bRace);
				pBrowserTeam->SetGameIndex(pDat->bGameIndex);
				pBrowserTeam->SetMaxMember(itMember->second);
			}
			else{
				sys_err("browser team already in the game");
			}
											  
		}
			break;
		
		case HEADER_GD_BROWSER_TEAM_UPDATE:
		{
			const TPacketBrowserTeamUpdate* pDat = (TPacketBrowserTeamUpdate*)c_pData;
			CBrowserTeam* pBrowserTeam = GetDungeonTeam(pDat->dwLeaderPID);
			if (pBrowserTeam)
			{
				pBrowserTeam->UpdateState(pDat->dwPID, pDat->bState);
			}
		}
			break;
	}
}

void CDungeonBrowserManager::InitBrowserCore(CPeer* pkPeer)
{
	for (itertype(m_map_BrowserTeams) it = m_map_BrowserTeams.begin(); it != m_map_BrowserTeams.end(); ++it)
	{
		TBrowserManagerInitial init;
		init.dwLeaderID = it->second->GetBrowserID();
		init.bGameIndex = it->second->GetGameIndex();
		sys_log(0, "CDungeonBrowserManager::InitBrowserCore: %d",init.dwLeaderID);
		pkPeer->Encode(&init, sizeof(TBrowserManagerInitial));		
	}
}


void CDungeonBrowserManager::Update()
{
	return;
}
