#ifndef __HEADER_DUNGEON_BROWSER_MANAGER__
#define __HEADER_DUNGEON_BROWSER_MANAGER__

#include "stdafx.h"
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#include "../../game/src/utils.h"

#include <map>
#include "DBManager.h"

class CPeer;

typedef struct SDungeonBrowserHandleInfo
{
	DWORD	dwLeaderID;

	SDungeonBrowserHandleInfo(DWORD dwLeaderID) : dwLeaderID(dwLeaderID)
	{
	}
} DungeonBrowserHandleInfo;

typedef std::map<DWORD,TMemberBrowser*> TMemberBrowserMap;


class CBrowserTeam
{
public:

	CBrowserTeam(DWORD dwLeaderID);
	~CBrowserTeam();

	DWORD	GetBrowserID() const { return m_dwLeaderID; }
	
	BYTE	GetGameIndex() const { return bGameIndex; }
	BYTE	SetGameIndex(BYTE newbGameIndex) { bGameIndex = newbGameIndex; }
	
	BYTE	SetMaxMember(BYTE newbMaxMember) { bMaxMember = newbMaxMember; }
	BYTE	GetMaxMember() const { return bMaxMember; }
	
	BYTE	GetActualMember() const {return m_Member_Map.size(); }

	void	LoadMember(SQLMsg* pMsg);
	void	LoadMember(CPeer* pkPeer, DWORD dwHandle = 0);
	bool	FindPlayer(DWORD dwPlayerID);
	void	AddMember(DWORD dwPlayerID, int iLevel, const char* strName, BYTE bRace);

	void	RemoveMember(DWORD dwPlayerID);
	void	AddPeer(CPeer* pkPeer);
	void	UpdateState(DWORD dwPlayerID, BYTE bState);
	void	ForwardPacket(BYTE bHeader, const void* c_pData, DWORD dwSize);


private:
	DWORD	m_dwLeaderID;

	boost::unordered_set<CPeer*>	m_set_ForwardPeer;

	TMemberBrowserMap						m_Member_Map;
	
	BYTE	bGameIndex;
	
	BYTE	bMaxMember;
	
};

class CDungeonBrowserManager : public singleton<CDungeonBrowserManager>
{
public:
	CDungeonBrowserManager();
	~CDungeonBrowserManager();

	void				Initialize();
	void				Destroy();

	CBrowserTeam*		GetDungeonTeam(DWORD dwLeaderID);
	void				DestroyDungeonTeam(DWORD dwLeaderID);

	void				Update();

	void				QueryResult(CPeer* pkPeer, SQLMsg* pMsg, int iQIDNum);
	void				ProcessPacket(CPeer* pkPeer, DWORD dwHandle, BYTE bHeader, const void* c_pData);

	WORD				GetBrowserTeamCount() const { return m_map_BrowserTeams.size(); }
	void				InitBrowserCore(CPeer* pkPeer);

private:
	DWORD				m_dwLastFlushBrowserTeam;

	std::map<DWORD, CBrowserTeam*>			m_map_BrowserTeams;
	std::map<DWORD, DWORD>			m_map_MaxMember;
	boost::unordered_set<CBrowserTeam*>		m_map_DelayedBrowserTeamSave;


};
#endif
