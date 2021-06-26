
#include <d3d9.h>
#include <d3dx9core.h>
#include<windows.h>
#include <Psapi.h>
#include <time.h>
#include <TlHelp32.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "d3d9.lib")
using namespace std;

typedef IDirect3D9* (__stdcall *DIRECT3DCREATE9)(unsigned int);
typedef long(__stdcall *PRESENT9)(IDirect3DDevice9* self, const RECT*, const RECT*, HWND, void*);

// для перехвата функции DirectX
PRESENT9 g_D3D9_Present = 0;
BYTE g_codeFragment_p9[5] = { 0, 0, 0, 0, 0 };
BYTE g_jmp_p9[5] = { 0, 0, 0, 0, 0 };
DWORD g_savedProtection_p9 = 0;
DWORD present9 = 0;
//индикатор определяющий нажатую клавишу
bool indicator = false;
bool indicator1 = false;

//Для изображения текста на экране
D3DRECT rec = { 0, 0, 0, 0 };
ID3DXFont *m_font = 0;
ID3DXFont *m_font1 = 0;
RECT fontRect = { 10, 15, 120, 120 };
RECT fontRect1 = { 50,15,350,120 };
D3DCOLOR bkgColor = 0;
D3DCOLOR fontColor = 0;

//Массивы координат от DirectX для изображения ESP на экране
D3DXVECTOR2 treangle1[5];
D3DXVECTOR2 treangle2[5];
D3DXVECTOR2 MyToch[5];
D3DXVECTOR2 HMT[5];
D3DXVECTOR2 square[4];
D3DXVECTOR2 hands[2];
//Цвет от DirectX для изображения ESP на экране
D3DCOLOR cool = D3DCOLOR_ARGB(255, 255, 50, 0);
D3DCOLOR pool = D3DCOLOR_ARGB(255, 0, 0, 255);
D3DCOLOR myt = D3DCOLOR_ARGB(255, 255, 0, 0); // точка , цель
D3DCOLOR myt1 = D3DCOLOR_ARGB(255, 0, 0, 0);
D3DCOLOR ColorHead = D3DCOLOR_ARGB(255, 255, 255, 0);
ID3DXLine *line;

//Глобальные переменные
DWORD client_dll;
DWORD engine_dll;
DWORD server_dll;
HANDLE HProc;
DWORD ProcId;
DWORD addr_client;
DWORD client_for_head;
RECT rect;
HWND hWnd;
HDC hDC;
HANDLE hProcess;

//оффсеты для цветных координат(прямые линии(цвет(синий, красный)))
const DWORD struct_onl_play_of = 0x4035C0;
const DWORD hp_onl_of = 0x19c;
const DWORD Team_onl_of = 0x198;
const DWORD player_onl_coordX = 0x1a0;
const DWORD player_onl_coordY = 0x1a4;
const DWORD player_onl_coordZ = 0x1a8;

//оффсеты для матрицы костей
const DWORD client_stuct_players = 0x3CF214;	
const DWORD distance_of_player = 0x10; 

//оффсет для видовой матрицы
const DWORD v_matrix_on = 0x4CF20C;//4CF20C

//втроенный вх в игру =)
DWORD fun_offset = 0x3B0C9C;

//для видовой матрицы персонажа
typedef struct D3MATRIX
{
	float m[4][4];
}*PD3MATRIX;
PD3MATRIX ViewMatrix;

//интерфейсный класс
class Interface
{
protected:
	int privileges;//(пока ни на что не влияет) в будущем для больших функций 0-нет , 1-да.  ( -если они будут=)- )
public:
	Interface(): privileges(0) {}
	virtual void FindModule() = 0;
	virtual int WorldToScreen(float* coord, D3DVIEWPORT9 viewport, float* pOut, float* Rost) = 0;
	virtual void ShowPos(IDirect3DDevice9* device) = 0;
	virtual void AimBot() = 0;//в будущем
};

struct PlayerEngineEsp : public Interface
{
	PlayerEngineEsp() {}
	void FindModule()
	{
		MODULEINFO ModuleInfoEngine = { 0 };
		HMODULE hModuleEngine = GetModuleHandle("engine.dll");
		GetModuleInformation(GetCurrentProcess(), hModuleEngine, &ModuleInfoEngine, sizeof(MODULEINFO));
		engine_dll = (DWORD)ModuleInfoEngine.lpBaseOfDll;//lpBaseOfDll; //будет содержать адрес начала engine.dll
	}

	void FindViewMatrix()
	{
		ViewMatrix = (PD3MATRIX)(engine_dll + v_matrix_on);
	}
	int WorldToScreen(float* coord, D3DVIEWPORT9 viewport, float* pOut, float* Rost) { return 0; }
	void ShowPos(IDirect3DDevice9* device) {}
	void AimBot(){}
	virtual ~PlayerEngineEsp() {}
};

