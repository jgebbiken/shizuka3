// vim:ts=4 sw=4
#include <map>
#include "stdafx.h"
#include "ClientManager.h"
#include "Main.h"
#include "Monarch.h"
#include "CsvReader.h"
#include "ProtoReader.h"

using namespace std;

extern int g_test_server;
extern std::string g_stLocaleNameColumn;

bool CClientManager::InitializeTables()
{
	if (!InitializeMobTable())
	{
		sys_err("InitializeMobTable FAILED");
		return false;
	}

	if (!InitializeItemTable())
	{
		sys_err("InitializeItemTable FAILED");
		return false; 
	}

	if (!InitializeShopTable())
	{
		sys_err("InitializeShopTable FAILED");
		return false;
	}

	if (!InitializeSkillTable())
	{
		sys_err("InitializeSkillTable FAILED");
		return false;
	}

	if (!InitializeRefineTable())
	{
		sys_err("InitializeRefineTable FAILED");
		return false;
	}

	if (!InitializeItemAttrTable())
	{
		sys_err("InitializeItemAttrTable FAILED");
		return false;
	}

	if (!InitializeItemRareTable())
	{
		sys_err("InitializeItemRareTable FAILED");
		return false;
	}

	if (!InitializeBanwordTable())
	{
		sys_err("InitializeBanwordTable FAILED");
		return false;
	}

	if (!InitializeLandTable())
	{
		sys_err("InitializeLandTable FAILED");
		return false;
	}

	if (!InitializeObjectProto())
	{
		sys_err("InitializeObjectProto FAILED");
		return false;
	}

	if (!InitializeObjectTable())
	{
		sys_err("InitializeObjectTable FAILED");
		return false;
	}

	if (!InitializeMonarch())
	{
		sys_err("InitializeMonarch FAILED");
		return false;
	}


	return true;
}

bool CClientManager::InitializeRefineTable()
{
	char query[2048];

	snprintf(query, sizeof(query),
			"SELECT id, cost, prob, vnum0, count0, vnum1, count1, vnum2, count2,  vnum3, count3, vnum4, count4 FROM refine_proto%s",
			GetTablePostfix());

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	if (!pRes->uiNumRows)
		return true;

	if (m_pRefineTable)
	{
		sys_log(0, "RELOAD: refine_proto");
		delete [] m_pRefineTable;
		m_pRefineTable = NULL;
	}

	m_iRefineTableSize = pRes->uiNumRows;

	m_pRefineTable	= new TRefineTable[m_iRefineTableSize];
	memset(m_pRefineTable, 0, sizeof(TRefineTable) * m_iRefineTableSize);

	TRefineTable* prt = m_pRefineTable;
	MYSQL_ROW data;

	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		//const char* s_szQuery = "SELECT src_vnum, result_vnum, cost, prob, "
		//"vnum0, count0, vnum1, count1, vnum2, count2,  vnum3, count3, vnum4, count4 "

		int col = 0;
		//prt->src_vnum = atoi(data[col++]);
		//prt->result_vnum = atoi(data[col++]);
		str_to_number(prt->id, data[col++]);
		str_to_number(prt->cost, data[col++]);
		str_to_number(prt->prob, data[col++]);

		for (int i = 0; i < REFINE_MATERIAL_MAX_NUM; i++)
		{
			str_to_number(prt->materials[i].vnum, data[col++]);
			str_to_number(prt->materials[i].count, data[col++]);
			if (prt->materials[i].vnum == 0)
			{
				prt->material_count = i;
				break;
			}
		}

		sys_log(0, "REFINE: id %ld cost %d prob %d mat1 %lu cnt1 %d", prt->id, prt->cost, prt->prob, prt->materials[0].vnum, prt->materials[0].count);

		prt++;
	}
	return true;
}

class FCompareVnum
{
	public:
		bool operator () (const TEntityTable & a, const TEntityTable & b) const
		{
			return (a.dwVnum < b.dwVnum);
		}
};

