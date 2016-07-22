#include "stdafx.h"
#include "../common/tables.h"
#include "dungeon_browser_manager.h"
#include "guild.h"
#include "db.h"
#include "desc_client.h"
#include "utils.h"
#include "item.h"
#include "char_manager.h"
#include "item_manager.h"
#include "desc.h"
#include "config.h"
#include "p2p.h"
#include "buffer_manager.h"
#include "packet.h"
#include "event.h"
#include "char.h"


CBrowserTeam::CBrowserTeam(DWORD dwLeaderID)
{
	m_dwLeaderID = dwLeaderID;
}

CBrowserTeam::~CBrowserTeam()
{
	for (std::set<LPCHARACTER>::iterator it = m_set_pkCurrentViewer.begin(); it != m_set_pkCurrentViewer.end(); ++it){
		if ((*it) && (*it)->GetDesc()){

			TPacketBrowserTeamRemove Teamc;
			Teamc.dwLeaderPID = GetDungeonID();
			Teamc.dwPID = (*it)->GetPlayerID();

			__ClientPacket(BROWSER_SUBHEADER_GC_REMOVE, &Teamc, sizeof(TPacketBrowserTeamRemove));

			(*it)->SetBrowser(NULL);
			__RemoveViewer((*it));
		}
	}
}

void CBrowserTeam::Load(BYTE bGameIndex)
{
	sys_log(0, "LoadBrowserTeam: %u", m_dwLeaderID);
	m_bGameIndex = bGameIndex;

}

void CBrowserTeam::AddMember(DWORD dwPlayerID, int iLevel, const char* strName, BYTE bRace)
{
	TMemberBrowserMap::iterator it = m_Member_Map.find(dwPlayerID);
	if (it == m_Member_Map.end())
	{
		//if the PlayerID doesnt exist
		TMemberBrowser TMemberBrowserInfo;
		memset(&TMemberBrowserInfo, 0, sizeof(TMemberBrowserInfo));
		TMemberBrowserInfo.dwPlayerID = dwPlayerID;
		str_lower(strName, TMemberBrowserInfo.strName, sizeof(TMemberBrowserInfo.strName));
		TMemberBrowserInfo.bState = 1;
		TMemberBrowserInfo.bLevel = iLevel;
		TMemberBrowserInfo.bRace = bRace;
		m_Member_Map.insert(std::pair<DWORD, TMemberBrowser>(TMemberBrowserInfo.dwPlayerID, TMemberBrowserInfo));

		TPacketBrowserTeamAdd Teamc;
		Teamc.dwLeaderPID = GetDungeonID();
		Teamc.dwPID = dwPlayerID;
		str_lower(strName, Teamc.strName, sizeof(Teamc.strName));
		Teamc.bLevel = iLevel;
		Teamc.bRace = bRace;
		
		LPCHARACTER pChar = CHARACTER_MANAGER::instance().FindByPID(dwPlayerID);

		if (pChar)
		{
			__AddViewer(pChar);	
			pChar->SetBrowser(this);
			SendBrowserJoinAllToOne(pChar);
		}
		__ClientPacket(BROWSER_SUBHEADER_GC_ADD, &Teamc, sizeof(TPacketBrowserTeamAdd));

	}
	sys_err("player already in the browser team : %u", dwPlayerID);
}

void CBrowserTeam::RemoveMember(DWORD dwPlayerID)
{
	if(dwPlayerID == GetDungeonID())
		return;
	TMemberBrowserMap::iterator it = m_Member_Map.find(dwPlayerID);
	if (it == m_Member_Map.end())
	{
		sys_err("cannot find this player in the browser team : %u", dwPlayerID);
		return;
	}
	m_Member_Map.erase(it);
	TPacketBrowserTeamRemove Teamc;
	Teamc.dwLeaderPID = GetDungeonID();
	Teamc.dwPID = dwPlayerID;

	LPCHARACTER pChar = CHARACTER_MANAGER::instance().FindByPID(dwPlayerID);

	__ClientPacket(BROWSER_SUBHEADER_GC_REMOVE, &Teamc, sizeof(TPacketBrowserTeamRemove));

	if (pChar)
	{
		__RemoveViewer(pChar);
		pChar->SetBrowser(NULL);
	}
}

bool CBrowserTeam::FindPlayer(DWORD dwPlayerID)
{
	sys_log(0, "FindPlayer; %d",m_Member_Map.size());
	TMemberBrowserMap::iterator it = m_Member_Map.find(dwPlayerID);
	if (it == m_Member_Map.end())
	{
		return false;
	}

	return true;

}