class PlayerClientEsp : virtual public Interface
{
public:
	PlayerClientEsp(int priv = 0) { privileges = priv; }

	void FindModule()
	{
		MODULEINFO ModuleInfoClient = { 0 };
		HMODULE hModuleClient = GetModuleHandle("client.dll");
		GetModuleInformation(GetCurrentProcess(), hModuleClient, &ModuleInfoClient, sizeof(MODULEINFO));
		client_dll = (DWORD)ModuleInfoClient.lpBaseOfDll;//lpBaseOfDll; //будет содержать адрес начала client.dll // для сканера сигнатур(в будущем(просто моя игра не обновляется, но изучить стоит))
	}

	int WorldToScreen(float* coord, D3DVIEWPORT9 viewport, float* pOut, float* Rost)
	{
		float w = 0.0f;
		pOut[0] = ViewMatrix->m[0][0] * (coord[0] - 0) + ViewMatrix->m[0][1] * coord[1] + ViewMatrix->m[0][2] * coord[2] + ViewMatrix->m[0][3];
		pOut[1] = ViewMatrix->m[1][0] * (coord[0] - 0) + ViewMatrix->m[1][1] * coord[1] + ViewMatrix->m[1][2] * coord[2] + ViewMatrix->m[1][3];
		w = ViewMatrix->m[3][0] * (coord[0] - 0) + ViewMatrix->m[3][1] * coord[1] + ViewMatrix->m[3][2] * coord[2] + ViewMatrix->m[3][3];
		if (w < 0.01f) { return 0; }
		float intw = 1.0f / w;
		pOut[0] *= intw;
		pOut[1] *= intw;
		int weight = (int)(viewport.Width);//(rect.right - rect.left);
		int height = (int)(viewport.Height);//(rect.bottom - rect.top);
		float x = weight / 2;
		float y = height / 2;
		x += 0.5*pOut[0] * weight + 0.5;
		y -= 0.5*pOut[1] * height + 0.5;
		pOut[0] = x;//+ rect.left
		pOut[1] = y;//+ rect.top
					//----------------------------------------------=-------------
		float w1 = 0.0f;
		Rost[0] = ViewMatrix->m[0][0] * (coord[0] - 0) + ViewMatrix->m[0][1] * coord[1] + ViewMatrix->m[0][2] * (coord[2] - 70) + ViewMatrix->m[0][3];
		Rost[1] = ViewMatrix->m[1][0] * (coord[0] - 0) + ViewMatrix->m[1][1] * coord[1] + ViewMatrix->m[1][2] * (coord[2] - 70) + ViewMatrix->m[1][3];
		w1 = ViewMatrix->m[3][0] * (coord[0] - 0) + ViewMatrix->m[3][1] * coord[1] + ViewMatrix->m[3][2] * (coord[2] - 70) + ViewMatrix->m[3][3];
		if (w1 < 0.01f) { return 0; }
		float intw1 = 1.0f / w1;
		Rost[0] *= intw1;
		Rost[1] *= intw1;
		int weight1 = (int)(viewport.Width);//(rect.right - rect.left);
		int height1 = (int)(viewport.Height);//(rect.bottom - rect.top);
		float x1 = weight1 / 2;
		float y1 = height1 / 2;
		x1 += 0.5*Rost[0] * weight1 + 0.5;
		y1 -= 0.5*Rost[1] * height1 + 0.5;
		Rost[0] = x1;//+ rect.left
		Rost[1] = y1;//+ rect.top
					 //----------------------------------------------=-------------
		float w2 = 0.0f;
		pOut[2] = ViewMatrix->m[0][0] * (coord[0] + 0) + ViewMatrix->m[0][1] * coord[1] + ViewMatrix->m[0][2] * (coord[2] - 70) + ViewMatrix->m[0][3];
		pOut[3] = ViewMatrix->m[1][0] * (coord[0] + 0) + ViewMatrix->m[1][1] * coord[1] + ViewMatrix->m[1][2] * (coord[2] - 70) + ViewMatrix->m[1][3];
		w2 = ViewMatrix->m[3][0] * (coord[0] + 0) + ViewMatrix->m[3][1] * coord[1] + ViewMatrix->m[3][2] * (coord[2] - 70) + ViewMatrix->m[3][3];
		if (w2 < 0.01f) { return 0; }
		float intw2 = 1.0f / w2;
		pOut[2] *= intw2;
		pOut[3] *= intw2;
		int weight2 = (int)(viewport.Width);//(rect.right - rect.left);
		int height2 = (int)(viewport.Height);//(rect.bottom - rect.top);
		float x2 = weight2 / 2;
		float y2 = height2 / 2;
		x2 += 0.5*pOut[2] * weight2 + 0.5;
		y2 -= 0.5*pOut[3] * height2 + 0.5;
		pOut[2] = x2;//+ rect.left
		pOut[3] = y2;//+ rect.top
					 //----------------------------------------------=-------------
		float w3 = 0.0f;
		Rost[2] = ViewMatrix->m[0][0] * (coord[0] + 0) + ViewMatrix->m[0][1] * coord[1] + ViewMatrix->m[0][2] * coord[2] + ViewMatrix->m[0][3];
		Rost[3] = ViewMatrix->m[1][0] * (coord[0] + 0) + ViewMatrix->m[1][1] * coord[1] + ViewMatrix->m[1][2] * coord[2] + ViewMatrix->m[1][3];
		w3 = ViewMatrix->m[3][0] * (coord[0] + 0) + ViewMatrix->m[3][1] * coord[1] + ViewMatrix->m[3][2] * coord[2] + ViewMatrix->m[3][3];
		if (w3 < 0.01f) { return 0; }
		float intw3 = 1.0f / w3;
		Rost[2] *= intw3;
		Rost[3] *= intw3;
		int weight3 = (int)(viewport.Width);//(rect.right - rect.left);
		int height3 = (int)(viewport.Height);//(rect.bottom - rect.top);
		float x3 = weight1 / 2;
		float y3 = height1 / 2;
		x3 += 0.5*Rost[2] * weight3 + 0.5;
		y3 -= 0.5*Rost[3] * height3 + 0.5;
		Rost[2] = x3;//+ rect.left
		Rost[3] = y3;//+ rect.top
					 //----------------------------------------------=-------------
		return 1;
	}

