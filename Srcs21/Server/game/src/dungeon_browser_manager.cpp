#include "stdafx.h"
#include "../common/tables.h"
#include "dungeon_browser_manager.h"
#include "dungeon.h"
#include "guild.h"
#include "db.h"
#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
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
#include "party.h"
#include "char.h"


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


EVENTINFO(DungeonBrowserEventInfo)
{
	DynamicCharacterPtr ch;
	int         	left_second;
	long			map_index;
	long			m_x;
	long			m_y;

	DungeonBrowserEventInfo()
		: ch()
		, left_second(0)
		, map_index(0)
		, m_x(0)
		, m_y(0)
	{
	}
};

EVENTINFO(DungeonBrowserCreatePartyEventInfo)
{
	TMemberBrowserMap	m_Member_Map;

	DWORD		dwLeaderID;
	CDungeon::IdType dungeon_id;

	DungeonBrowserCreatePartyEventInfo()
		: m_Member_Map()
		, dwLeaderID(0)
		, dungeon_id(0)
	{
	}
};

EVENTFUNC(dungeonbrowsercreateparty_event)
{
	sys_log(0, "dungeonbrowsercreateparty_event1:");
	
	DungeonBrowserCreatePartyEventInfo * info = dynamic_cast<DungeonBrowserCreatePartyEventInfo*>(event->info);
	sys_log(0, "dungeonbrowsercreateparty_event2:");
	if (info == NULL)
	{
		sys_err("dungeonbrowsercreateparty_event> <Factor> Null pointer");
		return 0;
	}
	sys_log(0, "dungeonbrowsercreateparty_event3:");
	LPDUNGEON pDungeon = CDungeonManager::instance().Find(info->dungeon_id);
	if (pDungeon == NULL) {
		return 0;
	}
	sys_log(0, "dungeonbrowsercreateparty_event4:");

	CBrowserTeam* browser = CDungeonBrowserManager::Instance().GetDungeonTeam(info->dwLeaderID);

	browser->mpk_Event_createparty = NULL;
	
	sys_log(0, "dungeonbrowsercreateparty_event5:");

	LPCHARACTER pChar = CHARACTER_MANAGER::instance().FindByPID(info->dwLeaderID);

	if (!pChar)
		return 0;
	sys_log(0, "dungeonbrowsercreateparty_event6:");
	
	if (pChar->GetParty())
	{
	
		LPPARTY pPartyOldAdmin = pChar->GetParty();

		if (pPartyOldAdmin->GetMemberCount() == 2)
		{
			// party disband
			CPartyManager::instance().DeleteParty(pPartyOldAdmin);
		}
		else
		{
			//pParty->SendPartyRemoveOneToAll(ch);
			pPartyOldAdmin->Quit(pChar->GetPlayerID());
			//pParty->SendPartyRemoveAllToOne(ch);
		}
	}
	sys_log(0, "dungeonbrowsercreateparty_event7:");


	LPPARTY pParty = CPartyManager::instance().CreateParty(pChar);

	for (TMemberBrowserMap::iterator it = info->m_Member_Map.begin() ; it != info->m_Member_Map.end(); ++it)
	{
		if (it->second.dwPlayerID != info->dwLeaderID)
		{
			LPCHARACTER pIt = CHARACTER_MANAGER::instance().FindByPID(it->second.dwPlayerID);

			if (!pIt)
				continue;
			
			
			if(pIt->GetParty())	
			{

				LPPARTY pPartyOld = pIt->GetParty();

				if (pPartyOld->GetMemberCount() == 2)
				{
					// party disband
					CPartyManager::instance().DeleteParty(pPartyOld);
				}
				else
				{
					//pParty->SendPartyRemoveOneToAll(ch);
					pPartyOld->Quit(pIt->GetPlayerID());
					//pParty->SendPartyRemoveAllToOne(ch);
				}
			}

			pParty->Join(pIt->GetPlayerID());
			pParty->Link(pIt);
			pParty->SendPartyInfoAllToOne(pIt);
		}

	}
	
	sys_log(0, "dungeonbrowsercreateparty_event9:");

	pDungeon->JoinDungeonParty(pParty);

	return 0;

}