void CBrowserTeam::UpdateState(DWORD dwPlayerID, BYTE bState)
{
	TMemberBrowserMap::iterator it = m_Member_Map.find(dwPlayerID);
	if (it == m_Member_Map.end())
	{
		sys_err("Cant find player in browser team");
		return;
	}
	it->second.bState = bState;

	TPacketBrowserTeamUpdate Teamc;
	Teamc.dwLeaderPID = GetDungeonID();
	Teamc.dwPID = it->second.dwPlayerID;
	Teamc.bState = bState;

	__ClientPacket(BROWSER_SUBHEADER_GC_UPDATE, &Teamc, sizeof(TPacketBrowserTeamUpdate));
}

namespace
{
	struct FBrowserChat
	{
		const char* c_pszText;

		FBrowserChat(const char* c_pszText)
			: c_pszText(c_pszText)
		{}

		void operator()(LPCHARACTER ch)
		{
			ch->ChatPacket(CHAT_TYPE_BROWSER, "%s", c_pszText);
		}
	};
}

void CBrowserTeam::P2PChat(const char* c_pszText)
{
	std::for_each(m_set_pkCurrentViewer.begin(), m_set_pkCurrentViewer.end(), FBrowserChat(c_pszText));
}

void CBrowserTeam::Chat(const char* c_pszText)
{
	sys_log(0, "Chat; %s",c_pszText);
	std::for_each(m_set_pkCurrentViewer.begin(), m_set_pkCurrentViewer.end(), FBrowserChat(c_pszText));

	TPacketGGBrowser p1;
	TPacketGGBrowserChat p2;

	p1.bHeader = HEADER_GG_BROWSER_TEAM;
	p1.bSubHeader = BROWSER_SUBHEADER_GG_CHAT;
	p1.dwLeaderID = GetDungeonID();
	strlcpy(p2.szText, c_pszText, sizeof(p2.szText));

	P2P_MANAGER::instance().Send(&p1, sizeof(TPacketGGBrowser));
	P2P_MANAGER::instance().Send(&p2, sizeof(TPacketGGBrowserChat));
}


void CBrowserTeam::SendBrowserJoinAllToOne(LPCHARACTER ch)
{
	sys_log(0, "SendBrowserJoinAllToOne; %d",ch->GetPlayerID());
	if (!ch->GetDesc())
		return;
	
	ch->SetBrowser(this);

	for (TMemberBrowserMap::iterator it = m_Member_Map.begin(); it != m_Member_Map.end(); ++it)
	{
		TPacketBrowserTeamAdd Teamc;
		Teamc.bLevel = it->second.bLevel;
		Teamc.bRace = it->second.bRace;
		Teamc.dwLeaderPID = GetDungeonID();
		Teamc.dwPID = it->second.dwPlayerID;
		strlcpy(Teamc.strName, it->second.strName, sizeof(Teamc.strName));
		__ClientPacket(BROWSER_SUBHEADER_GC_ADD, &Teamc, sizeof(TPacketBrowserTeamAdd),ch);
	}
}

void CBrowserTeam::__ViewerPacket(const void* c_pData, size_t size)
{
	for (std::set<LPCHARACTER>::iterator it = m_set_pkCurrentViewer.begin(); it != m_set_pkCurrentViewer.end(); ++it)
	if (*it != NULL) {
		(*it)->GetDesc()->Packet(c_pData, size);
	}
}

bool CBrowserTeam::__IsViewer(LPCHARACTER ch)
{
	return m_set_pkCurrentViewer.find(ch) != m_set_pkCurrentViewer.end();
}

void CBrowserTeam::__AddViewer(LPCHARACTER ch)
{
	if (!__IsViewer(ch))
		m_set_pkCurrentViewer.insert(ch);
}
void CBrowserTeam::__RemoveViewer(LPCHARACTER ch)
{
	m_set_pkCurrentViewer.erase(ch);
}

void CBrowserTeam::__ClientPacket(BYTE subheader, const void* c_pData, size_t size, LPCHARACTER ch)
{
	TPacketGCBrowserTeam packet;
	packet.header = HEADER_GC_BROWSER_TEAM;
	packet.size = sizeof(TPacketGCBrowserTeam)+size;
	packet.subheader = subheader;

	TEMP_BUFFER buf;
	buf.write(&packet, sizeof(TPacketGCBrowserTeam));
	if (size)
		buf.write(c_pData, size);

	if (ch)
		ch->GetDesc()->Packet(buf.read_peek(), buf.size());
	else
		__ViewerPacket(buf.read_peek(), buf.size());
}