	void ShowPos(IDirect3DDevice9* device)
	{
		addr_client = *reinterpret_cast<DWORD*>(client_dll + struct_onl_play_of);
		int hp, team_play;
		D3DVIEWPORT9 viewport;
		device->GetViewport(&viewport);
		float coord[3], pOut[4], Rost[4];
		for (int i = 0; i < 34; i++)
		{
			coord[0] = *reinterpret_cast<float*>(addr_client + player_onl_coordX + i * 0x140);
		    coord[1] = *reinterpret_cast<float*>(addr_client + player_onl_coordY + i * 0x140);
			coord[2] = *reinterpret_cast<float*>(addr_client + player_onl_coordZ + i * 0x140);//за высоту 
			if ((coord[0] == 0) || (coord[1] == 0)) { continue; }
			hp = *reinterpret_cast<int*>(addr_client + hp_onl_of + i * 0x140);
			if ((WorldToScreen(coord, viewport, pOut, Rost) == 1) && (hp > 0))
			{
				team_play = *reinterpret_cast<int*>(addr_client + Team_onl_of + i * 0x140);
				if (team_play == 3)
				{
					treangle2[0] = D3DXVECTOR2(pOut[0] - 10, pOut[1]);
					treangle2[1] = D3DXVECTOR2(Rost[0] - 10, Rost[1]);
					treangle2[2] = D3DXVECTOR2(pOut[2] + 10, pOut[3]);
					treangle2[3] = D3DXVECTOR2(Rost[2] + 10, Rost[3]);
					treangle2[4] = D3DXVECTOR2(pOut[0] - 10, pOut[1]);
					line->Begin();
					line->Draw(treangle2, 5, pool);
					line->End();
				}
				else if (team_play == 2)
				{
					treangle1[0] = D3DXVECTOR2(pOut[0] - 10, pOut[1]);
					treangle1[1] = D3DXVECTOR2(Rost[0] - 10, Rost[1]);
					treangle1[2] = D3DXVECTOR2(pOut[2] + 10, pOut[3]);
					treangle1[3] = D3DXVECTOR2(Rost[2] + 10, Rost[3]);
					treangle1[4] = D3DXVECTOR2(pOut[0] - 10, pOut[1]);
					line->Begin();
					line->Draw(treangle1, 5, cool);
					line->End();
				}
			}
		}
	}

	void AimBot(){}

	virtual ~PlayerClientEsp(){}
};

class PlayerClientHead : public Interface
{
	D3DCOLOR color;
public:
	PlayerClientHead(int priv = 0) { privileges = priv; }

	void FindModule(){}//уже нашли 

	void SetColor(int x, int y, int z) { color = D3DCOLOR_ARGB(255, x, y, z); }

