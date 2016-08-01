#ifndef _DUNGEON_HEADER_H_
#define _DUNGEON_HEADER_H_

#include "stdafx.h"
#include <boost/unordered_set.hpp>
#include "event.h"


typedef struct _SQLMsg SQLMsg;
class CBrowserTeam;
enum EDungeonGames {
	DUNGEON_ICE_RUN,
	DUNGEON_FIRE_RUN,
	DUNGEON_GAME_MAX_NUM
};

typedef std::map<DWORD, TMemberBrowser> TMemberBrowserMap;
class CBrowserTeam
{
public:
	CBrowserTeam(DWORD dwLeaderID);
	~CBrowserTeam();

	DWORD GetDungeonID() const { return m_dwLeaderID; }
	int	GetGameIndex() const { return m_bGameIndex; }
	void	SetGameIndex(int newbGameIndex) { m_bGameIndex = newbGameIndex; }

	long		GetMapIndex() const { return map_index; }
	void	SetMapIndex(long newbMapIndex) { map_index = newbMapIndex; }
	
	int		GetDungeonMemberCount() const {return m_Member_Map.size(); }
	

	//only for load
	void Load(int bGameIndex);
	void LoadMember(TMemberBrowser* tMem, DWORD dwSize);


	void	AddMember(DWORD dwPlayerID, int iLevel, const char* strName, BYTE bRace);
	void	RemoveMember(DWORD dwPlayerID);
	bool	FindPlayer(DWORD dwPlayerID);
	bool	AllOnline();
	void	UpdateState(DWORD dwPlayerID, BYTE bState);
	
	void		Chat(const char* c_pszText);
	void		P2PChat(const char* c_pszText); // 길드 채팅

	void		CreateDungeon();

	void		CreateDungeonP2P();

	void		JoinAll(long mapindex,long mapx,long mapy);

	void		SendBrowserJoinOneToAll(DWORD dwPID);
	void		SendBrowserRemoveOneToAll(DWORD pid);
	void		SendBrowserJoinAllToOne(LPCHARACTER ch);

	//to client
	void	__AddViewer(LPCHARACTER ch);
	void	__RemoveViewer(LPCHARACTER ch);
	bool	__IsViewer(LPCHARACTER ch);
	void	__ClientPacket(BYTE subheader, const void* c_pData, size_t size, LPCHARACTER ch = NULL);
	void	__ViewerPacket(const void* c_pData, size_t size);

	LPEVENT			mpk_Event_createparty;

private:
	DWORD	m_dwLeaderID;
	int	m_bGameIndex;
	long		map_index;
	long	m_x;
	long	m_y;
	TMemberBrowserMap						m_Member_Map;
	bool	m_bMemberLoaded;
	typedef CHARACTER_SET TBrowserMemberOnlineContainer;
	TBrowserMemberOnlineContainer	m_set_memberOnline;

};

class CDungeonBrowserManager : public singleton<CDungeonBrowserManager>
{
public:
	CDungeonBrowserManager();
	~CDungeonBrowserManager();

	void Destroy();

	CBrowserTeam*		GetDungeonTeam(DWORD dwLeaderID);

	bool				SetBrowser(LPCHARACTER pkChr);

	bool				AllOnline(DWORD dwLeaderID);


	//to db

	void				LoadMember(DWORD dwLeaderID);


	void				DestroyDungeonTeam(DWORD dwLeaderID);
	void				CreateTeam(LPCHARACTER ch, int bGameIndex);
	void				AddMember(LPCHARACTER ch, DWORD dwLeaderID);
	void				RemoveMember(LPCHARACTER ch, DWORD dwPlayerID);
	void				P2PLogin(LPCHARACTER ch);
	void				P2PLogout(LPCHARACTER ch);

	void				JoinAlltoDungeon(DWORD dwLeaderId);
	


	//from Db
	void				DB_P2PLogin(DWORD dwLeaderID, DWORD dwPlayerID);
	void				DB_P2PLogout(DWORD dwLeaderID, DWORD dwPlayerID);
	void				DB_DestroyDungeonTeam(DWORD dwLeaderID);
	void				DB_CreateTeam(DWORD dwLeaderID, int bGameIndex, long map_index);


private:
	std::map<DWORD, CBrowserTeam*>			m_map_BrowserTeams;
};
#endif