void CBrowserTeam::LoadMember(TMemberBrowser* tMem, DWORD dwSize)
{
	if (m_bMemberLoaded)
		return;
	
	
	sys_log(0, "LoadMember size; %d",dwSize);

	for ( WORD i = 0; i < dwSize; i++)
	{
		sys_log(0, "dwPlayerID; %d",tMem->dwPlayerID);
		sys_log(0, "strName; %d",tMem->strName);
		sys_log(0, "bState; %d",tMem->bState);
		sys_log(0, "bLevel; %d",tMem->bLevel);
		sys_log(0, "bRace; %d",tMem->bRace);
		if (FindPlayer(tMem->dwPlayerID))
			continue;

		TMemberBrowser TMemberBrowserInfo;
		memset(&TMemberBrowserInfo, 0, sizeof(TMemberBrowserInfo));
		TMemberBrowserInfo.dwPlayerID = tMem->dwPlayerID;
		strlcpy(TMemberBrowserInfo.strName, tMem->strName, sizeof(TMemberBrowserInfo.strName));
		TMemberBrowserInfo.bState = tMem->bState;
		TMemberBrowserInfo.bLevel = tMem->bLevel;
		TMemberBrowserInfo.bRace = tMem->bRace;
		m_Member_Map.insert(std::pair<DWORD, TMemberBrowser>(TMemberBrowserInfo.dwPlayerID, TMemberBrowserInfo));
		tMem++;
	}

	m_bMemberLoaded = true;
}


// MANAGER

CDungeonBrowserManager::CDungeonBrowserManager()
{

}

CDungeonBrowserManager::~CDungeonBrowserManager()
{
	Destroy();
}

bool CDungeonBrowserManager::SetBrowser(LPCHARACTER pkChar)
{
	for (itertype(m_map_BrowserTeams) it = m_map_BrowserTeams.begin(); it != m_map_BrowserTeams.end(); ++it){
		if (it->second->FindPlayer(pkChar->GetPlayerID())){
			it->second->SendBrowserJoinAllToOne(pkChar);
			return true;
		}
	}
	return false;
}

void CDungeonBrowserManager::Destroy()
{
	for (itertype(m_map_BrowserTeams) it = m_map_BrowserTeams.begin(); it != m_map_BrowserTeams.end(); ++it)
		delete (*it).second;
	m_map_BrowserTeams.clear();
}

void CDungeonBrowserManager::CreateTeam(LPCHARACTER ch, BYTE bGameIndex)
{
	if (test_server)
		sys_log(0, "CBrowserTeam::CreateTeam %u", ch->GetPlayerID());

	TPacketBrowserTeamCreate Teamc;
	Teamc.bGameIndex = bGameIndex;
	Teamc.bLevel = ch->GetLevel();
	Teamc.bRace = ch->GetRaceNum();
	Teamc.dwLeaderPID = ch->GetPlayerID();
	str_lower(ch->GetName(), Teamc.strName, sizeof(Teamc.strName));

	db_clientdesc->DBPacket(HEADER_GD_BROWSER_TEAM_CREATE, ch->GetDesc()->GetHandle(), &Teamc, sizeof(TPacketBrowserTeamCreate));
}

void CDungeonBrowserManager::LoadMember(DWORD dwLeaderID)
{
	if (test_server)
		sys_log(0, "CBrowserTeam::LoadMember %u", dwLeaderID);

	db_clientdesc->DBPacketHeader(HEADER_GD_BROWSER_TEAM_LOAD, 0, sizeof(DWORD));
	db_clientdesc->Packet(&dwLeaderID, sizeof(DWORD));
}

void CDungeonBrowserManager::AddMember(LPCHARACTER ch, DWORD dwLeaderID)
{
	if (test_server)
		sys_log(0, "CBrowserTeam::AddMember %u", ch->GetPlayerID());

	TPacketBrowserTeamAdd Teamc;
	Teamc.bLevel = ch->GetLevel();
	Teamc.bRace = ch->GetRaceNum();
	Teamc.dwLeaderPID = dwLeaderID;
	Teamc.dwPID = ch->GetPlayerID();
	str_lower(ch->GetName(), Teamc.strName, sizeof(Teamc.strName));

	db_clientdesc->DBPacket(HEADER_GD_BROWSER_TEAM_ADD, ch->GetDesc()->GetHandle(), &Teamc, sizeof(TPacketBrowserTeamAdd));
}

void CDungeonBrowserManager::RemoveMember(LPCHARACTER ch, DWORD dwPlayerID)
{
	if (test_server)
		sys_log(0, "CBrowserTeam::RemoveMember %u", ch->GetPlayerID());

	TPacketBrowserTeamRemove Teamc;
	Teamc.dwLeaderPID = ch->GetPlayerID();
	Teamc.dwPID = dwPlayerID;

	db_clientdesc->DBPacket(HEADER_GD_BROWSER_TEAM_REMOVE, ch->GetDesc()->GetHandle(), &Teamc, sizeof(TPacketBrowserTeamRemove));
}