bool CClientManager::InitializeMobTable()
{
	char query[2048];
	fprintf(stderr,"Loading mob_proto from MySQL ");
	snprintf(query, sizeof(query),
			"SELECT vnum,name,%s,rank,type,battle_type,level,size,ai_flag,mount_capacity,setRaceFlag,setImmuneFlag,"
"empire,folder,on_click,st,dx,ht,iq,damage_min,damage_max,max_hp,regen_cycle,regen_percent,gold_min,"
"gold_max,exp,def,attack_speed,move_speed,aggressive_hp_pct,aggressive_sight,attack_range,drop_item,"
"resurrection_vnum,enchant_curse,enchant_slow,enchant_poison,enchant_stun,enchant_critical,enchant_penetrate,"
"resist_sword,resist_twohand,resist_dagger,resist_bell,resist_fan,resist_bow,resist_fire,resist_elect,"
"resist_magic,resist_wind,resist_poison,dam_multiply,summon,drain_sp,mob_color,polymorph_item,skill_level0,"
"skill_vnum0,skill_level1,skill_vnum1,sp_berserk,sp_stoneskin,sp_godspeed,sp_deathblow,sp_revive,skill_level2,"
"skill_vnum2,skill_level3,skill_vnum3,skill_level4,skill_vnum4 FROM mob_proto%s"
,g_stLocaleNameColumn.c_str(),
			GetTablePostfix());

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	if (!pRes->uiNumRows)
		return false;

	if (!m_vec_mobTable.empty())
	{
		sys_log(0, "RELOAD: mob_proto");
		m_vec_mobTable.clear();
	}
	int size = pRes->uiNumRows;
	m_vec_mobTable.resize(size);
	memset(&m_vec_mobTable[0], 0, sizeof(TMobTable) * m_vec_mobTable.size());
	TMobTable * mob_table = &m_vec_mobTable[0];
	
	MYSQL_ROW data;
	//return true;
	set<int> vnumSet;
	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		
		/*
		"SELECT vnum,name,locale_name,rank,type,battle_type,level,size,ai_flag,mount_capacity,setRaceFlag,setImmuneFlag,"
"empire,folder,on_click,st,dx,ht,iq,damage_min,damage_max,max_hp,regen_cycle,regen_percent,gold_min,"
"gold_max,exp,def,attack_speed,move_speed,aggressive_hp_pct,aggressive_sight,attack_range,drop_item,"
"resurrection_vnum,enchant_curse,enchant_slow,enchant_poison,enchant_stun,enchant_critical,enchant_penetrate,"
"resist_sword,resist_twohand,resist_dagger,resist_bell,resist_fan,resist_bow,resist_fire,resist_elect,"
"resist_magic,resist_wind,resist_poison,dam_multiply,summon,drain_sp,mob_color,polymorph_item,skill_level0,"
"skill_vnum0,skill_level1,skill_vnum1,sp_berserk,sp_stoneskin,sp_godspeed,sp_deathblow,sp_revive,skill_level2,"
"skill_vnum2,skill_level3,skill_vnum3,skill_level4,skill_vnum4 FROM mob_proto%s */
		int col = 0;
		str_to_number(mob_table->dwVnum, data[col++]);
		if(mob_table->dwVnum ==0) continue;
		strlcpy(mob_table->szName,data[col++] , sizeof(mob_table->szName));
		strlcpy(mob_table->szLocaleName, data[col++], sizeof(mob_table->szLocaleName));
		str_to_number(mob_table->bRank,data[col++]);
		str_to_number(mob_table->bType,data[col++]);
		str_to_number(mob_table->bBattleType,data[col++]);
		str_to_number(mob_table->bLevel,data[col++]);
		str_to_number(mob_table->bSize,data[col++]);
		//AI_FLAG
		mob_table->dwAIFlag = get_Mob_AIFlag_Value(data[col++]);
		//mount_capacity;
		col++;
		//RACE_FLAG
		mob_table->dwRaceFlag = get_Mob_RaceFlag_Value(data[col++]);
		//IMMUNE_FLAG
		mob_table->dwImmuneFlag = get_Mob_ImmuneFlag_Value(data[col++]);
		mob_table->bEmpire = atoi(data[col++]);
		strlcpy(mob_table->szFolder, data[col++], sizeof(mob_table->szFolder));
		mob_table->bOnClickType = atoi(data[col++]);
		mob_table->bStr = atoi(data[col++]);
		mob_table->bDex = atoi(data[col++]);
		mob_table->bCon = atoi(data[col++]);
		mob_table->bInt = atoi(data[col++]);
		mob_table->dwDamageRange[0] = atoi(data[col++]);
		mob_table->dwDamageRange[1] = atoi(data[col++]);
		mob_table->dwMaxHP = atoi(data[col++]);
		mob_table->bRegenCycle = atoi(data[col++]);
		mob_table->bRegenPercent = atoi(data[col++]);
		mob_table->dwGoldMin = atoi(data[col++]);
		mob_table->dwGoldMax = atoi(data[col++]);
		mob_table->dwExp = atoi(data[col++]);
		mob_table->wDef = atoi(data[col++]);
		mob_table->sAttackSpeed = atoi(data[col++]);
		mob_table->sMovingSpeed = atoi(data[col++]);
		mob_table->bAggresiveHPPct = atoi(data[col++]);
		mob_table->wAggressiveSight = atoi(data[col++]);
		mob_table->wAttackRange = atoi(data[col++]);	
		str_to_number(mob_table->dwDropItemVnum, data[col++]);	//32
		str_to_number(mob_table->dwResurrectionVnum, data[col++]);
		for (int i = 0; i < MOB_ENCHANTS_MAX_NUM; ++i)
			str_to_number(mob_table->cEnchants[i], data[col++]);

		for (int i = 0; i < MOB_RESISTS_MAX_NUM; ++i)
			str_to_number(mob_table->cResists[i], data[col++]);

		str_to_number(mob_table->fDamMultiply, data[col++]);
		str_to_number(mob_table->dwSummonVnum, data[col++]);
		str_to_number(mob_table->dwDrainSP, data[col++]);

		//Mob_Color
		++col;

		str_to_number(mob_table->dwPolymorphItemVnum, data[col++]);

		str_to_number(mob_table->Skills[0].bLevel, data[col++]);
		str_to_number(mob_table->Skills[0].dwVnum, data[col++]);
		str_to_number(mob_table->Skills[1].bLevel, data[col++]);
		str_to_number(mob_table->Skills[1].dwVnum, data[col++]);
		str_to_number(mob_table->Skills[2].bLevel, data[col++]);
		str_to_number(mob_table->Skills[2].dwVnum, data[col++]);
		str_to_number(mob_table->Skills[3].bLevel, data[col++]);
		str_to_number(mob_table->Skills[3].dwVnum, data[col++]);
		str_to_number(mob_table->Skills[4].bLevel, data[col++]);
		str_to_number(mob_table->Skills[4].dwVnum, data[col++]);

		str_to_number(mob_table->bBerserkPoint, data[col++]);
		str_to_number(mob_table->bStoneSkinPoint, data[col++]);
		str_to_number(mob_table->bGodSpeedPoint, data[col++]);
		str_to_number(mob_table->bDeathBlowPoint, data[col++]);
		str_to_number(mob_table->bRevivePoint, data[col++]);

		//?A?ˇ vnum ?ß°ˇ
		vnumSet.insert(mob_table->dwVnum);
		
		
		//fprintf(stderr, "MOB #%d %s %s level: %u rank: %u empire: %d\n", mob_table->dwVnum, mob_table->szName, mob_table->szLocaleName, mob_table->bLevel, mob_table->bRank, mob_table->bEmpire);
		sys_log(0, "MOB #%-5d %-24s %-24s level: %-3u rank: %u empire: %d", mob_table->dwVnum, mob_table->szName, mob_table->szLocaleName, mob_table->bLevel, mob_table->bRank, mob_table->bEmpire);
		++mob_table;
	}
	fprintf(stderr," Complete! %d/%d Mobs loaded.\r\n",size,vnumSet.size());
	sort(m_vec_mobTable.begin(), m_vec_mobTable.end(), FCompareVnum());
	return true;

}