	int WorldToScreen(float* coord, D3DVIEWPORT9 viewport, float* pOut, float* Rost)
	{
		float w = 0.0f;
		pOut[0] = ViewMatrix->m[0][0] * (coord[0] - 5) + ViewMatrix->m[0][1] * coord[1] + ViewMatrix->m[0][2] * coord[2] + ViewMatrix->m[0][3];
		pOut[1] = ViewMatrix->m[1][0] * (coord[0] - 5) + ViewMatrix->m[1][1] * coord[1] + ViewMatrix->m[1][2] * coord[2] + ViewMatrix->m[1][3];
		w = ViewMatrix->m[3][0] * (coord[0] - 5) + ViewMatrix->m[3][1] * coord[1] + ViewMatrix->m[3][2] * coord[2] + ViewMatrix->m[3][3];
		if (w < 0.01f) { return 0; }
		float intw = 1.0f / w;
		pOut[0] *= intw;
		pOut[1] *= intw;
		int weight = (int)(viewport.Width);//(rect.right - rect.left);
		int height = (int)(viewport.Height);//(rect.bottom - rect.top);
		float x = weight / 2;
		float y = height / 2;
		x += 0.5*pOut[0] * weight + 0.5;
		y -= 0.5*pOut[1] * height + 0.5;
		pOut[0] = x;//+ rect.left
		pOut[1] = y;//+ rect.top
					//----------------------------------------------=-------------
		float w1 = 0.0f;
		Rost[0] = ViewMatrix->m[0][0] * (coord[0] - 5) + ViewMatrix->m[0][1] * coord[1] + ViewMatrix->m[0][2] * (coord[2] + 10) + ViewMatrix->m[0][3];
		Rost[1] = ViewMatrix->m[1][0] * (coord[0] - 5) + ViewMatrix->m[1][1] * coord[1] + ViewMatrix->m[1][2] * (coord[2] + 10) + ViewMatrix->m[1][3];
		w1 = ViewMatrix->m[3][0] * (coord[0] - 5) + ViewMatrix->m[3][1] * coord[1] + ViewMatrix->m[3][2] * (coord[2] + 10) + ViewMatrix->m[3][3];
		if (w1 < 0.01f) { return 0; }
		float intw1 = 1.0f / w1;
		Rost[0] *= intw1;
		Rost[1] *= intw1;
		int weight1 = (int)(viewport.Width);//(rect.right - rect.left);
		int height1 = (int)(viewport.Height);//(rect.bottom - rect.top);
		float x1 = weight1 / 2;
		float y1 = height1 / 2;
		x1 += 0.5*Rost[0] * weight1 + 0.5;
		y1 -= 0.5*Rost[1] * height1 + 0.5;
		Rost[0] = x1;//+ rect.left
		Rost[1] = y1;//+ rect.top
					 //----------------------------------------------=-------------
		float w2 = 0.0f;
		pOut[2] = ViewMatrix->m[0][0] * (coord[0] + 5) + ViewMatrix->m[0][1] * coord[1] + ViewMatrix->m[0][2] * (coord[2] + 10) + ViewMatrix->m[0][3];
		pOut[3] = ViewMatrix->m[1][0] * (coord[0] + 5) + ViewMatrix->m[1][1] * coord[1] + ViewMatrix->m[1][2] * (coord[2] + 10) + ViewMatrix->m[1][3];
		w2 = ViewMatrix->m[3][0] * (coord[0] + 5) + ViewMatrix->m[3][1] * coord[1] + ViewMatrix->m[3][2] * (coord[2] + 10) + ViewMatrix->m[3][3];
		if (w2 < 0.01f) { return 0; }
		float intw2 = 1.0f / w2;
		pOut[2] *= intw2;
		pOut[3] *= intw2;
		int weight2 = (int)(viewport.Width);//(rect.right - rect.left);
		int height2 = (int)(viewport.Height);//(rect.bottom - rect.top);
		float x2 = weight2 / 2;
		float y2 = height2 / 2;
		x2 += 0.5*pOut[2] * weight2 + 0.5;
		y2 -= 0.5*pOut[3] * height2 + 0.5;
		pOut[2] = x2;//+ rect.left
		pOut[3] = y2;//+ rect.top
					 //----------------------------------------------=-------------
		float w3 = 0.0f;
		Rost[2] = ViewMatrix->m[0][0] * (coord[0] + 5) + ViewMatrix->m[0][1] * coord[1] + ViewMatrix->m[0][2] * coord[2] + ViewMatrix->m[0][3];
		Rost[3] = ViewMatrix->m[1][0] * (coord[0] + 5) + ViewMatrix->m[1][1] * coord[1] + ViewMatrix->m[1][2] * coord[2] + ViewMatrix->m[1][3];
		w3 = ViewMatrix->m[3][0] * (coord[0] + 5) + ViewMatrix->m[3][1] * coord[1] + ViewMatrix->m[3][2] * coord[2] + ViewMatrix->m[3][3];
		if (w3 < 0.01f) { return 0; }
		float intw3 = 1.0f / w3;
		Rost[2] *= intw3;
		Rost[3] *= intw3;
		int weight3 = (int)(viewport.Width);//(rect.right - rect.left);
		int height3 = (int)(viewport.Height);//(rect.bottom - rect.top);
		float x3 = weight1 / 2;
		float y3 = height1 / 2;
		x3 += 0.5*Rost[2] * weight3 + 0.5;
		y3 -= 0.5*Rost[3] * height3 + 0.5;
		Rost[2] = x3;//+ rect.left
		Rost[3] = y3;//+ rect.top
					 //----------------------------------------------=-------------
		return 1;
	}