EVENTFUNC(dungeonbrowser_event)
{
	DungeonBrowserEventInfo * info = dynamic_cast<DungeonBrowserEventInfo*>(event->info);

	if (info == NULL)
	{
		sys_err("dungeonbrowser_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}
	//LPDESC d = ch->GetDesc();

	if (info->left_second <= 0)
	{
		ch->m_pkDungeonBrowserEvent = NULL;
		
		sys_log(0, "DungeonBrowserEventInfo:%d,%d,%d",info->map_index,info->m_x,info->m_y);
		ch->WarpSet(info->m_x, info->m_y, info->map_index);

		// go to dungeon
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "In %d Sekunden wirst in in den Dungeon geportet!", info->left_second);
		--info->left_second;
	}

	return PASSES_PER_SEC(1);

}

namespace
{
struct FBrowserJoin
	{
		void operator()(LPCHARACTER ch)
		{
			if (!ch)
				return;

			DungeonBrowserEventInfo* info = AllocEventInfo<DungeonBrowserEventInfo>();

			info->ch = ch;
			info->left_second = 10;

			ch->m_pkDungeonBrowserEvent = event_create(dungeonbrowser_event, info, 1);
		}

	};
}

void CBrowserTeam::CreateDungeon()
{
	if (g_bChannel == 99)
	{
		LPDUNGEON pDungeon = CDungeonManager::instance().Create(GetMapIndex());
		if (!pDungeon)
		{
			sys_err("cannot create dungeon %d", GetMapIndex());
			return;
		}
		pDungeon->SetDungeonTeam();
		sys_log(0, "CreateDungeon:");

		LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(pDungeon->GetMapIndex());
		long m_x = pkSectreeMap->m_setting.posSpawn.x;
		long m_y = pkSectreeMap->m_setting.posSpawn.y;
		sys_log(0, "CreateDungeon1:");

		TPacketBrowserTeamGiveWarp Teamc;

		Teamc.dwLeaderID = GetDungeonID();
		Teamc.mapx = m_x;
		Teamc.mapy = m_y;
		Teamc.map_index = pDungeon->GetMapIndex();
		db_clientdesc->DBPacket(HEADER_GD_BROWSER_TEAM_GIVEWARP, 0, &Teamc, sizeof(TPacketBrowserTeamGiveWarp));

		JoinAll(Teamc.map_index, Teamc.mapx, Teamc.mapy);

		DungeonBrowserCreatePartyEventInfo* info = AllocEventInfo<DungeonBrowserCreatePartyEventInfo>();

		info->dwLeaderID = GetDungeonID();
		info->m_Member_Map = m_Member_Map;
		info->dungeon_id = pDungeon->GetId();

		mpk_Event_createparty = event_create(dungeonbrowsercreateparty_event, info, PASSES_PER_SEC(25));
		
	}
	else 
	{
		sys_log(0, "CreateDungeon3:");

		TPacketBrowserTeamDestroy Teamc;
		Teamc.dwLeaderPID = GetDungeonID();

		db_clientdesc->DBPacket(HEADER_GD_BROWSER_TEAM_GOTOCOPY, 0, &Teamc, sizeof(TPacketBrowserTeamDestroy));
	}

}

void CBrowserTeam::CreateDungeonP2P()
{
	if (g_bChannel == 99)
	{
		sys_log(0, "CreateDungeonP2P5:");
		LPDUNGEON pDungeon = CDungeonManager::instance().Create(GetMapIndex());
		if (!pDungeon)
		{
			sys_err("cannot create dungeon %d", GetMapIndex());
			return;
		}
		pDungeon->SetDungeonTeam();
		sys_log(0, "CreateDungeonP2P6:");
		LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(pDungeon->GetMapIndex());
		long m_x = pkSectreeMap->m_setting.posSpawn.x;
		long m_y = pkSectreeMap->m_setting.posSpawn.y;
		sys_log(0, "CreateDungeonP2P7:");

		TPacketBrowserTeamGiveWarp Teamc;

		Teamc.dwLeaderID = GetDungeonID();
		Teamc.mapx = m_x;
		Teamc.mapy = m_y;
		Teamc.map_index = pDungeon->GetMapIndex();
		db_clientdesc->DBPacket(HEADER_GD_BROWSER_TEAM_GIVEWARP, 0, &Teamc, sizeof(TPacketBrowserTeamGiveWarp));


		JoinAll(Teamc.map_index, Teamc.mapx, Teamc.mapy);

		DungeonBrowserCreatePartyEventInfo* info = AllocEventInfo<DungeonBrowserCreatePartyEventInfo>();

		info->dwLeaderID = GetDungeonID();
		info->m_Member_Map = m_Member_Map;		
		info->dungeon_id = pDungeon->GetId();

		mpk_Event_createparty = event_create(dungeonbrowsercreateparty_event, info, PASSES_PER_SEC(25));

	}
}



void CBrowserTeam::JoinAll(long mapindex, long mapx, long mapy)
{
	sys_log(0, "JoinAll:%d,%d,%d",mapindex,mapx,mapy);
	for (TBrowserMemberOnlineContainer::iterator it = m_set_memberOnline.begin(); it != m_set_memberOnline.end();++it) {
		if (*it != NULL) {
			DungeonBrowserEventInfo* info = AllocEventInfo<DungeonBrowserEventInfo>();
			sys_log(0, "JoinAll: %u", (*it)->GetPlayerID());

			(*it)->SetQuestFlag("savemapindex", (*it)->GetMapIndex());
			(*it)->SetQuestFlag("savemapx", (*it)->GetX());
			(*it)->SetQuestFlag("savemapy", (*it)->GetY());

			info->ch = (*it);
			info->left_second = 10;
			info->map_index = mapindex;
			info->m_x = mapx;
			info->m_y = mapy;
			sys_log(0, "JoinAll1: %u", (*it)->GetPlayerID());
			(*it)->m_pkDungeonBrowserEvent = event_create(dungeonbrowser_event, info, 1);
		}
	}
}


CBrowserTeam::CBrowserTeam(DWORD dwLeaderID)
{
	m_dwLeaderID = dwLeaderID;
	mpk_Event_createparty = NULL;
}

CBrowserTeam::~CBrowserTeam()
{
	mpk_Event_createparty = NULL;

	for (TBrowserMemberOnlineContainer::iterator it = m_set_memberOnline.begin(); it != m_set_memberOnline.end();++it) {
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

void CBrowserTeam::Load(int bGameIndex)
{
	sys_log(0, "LoadBrowserTeam: %u", m_dwLeaderID);
	m_bGameIndex = bGameIndex;

}

bool CBrowserTeam::AllOnline()
{

	for (TMemberBrowserMap::iterator it = m_Member_Map.begin(); it != m_Member_Map.end(); ++it)
	{
		if (it->second.bState == 0)
			return false;
	}

	return true;
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
		
		__ClientPacket(BROWSER_SUBHEADER_GC_ADD, &Teamc, sizeof(TPacketBrowserTeamAdd));

		if (pChar)
		{
			__AddViewer(pChar);	
			SendBrowserJoinAllToOne(pChar);
		}
		

	}
	else
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
	sys_log(0, "UpdateState1: %u :%u", dwPlayerID,bState);
	
	TMemberBrowserMap::iterator it = m_Member_Map.find(dwPlayerID);
	if (it == m_Member_Map.end())
	{
		sys_err("Cant find player in browser team");
		return;
	}
	sys_log(0, "UpdateState2: %u", dwPlayerID);
	
		
	m_Member_Map[dwPlayerID].bState = bState;
	sys_log(0, "UpdateState3: %u", dwPlayerID);

	TPacketBrowserTeamUpdate Teamc;
	sys_log(0, "UpdateState4: %u", dwPlayerID);
	Teamc.dwLeaderPID = GetDungeonID();
	sys_log(0, "UpdateState5: %u", dwPlayerID);
	Teamc.dwPID = it->second.dwPlayerID;
	sys_log(0, "UpdateState6: %u", dwPlayerID);
	Teamc.bState = bState;
	sys_log(0, "UpdateState7: %u", dwPlayerID);

	__ClientPacket(BROWSER_SUBHEADER_GC_UPDATE, &Teamc, sizeof(TPacketBrowserTeamUpdate));
	sys_log(0, "UpdateState9: %u", dwPlayerID);
}



void CBrowserTeam::P2PChat(const char* c_pszText)
{
	for (TBrowserMemberOnlineContainer::iterator it = m_set_memberOnline.begin(); it != m_set_memberOnline.end();++it) {
		if (*it != NULL) {
			(*it)->ChatPacket(CHAT_TYPE_BROWSER, "%s", c_pszText);
			sys_log(0, "P2PChat: %u", (*it)->GetPlayerID());
		}
	}
}

void CBrowserTeam::Chat(const char* c_pszText)
{
	sys_log(0, "Chat; %s",c_pszText);
	for (TBrowserMemberOnlineContainer::iterator it = m_set_memberOnline.begin(); it != m_set_memberOnline.end();++it) {
		if (*it != NULL) {
			(*it)->ChatPacket(CHAT_TYPE_BROWSER, "%s", c_pszText);
		}
	}

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
	sys_log(0, "SendBrowserJoinAllToOne; %d size : %d ",ch->GetPlayerID(),m_Member_Map.size());
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
	for (TBrowserMemberOnlineContainer::iterator it = m_set_memberOnline.begin(); it != m_set_memberOnline.end() ; ++it){
		(*it)->GetDesc()->Packet(c_pData, size);
	}
}

bool CBrowserTeam::__IsViewer(LPCHARACTER ch)
{
	return m_set_memberOnline.find(ch) != m_set_memberOnline.end();
}

void CBrowserTeam::__AddViewer(LPCHARACTER ch)
{
	sys_log(0, "__AddViewer: %u", ch->GetPlayerID());
	if (!__IsViewer(ch))
		m_set_memberOnline.insert(ch);
}
void CBrowserTeam::__RemoveViewer(LPCHARACTER ch)
{
	m_set_memberOnline.erase(ch);
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

bool CDungeonBrowserManager::AllOnline(DWORD dwLeaderID)
{
	CBrowserTeam* pBrowserTeam;
	if (!(pBrowserTeam = GetDungeonTeam(dwLeaderID)))
		return false;

	return pBrowserTeam->AllOnline();;
}


void CDungeonBrowserManager::Destroy()
{
	for (itertype(m_map_BrowserTeams) it = m_map_BrowserTeams.begin(); it != m_map_BrowserTeams.end(); ++it)
		delete (*it).second;
	m_map_BrowserTeams.clear();
}

void CDungeonBrowserManager::CreateTeam(LPCHARACTER ch, int bGameIndex)
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
		sys_log(0, "CBrowserTeam::DestroyDungeonTeam %u", dwLeaderID);

	TPacketBrowserTeamDestroy tMec;
	tMec.dwLeaderPID = dwLeaderID;

	db_clientdesc->DBPacket(HEADER_GD_BROWSER_TEAM_DESTROY, 0, &tMec, sizeof(TPacketBrowserTeamDestroy));
}

void CDungeonBrowserManager::P2PLogin( LPCHARACTER ch)
{
	if (test_server)
		sys_log(0, "CBrowserTeam::P2PLogin %u", ch->GetPlayerID());

	DWORD dwLeaderID = 0;

	for (itertype(m_map_BrowserTeams) it = m_map_BrowserTeams.begin(); it != m_map_BrowserTeams.end(); ++it){
		if (it->second->FindPlayer(ch->GetPlayerID())){
			dwLeaderID = it->first;
			break;
		}
	}
	if(dwLeaderID != 0)
	{
		m_map_BrowserTeams[dwLeaderID]->__AddViewer(ch);
		TPacketBrowserTeamUpdate tMec;
		tMec.dwLeaderPID = dwLeaderID;
		tMec.dwPID = ch->GetPlayerID();
		tMec.bState = 1;
		
		db_clientdesc->DBPacket(HEADER_GD_BROWSER_TEAM_UPDATE, 0, &tMec, sizeof(TPacketBrowserTeamUpdate));
	}
}

void CDungeonBrowserManager::P2PLogout(LPCHARACTER ch)
{
	if (test_server)
		sys_log(0, "CBrowserTeam::P2PLogout %u", ch->GetPlayerID());

	DWORD dwLeaderID = 0;

	for (itertype(m_map_BrowserTeams) it = m_map_BrowserTeams.begin(); it != m_map_BrowserTeams.end(); ++it){
		sys_log(0, "CBrowserTeam::m_map_BrowserTeams Packet %u", it->first);
		if (it->second->FindPlayer(ch->GetPlayerID())){
			dwLeaderID = it->first;
			sys_log(0, "CBrowserTeam::P2PLogout Packet %u", dwLeaderID);
			break;
		}
	}
	if(dwLeaderID != 0)
	{
		sys_log(0, "CBrowserTeam::P2PLogout Packet %u", dwLeaderID);
		TPacketBrowserTeamUpdate tMec;
		tMec.dwLeaderPID = dwLeaderID;
		tMec.dwPID = ch->GetPlayerID();
		tMec.bState = 0;
		
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
	sys_log(0, "CBrowserTeam::DB_P2PLogin Packet %u %u", dwLeaderID,dwPlayerID);
	CBrowserTeam* pBrowserTeam;
	if (!(pBrowserTeam = GetDungeonTeam(dwLeaderID)))
		return;

	pBrowserTeam->UpdateState(dwPlayerID, 1);
}
void CDungeonBrowserManager::DB_P2PLogout(DWORD dwLeaderID, DWORD dwPlayerID)
{
	sys_log(0, "CBrowserTeam::DB_P2PLogout Packet %u %u", dwLeaderID,dwPlayerID);
	CBrowserTeam* pBrowserTeam;
	if (!(pBrowserTeam = GetDungeonTeam(dwLeaderID)))
		return;

	pBrowserTeam->UpdateState(dwPlayerID, 0);
}

void CDungeonBrowserManager::DB_CreateTeam(DWORD dwLeaderID, int bGameIndex,long map_index)
{
	CBrowserTeam* pBrowserTeam;
	if ((pBrowserTeam = GetDungeonTeam(dwLeaderID)))
		return;
	
	sys_log(0, "CBrowserTeam::DB_CreateTeam Packet %u:%u", dwLeaderID,bGameIndex);
	
	CBrowserTeam* pBrowserTeam1 = new CBrowserTeam(dwLeaderID);
	pBrowserTeam1->SetGameIndex(bGameIndex);
	pBrowserTeam1->SetMapIndex(map_index);
	sys_log(0, "CBrowserTeam::DB_CreateTeam Packet %u:", pBrowserTeam1->GetDungeonID());
	m_map_BrowserTeams.insert(std::pair<DWORD, CBrowserTeam*>(dwLeaderID, pBrowserTeam1));
	sys_log(0, "CBrowserTeam::DB_CreateTeam Packet1 %u:", GetDungeonTeam(dwLeaderID)->GetDungeonID());
}

void CDungeonBrowserManager::JoinAlltoDungeon(DWORD dwLeaderId)
{
	CBrowserTeam* pBrowserTeam;
	if (!(pBrowserTeam = GetDungeonTeam(dwLeaderId)))
		return;
	pBrowserTeam->CreateDungeon();

}

void CDungeonBrowserManager::DB_DestroyDungeonTeam(DWORD dwLeaderID)
{
	CBrowserTeam* pBrowserTeam;
	if (!(pBrowserTeam = GetDungeonTeam(dwLeaderID)))
		return;

	delete pBrowserTeam;
	m_map_BrowserTeams.erase(dwLeaderID);
}