bool CClientManager::InitializeShopTable()
{
	MYSQL_ROW	data;
	int		col;

	static const char * s_szQuery = 
		"SELECT "
		"shop.vnum, "
		"shop.npc_vnum, "
		"shop_item.item_vnum, "
		"shop_item.count "
		"FROM shop LEFT JOIN shop_item "
		"ON shop.vnum = shop_item.shop_vnum ORDER BY shop.vnum, shop_item.item_vnum";

	std::auto_ptr<SQLMsg> pkMsg2(CDBManager::instance().DirectQuery(s_szQuery));

	// shop의 vnum은 있는데 shop_item 이 없을경우... 실패로 처리되니 주의 요망.
	// 고처야할부분
	SQLResult * pRes2 = pkMsg2->Get();

	if (!pRes2->uiNumRows)
	{
		sys_err("InitializeShopTable : Table count is zero.");
		return false;
	}

	std::map<int, TShopTable *> map_shop;

	if (m_pShopTable)
	{
		delete [] (m_pShopTable);
		m_pShopTable = NULL;
	}

	TShopTable * shop_table = m_pShopTable;

	while ((data = mysql_fetch_row(pRes2->pSQLResult)))
	{
		col = 0;

		int iShopVnum = 0;
		str_to_number(iShopVnum, data[col++]);

		if (map_shop.end() == map_shop.find(iShopVnum))
		{
			shop_table = new TShopTable;
			memset(shop_table, 0, sizeof(TShopTable));
			shop_table->dwVnum	= iShopVnum;

			map_shop[iShopVnum] = shop_table;
		}
		else
			shop_table = map_shop[iShopVnum];

		str_to_number(shop_table->dwNPCVnum, data[col++]);

		if (!data[col])	// 아이템이 하나도 없으면 NULL이 리턴 되므로..
			continue;

		TShopItemTable * pItem = &shop_table->items[shop_table->byItemCount];

		str_to_number(pItem->vnum, data[col++]);
		str_to_number(pItem->count, data[col++]);

		++shop_table->byItemCount;
	}

	m_pShopTable = new TShopTable[map_shop.size()];
	m_iShopTableSize = map_shop.size();

	typeof(map_shop.begin()) it = map_shop.begin();

	int i = 0;

	while (it != map_shop.end())
	{
		thecore_memcpy((m_pShopTable + i), (it++)->second, sizeof(TShopTable));
		sys_log(0, "SHOP: #%d items: %d", (m_pShopTable + i)->dwVnum, (m_pShopTable + i)->byItemCount);
		++i;
	}

	return true;
}

bool CClientManager::InitializeQuestItemTable()
{
	using namespace std;

	static const char * s_szQuery = "SELECT vnum, name, %s FROM quest_item_proto ORDER BY vnum";

	char query[1024];
	snprintf(query, sizeof(query), s_szQuery, g_stLocaleNameColumn.c_str());

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	if (!pRes->uiNumRows)
	{
		sys_err("query error or no rows: %s", query);
		return false;
	}

	MYSQL_ROW row;

	while ((row = mysql_fetch_row(pRes->pSQLResult)))
	{
		int col = 0;

		TItemTable tbl;
		memset(&tbl, 0, sizeof(tbl));

		str_to_number(tbl.dwVnum, row[col++]);

		if (row[col])
			strlcpy(tbl.szName, row[col], sizeof(tbl.szName));

		col++;

		if (row[col])
			strlcpy(tbl.szLocaleName, row[col], sizeof(tbl.szLocaleName));

		col++;

		if (m_map_itemTableByVnum.find(tbl.dwVnum) != m_map_itemTableByVnum.end())
		{
			sys_err("QUEST_ITEM_ERROR! %lu vnum already exist! (name %s)", tbl.dwVnum, tbl.szLocaleName);
			continue;
		}

		tbl.bType = ITEM_QUEST; // quest_item_proto 테이블에 있는 것들은 모두 ITEM_QUEST 유형
		tbl.bSize = 1;

		m_vec_itemTable.push_back(tbl);
	}

	return true;
}