	void ShowPos(IDirect3DDevice9* device)
	{
		D3DVIEWPORT9 viewport;
		device->GetViewport(&viewport);
		DWORD adrhh, coorbody;
		float HeadCoor[3], coord[3], hand1[3], hand2[3];
		float pOut[4];//надо сделать квадрат 
		float Rost[4];
		int hpl;
		for (int i = 1; i < 34; i++)//34
		{
			adrhh = *reinterpret_cast<DWORD*>(client_dll + (client_stuct_players + 0x08 * i));// i
			if (adrhh <= 1) { continue; }
			hpl = *reinterpret_cast<int*>(adrhh + 0xd9c);//было 0xda4 
			if (hpl <= 0) { continue; }
			coorbody = *reinterpret_cast<DWORD*>(adrhh + 0x4a0);//4a8
			HeadCoor[0] = *reinterpret_cast<float*>(coorbody + 0x2ac);//0x93c//0x2ac// 0x30c+//0x5dc+
			HeadCoor[1] = *reinterpret_cast<float*>(coorbody + 0x2bc);//0x94c//0x2bc// 0x31c+//0x5ec+
			HeadCoor[2] = *reinterpret_cast<float*>(coorbody + 0x2cc);//0x95c//0x2cc// 0x32c+//0x5fc+
			if (HeadCoor[0] == 0 || HeadCoor[1] == 0 || HeadCoor[2] == 0) { continue; }
			coord[0] = *reinterpret_cast<float*>(adrhh + 0x1e4);
			coord[1] = *reinterpret_cast<float*>(adrhh + 0x1e8);
			coord[2] = *reinterpret_cast<float*>(adrhh + 0x1ec);
			if ((coord[0] == 0) || (coord[1] == 0)) { continue; }
			if ((HeadCoor[0] > coord[0] + 30) || (HeadCoor[0] < coord[0] - 30))
			{
				HeadCoor[0] = 0; coord[0] = 0;
				HeadCoor[1] = 0; coord[1] = 0;
				HeadCoor[2] = 0; coord[2] = 0;
			}
			if ((hpl != 1) && (WorldToScreen(HeadCoor, viewport, pOut, Rost) == 1))
			{
				square[0] = D3DXVECTOR2(pOut[0], pOut[1]);
				square[1] = D3DXVECTOR2(Rost[0], Rost[1]);
				square[2] = D3DXVECTOR2(pOut[2], pOut[3]);
				square[3] = D3DXVECTOR2(Rost[2], Rost[3]);
				line->Begin();
				line->Draw(square, 4, color);//4
				line->End();
			}
		}
	}

	void AimBot() {}

	virtual ~PlayerClientHead() {}
};

class PlayerClientСourse : public Interface
{
	D3DCOLOR color;
public:
	PlayerClientСourse(int priv = 0) { privileges = priv; }

	void FindModule() {}

	void SetColor(int x, int y, int z) { color = D3DCOLOR_ARGB(255, x, y, z); }

	int WorldToScreen(float* coord, D3DVIEWPORT9 viewport, float* pOut, float* Rost)
	{
		float w = 0.0f;
		pOut[0] = ViewMatrix->m[0][0] * coord[0] + ViewMatrix->m[0][1] * coord[1] + ViewMatrix->m[0][2] * coord[2] + ViewMatrix->m[0][3];
		pOut[1] = ViewMatrix->m[1][0] * coord[0] + ViewMatrix->m[1][1] * coord[1] + ViewMatrix->m[1][2] * coord[2] + ViewMatrix->m[1][3];
		w = ViewMatrix->m[3][0] * coord[0] + ViewMatrix->m[3][1] * coord[1] + ViewMatrix->m[3][2] * coord[2] + ViewMatrix->m[3][3];
		if (w < 0.01f) { return 0; }
		float intw = 1.0f / w;
		pOut[0] *= intw;
		pOut[1] *= intw;
		int weight = (int)(viewport.Width);
		int height = (int)(viewport.Height);
		float x = weight / 2;
		float y = height / 2;
		x += 0.5*pOut[0] * weight + 0.5;
		y -= 0.5*pOut[1] * height + 0.5;
		pOut[0] = x;
		pOut[1] = y;
					//----------------------------------------------=-------------
		float help[3];
		float w1 = 0.0f;
		help[0] = ViewMatrix->m[0][0] * (Rost[0] + 5) + ViewMatrix->m[0][1] * (Rost[1] - 5) + ViewMatrix->m[0][2] * (Rost[2] + 3) + ViewMatrix->m[0][3];
		help[1] = ViewMatrix->m[1][0] * (Rost[0] + 5) + ViewMatrix->m[1][1] * (Rost[1] - 5) + ViewMatrix->m[1][2] * (Rost[2] + 3) + ViewMatrix->m[1][3];
		w1 = ViewMatrix->m[3][0] * (Rost[0] + 5) + ViewMatrix->m[3][1] * (Rost[1] - 5) + ViewMatrix->m[3][2] * (Rost[2] + 3) + ViewMatrix->m[3][3];
		if (w1 < 0.01f) { return 0; }
		float intw1 = 1.0f / w1;
		help[0] *= intw1;
		help[1] *= intw1;
		int weight1 = (int)(viewport.Width);
		int height1 = (int)(viewport.Height);
		float x1 = weight1 / 2;
		float y1 = height1 / 2;
		x1 += 0.5*help[0] * weight1 + 0.5;
		y1 -= 0.5*help[1] * height1 + 0.5;
		help[0] = x1;
		help[1] = y1;
		Rost[0] = help[0];
		Rost[1] = help[1];
		return 1;
	}

