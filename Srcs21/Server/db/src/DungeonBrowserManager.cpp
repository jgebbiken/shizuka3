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

CBrowserTeam::CBrowserTeam()
{
	m_Member_Map.clear();
}

CBrowserTeam::~CBrowserTeam()
{
	m_Member_Map.clear();
}


void CBrowserTeam::ForwardPacket(BYTE bHeader, const void* c_pData, DWORD dwSize, CPeer* pkExceptPeer)
{
	sys_log(0, "CDungeonBrowserManager::ForwardPacket: %u",GetBrowserID());
	for (itertype(m_set_ForwardPeer) it = m_set_ForwardPeer.begin(); it != m_set_ForwardPeer.end(); ++it)
	{
		if (*it == pkExceptPeer)
			continue;

		(*it)->EncodeHeader(bHeader, 0, sizeof(DWORD)+dwSize);
		sys_log(0, "CDungeonBrowserManager::ForwardPacket: %u",GetBrowserID());
		(*it)->EncodeDWORD(GetBrowserID());
		sys_log(0, "CDungeonBrowserManager::ForwardPacket1: %u",GetBrowserID());
		(*it)->Encode(c_pData, dwSize);
	}
}

void CBrowserTeam::ForwardPacketCopyCore(BYTE bHeader, const void* c_pData, DWORD dwSize)
{
	for (itertype(m_set_ForwardPeer) it = m_set_ForwardPeer.begin(); it != m_set_ForwardPeer.end(); ++it)
	{
		if ((*it)->GetChannel() == 99)
		{
			(*it)->EncodeHeader(bHeader, 0, sizeof(DWORD)+dwSize);
			sys_log(0, "CDungeonBrowserManager::ForwardPacketCopyCore: %u", GetBrowserID());
			(*it)->EncodeDWORD(GetBrowserID());
			sys_log(0, "CDungeonBrowserManager::ForwardPacketCopyCore: %u", GetBrowserID());
			(*it)->Encode(c_pData, dwSize);
		
		}
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
			TMemberBrowser tMem;
			MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
			int col = 0;
			str_to_number(tMem.dwPlayerID, row[col++]);
			strlcpy(tMem.strName, row[col++], sizeof(tMem.strName));
			str_to_number(tMem.bState, row[col++]);
			str_to_number(tMem.bLevel, row[col++]);
			str_to_number(tMem.bRace, row[col++]);
		

			if (g_test_server1)
				sys_log(0, "CBrowserTeam::LoadMember %u: LoadPlayer :%u",GetBrowserID(),tMem.dwPlayerID);

			m_Member_Map[tMem.dwPlayerID] = tMem;
			
			sys_log(0, "CBrowserTeam::LoadPlayer Check ID  :%u",m_Member_Map[tMem.dwPlayerID].dwPlayerID);
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
		pkPeer->Encode(&(it->second), sizeof(TMemberBrowser));
		sys_log(0, "CBrowserTeam::Encode : %d",it->second.dwPlayerID);
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
		TMemberBrowser TMemberBrowserInfo;
		TMemberBrowserInfo.dwPlayerID = dwPlayerID;
		strlcpy(TMemberBrowserInfo.strName, strName, sizeof(TMemberBrowserInfo.strName));
		TMemberBrowserInfo.bState = 1;
		TMemberBrowserInfo.bLevel = iLevel;
		TMemberBrowserInfo.bRace = bRace;
		m_Member_Map.insert(std::pair<DWORD, TMemberBrowser>(TMemberBrowserInfo.dwPlayerID, TMemberBrowserInfo));
		
		char szQuery[256];
		snprintf(szQuery, sizeof(szQuery), "INSERT INTO player.browser_team_member (leader_id,player_id,name,state,level,race) VALUES (%u,%u,'%s',%u,%u,%u)", m_dwLeaderID, TMemberBrowserInfo.dwPlayerID, TMemberBrowserInfo.strName, TMemberBrowserInfo.bState, iLevel, TMemberBrowserInfo.bRace);
		CDBManager::Instance().AsyncQuery(szQuery);

		TPacketBrowserTeamAddDB* pTeamD;
		pTeamD->dwPID = dwPlayerID;
		strlcpy(pTeamD->strName, strName, sizeof(pTeamD->strName));
		pTeamD->bLevel = iLevel;
		pTeamD->bRace = bRace;

		ForwardPacket(HEADER_DG_BROWSER_TEAM_ADD, pTeamD, sizeof(TPacketBrowserTeamAddDB));
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
	
	char szDeleteQuery[256];
	snprintf(szDeleteQuery, sizeof(szDeleteQuery), "DELETE FROM player.browser_team_member WHERE player_id = %u", dwPlayerID);
	CDBManager::Instance().AsyncQuery(szDeleteQuery);

	TPacketBrowserTeamRemoveDB* pTeamD;
	pTeamD->dwPID = dwPlayerID;

	ForwardPacket(HEADER_DG_BROWSER_TEAM_REMOVE, pTeamD, sizeof(TPacketBrowserTeamRemoveDB));
}

void CBrowserTeam::UpdateState(DWORD dwPlayerID, BYTE bState)
{
	sys_log(0, "CDungeonBrowserManager::UpdateState: %u", GetBrowserID());
	TMemberBrowserMap::iterator it = m_Member_Map.find(dwPlayerID);
	if (it == m_Member_Map.end())
	{
		sys_err("cannot find this player in the browser team : %u", dwPlayerID);
		return;
	}
	m_Member_Map[dwPlayerID].bState = bState;
	TPacketBrowserTeamUpdateDB* pTeamD;
	pTeamD->dwPID = dwPlayerID;
	pTeamD->bState = bState;
	ForwardPacket(HEADER_DG_BROWSER_TEAM_UPDATE, pTeamD, sizeof(TPacketBrowserTeamUpdateDB));
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

void CDungeonBrowserManager::AddPeerManager(CPeer* pkPeer)
{
	if (m_set_ForwardPeerManager.find(pkPeer) == m_set_ForwardPeerManager.end())
		m_set_ForwardPeerManager.insert(pkPeer);
}


void CDungeonBrowserManager::Initialize()
{
	std::auto_ptr<SQLMsg> pMsg(CDBManager::Instance().DirectQuery("SELECT leader_id, game_index FROM dungeon_browser"));
	uint32_t uiNumRows = pMsg->Get()->uiAffectedRows;
	
	// Dungeon max member
	
	std::auto_ptr<SQLMsg> pMsg1(CDBManager::Instance().DirectQuery("SELECT GameIndex, max_member, map_index FROM dungeons"));
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
			
			long imap_index;
			str_to_number(imap_index, row[col++]);
			
			TDungeoninit pDungeonInit;
			pDungeonInit.dwMaxMember = dwMaxMember;
			pDungeonInit.map_index = imap_index;
			sys_log(0, "CDungeonBrowserManager::Initialize: map_index %u", pDungeonInit.map_index);

			m_map_MaxMember.insert(std::pair<DWORD, TDungeoninit>(dwGameIndex, pDungeonInit));

		}
	}
	

	if (uiNumRows)
	{
		for (int i = 0; i < uiNumRows; i++)
		{
			MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
			int col = 0;

			DWORD dwleaderID;
			str_to_number(dwleaderID, row[col++]);

			int bGameIndex;
			str_to_number(bGameIndex, row[col++]);

			CBrowserTeam* pBrowserTeam = new CBrowserTeam();
			pBrowserTeam->SetBrowserID(dwleaderID);
			pBrowserTeam->SetGameIndex(bGameIndex);
			
			itertype(m_map_MaxMember) itMember = m_map_MaxMember.find(bGameIndex);
				if (itMember == m_map_MaxMember.end())
				{
					sys_err("cannot find this gameindex in the browser : %u", bGameIndex);
					return;
				}	
			pBrowserTeam->SetMaxMember(itMember->second.dwMaxMember);
			pBrowserTeam->SetMapIndex(itMember->second.map_index);
			sys_log(0, "CDungeonBrowserManager::Initialize: map_index %u", itMember->second.map_index);
				
			m_map_BrowserTeams.insert(std::pair<DWORD, CBrowserTeam*>(dwleaderID, pBrowserTeam));

			char szItemQuery[512];
			snprintf(szItemQuery, sizeof(szItemQuery), "SELECT player_id, name, state, level, race FROM player.browser_team_member WHERE leader_id = %u", dwleaderID);
			CDBManager::Instance().ReturnQuery(szItemQuery, QID_BROWSER_MEMBER_LOAD, 0, new DungeonBrowserHandleInfo(dwleaderID));
			
			// log
			//if (g_test_server1)
			sys_log(0, "CDungeonBrowserManager::Initialize: Load Memberlist: %u", dwleaderID);


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
	sys_log(0, "CDungeonBrowserManager::DestroyDungeonTeam1: %u", dwLeaderID);
	char szDeleteQuery[256];
	snprintf(szDeleteQuery, sizeof(szDeleteQuery), "DELETE FROM player.dungeon_browser WHERE leader_id = %u", dwLeaderID);
	CDBManager::Instance().AsyncQuery(szDeleteQuery);
	sys_log(0, "CDungeonBrowserManager::DestroyDungeonTeam2: %u", dwLeaderID);
	char szDeleteQuery1[256];
	snprintf(szDeleteQuery1, sizeof(szDeleteQuery1), "DELETE FROM player.browser_team_member WHERE leader_id = %u", dwLeaderID);
	CDBManager::Instance().AsyncQuery(szDeleteQuery1);
	sys_log(0, "CDungeonBrowserManager::DestroyDungeonTeam3: %u", dwLeaderID);

	m_map_BrowserTeams.erase(dwLeaderID);
	sys_log(0, "CDungeonBrowserManager::DestroyDungeonTeam4: %u", dwLeaderID);
}

void CDungeonBrowserManager::ProcessPacket(CPeer* pkPeer, DWORD dwHandle, BYTE bHeader, const void* c_pData)
{
	if (g_test_server1)
		sys_log(0, "CDungeonBrowserManager::ProcessPacket: %u", bHeader);

	switch (bHeader)
	{
		case HEADER_GD_BROWSER_TEAM_ADD:
			{
				sys_log(0, "CDungeonBrowserManager::HEADER_GD_BROWSER_TEAM_ADD:");
				const TPacketBrowserTeamAdd* pDat = (TPacketBrowserTeamAdd*)c_pData;
				CBrowserTeam* pBrowserTeam = GetDungeonTeam(pDat->dwLeaderPID);
				if (pBrowserTeam)
				{
					bool alreayin = false;
					for (itertype(m_map_BrowserTeams) it = m_map_BrowserTeams.begin(); it != m_map_BrowserTeams.end(); ++it){
						if (it->second->FindPlayer(pDat->dwPID)){
							alreayin = true;
							sys_log(0, "CDungeonBrowserManager::alreayin:");
						}
					}
					
					sys_log(0, "CDungeonBrowserManager::HEADER_GD_BROWSER_TEAM_ADD: %d : %d",pBrowserTeam->GetMaxMember(), pBrowserTeam->GetActualMember());
					if (!alreayin && pBrowserTeam->GetMaxMember() > pBrowserTeam->GetActualMember())
						pBrowserTeam->AddMember(pDat->dwPID, pDat->bLevel, pDat->strName, pDat->bRace);
				}
			}
			break;

		case HEADER_GD_BROWSER_TEAM_LOAD:
			{
				DWORD dwLeaderID = *(DWORD*)c_pData;
				CBrowserTeam* pBrowserTeam = GetDungeonTeam(dwLeaderID);
				
				AddPeerManager(pkPeer);

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
					sys_log(0, "HEADER_GD_BROWSER_TEAM_DESTROY %u", pDat->dwLeaderPID);
					TPacketBrowserTeamDestroy* tTeamD;
					sys_log(0, "HEADER_GD_BROWSER_TEAM_DESTROY1 %u", pDat->dwLeaderPID);
					tTeamD->dwLeaderPID = pDat->dwLeaderPID;
					sys_log(0, "HEADER_GD_BROWSER_TEAM_DESTROY2 %u", pDat->dwLeaderPID);
					pBrowserTeam->ForwardPacket(HEADER_DG_BROWSER_TEAM_DESTROY, tTeamD, sizeof(TPacketBrowserTeamDestroy));
					sys_log(0, "HEADER_GD_BROWSER_TEAM_DESTROY3 %u", pDat->dwLeaderPID);
					DestroyDungeonTeam(pDat->dwLeaderPID);
				}							   
			}
			break;
		case HEADER_GD_BROWSER_TEAM_GOTOCOPY:
			{
				const TPacketBrowserTeamDestroy* pDat = (TPacketBrowserTeamDestroy*)c_pData;
				CBrowserTeam* pBrowserTeam = GetDungeonTeam(pDat->dwLeaderPID);
				if (pBrowserTeam)
				{
					sys_log(0, "CreateBrowserTeam %u", pDat->dwLeaderPID);
					TPacketBrowserTeamDestroy* tTeamD;
					sys_log(0, "CreateBrowserTeam %u", pDat->dwLeaderPID);
					tTeamD->dwLeaderPID = pDat->dwLeaderPID;
					sys_log(0, "CreateBrowserTeam %u", pDat->dwLeaderPID);
					pBrowserTeam->ForwardPacketCopyCore(HEADER_DG_BROWSER_TEAM_GOTOCOPY, tTeamD, sizeof(TPacketBrowserTeamDestroy));
				}
			}
			break;
		case HEADER_GD_BROWSER_TEAM_GIVEWARP:
		{
			const TPacketBrowserTeamGiveWarp* pDat = (TPacketBrowserTeamGiveWarp*)c_pData;
			CBrowserTeam* pBrowserTeam = GetDungeonTeam(pDat->dwLeaderID);
			if (pBrowserTeam)
			{
				TPacketBrowserTeamGiveWarp* tTeamD;
				sys_log(0, "HEADER_GD_BROWSER_TEAM_GIVEWARP %u", pDat->dwLeaderID);
				tTeamD->dwLeaderID = pDat->dwLeaderID;
				sys_log(0, "HEADER_GD_BROWSER_TEAM_GIVEWARP1 %u", pDat->mapx);
				tTeamD->mapx = pDat->mapx;
				sys_log(0, "HEADER_GD_BROWSER_TEAM_GIVEWARP2 %u", pDat->mapy);
				tTeamD->mapy = pDat->mapy;
				sys_log(0, "HEADER_GD_BROWSER_TEAM_GIVEWARP3 %u", pDat->map_index);
				tTeamD->map_index = pDat->map_index;
				sys_log(0, "HEADER_GD_BROWSER_TEAM_GIVEWARP4 %u", pDat->dwLeaderID);
				pBrowserTeam->ForwardPacket(HEADER_DG_BROWSER_TEAM_GIVEWARP, tTeamD, sizeof(TPacketBrowserTeamGiveWarp), pkPeer);
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

				CBrowserTeam* pnewBrowserTeam = new CBrowserTeam();
				sys_log(0, "CreateBrowserTeam %u", pDat->dwLeaderPID);
				pnewBrowserTeam->SetBrowserID(pDat->dwLeaderPID);
				pnewBrowserTeam->SetGameIndex(pDat->bGameIndex);
				pnewBrowserTeam->SetMaxMember(itMember->second.dwMaxMember);
				pnewBrowserTeam->SetMapIndex(itMember->second.map_index);
				for (itertype(m_set_ForwardPeerManager) it = m_set_ForwardPeerManager.begin(); it != m_set_ForwardPeerManager.end(); ++it)
				{	
					pnewBrowserTeam->AddPeer(*it);
				}
				m_map_BrowserTeams.insert(std::pair<DWORD, CBrowserTeam*>(pDat->dwLeaderPID, pnewBrowserTeam));

				char szQuery[256];
				snprintf(szQuery, sizeof(szQuery), "INSERT INTO dungeon_browser (leader_id,game_index) VALUES (%u,%u)", pDat->dwLeaderPID, pDat->bGameIndex);
				CDBManager::Instance().AsyncQuery(szQuery);

				TPacketBrowserTeamCreateDB* pTeamD;
				pTeamD->bGameIndex = pDat->bGameIndex;	
				pTeamD->map_index = itMember->second.map_index;
				

				pnewBrowserTeam->ForwardPacket(HEADER_DG_BROWSER_TEAM_CREATE, pTeamD, sizeof(TPacketBrowserTeamCreateDB));

				pnewBrowserTeam->AddMember(pDat->dwLeaderPID, pDat->bLevel, pDat->strName, pDat->bRace);
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
				sys_log(0, "CreateBrowserTeam %u", pBrowserTeam->GetBrowserID());
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
		init.map_index = it->second->GetMapIndex();
		sys_log(0, "CDungeonBrowserManager::InitBrowserCore: %d",init.map_index);
		pkPeer->Encode(&init, sizeof(TBrowserManagerInitial));		
	}
}


void CDungeonBrowserManager::Update()
{
	return;
}