bool CClientManager::InitializeItemTable()
{
	char query[2048];
	fprintf(stderr,"Loading item_proto from MySQL");
	snprintf(query, sizeof(query),
	"SELECT vnum,name,%s,type,subtype,weight,size,antiflag,flag,wearflag,immuneflag+0,gold,shop_buy_price,refined_vnum,"
	"refine_set,magic_pct,limittype0,limitvalue0,limittype1,limitvalue1,applytype0,applyvalue0,"
	"applytype1,applyvalue1,applytype2,applyvalue2,value0,value1,value2,value3,value4,value5,socket_pct,addon_type FROM item_proto%s  ORDER BY vnum",
	g_stLocaleNameColumn.c_str(),
	GetTablePostfix());

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	if (!pRes->uiNumRows)
			return false;
	int addNumber = pRes->uiNumRows;
	if (!m_vec_itemTable.empty())
	{
			sys_log(0, "RELOAD: item_proto");
			m_vec_itemTable.clear();
			m_map_itemTableByVnum.clear();
	}

	m_vec_itemTable.resize(addNumber-1);
	memset(&m_vec_itemTable[0], 0, sizeof(TItemTable) * m_vec_itemTable.size());
	TItemTable * item_table = &m_vec_itemTable[0];

	MYSQL_ROW data;
	//return true;
	set<int> vnumSet;
	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		str_to_number(item_table->dwVnum, data[0]);
		strlcpy(item_table->szName,data[1] , sizeof(item_table->szName));
		strlcpy(item_table->szLocaleName, data[2], sizeof(item_table->szLocaleName));
		str_to_number(item_table->bType, data[3]);
		str_to_number(item_table->bSubType, data[4]);
		str_to_number(item_table->bWeight, data[5]);
		str_to_number(item_table->bSize, data[6]);
		str_to_number(item_table->dwAntiFlags, data[7]);
		str_to_number(item_table->dwFlags, data[8]);
		str_to_number(item_table->dwWearFlags, data[9]);
		str_to_number(item_table->dwImmuneFlag, data[10]);
		str_to_number(item_table->dwGold, data[11]);
		str_to_number(item_table->dwShopBuyPrice, data[12]);
		str_to_number(item_table->dwRefinedVnum, data[13]);
		str_to_number(item_table->wRefineSet, data[14]);
		str_to_number(item_table->bAlterToMagicItemPct, data[15]);
		item_table->cLimitRealTimeFirstUseIndex = -1;
		item_table->cLimitTimerBasedOnWearIndex = -1;
	   
		str_to_number(item_table->aLimits[0].bType, data[16]);
		str_to_number(item_table->aLimits[0].lValue, data[17]);
		if (LIMIT_REAL_TIME_START_FIRST_USE == item_table->aLimits[0].bType)
				item_table->cLimitRealTimeFirstUseIndex = (char)0;
		if (LIMIT_TIMER_BASED_ON_WEAR == item_table->aLimits[0].bType)
				item_table->cLimitTimerBasedOnWearIndex = (char)0;
			   
		str_to_number(item_table->aLimits[1].bType, data[18]);
		str_to_number(item_table->aLimits[1].lValue, data[19]);
		if (LIMIT_REAL_TIME_START_FIRST_USE == item_table->aLimits[1].bType)
				item_table->cLimitRealTimeFirstUseIndex = (char)1;
		if (LIMIT_TIMER_BASED_ON_WEAR == item_table->aLimits[1].bType)
				item_table->cLimitTimerBasedOnWearIndex = (char)1;
	   

		str_to_number(item_table->aApplies[0].bType, data[20]);
		str_to_number(item_table->aApplies[0].lValue, data[21]);
	   
		str_to_number(item_table->aApplies[1].bType, data[22]);
		str_to_number(item_table->aApplies[1].lValue, data[23]);
	   
		str_to_number(item_table->aApplies[2].bType, data[24]);
		str_to_number(item_table->aApplies[2].lValue, data[25]);
	   

		str_to_number(item_table->alValues[0], data[26]);
		str_to_number(item_table->alValues[1], data[27]);
		str_to_number(item_table->alValues[2], data[28]);
		str_to_number(item_table->alValues[3], data[29]);
		str_to_number(item_table->alValues[4], data[30]);
str_to_number(item_table->alValues[5], data[31]);
			   
		str_to_number(item_table->bGainSocketPct, data[32]);
		str_to_number(item_table->sAddonType, data[33]);


		vnumSet.insert(item_table->dwVnum);
		m_map_itemTableByVnum.insert(std::map<DWORD, TItemTable *>::value_type(item_table->dwVnum, item_table));
		sys_log(0, "ITEM: #%-5lu %-24s %-24s VAL: %d %ld %d %d %d %d WEAR %d ANTI %d IMMUNE %d REFINE %lu REFINE_SET %u MAGIC_PCT %u",
						item_table->dwVnum,
						item_table->szName,
						item_table->szLocaleName,
						item_table->alValues[0],
						item_table->alValues[1],
						item_table->alValues[2],
						item_table->alValues[3],
						item_table->alValues[4],
						item_table->alValues[5],
						item_table->dwWearFlags,
						item_table->dwAntiFlags,
						item_table->dwImmuneFlag,
						item_table->dwRefinedVnum,
						item_table->wRefineSet,
						item_table->bAlterToMagicItemPct);

		item_table++;
	}
	fprintf(stderr," Complete! %d Items loaded.\r\n",addNumber);
	return true;

}


bool CClientManager::InitializeSkillTable()
{
	char query[4096];
	snprintf(query, sizeof(query),
		"SELECT dwVnum, szName, bType, bMaxLevel, dwSplashRange, "
		"szPointOn, szPointPoly, szSPCostPoly, szDurationPoly, szDurationSPCostPoly, "
		"szCooldownPoly, szMasterBonusPoly, setFlag+0, setAffectFlag+0, "
		"szPointOn2, szPointPoly2, szDurationPoly2, setAffectFlag2+0, "
		"szPointOn3, szPointPoly3, szDurationPoly3, szGrandMasterAddSPCostPoly, "
		"bLevelStep, bLevelLimit, prerequisiteSkillVnum, prerequisiteSkillLevel, iMaxHit, szSplashAroundDamageAdjustPoly, eSkillType+0, dwTargetRange "
		"FROM skill_proto%s ORDER BY dwVnum",
		GetTablePostfix());

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	if (!pRes->uiNumRows)
	{
		sys_err("no result from skill_proto");
		return false;
	}

	if (!m_vec_skillTable.empty())
	{
		sys_log(0, "RELOAD: skill_proto");
		m_vec_skillTable.clear();
	}

	m_vec_skillTable.reserve(pRes->uiNumRows);

	MYSQL_ROW	data;
	int		col;

	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		TSkillTable t;
		memset(&t, 0, sizeof(t));

		col = 0;

		str_to_number(t.dwVnum, data[col++]);
		strlcpy(t.szName, data[col++], sizeof(t.szName));
		str_to_number(t.bType, data[col++]);
		str_to_number(t.bMaxLevel, data[col++]);
		str_to_number(t.dwSplashRange, data[col++]);

		strlcpy(t.szPointOn, data[col++], sizeof(t.szPointOn));
		strlcpy(t.szPointPoly, data[col++], sizeof(t.szPointPoly));
		strlcpy(t.szSPCostPoly, data[col++], sizeof(t.szSPCostPoly));
		strlcpy(t.szDurationPoly, data[col++], sizeof(t.szDurationPoly));
		strlcpy(t.szDurationSPCostPoly, data[col++], sizeof(t.szDurationSPCostPoly));
		strlcpy(t.szCooldownPoly, data[col++], sizeof(t.szCooldownPoly));
		strlcpy(t.szMasterBonusPoly, data[col++], sizeof(t.szMasterBonusPoly));

		str_to_number(t.dwFlag, data[col++]);
		str_to_number(t.dwAffectFlag, data[col++]);

		strlcpy(t.szPointOn2, data[col++], sizeof(t.szPointOn2));
		strlcpy(t.szPointPoly2, data[col++], sizeof(t.szPointPoly2));
		strlcpy(t.szDurationPoly2, data[col++], sizeof(t.szDurationPoly2));
		str_to_number(t.dwAffectFlag2, data[col++]);

		// ADD_GRANDMASTER_SKILL
		strlcpy(t.szPointOn3, data[col++], sizeof(t.szPointOn3));
		strlcpy(t.szPointPoly3, data[col++], sizeof(t.szPointPoly3));
		strlcpy(t.szDurationPoly3, data[col++], sizeof(t.szDurationPoly3));

		strlcpy(t.szGrandMasterAddSPCostPoly, data[col++], sizeof(t.szGrandMasterAddSPCostPoly));
		// END_OF_ADD_GRANDMASTER_SKILL

		str_to_number(t.bLevelStep, data[col++]);
		str_to_number(t.bLevelLimit, data[col++]);
		str_to_number(t.preSkillVnum, data[col++]);
		str_to_number(t.preSkillLevel, data[col++]);

		str_to_number(t.lMaxHit, data[col++]);

		strlcpy(t.szSplashAroundDamageAdjustPoly, data[col++], sizeof(t.szSplashAroundDamageAdjustPoly));

		str_to_number(t.bSkillAttrType, data[col++]);
		str_to_number(t.dwTargetRange, data[col++]);

		sys_log(0, "SKILL: #%d %s flag %u point %s affect %u cooldown %s", t.dwVnum, t.szName, t.dwFlag, t.szPointOn, t.dwAffectFlag, t.szCooldownPoly);

		m_vec_skillTable.push_back(t);
	}

	return true;
}