void CDungeonBrowserManager::DestroyDungeonTeam(DWORD dwLeaderID)
{
	if (test_server)
		sys_log(0, "CBrowserTeam::DestoryDungeonTeam %u", dwLeaderID);

	TPacketBrowserTeamDestroy tMec;
	tMec.dwLeaderPID = dwLeaderID;

	db_clientdesc->DBPacket(HEADER_GD_BROWSER_TEAM_DESTROY, 0, &tMec, sizeof(TPacketBrowserTeamDestroy));
}

void CDungeonBrowserManager::P2PLogin( DWORD dwPlayerID)
{
	if (test_server)
		sys_log(0, "CBrowserTeam::P2PLogin %u", dwPlayerID);

	DWORD dwLeaderID = 0;

	for (itertype(m_map_BrowserTeams) it = m_map_BrowserTeams.begin(); it != m_map_BrowserTeams.end(); ++it){
		if (it->second->FindPlayer(dwPlayerID)){
			dwLeaderID = it->first;
			break;
		}
	}
	if(dwLeaderID != 0)
	{

		TPacketBrowserTeamUpdate tMec;
		tMec.dwLeaderPID = dwLeaderID;
		tMec.dwPID = dwPlayerID;
		tMec.bState = 1;
		
		db_clientdesc->DBPacket(HEADER_GD_BROWSER_TEAM_UPDATE, 0, &tMec, sizeof(TPacketBrowserTeamUpdate));
	}
}

void CDungeonBrowserManager::P2PLogout(DWORD dwPlayerID)
{
	if (test_server)
		sys_log(0, "CBrowserTeam::P2PLogin %u", dwPlayerID);

	DWORD dwLeaderID = 0;

	for (itertype(m_map_BrowserTeams) it = m_map_BrowserTeams.begin(); it != m_map_BrowserTeams.end(); ++it){
		sys_log(0, "CBrowserTeam::m_map_BrowserTeams Packet %u", it->first);
		if (it->second->FindPlayer(dwPlayerID)){
			dwLeaderID = it->first;
			sys_log(0, "CBrowserTeam::P2PLogin Packet %u", dwLeaderID);
			break;
		}
	}
	if(dwLeaderID != 0)
	{
		sys_log(0, "CBrowserTeam::P2PLogin Packet %u", dwLeaderID);

		TPacketBrowserTeamUpdate tMec;
		tMec.dwLeaderPID = dwLeaderID;
		tMec.dwPID = dwPlayerID;
		tMec.bState = 1;
		
		db_clientdesc->DBPacket(HEADER_GD_BROWSER_TEAM_UPDATE, 0, &tMec, sizeof(TPacketBrowserTeamUpdate));
	}
}

CBrowserTeam* CDungeonBrowserManager::GetDungeonTeam(DWORD dwLeaderID)
{
	itertype(m_map_BrowserTeams) it = m_map_BrowserTeams.find(dwLeaderID);
	if (it == m_map_BrowserTeams.end())
		return NULL;
	return it->second;

}

void CDungeonBrowserManager::DB_P2PLogin(DWORD dwLeaderID, DWORD dwPlayerID)
{
	CBrowserTeam* pBrowserTeam;
	if (!(pBrowserTeam = GetDungeonTeam(dwLeaderID)))
		return;

	pBrowserTeam->UpdateState(dwPlayerID, 1);
}
void CDungeonBrowserManager::DB_P2PLogout(DWORD dwLeaderID, DWORD dwPlayerID)
{
	CBrowserTeam* pBrowserTeam;
	if (!(pBrowserTeam = GetDungeonTeam(dwLeaderID)))
		return;

	pBrowserTeam->UpdateState(dwPlayerID, 0);
}

void CDungeonBrowserManager::DB_CreateTeam(DWORD dwLeaderID, BYTE bGameIndex)
{
	CBrowserTeam* pBrowserTeam;
	if ((pBrowserTeam = GetDungeonTeam(dwLeaderID)))
		return;
	
	sys_log(0, "CBrowserTeam::DB_CreateTeam Packet %u:%u", dwLeaderID,bGameIndex);
	
	CBrowserTeam* pBrowserTeam1 = new CBrowserTeam(dwLeaderID);
	pBrowserTeam1->SetGameIndex(bGameIndex);
	sys_log(0, "CBrowserTeam::DB_CreateTeam Packet %u:", pBrowserTeam1->GetDungeonID());
	m_map_BrowserTeams.insert(std::pair<DWORD, CBrowserTeam*>(dwLeaderID, pBrowserTeam1));
	sys_log(0, "CBrowserTeam::DB_CreateTeam Packet1 %u:", GetDungeonTeam(dwLeaderID)->GetDungeonID());
}

void CDungeonBrowserManager::DB_DestroyDungeonTeam(DWORD dwLeaderID)
{
	CBrowserTeam* pBrowserTeam;
	if (!(pBrowserTeam = GetDungeonTeam(dwLeaderID)))
		return;

	delete pBrowserTeam;
	m_map_BrowserTeams.erase(dwLeaderID);
}


