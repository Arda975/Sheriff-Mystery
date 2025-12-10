#pragma once

#include <windows.h>
#include "Resource.h"
#include "GameEngine.h"
#include "Bitmap.h"
#include "Sprite.h"
#include "Background.h"
#include "VillageBackground.h"
#include <vector>
#include <utility>
class Villager;

extern std::vector<Villager*> villagers;
extern std::vector<POINT> g_killPositions;
extern bool g_gameOver;

extern HINSTANCE  _hInstance;
extern GameEngine* _pGame;
extern HDC               _hOffscreenDC;
extern HBITMAP           _hOffscreenBitmap;
extern VillageBackground* _pBackground;
extern Bitmap* _pCharacterBitmap;
extern Sprite* _pCharacterSprite;
extern Bitmap* _pVillagerBitmap;
extern Sprite* _pVillagerSprite;
extern int frameIndex;
extern int g_wrongKillChance;
extern char g_gameOverMessage[128];
extern POINT g_cameraPos;
const int MAP_WIDTH = 2000;
const int MAP_HEIGHT = 2000;
const float g_zoom = 1.0f; // 1.0f = normal, 1.5f = %50 daha yakýn
extern bool godmode;


// Fonksiyon prototipleri (eðer gerekiyorsa)
RECT GetPrisonRect();
BOOL GameInitialize(HINSTANCE hInstance);
void GameStart(HWND hWindow);
void GameEnd();
void GameActivate(HWND hWindow);
void GameDeactivate(HWND hWindow);
void GamePaint(HDC hDC);
void GameCycle();
void HandleKeys();
void MouseButtonDown(int x, int y, BOOL bLeft);
void MouseButtonUp(int x, int y, BOOL bLeft);
void MouseMove(int x, int y);
BOOL SpriteCollision(Sprite* pSpriteHitter, Sprite* pSpriteHittee);
static bool LineIntersectsLine(POINT p1, POINT p2, POINT p3, POINT p4);
static bool SegmentIntersectsRect(POINT a, POINT b, const RECT& r);