bool CClientManager::InitializeBanwordTable()
{
	m_vec_banwordTable.clear();

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery("SELECT word FROM banword"));

	SQLResult * pRes = pkMsg->Get();

	if (pRes->uiNumRows == 0)
		return true;

	MYSQL_ROW data;

	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		TBanwordTable t;

		if (data[0])
		{
			strlcpy(t.szWord, data[0], sizeof(t.szWord));
			m_vec_banwordTable.push_back(t);
		}
	}

	sys_log(0, "BANWORD: total %d", m_vec_banwordTable.size());
	return true;
}

bool CClientManager::InitializeItemAttrTable()
{
	char query[4096];
	snprintf(query, sizeof(query),
			"SELECT apply, apply+0, prob, lv1, lv2, lv3, lv4, lv5, weapon, body, wrist, foots, neck, head, shield, ear FROM item_attr%s ORDER BY apply",
			GetTablePostfix());

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	if (!pRes->uiNumRows)
	{
		sys_err("no result from item_attr");
		return false;
	}

	if (!m_vec_itemAttrTable.empty())
	{
		sys_log(0, "RELOAD: item_attr");
		m_vec_itemAttrTable.clear();
	}

	m_vec_itemAttrTable.reserve(pRes->uiNumRows);

	MYSQL_ROW	data;

	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		TItemAttrTable t;

		memset(&t, 0, sizeof(TItemAttrTable));

		int col = 0;

		strlcpy(t.szApply, data[col++], sizeof(t.szApply));
		str_to_number(t.dwApplyIndex, data[col++]);
		str_to_number(t.dwProb, data[col++]);
		str_to_number(t.lValues[0], data[col++]);
		str_to_number(t.lValues[1], data[col++]);
		str_to_number(t.lValues[2], data[col++]);
		str_to_number(t.lValues[3], data[col++]);
		str_to_number(t.lValues[4], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_WEAPON], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_BODY], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_WRIST], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_FOOTS], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_NECK], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_HEAD], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_SHIELD], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_EAR], data[col++]);

		sys_log(0, "ITEM_ATTR: %-20s %4lu { %3d %3d %3d %3d %3d } { %d %d %d %d %d %d %d }",
				t.szApply,
				t.dwProb,
				t.lValues[0],
				t.lValues[1],
				t.lValues[2],
				t.lValues[3],
				t.lValues[4],
				t.bMaxLevelBySet[ATTRIBUTE_SET_WEAPON],
				t.bMaxLevelBySet[ATTRIBUTE_SET_BODY],
				t.bMaxLevelBySet[ATTRIBUTE_SET_WRIST],
				t.bMaxLevelBySet[ATTRIBUTE_SET_FOOTS],
				t.bMaxLevelBySet[ATTRIBUTE_SET_NECK],
				t.bMaxLevelBySet[ATTRIBUTE_SET_HEAD],
				t.bMaxLevelBySet[ATTRIBUTE_SET_SHIELD],
				t.bMaxLevelBySet[ATTRIBUTE_SET_EAR]);

		m_vec_itemAttrTable.push_back(t);
	}

	return true;
}

bool CClientManager::InitializeItemRareTable()
{
	char query[4096];
	snprintf(query, sizeof(query),
			"SELECT apply, apply+0, prob, lv1, lv2, lv3, lv4, lv5, weapon, body, wrist, foots, neck, head, shield, ear FROM item_attr_rare%s ORDER BY apply",
			GetTablePostfix());

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	if (!pRes->uiNumRows)
	{
		sys_err("no result from item_attr_rare");
		return false;
	}

	if (!m_vec_itemRareTable.empty())
	{
		sys_log(0, "RELOAD: item_attr_rare");
		m_vec_itemRareTable.clear();
	}

	m_vec_itemRareTable.reserve(pRes->uiNumRows);

	MYSQL_ROW	data;

	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		TItemAttrTable t;

		memset(&t, 0, sizeof(TItemAttrTable));

		int col = 0;

		strlcpy(t.szApply, data[col++], sizeof(t.szApply));
		str_to_number(t.dwApplyIndex, data[col++]);
		str_to_number(t.dwProb, data[col++]);
		str_to_number(t.lValues[0], data[col++]);
		str_to_number(t.lValues[1], data[col++]);
		str_to_number(t.lValues[2], data[col++]);
		str_to_number(t.lValues[3], data[col++]);
		str_to_number(t.lValues[4], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_WEAPON], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_BODY], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_WRIST], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_FOOTS], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_NECK], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_HEAD], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_SHIELD], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_EAR], data[col++]);

		sys_log(0, "ITEM_RARE: %-20s %4lu { %3d %3d %3d %3d %3d } { %d %d %d %d %d %d %d }",
				t.szApply,
				t.dwProb,
				t.lValues[0],
				t.lValues[1],
				t.lValues[2],
				t.lValues[3],
				t.lValues[4],
				t.bMaxLevelBySet[ATTRIBUTE_SET_WEAPON],
				t.bMaxLevelBySet[ATTRIBUTE_SET_BODY],
				t.bMaxLevelBySet[ATTRIBUTE_SET_WRIST],
				t.bMaxLevelBySet[ATTRIBUTE_SET_FOOTS],
				t.bMaxLevelBySet[ATTRIBUTE_SET_NECK],
				t.bMaxLevelBySet[ATTRIBUTE_SET_HEAD],
				t.bMaxLevelBySet[ATTRIBUTE_SET_SHIELD],
				t.bMaxLevelBySet[ATTRIBUTE_SET_EAR]);

		m_vec_itemRareTable.push_back(t);
	}

	return true;
}

