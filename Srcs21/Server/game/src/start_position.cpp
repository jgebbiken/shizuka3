#include "stdafx.h"
#include "start_position.h"


char g_nation_name[4][32] =
{
	"",
	"�ż���",
	"õ����",
	"���뱹",
};

//	LC_TEXT("�ż���")
//	LC_TEXT("õ����")
//	LC_TEXT("���뱹")

long g_start_map[4] =
{
	0,	// reserved
	3,	// �ż���
	23,	// õ����
	43	// ���뱹
};

DWORD g_start_position[4][2] =
{
	{      0,      0 },	// reserved
	{ 469300, 964200 },	// �ż���
	{  55700, 157900 },	// õ����
	{ 969600, 278400 }	// ���뱹
};


DWORD arena_return_position[4][2] =
{
	{       0,  0       },
	{   347600, 882700  }, // �ھ���
	{   138600, 236600  }, // ������
	{   857200, 251800  }  // �ڶ���
};


DWORD g_create_position[4][2] = 
{
	{		0,		0 },
	{ 358800, 876600 },
	{ 141200, 231500 },
	{ 874200, 250700 },	
};

DWORD g_create_position_canada[4][2] = 
{
	{		0,		0 },
	{ 457100, 946900 },
	{ 45700, 166500 },
	{ 966300, 288300 },	
};