	void ShowPos(IDirect3DDevice9* device)
	{
		D3DVIEWPORT9 viewport;
		device->GetViewport(&viewport);
		DWORD adrhh, coorbody;
		float coord[3], hand1[3], hand2[3];
		float pOut[2];
		//float Rost[2];
		int hpl;
		
		for (int i = 1; i < 34; i++)//34
		{
			adrhh = *reinterpret_cast<DWORD*>(client_dll + (client_stuct_players + 0x08 * i));
			if (adrhh <= 1) { continue; }
			hpl = *reinterpret_cast<int*>(adrhh + 0xd9c);//было 0xda4 
			if (hpl <= 0) { continue; }
			coorbody = *reinterpret_cast<DWORD*>(adrhh + 0x4a0);//4a8
			hand1[0] = *reinterpret_cast<float*>(coorbody + 0x57c);//0x93c//0x2ac// 0x30c+//0x5dc+
			hand1[1] = *reinterpret_cast<float*>(coorbody + 0x58c);//0x94c//0x2bc// 0x31c+//0x5ec+
			hand1[2] = *reinterpret_cast<float*>(coorbody + 0x59c);//0x95c//0x2cc// 0x32c+//0x5fc+

			hand2[0] = *reinterpret_cast<float*>(coorbody + 0x5dc);//0x93c//0x2ac// 0x30c+//0x5dc+			 //0x1ec
			hand2[1] = *reinterpret_cast<float*>(coorbody + 0x5ec);//0x94c//0x2bc// 0x31c+//0x5ec+не подходит//0x1fc
			hand2[2] = *reinterpret_cast<float*>(coorbody + 0x5fc);//0x95c//0x2cc// 0x32c+//0x5fc+			 //0x20c

			if (hand1[0] == 0 || hand1[1] == 0 || hand1[2] == 0) { continue; }
			coord[0] = *reinterpret_cast<float*>(adrhh + 0x1e4);
			coord[1] = *reinterpret_cast<float*>(adrhh + 0x1e8);
			coord[2] = *reinterpret_cast<float*>(adrhh + 0x1ec);
			if ((coord[0] == 0) || (coord[1] == 0)) { continue; }
			if ((hand1[0] > coord[0] + 30) || (hand1[0] < coord[0] - 30))
			{
				hand1[0] = 0; hand2[0] = 0;
				hand1[1] = 0; hand2[1] = 0;
				hand1[2] = 0; hand2[2] = 0;
			}
			if ((hpl != 1) && (WorldToScreen(hand1, viewport, pOut, hand2) == 1))
			{
				hands[0] = D3DXVECTOR2(pOut[0], pOut[1]);
				hands[1] = D3DXVECTOR2(hand2[0], hand2[1]);
				line->Begin();
				line->Draw(hands, 2, color);//4
				line->End();
			}
		}
	}

	void AimBot() {}

	virtual ~PlayerClientСourse() {}
};


class PolimorfCheatOn //Чтобы воспользоваться классом, объекты должны поддерживать интерфейс Iterface
{
	Interface** In;//Массив указателей Iterface*
	int n;//Текущее количество указателей в массиве
	int N;//Размерность массива

public:
	
	PolimorfCheatOn(int Nfig) : N(Nfig), n(0)
	{ 
		In = new Interface*[N]; 
	}

