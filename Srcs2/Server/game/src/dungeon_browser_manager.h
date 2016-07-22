#ifndef _DUNGEON_HEADER_H_
#define _DUNGEON_HEADER_H_

#include "stdafx.h"
#include <boost/unordered_set.hpp>


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
	BYTE	GetGameIndex() const { return m_bGameIndex; }
	void	SetGameIndex(BYTE newbGameIndex) { m_bGameIndex = newbGameIndex; }

	//only for load
	void Load(BYTE bGameIndex);
	void LoadMember(TMemberBrowser* tMem, DWORD dwSize);


	void	AddMember(DWORD dwPlayerID, int iLevel, const char* strName, BYTE bRace);
	void	RemoveMember(DWORD dwPlayerID);
	bool	FindPlayer(DWORD dwPlayerID);
	void	UpdateState(DWORD dwPlayerID, BYTE bState);
	void		Chat(const char* c_pszText);
	void		P2PChat(const char* c_pszText); // 길드 채팅

	void		SendBrowserJoinOneToAll(DWORD dwPID);
	void		SendBrowserRemoveOneToAll(DWORD pid);
	void		SendBrowserJoinAllToOne(LPCHARACTER ch);

	//to client
	void	__AddViewer(LPCHARACTER ch);
	void	__RemoveViewer(LPCHARACTER ch);
	bool	__IsViewer(LPCHARACTER ch);
	void	__ClientPacket(BYTE subheader, const void* c_pData, size_t size, LPCHARACTER ch = NULL);
	void	__ViewerPacket(const void* c_pData, size_t size);

private:
	DWORD	m_dwLeaderID;
	BYTE	m_bGameIndex;
	TMemberBrowserMap						m_Member_Map;
	bool	m_bMemberLoaded;
	std::set<LPCHARACTER>	m_set_pkCurrentViewer;




};

class CDungeonBrowserManager : public singleton<CDungeonBrowserManager>
{
public:
	CDungeonBrowserManager();
	~CDungeonBrowserManager();

	void Destroy();

	CBrowserTeam*		GetDungeonTeam(DWORD dwLeaderID);

	bool				SetBrowser(LPCHARACTER pkChr);

	//to db

	void				LoadMember(DWORD dwLeaderID);


	void				DestroyDungeonTeam(DWORD dwLeaderID);
	void				CreateTeam(LPCHARACTER ch, BYTE bGameIndex);
	void				AddMember(LPCHARACTER ch, DWORD dwLeaderID);
	void				RemoveMember(LPCHARACTER ch, DWORD dwPlayerID);
	void				P2PLogin(DWORD dwPlayerID);
	void				P2PLogout(DWORD dwPlayerID);
	


	//from Db
	void				DB_P2PLogin(DWORD dwLeaderID, DWORD dwPlayerID);
	void				DB_P2PLogout(DWORD dwLeaderID, DWORD dwPlayerID);
	void				DB_DestroyDungeonTeam(DWORD dwLeaderID);
	void				DB_CreateTeam(DWORD dwLeaderID, BYTE bGameIndex);


private:
	std::map<DWORD, CBrowserTeam*>			m_map_BrowserTeams;
};
#endif