bool CClientManager::InitializeLandTable()
{
	using namespace building;

	char query[4096];

	snprintf(query, sizeof(query),
		"SELECT id, map_index, x, y, width, height, guild_id, guild_level_limit, price "
		"FROM land%s WHERE enable='YES' ORDER BY id",
		GetTablePostfix());

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	if (!m_vec_kLandTable.empty())
	{
		sys_log(0, "RELOAD: land");
		m_vec_kLandTable.clear();
	}

	m_vec_kLandTable.reserve(pRes->uiNumRows);

	MYSQL_ROW	data;

	if (pRes->uiNumRows > 0)
		while ((data = mysql_fetch_row(pRes->pSQLResult)))
		{
			TLand t;

			memset(&t, 0, sizeof(t));

			int col = 0;

			str_to_number(t.dwID, data[col++]);
			str_to_number(t.lMapIndex, data[col++]);
			str_to_number(t.x, data[col++]);
			str_to_number(t.y, data[col++]);
			str_to_number(t.width, data[col++]);
			str_to_number(t.height, data[col++]);
			str_to_number(t.dwGuildID, data[col++]);
			str_to_number(t.bGuildLevelLimit, data[col++]);
			str_to_number(t.dwPrice, data[col++]);

			sys_log(0, "LAND: %lu map %-4ld %7ldx%-7ld w %-4ld h %-4ld", t.dwID, t.lMapIndex, t.x, t.y, t.width, t.height);

			m_vec_kLandTable.push_back(t);
		}

	return true;
}

void parse_pair_number_string(const char * c_pszString, std::vector<std::pair<int, int> > & vec)
{
	// format: 10,1/20,3/300,50
	const char * t = c_pszString;
	const char * p = strchr(t, '/');
	std::pair<int, int> k;

	char szNum[32 + 1];
	char * comma;

	while (p)
	{
		if (isnhdigit(*t))
		{
			strlcpy(szNum, t, MIN(sizeof(szNum), (p-t)+1));

			comma = strchr(szNum, ',');

			if (comma)
			{
				*comma = '\0';
				str_to_number(k.second, comma+1);
			}
			else
				k.second = 0;

			str_to_number(k.first, szNum);
			vec.push_back(k);
		}

		t = p + 1;
		p = strchr(t, '/');
	}

	if (isnhdigit(*t))
	{
		strlcpy(szNum, t, sizeof(szNum));

		comma = strchr(const_cast<char*>(t), ',');

		if (comma)
		{
			*comma = '\0';
			str_to_number(k.second, comma+1);
		}
		else
			k.second = 0;

		str_to_number(k.first, szNum);
		vec.push_back(k);
	}
}

bool CClientManager::InitializeObjectProto()
{
	using namespace building;

	char query[4096];
	snprintf(query, sizeof(query),
			"SELECT vnum, price, materials, upgrade_vnum, upgrade_limit_time, life, reg_1, reg_2, reg_3, reg_4, npc, group_vnum, dependent_group "
			"FROM object_proto%s ORDER BY vnum",
			GetTablePostfix());

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	if (!m_vec_kObjectProto.empty())
	{
		sys_log(0, "RELOAD: object_proto");
		m_vec_kObjectProto.clear();
	}

	m_vec_kObjectProto.reserve(MAX(0, pRes->uiNumRows));

	MYSQL_ROW	data;

	if (pRes->uiNumRows > 0)
		while ((data = mysql_fetch_row(pRes->pSQLResult)))
		{
			TObjectProto t;

			memset(&t, 0, sizeof(t));

			int col = 0;

			str_to_number(t.dwVnum, data[col++]);
			str_to_number(t.dwPrice, data[col++]);

			std::vector<std::pair<int, int> > vec;
			parse_pair_number_string(data[col++], vec);

			for (unsigned int i = 0; i < OBJECT_MATERIAL_MAX_NUM && i < vec.size(); ++i)
			{
				std::pair<int, int> & r = vec[i];

				t.kMaterials[i].dwItemVnum = r.first;
				t.kMaterials[i].dwCount = r.second;
			}

			str_to_number(t.dwUpgradeVnum, data[col++]);
			str_to_number(t.dwUpgradeLimitTime, data[col++]);
			str_to_number(t.lLife, data[col++]);
			str_to_number(t.lRegion[0], data[col++]);
			str_to_number(t.lRegion[1], data[col++]);
			str_to_number(t.lRegion[2], data[col++]);
			str_to_number(t.lRegion[3], data[col++]);

			// ADD_BUILDING_NPC
			str_to_number(t.dwNPCVnum, data[col++]);
			str_to_number(t.dwGroupVnum, data[col++]);
			str_to_number(t.dwDependOnGroupVnum, data[col++]);

			t.lNPCX = 0;
			t.lNPCY = MAX(t.lRegion[1], t.lRegion[3])+300;
			// END_OF_ADD_BUILDING_NPC

			sys_log(0, "OBJ_PROTO: vnum %lu price %lu mat %lu %lu",
					t.dwVnum, t.dwPrice, t.kMaterials[0].dwItemVnum, t.kMaterials[0].dwCount);

			m_vec_kObjectProto.push_back(t);
		}

	return true;
}

bool CClientManager::InitializeObjectTable()
{
	using namespace building;

	char query[4096];
	snprintf(query, sizeof(query), "SELECT id, land_id, vnum, map_index, x, y, x_rot, y_rot, z_rot, life FROM object%s ORDER BY id", GetTablePostfix());

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	if (!m_map_pkObjectTable.empty())
	{
		sys_log(0, "RELOAD: object");
		m_map_pkObjectTable.clear();
	}

	MYSQL_ROW data;

	if (pRes->uiNumRows > 0)
		while ((data = mysql_fetch_row(pRes->pSQLResult)))
		{
			TObject * k = new TObject;

			memset(k, 0, sizeof(TObject));

			int col = 0;

			str_to_number(k->dwID, data[col++]);
			str_to_number(k->dwLandID, data[col++]);
			str_to_number(k->dwVnum, data[col++]);
			str_to_number(k->lMapIndex, data[col++]);
			str_to_number(k->x, data[col++]);
			str_to_number(k->y, data[col++]);
			str_to_number(k->xRot, data[col++]);
			str_to_number(k->yRot, data[col++]);
			str_to_number(k->zRot, data[col++]);
			str_to_number(k->lLife, data[col++]);

			sys_log(0, "OBJ: %lu vnum %lu map %-4ld %7ldx%-7ld life %ld", 
					k->dwID, k->dwVnum, k->lMapIndex, k->x, k->y, k->lLife);

			m_map_pkObjectTable.insert(std::make_pair(k->dwID, k));
		}

	return true;
}