	//virtual void FindModulePolim() { In->FindModule(); }//так как ищем до if(indicator)
	void Insert(Interface* pF)
	{ 
		if (n < N)
		{
			In[n++] = pF;
		}
	}
	virtual void ShowPosPolim(IDirect3DDevice9* device)//Полиморфизм
	{ 
		for (int i = 0; i < n; i++)
		{
			In[i]->ShowPos(device);
		}
	}
	virtual void AimBot() //Полиморфизм
	{ 
		for (int i = 0; i < n; i++)
		{
			In[i]->AimBot();
		}
	}
	void PlayPlayer(IDirect3DDevice9* device)
	{
		ShowPosPolim(device); AimBot();
	}
	virtual ~PolimorfCheatOn() { delete[]In; }
};

void ItIsFunny()
{
	DWORD clfun = 0;
	DWORD ProtectOne = 0;
	int mas[] = {2};
	if (GetAsyncKeyState(VK_F9))
	{
		clfun = (DWORD)(client_dll + fun_offset);
		VirtualProtect((void*)clfun, 1, PAGE_EXECUTE_READWRITE, &ProtectOne);
		memcpy((void*)clfun, mas, 1);
		VirtualProtect((void*)clfun, 1, ProtectOne, &ProtectOne);
		Beep(500, 400);
	}
}

void ItIsNotFunny()
{
	DWORD clfun = 0;
	DWORD ProtectOne = 0;
	int mas[] = { 1 };
	if (GetAsyncKeyState(VK_F11))
	{
		clfun = (DWORD)(client_dll + fun_offset);
		VirtualProtect((void*)clfun, 1, PAGE_EXECUTE_READWRITE, &ProtectOne);
		memcpy((void*)clfun, mas, 1);
		VirtualProtect((void*)clfun, 1, ProtectOne, &ProtectOne);
		Beep(500, 400);
	}
}

void MyDisplay(D3DVIEWPORT9 viewport)
{
	float TocOut[3], TocOut2[3];
	int weidtht = (int)(viewport.Width);
	int heightt = (int)(viewport.Height);
	int Wt = weidtht / 2;
	int Ht = heightt / 2;
	TocOut[0] = Wt;
	TocOut[1] = Ht;
	TocOut2[0] = Wt;
	TocOut2[1] = Ht;
	MyToch[0] = D3DXVECTOR2(TocOut[0] - 1, TocOut[1] - 1);
	MyToch[1] = D3DXVECTOR2(TocOut[0] + 1, TocOut[1] - 1);
	MyToch[2] = D3DXVECTOR2(TocOut[0] + 1, TocOut[1] + 1);
	MyToch[3] = D3DXVECTOR2(TocOut[0] - 1, TocOut[1] + 1);
	MyToch[4] = D3DXVECTOR2(TocOut[0] - 1, TocOut[1] - 1);

	HMT[0] = D3DXVECTOR2(TocOut2[0] - 2, TocOut2[1] - 2);
	HMT[1] = D3DXVECTOR2(TocOut2[0] + 2, TocOut2[1] - 2);
	HMT[2] = D3DXVECTOR2(TocOut2[0] + 2, TocOut2[1] + 2);
	HMT[3] = D3DXVECTOR2(TocOut2[0] - 2, TocOut2[1] + 2);
	HMT[4] = D3DXVECTOR2(TocOut2[0] - 2, TocOut2[1] - 2);

	line->Begin();
	line->Draw(MyToch, 5, myt);
	line->Draw(HMT, 5, myt1);
	line->End();
}

// перехват функции DirectX и её дальнейшее использование - начало
class InterceptionDirectX
{
public:
	InterceptionDirectX() {}
	static DWORD __stdcall TF(void* lpParam)
	{
		GetDevice9Methods();
		HookDevice9Methods();
		return 0;
	}
	
	static DWORD __stdcall KeyboardHook(void* lpParam)
	{
		while (1)
		{
			if (GetAsyncKeyState(VK_F3))
			{
				indicator = !indicator;
				//Beep(500, 200);
			}
			Sleep(100);
		}
	}

	static DWORD __stdcall KeyboardHook1(void* lpParam)
	{
		while (1)
		{
			if (GetAsyncKeyState(VK_F5))
			{
				indicator1 = !indicator1;
				Beep(500, 200);
			}
			Sleep(100);
		}
	}

