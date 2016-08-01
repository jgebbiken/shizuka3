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

typedef struct SDungeoninit
{
	DWORD	dwMaxMember;
	long	map_index;
	
} TDungeoninit;

typedef std::map<DWORD,TMemberBrowser> TMemberBrowserMap;


class CBrowserTeam
{
public:

	CBrowserTeam();
	~CBrowserTeam();

	DWORD	GetBrowserID() const { return m_dwLeaderID; }
	DWORD	SetBrowserID(DWORD newBrowserID) { sys_log(0, "SetBrowserID %u",newBrowserID); m_dwLeaderID = newBrowserID; }
	
	
	int	GetGameIndex() const { return bGameIndex; }
	int	SetGameIndex(int newbGameIndex) { bGameIndex = newbGameIndex; }
	
	long		GetMapIndex() const { return map_index; }
	long		SetMapIndex(long newbMapIndex) { map_index = newbMapIndex; }
	
	int	SetMaxMember(int newbMaxMember) { bMaxMember = newbMaxMember; }
	int	GetMaxMember() const { return bMaxMember; }
	
	int	GetActualMember() const {return m_Member_Map.size(); }

	void	LoadMember(SQLMsg* pMsg);
	void	LoadMember(CPeer* pkPeer, DWORD dwHandle = 0);
	bool	FindPlayer(DWORD dwPlayerID);
	void	AddMember(DWORD dwPlayerID, int iLevel, const char* strName, BYTE bRace);

	void	RemoveMember(DWORD dwPlayerID);
	void	AddPeer(CPeer* pkPeer);
	void	UpdateState(DWORD dwPlayerID, BYTE bState);
	void	ForwardPacket(BYTE bHeader, const void* c_pData, DWORD dwSize, CPeer* pkExceptPeer = NULL);

	void	ForwardPacketCopyCore(BYTE bHeader, const void* c_pData, DWORD dwSize);


private:
	DWORD	m_dwLeaderID;

	boost::unordered_set<CPeer*>	m_set_ForwardPeer;

	TMemberBrowserMap						m_Member_Map;
	
	int	bGameIndex;
	
	int	bMaxMember;
	
	long		map_index;
	
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
	void	AddPeerManager(CPeer* pkPeer);

	void				Update();

	void				QueryResult(CPeer* pkPeer, SQLMsg* pMsg, int iQIDNum);
	void				ProcessPacket(CPeer* pkPeer, DWORD dwHandle, BYTE bHeader, const void* c_pData);

	WORD				GetBrowserTeamCount() const { return m_map_BrowserTeams.size(); }
	void				InitBrowserCore(CPeer* pkPeer);

private:
	DWORD				m_dwLastFlushBrowserTeam;

	std::map<DWORD, CBrowserTeam*>			m_map_BrowserTeams;
	std::map<DWORD, TDungeoninit>			m_map_MaxMember;
	boost::unordered_set<CBrowserTeam*>		m_map_DelayedBrowserTeamSave;
	
	boost::unordered_set<CPeer*>	m_set_ForwardPeerManager;


};
#endif