bool CClientManager::InitializeMonarch()
{
	CMonarch::instance().LoadMonarch();

	return true;
}

bool CClientManager::MirrorMobTableIntoDB()
{
	for (itertype(m_vec_mobTable) it = m_vec_mobTable.begin(); it != m_vec_mobTable.end(); it++)
	{
		const TMobTable& t = *it;
		char query[4096];
		if (g_stLocaleNameColumn == "name")
		{
			snprintf(query, sizeof(query),
				"replace into mob_proto%s "
				"("
				"vnum, name, type, rank, battle_type, level, size, ai_flag, setRaceFlag, setImmuneFlag, "
				"on_click, empire, drop_item, resurrection_vnum, folder, "
				"st, dx, ht, iq, damage_min, damage_max, max_hp, regen_cycle, regen_percent, exp, "
				"gold_min, gold_max, def, attack_speed, move_speed, aggressive_hp_pct, aggressive_sight, attack_range, polymorph_item, "

				"enchant_curse, enchant_slow, enchant_poison, enchant_stun, enchant_critical, enchant_penetrate, "
				"resist_sword, resist_twohand, resist_dagger, resist_bell, resist_fan, resist_bow, "
				"resist_fire, resist_elect, resist_magic, resist_wind, resist_poison, "
				"dam_multiply, summon, drain_sp, "

				"skill_vnum0, skill_level0, skill_vnum1, skill_level1, skill_vnum2, skill_level2, "
				"skill_vnum3, skill_level3, skill_vnum4, skill_level4, "
				"sp_berserk, sp_stoneskin, sp_godspeed, sp_deathblow, sp_revive"
				") "
				"values ("

				"%d, \"%s\", %d, %d, %d, %d, %d, %u, %u, %u, " 
				"%d, %d, %d, %d, '%s', "
				"%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, "
				"%d, %d, %d, %d, %d, %d, %d, %d, %d, "

				"%d, %d, %d, %d, %d, %d, "
				"%d, %d, %d, %d, %d, %d, "
				"%d, %d, %d, %d, %d, "
				"%f, %d, %d, "

				"%d, %d, %d, %d, %d, %d, "
				"%d, %d, %d, %d, "
				"%d, %d, %d, %d, %d"
				")",
				GetTablePostfix(), /*g_stLocaleNameColumn.c_str(),*/

				t.dwVnum, t.szName, /*t.szLocaleName, */t.bType, t.bRank, t.bBattleType, t.bLevel, t.bSize, t.dwAIFlag, t.dwRaceFlag, t.dwImmuneFlag,
				t.bOnClickType, t.bEmpire, t.dwDropItemVnum, t.dwResurrectionVnum, t.szFolder,
				t.bStr, t.bDex, t.bCon, t.bInt, t.dwDamageRange[0], t.dwDamageRange[1], t.dwMaxHP, t.bRegenCycle, t.bRegenPercent, t.dwExp,

				t.dwGoldMin, t.dwGoldMax, t.wDef, t.sAttackSpeed, t.sMovingSpeed, t.bAggresiveHPPct, t.wAggressiveSight, t.wAttackRange, t.dwPolymorphItemVnum,
				t.cEnchants[0], t.cEnchants[1], t.cEnchants[2], t.cEnchants[3], t.cEnchants[4], t.cEnchants[5],
				t.cResists[0], t.cResists[1], t.cResists[2], t.cResists[3], t.cResists[4], t.cResists[5],
				t.cResists[6], t.cResists[7], t.cResists[8], t.cResists[9], t.cResists[10], 
				t.fDamMultiply, t.dwSummonVnum, t.dwDrainSP, 

				t.Skills[0].dwVnum, t.Skills[0].bLevel, t.Skills[1].dwVnum, t.Skills[1].bLevel, t.Skills[2].dwVnum, t.Skills[2].bLevel, 
				t.Skills[3].dwVnum, t.Skills[3].bLevel, t.Skills[4].dwVnum, t.Skills[4].bLevel, 
				t.bBerserkPoint, t.bStoneSkinPoint, t.bGodSpeedPoint, t.bDeathBlowPoint, t.bRevivePoint
				);
		}
		else
		{
			snprintf(query, sizeof(query),
				"replace into mob_proto%s "
				"("
				"vnum, name, %s, type, rank, battle_type, level, size, ai_flag, setRaceFlag, setImmuneFlag, "
				"on_click, empire, drop_item, resurrection_vnum, folder, "
				"st, dx, ht, iq, damage_min, damage_max, max_hp, regen_cycle, regen_percent, exp, "
				"gold_min, gold_max, def, attack_speed, move_speed, aggressive_hp_pct, aggressive_sight, attack_range, polymorph_item, "

				"enchant_curse, enchant_slow, enchant_poison, enchant_stun, enchant_critical, enchant_penetrate, "
				"resist_sword, resist_twohand, resist_dagger, resist_bell, resist_fan, resist_bow, "
				"resist_fire, resist_elect, resist_magic, resist_wind, resist_poison, "
				"dam_multiply, summon, drain_sp, "

				"skill_vnum0, skill_level0, skill_vnum1, skill_level1, skill_vnum2, skill_level2, "
				"skill_vnum3, skill_level3, skill_vnum4, skill_level4, "
				"sp_berserk, sp_stoneskin, sp_godspeed, sp_deathblow, sp_revive"
				") "
				"values ("

				"%d, \"%s\", \"%s\", %d, %d, %d, %d, %d, %u, %u, %u, " 
				"%d, %d, %d, %d, '%s', "
				"%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, "
				"%d, %d, %d, %d, %d, %d, %d, %d, %d, "

				"%d, %d, %d, %d, %d, %d, "
				"%d, %d, %d, %d, %d, %d, "
				"%d, %d, %d, %d, %d, "
				"%f, %d, %d, "

				"%d, %d, %d, %d, %d, %d, "
				"%d, %d, %d, %d, "
				"%d, %d, %d, %d, %d"
				")",
				GetTablePostfix(), g_stLocaleNameColumn.c_str(),

				t.dwVnum, t.szName, t.szLocaleName, t.bType, t.bRank, t.bBattleType, t.bLevel, t.bSize, t.dwAIFlag, t.dwRaceFlag, t.dwImmuneFlag,
				t.bOnClickType, t.bEmpire, t.dwDropItemVnum, t.dwResurrectionVnum, t.szFolder,
				t.bStr, t.bDex, t.bCon, t.bInt, t.dwDamageRange[0], t.dwDamageRange[1], t.dwMaxHP, t.bRegenCycle, t.bRegenPercent, t.dwExp,

				t.dwGoldMin, t.dwGoldMax, t.wDef, t.sAttackSpeed, t.sMovingSpeed, t.bAggresiveHPPct, t.wAggressiveSight, t.wAttackRange, t.dwPolymorphItemVnum,
				t.cEnchants[0], t.cEnchants[1], t.cEnchants[2], t.cEnchants[3], t.cEnchants[4], t.cEnchants[5],
				t.cResists[0], t.cResists[1], t.cResists[2], t.cResists[3], t.cResists[4], t.cResists[5],
				t.cResists[6], t.cResists[7], t.cResists[8], t.cResists[9], t.cResists[10], 
				t.fDamMultiply, t.dwSummonVnum, t.dwDrainSP, 

				t.Skills[0].dwVnum, t.Skills[0].bLevel, t.Skills[1].dwVnum, t.Skills[1].bLevel, t.Skills[2].dwVnum, t.Skills[2].bLevel, 
				t.Skills[3].dwVnum, t.Skills[3].bLevel, t.Skills[4].dwVnum, t.Skills[4].bLevel, 
				t.bBerserkPoint, t.bStoneSkinPoint, t.bGodSpeedPoint, t.bDeathBlowPoint, t.bRevivePoint
				);
		}

		CDBManager::instance().AsyncQuery(query);
	}
	return true;
}