	void DrawIndicator(void* self)
	{	
		PlayerEngineEsp PEE;
		PlayerClientEsp PCE;
		PlayerClientHead PCH;    //<---
		PlayerClientСourse PCC;  //<---

		PCH.SetColor(255, 255, 0);
		PCC.SetColor(255, 255, 0);
		PCE.FindModule();
		PEE.FindModule(); 
		PEE.FindViewMatrix();

		IDirect3DDevice9* dev = (IDirect3DDevice9*)self;
		dev->BeginScene();
		//D3DXCreateFont(dev, 16, 0, FW_NORMAL, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, 0 | FF_DONTCARE, TEXT("Arial"), &m_font);
		D3DXCreateFont(dev, 16, 0, FW_NORMAL, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, 0 | FF_DONTCARE, TEXT("Arial"), &m_font1);

		D3DVIEWPORT9 viewport;
		dev->GetViewport(&viewport);

		D3DXCreateLine(dev, &line);
		line->SetWidth(2.0f);
		line->SetPattern(0xffffffff);
		line->SetAntialias(FALSE);

		ItIsFunny();
		ItIsNotFunny();

		PolimorfCheatOn MPlayer(4);//Полиморфизм

		if (indicator)
		{
			m_font1->DrawTextA(0, "just do it --- > K I L L  them  A L L", -1, &fontRect1, 0, D3DCOLOR_ARGB(255, 255, 0, 0));

			MPlayer.Insert(&PCE);
			MPlayer.Insert(&PEE);
			if (indicator1)
			{
				MPlayer.Insert(&PCH);    //<---
				MPlayer.Insert(&PCC);    //<---
			}
			MPlayer.PlayPlayer(dev);
			MyDisplay(viewport);
		}
		
		else
		{
			//m_font->DrawTextA(0, "GameHackLab[Ru]", -1, &fontRect, 0, D3DCOLOR_ARGB(255, 0, 255, 0));
		}
		//m_font->Release();
		m_font1->Release();
		line->Release();
		dev->EndScene();
		dev->Clear(1, &rec, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
	}
	friend void GetDevice9Methods();
	friend void HookDevice9Methods();

	~InterceptionDirectX(){}
};

long __stdcall HookedPresent9(IDirect3DDevice9* self, const RECT* src, const RECT* dest, HWND hWnd, void* unused)
{
	InterceptionDirectX IntDir;
	BYTE* codeDest = (BYTE*)g_D3D9_Present;
	codeDest[0] = g_codeFragment_p9[0];
	*((DWORD*)(codeDest + 1)) = *((DWORD*)(g_codeFragment_p9 + 1));
	IntDir.DrawIndicator(self);
	DWORD res = g_D3D9_Present(self, src, dest, hWnd, unused);
	codeDest[0] = g_jmp_p9[0];
	*((DWORD*)(codeDest + 1)) = *((DWORD*)(g_jmp_p9 + 1));
	return res;
}

void GetDevice9Methods()
{
	HWND hWnd = CreateWindowA("STATIC", "dummy", 0, 0, 0, 0, 0, 0, 0, 0, 0);
	HMODULE hD3D9 = LoadLibrary("d3d9");
	DIRECT3DCREATE9 Direct3DCreate9 = (DIRECT3DCREATE9)GetProcAddress(hD3D9, "Direct3DCreate9");
	IDirect3D9 *d3d = Direct3DCreate9(D3D_SDK_VERSION);
	D3DDISPLAYMODE d3ddm;
	d3d->GetAdapterDisplayMode(0, &d3ddm);
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = 1;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = d3ddm.Format;
	IDirect3DDevice9* d3dDevice = 0;
	d3d->CreateDevice(0, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3dDevice);
	DWORD* vtablePtr = (DWORD*)(*((DWORD*)d3dDevice));
	present9 = vtablePtr[17] - (DWORD)hD3D9;
	d3dDevice->Release();
	d3d->Release();
	FreeLibrary(hD3D9);
	CloseHandle(hWnd);
	//DestroyWindow(hWnd);
}

void HookDevice9Methods()
{
	HMODULE hD3D9 = GetModuleHandle("d3d9.dll");
	g_D3D9_Present = (PRESENT9)((DWORD)hD3D9 + present9);
	g_jmp_p9[0] = 0xE9;
	DWORD addr = (DWORD)HookedPresent9 - (DWORD)g_D3D9_Present - 5;
	memcpy(g_jmp_p9 + 1, &addr, sizeof(DWORD));
	memcpy(g_codeFragment_p9, g_D3D9_Present, 5);
	VirtualProtect(g_D3D9_Present, 8, PAGE_EXECUTE_READWRITE, &g_savedProtection_p9);
	memcpy(g_D3D9_Present, g_jmp_p9, 5);
}
// перехват функции DirectX и её дальнейшее использование - конец

//-------------------------------------------------------------------------------------
int __stdcall DllMain(HINSTANCE hInst, DWORD ul_reason_for_call, void* lpReserved)
{
	InterceptionDirectX InDir;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		CreateThread(0, 0, &InDir.TF, 0, 0, 0);//CreateThread(0, 0, &TF, 0, 0, 0);
		CreateThread(0, 0, &InDir.KeyboardHook, 0, 0, 0);
		CreateThread(0, 0, &InDir.KeyboardHook1, 0, 0, 0);
	}
	return 1;
}