bool CClientManager::MirrorItemTableIntoDB()
{
	for (itertype(m_vec_itemTable) it = m_vec_itemTable.begin(); it != m_vec_itemTable.end(); it++)
	{
		if (g_stLocaleNameColumn != "name")
		{
			const TItemTable& t = *it;
			char query[4096];
			snprintf(query, sizeof(query),
				"replace into item_proto%s ("
				"vnum, type, subtype, name, %s, gold, shop_buy_price, weight, size, "
				"flag, wearflag, antiflag, immuneflag, "
				"refined_vnum, refine_set, magic_pct, socket_pct, addon_type, "
				"limittype0, limitvalue0, limittype1, limitvalue1, "
				"applytype0, applyvalue0, applytype1, applyvalue1, applytype2, applyvalue2, "
				"value0, value1, value2, value3, value4, value5 ) "
				"values ("
				"%d, %d, %d, \"%s\", \"%s\", %d, %d, %d, %d, "
				"%d, %d, %d, %d, "
				"%d, %d, %d, %d, %d, "
				"%d, %d, %d, %d, "
				"%d, %d, %d, %d, %d, %d, "
				"%d, %d, %d, %d, %d, %d )",
				GetTablePostfix(), g_stLocaleNameColumn.c_str(), 
				t.dwVnum, t.bType, t.bSubType, t.szName, t.szLocaleName, t.dwGold, t.dwShopBuyPrice, t.bWeight, t.bSize,
				t.dwFlags, t.dwWearFlags, t.dwAntiFlags, t.dwImmuneFlag, 
				t.dwRefinedVnum, t.wRefineSet, t.bAlterToMagicItemPct, t.bGainSocketPct, t.sAddonType,
				t.aLimits[0].bType, t.aLimits[0].lValue, t.aLimits[1].bType, t.aLimits[1].lValue,
				t.aApplies[0].bType, t.aApplies[0].lValue, t.aApplies[1].bType, t.aApplies[1].lValue, t.aApplies[2].bType, t.aApplies[2].lValue,
				t.alValues[0], t.alValues[1], t.alValues[2], t.alValues[3], t.alValues[4], t.alValues[5]);
			CDBManager::instance().AsyncQuery(query);
		}
		else
		{
			const TItemTable& t = *it;
			char query[4096];
			snprintf(query, sizeof(query),
				"replace into item_proto%s ("
				"vnum, type, subtype, name, gold, shop_buy_price, weight, size, "
				"flag, wearflag, antiflag, immuneflag, "
				"refined_vnum, refine_set, magic_pct, socket_pct, addon_type, "
				"limittype0, limitvalue0, limittype1, limitvalue1, "
				"applytype0, applyvalue0, applytype1, applyvalue1, applytype2, applyvalue2, "
				"value0, value1, value2, value3, value4, value5 ) "
				"values ("
				"%d, %d, %d, \"%s\", %d, %d, %d, %d, "
				"%d, %d, %d, %d, "
				"%d, %d, %d, %d, %d, "
				"%d, %d, %d, %d, "
				"%d, %d, %d, %d, %d, %d, "
				"%d, %d, %d, %d, %d, %d )",
				GetTablePostfix(), 
				t.dwVnum, t.bType, t.bSubType, t.szName, t.dwGold, t.dwShopBuyPrice, t.bWeight, t.bSize,
				t.dwFlags, t.dwWearFlags, t.dwAntiFlags, t.dwImmuneFlag, 
				t.dwRefinedVnum, t.wRefineSet, t.bAlterToMagicItemPct, t.bGainSocketPct, t.sAddonType,
				t.aLimits[0].bType, t.aLimits[0].lValue, t.aLimits[1].bType, t.aLimits[1].lValue,
				t.aApplies[0].bType, t.aApplies[0].lValue, t.aApplies[1].bType, t.aApplies[1].lValue, t.aApplies[2].bType, t.aApplies[2].lValue,
				t.alValues[0], t.alValues[1], t.alValues[2], t.alValues[3], t.alValues[4], t.alValues[5]);
			CDBManager::instance().AsyncQuery(query);
		}
	}
	return true;
}
