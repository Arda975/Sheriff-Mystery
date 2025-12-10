//-----------------------------------------------------------------
// Roids Application
// C++ Source - Roids.cpp
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------
#include <windows.h>
#include <tchar.h>
#include <psapi.h>
#include <numeric>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include <algorithm>
#include "SheriffGame.h"
#include "Sprite.h"
#include "VillageBackground.h"
//-----------------------------------------------------------------
// Game Engine Functions
//-----------------------------------------------------------------
#include "Villager.h"
std::vector<Villager*> villagers;
std::vector<POINT> g_killPositions;

// Global variable definitions
HDC _hOffscreenDC = NULL;
HBITMAP _hOffscreenBitmap = NULL;
//StarryBackground* _pBackground = nullptr;
VillageBackground* _pBackground = nullptr;
Sprite* _pCharacterSprite = nullptr;
GameEngine* _pGame = nullptr;
Bitmap* _pVillagerBitmap = nullptr;
Bitmap* _pCharacterBitmap = nullptr;
HWND g_hNewGameButton = NULL;

Direction _lastPlayerDir = DOWN; // Başlangıçta aşağıya baksın

int walkUpFrames[] = { 73,74, 75, 76, 77, 78, 79, 80 };
int walkLeftFrames[] = { 81, 82,83, 84, 85, 86, 87, 88, 89 };
int walkDownFrames[] = { 90, 91,92, 93, 94, 95, 96, 97,98 };
int walkRightFrames[] = { 99, 100,101,102,103,104,105,106,107 };


char g_killMessage[64] = "";
int g_killMessageTimer = 0;

bool g_gameOver = false;

int g_wrongKillChance = 15;

int g_maxKillsAllowed = 15;
int total_villager = 40;

int g_screenWidth = 0;
int g_screenHeight = 0;
HINSTANCE _hInstance = NULL;
RECT GetPrisonRect()
{
    int prisonWidth = 80;
    int prisonHeight = 80;
    int left = (MAP_WIDTH - prisonWidth) / 2;
    int top = (MAP_HEIGHT - prisonHeight) / 2;
    int right = left + prisonWidth;
    int bottom = top + prisonHeight;
    RECT prisonRect = { left, top, right, bottom };
    return prisonRect;
}

extern bool g_gameOver;
char g_gameOverMessage[128] = "Killer is Dead! Game Over.";

bool godmode;

POINT g_cameraPos = { 0, 0 };


BOOL GameInitialize(HINSTANCE hInstance)
{
    // Create the game engine
    _pGame = new GameEngine(hInstance, TEXT("Roids"),
        TEXT("Roids"), IDI_ROIDS, IDI_ROIDS_SM, 500, 400);
    if (_pGame == NULL)
        return FALSE;

    // Set the frame rate
    _pGame->SetFrameRate(10);

    // Store the instance handle
    _hInstance = hInstance;

    return TRUE;
}
void UpdateCamera()
{
    RECT playerRect = _pCharacterSprite->GetPosition();
    int playerX = (playerRect.left + playerRect.right) / 2;
    int playerY = (playerRect.top + playerRect.bottom) / 2;

    // Zoom'u uygula
    g_cameraPos.x = (int)(playerX - g_screenWidth / (2 * g_zoom));
    g_cameraPos.y = (int)(playerY - g_screenHeight / (2 * g_zoom));

    // Clamp camera to map bounds
    if (g_cameraPos.x < 0) g_cameraPos.x = 0;
    if (g_cameraPos.y < 0) g_cameraPos.y = 0;
    if (g_cameraPos.x > MAP_WIDTH - g_screenWidth / g_zoom) g_cameraPos.x = MAP_WIDTH - (int)(g_screenWidth / g_zoom);
    if (g_cameraPos.y > MAP_HEIGHT - g_screenHeight / g_zoom) g_cameraPos.y = MAP_HEIGHT - (int)(g_screenHeight / g_zoom);
}


void GameStart(HWND hWindow)
{
    godmode = false;
    g_killPositions.clear();

    if (g_hNewGameButton) {
        DestroyWindow(g_hNewGameButton);
        g_hNewGameButton = NULL;
    }

    // Reset global state
    g_gameOver = false;
    g_wrongKillChance = 15;
    g_maxKillsAllowed = 15;
    g_killMessage[0] = '\0';
    g_killMessageTimer = 0;
    strcpy_s(g_gameOverMessage, "Killer is Dead! Game Over.");
    g_cameraPos.x = 0;
    g_cameraPos.y = 0;

    // Clear villagers if not empty (for restart support)
    for (auto& v : villagers) {
        delete v;
        v = nullptr;
    }
    villagers.clear();


   // PlaySound(TEXT("Sounds\\thriller-sound.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);

    // Get screen size
    g_screenWidth = GetSystemMetrics(SM_CXSCREEN);
    g_screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Set window style to borderless fullscreen
    SetWindowLong(hWindow, GWL_STYLE, WS_POPUP | WS_VISIBLE);
    SetWindowPos(hWindow, HWND_TOP, 0, 0, g_screenWidth, g_screenHeight, SWP_SHOWWINDOW);

    // Seed the random number generator
    srand(GetTickCount());

    // Create the offscreen device context and bitmap
    HDC hWindowDC = GetDC(hWindow);
    _hOffscreenDC = CreateCompatibleDC(hWindowDC);
    _hOffscreenBitmap = CreateCompatibleBitmap(hWindowDC, g_screenWidth, g_screenHeight);
    SelectObject(_hOffscreenDC, _hOffscreenBitmap);
    ReleaseDC(hWindow, hWindowDC);

    if (!_hOffscreenDC || !_hOffscreenBitmap) {
        MessageBox(hWindow, TEXT("Offscreen DC veya Bitmap oluşturulamadı!"), TEXT("Hata"), MB_OK | MB_ICONERROR);
        exit(1);
    }
    SelectObject(_hOffscreenDC, _hOffscreenBitmap);

    // Create the starry background
    _pBackground = new VillageBackground(MAP_WIDTH, MAP_HEIGHT);
    RECT rcBounds = { 0, 0, MAP_WIDTH, MAP_HEIGHT };
    HDC hDC = GetDC(hWindow);
    _pCharacterBitmap = new Bitmap(hDC, TEXT("sheriff.bmp"));
    _pVillagerBitmap = new Bitmap(hDC, TEXT("villager.bmp"));
    ReleaseDC(hWindow, hDC);

    RECT prisonRect = GetPrisonRect();
    RECT expandedPrisonRect = prisonRect;
    const int prisonMargin = 100; 
    expandedPrisonRect.left -= prisonMargin;
    expandedPrisonRect.top -= prisonMargin;
    expandedPrisonRect.right += prisonMargin;
    expandedPrisonRect.bottom += prisonMargin;

    for (int i = 0; i < total_villager; ++i)
    {
        Sprite* villagerSprite = new Sprite(_pVillagerBitmap, rcBounds, BA_STOP);
        int margin = 100;
        int frameW = 64, frameH = 64;
        RECT r;
        int tryCount = 0;
        bool valid = false;
        while (tryCount++ < 100) {
            int x = margin + rand() % (MAP_WIDTH - margin * 2 - frameW);
            int y = margin + rand() % (MAP_HEIGHT - margin * 2 - frameH);
            SetRect(&r, x, y, x + frameW, y + frameH);

            // 1. Tüm engellerle çakışma kontrolü (ağaç, ev, çatı)
            bool collision = false;
            for (const RECT& obs : _pBackground->GetObstacles()) {
                RECT intersect;
                if (IntersectRect(&intersect, &r, &obs)) {
                    collision = true;
                    break;
                }
            }
            // 2. Diğer köylülerle çakışma kontrolü
            if (!collision) {
                for (auto v : villagers) {
                    RECT vRect = v->GetSprite()->GetPosition();
                    RECT intersect;
                    if (IntersectRect(&intersect, &r, &vRect)) {
                        collision = true;
                        break;
                    }
                }
            }
            // 3. Hapishane çevresiyle çakışma kontrolü
            if (!collision) {
                if (!(r.right <= expandedPrisonRect.left || r.left >= expandedPrisonRect.right ||
                    r.bottom <= expandedPrisonRect.top || r.top >= expandedPrisonRect.bottom)) {
                    collision = true;
                }
            }
            if (!collision) {
                valid = true;
                break;
            }
        }
        if (!valid) {
            delete villagerSprite;
            continue;
        }
        villagerSprite->SetPosition(r);
        Villager* villager = new Villager(villagerSprite);
        villager->SetID(i);
        villagers.push_back(villager);
    }


    // Rastgele bir köylüyü katil yap
    if (!villagers.empty()) {
        int killerIndex = rand() % villagers.size();
        villagers[killerIndex]->SetRageLevel(40);
    }

    // Karakter sprite'ını oluştur
    _pCharacterSprite = new Sprite(_pCharacterBitmap, rcBounds, BA_STOP);
    _pGame->AddSprite(_pCharacterSprite);

    // --- Başlangıçta bir cinayet oluştur ---
    if (villagers.size() >= 3) {
        // 1. Katili ve kurbanı seç
        int killerIdx = -1, victimIdx = -1;
        // Katil zaten seçilmişti, bul:
        for (size_t i = 0; i < villagers.size(); ++i) {
            if (villagers[i]->GetRageLevel() >= 40) {
                killerIdx = (int)i;
                break;
            }
        }
        // Kurbanı rastgele seç (katil olmayan ve farklı biri)
        do {
            victimIdx = rand() % villagers.size();
        } while (victimIdx == killerIdx);

        Villager* killer = villagers[killerIdx];
        Villager* victim = villagers[victimIdx];

        // Kurbanı öldür
        victim->SetState(DYING);
        PlaySound(TEXT("Sounds\\gunshot.wav"), NULL, SND_FILENAME | SND_ASYNC);

        // Kurbanın pozisyonunu mini harita için kaydet
        RECT victimRect = victim->GetSprite()->GetPosition();
        POINT killPos;
        killPos.x = (victimRect.left + victimRect.right) / 2;
        killPos.y = (victimRect.top + victimRect.bottom) / 2;
        g_killPositions.push_back(killPos);


        // 2. Rastgele 2 tanık seç (katil ve kurban hariç)
        std::vector<int> witnessCandidates;
        for (size_t i = 0; i < villagers.size(); ++i) {
            if ((int)i != killerIdx && (int)i != victimIdx)
                witnessCandidates.push_back((int)i);
        }
        std::random_shuffle(witnessCandidates.begin(), witnessCandidates.end());

        int witnessCount = std::min(2, (int)witnessCandidates.size());
        for (int i = 0; i < witnessCount; ++i) {
            Villager* witness = villagers[witnessCandidates[i]];
            // Tanığın doğruyu mu söyleyeceğine rastgele karar ver
            bool willTellTruth = (rand() % 2 == 0);

            witness->_lastKiller = willTellTruth ? killer : villagers[witnessCandidates[(i + 1) % witnessCandidates.size()]];
            witness->_memoryTimer = 0;
            witness->SetIsScared(true);
            witness->SetState(FLEEING);
            witness->SetFleeTimer(180);
            witness->SetIsLying(!willTellTruth);
            if (!willTellTruth) {
                int fakeIdx;
                do {
                    fakeIdx = rand() % villagers.size();
                } while (fakeIdx == killerIdx || fakeIdx == witnessCandidates[i]);
                witness->SetLieAnswer(villagers[fakeIdx]);
            }
            else {
                witness->SetLieAnswer(nullptr);
            }


        }
    }


    // Sprite sheet'teki çerçeve sayısını ve gecikmeyi ayarla
    _pCharacterSprite->SetFrameDelay(2); // Her çerçeve için gecikme

    // Hapishane ortasını bul
    prisonRect = GetPrisonRect();
    int prisonCenterX = (prisonRect.left + prisonRect.right) / 2;
    int prisonCenterY = (prisonRect.top + prisonRect.bottom) / 2;

    // Karakteri hapishanenin biraz sağına yerleştir
    int frameW = 64;
    int frameH = 64;
    int startX = prisonCenterX + 120; // 120 piksel sağında başlasın
    int startY = prisonCenterY - frameH / 2;
    RECT rcCharacter = { startX, startY, startX + frameW, startY + frameH };
    _pCharacterSprite->SetPosition(rcCharacter);
}


void GameEnd()
{
    // Cleanup the offscreen device context and bitmapx
    DeleteObject(_hOffscreenBitmap);
    DeleteDC(_hOffscreenDC);
    delete _pVillagerBitmap;

    for (auto& v : villagers) {
        delete v;
        v = nullptr;
    }
    villagers.clear();



    // Cleanup the background
    delete _pBackground;
    _pBackground = nullptr;
    // Cleanup the sprites
    //_pGame->CleanupSprites();

    // Cleanup the game engine
    delete _pGame;
}

void GameActivate(HWND hWindow)
{
}

void GameDeactivate(HWND hWindow)
{
}
static bool LineIntersectsLine(POINT p1, POINT p2, POINT p3, POINT p4) {
    auto cross = [](POINT u, POINT v) { return (long)u.x * v.y - (long)u.y * v.x; };
    POINT r = { p2.x - p1.x, p2.y - p1.y };
    POINT s = { p4.x - p3.x, p4.y - p3.y };
    long rxs = cross(r, s);
    if (rxs == 0) return false;  // paralel
    POINT qp = { p3.x - p1.x, p3.y - p1.y };
    double t = (double)cross(qp, s) / rxs;
    double u = (double)cross(qp, r) / rxs;
    return (t >= 0 && t <= 1 && u >= 0 && u <= 1);
}
static bool SegmentIntersectsRect(POINT a, POINT b, const RECT& r) {
    // dört kenarla test
    if (LineIntersectsLine(a, b, { r.left, r.top }, { r.right, r.top }))    return true;
    if (LineIntersectsLine(a, b, { r.right, r.top }, { r.right, r.bottom })) return true;
    if (LineIntersectsLine(a, b, { r.right, r.bottom }, { r.left,  r.bottom })) return true;
    if (LineIntersectsLine(a, b, { r.left,  r.bottom }, { r.left,  r.top }))    return true;
    return false;
}
void GamePaint(HDC hDC)
{
    if (!hDC) return;

    // Arka planı kameraya göre çiz
    _pBackground->Draw(
        _hOffscreenDC,
        g_cameraPos.x,
        g_cameraPos.y,
        g_screenWidth,
        g_screenHeight,
        g_zoom
    );
    if(godmode) {
        // (1) Şerif pozisyonu ve vision dikdörtgenini hesapla
        RECT sr = _pCharacterSprite->GetPosition();
        const int VW = 1200, VH = 800;
        int cx = (sr.left + sr.right) / 2, cy = (sr.top + sr.bottom) / 2;
        RECT vis;
        switch (_lastPlayerDir) {
        case UP:    vis = { cx - VW / 2, cy - VH, cx + VW / 2, cy };     break;
        case DOWN:  vis = { cx - VW / 2, cy,     cx + VW / 2, cy + VH }; break;
        case LEFT:  vis = { cx - VH,    cy - VW / 2, cx,    cy + VW / 2 }; break;
        default:    vis = { cx,       cy - VW / 2, cx + VH, cy + VW / 2 }; break;
        }

        // (2) Vision’ı çiz (gri çerçeve)
        HPEN  penG = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
        HPEN  oldP = (HPEN)SelectObject(_hOffscreenDC, penG);
        HBRUSH oldB = (HBRUSH)SelectObject(_hOffscreenDC, GetStockObject(NULL_BRUSH));
        Rectangle(_hOffscreenDC,
            LONG((vis.left - g_cameraPos.x) * g_zoom),
            LONG((vis.top - g_cameraPos.y) * g_zoom),
            LONG((vis.right - g_cameraPos.x) * g_zoom),
            LONG((vis.bottom - g_cameraPos.y) * g_zoom));
        SelectObject(_hOffscreenDC, oldP);
        SelectObject(_hOffscreenDC, oldB);
        DeleteObject(penG);

        // (3) Şerif merkezi
        POINT spt = { cx, cy };

        // (4) Her köylü için: önce vision içinde mi, sonra ray testi
        for (Villager* v : villagers)
        {
            Sprite* s = v->GetSprite();
            RECT  wp = s->GetPosition();
            POINT vp = { (wp.left + wp.right) / 2,(wp.top + wp.bottom) / 2 };

            if (vp.x < vis.left || vp.x > vis.right ||
                vp.y < vis.top || vp.y > vis.bottom)
                continue;

            bool blocked = false;
            for (const RECT& obs : _pBackground->GetObstacles())
            {
                if (SegmentIntersectsRect(spt, vp, obs))
                {
                    blocked = true;
                    break;
                }
            }
            if (blocked) continue;

            // (5) Ekrana çiz
            RECT orig = wp;
            RECT draw = {
              LONG((orig.left - g_cameraPos.x) * g_zoom),
              LONG((orig.top - g_cameraPos.y) * g_zoom),
              LONG((orig.right - g_cameraPos.x) * g_zoom),
              LONG((orig.bottom - g_cameraPos.y) * g_zoom)
            };
            s->SetPosition(draw);
            v->Draw(_hOffscreenDC);
            s->SetPosition(orig);

            // (6) ID’sini sprite’ın üstüne çiz
            {
                SetTextColor(_hOffscreenDC, RGB(255, 255, 255));
                SetBkMode(_hOffscreenDC, TRANSPARENT);

                char buffer[16];
                int id = v->GetID();  // kendi GetID() fonksiyonun
                snprintf(buffer, sizeof(buffer), "%d", id);

                // soldan 5*zoom, üstten 15*zoom yukarı koyduk
                int tx = draw.left + int(20 * g_zoom);
                int ty = draw.top - int(10 * g_zoom);

                TextOutA(_hOffscreenDC, tx, ty, buffer, (int)strlen(buffer));


            }
        }

    }
    else {
        for (Villager* v : villagers)
        {
            Sprite* sprite = v->GetSprite();
            RECT origPos = sprite->GetPosition();
            RECT drawPos;
            drawPos.left = (LONG)((origPos.left - g_cameraPos.x) * g_zoom);
            drawPos.top = (LONG)((origPos.top - g_cameraPos.y) * g_zoom);
            drawPos.right = (LONG)((origPos.right - g_cameraPos.x) * g_zoom);
            drawPos.bottom = (LONG)((origPos.bottom - g_cameraPos.y) * g_zoom);

            sprite->SetPosition(drawPos);
            v->Draw(_hOffscreenDC);
            sprite->SetPosition(origPos);

            {
                SetTextColor(_hOffscreenDC, RGB(255, 255, 255));
                SetBkMode(_hOffscreenDC, TRANSPARENT);

                char buffer[16];
                int id = v->GetID();  // kendi GetID() fonksiyonun
                snprintf(buffer, sizeof(buffer), "%d", id);

                // soldan 5*zoom, üstten 15*zoom yukarı koyduk
                int tx = drawPos.left + int(20 * g_zoom);
                int ty = drawPos.top - int(10 * g_zoom);

                TextOutA(_hOffscreenDC, tx, ty, buffer, (int)strlen(buffer));


            }
        }



    }
    



    

    // Karakteri kameraya göre ve zoom ile çiz
    {
        RECT origPos = _pCharacterSprite->GetPosition();
        RECT drawPos;
        drawPos.left = (LONG)((origPos.left - g_cameraPos.x) * g_zoom);
        drawPos.top = (LONG)((origPos.top - g_cameraPos.y) * g_zoom);
        drawPos.right = (LONG)((origPos.right - g_cameraPos.x) * g_zoom);
        drawPos.bottom = (LONG)((origPos.bottom - g_cameraPos.y) * g_zoom);

        _pCharacterSprite->SetPosition(drawPos);
        _pCharacterSprite->Draw(_hOffscreenDC);
        _pCharacterSprite->SetPosition(origPos);
    }

    // Hapishaneyi kameraya göre ve zoom ile çiz
    RECT prisonRect = GetPrisonRect();
    RECT prisonDrawRect;
    prisonDrawRect.left = (LONG)((prisonRect.left - g_cameraPos.x) * g_zoom);
    prisonDrawRect.top = (LONG)((prisonRect.top - g_cameraPos.y) * g_zoom);
    prisonDrawRect.right = (LONG)((prisonRect.right - g_cameraPos.x) * g_zoom);
    prisonDrawRect.bottom = (LONG)((prisonRect.bottom - g_cameraPos.y) * g_zoom);

    // Parmaklık efekti (dikey ve yatay çizgiler)
    HPEN hBarPen = CreatePen(PS_SOLID, (int)(2 * g_zoom), RGB(180, 180, 180));
    HPEN hOldBarPen = (HPEN)SelectObject(_hOffscreenDC, hBarPen);

    int barCount = 5;
    int barSpacingX = (prisonDrawRect.right - prisonDrawRect.left) / (barCount + 1);
    int barSpacingY = (prisonDrawRect.bottom - prisonDrawRect.top) / (barCount + 1);

    // Dikey parmaklıklar
    for (int i = 1; i <= barCount; ++i) {
        int x = prisonDrawRect.left + i * barSpacingX;
        MoveToEx(_hOffscreenDC, x, prisonDrawRect.top, NULL);
        LineTo(_hOffscreenDC, x, prisonDrawRect.bottom);
    }
    // Yatay parmaklıklar
    for (int i = 1; i <= 2; ++i) {
        int y = prisonDrawRect.top + i * barSpacingY;
        MoveToEx(_hOffscreenDC, prisonDrawRect.left, y, NULL);
        LineTo(_hOffscreenDC, prisonDrawRect.right, y);
    }

    SelectObject(_hOffscreenDC, hOldBarPen);
    DeleteObject(hBarPen);

    // Kenarlara koyu gri çerçeve
    HPEN hBorderPen = CreatePen(PS_SOLID, (int)(4 * g_zoom), RGB(40, 40, 40));
    HPEN hOldBorderPen = (HPEN)SelectObject(_hOffscreenDC, hBorderPen);
    SelectObject(_hOffscreenDC, GetStockObject(HOLLOW_BRUSH));
    Rectangle(_hOffscreenDC, prisonDrawRect.left, prisonDrawRect.top, prisonDrawRect.right, prisonDrawRect.bottom);
    SelectObject(_hOffscreenDC, hOldBorderPen);
    DeleteObject(hBorderPen);


    // Rage level ve görüş durumu yazılarını kameraya göre ve zoom ile çiz
    for (size_t i = 0; i < villagers.size(); ++i)
    {
        Sprite* sprite = villagers[i]->GetSprite();
        if (sprite == _pCharacterSprite)
            continue;

        RECT pos = sprite->GetPosition();
        RECT drawPos;
        drawPos.left = (LONG)((pos.left - g_cameraPos.x) * g_zoom);
        drawPos.top = (LONG)((pos.top - g_cameraPos.y) * g_zoom);
        drawPos.right = (LONG)((pos.right - g_cameraPos.x) * g_zoom);
        drawPos.bottom = (LONG)((pos.bottom - g_cameraPos.y) * g_zoom);

        SetTextColor(hDC, RGB(255, 255, 255));
        SetBkMode(hDC, TRANSPARENT);



        /*
        for (size_t j = 0; j < villagers.size(); ++j)
        {
            if (i != j && villagers[i]->CanSee(villagers[j]))
            {
                char seeBuffer[32];
                snprintf(seeBuffer, sizeof(seeBuffer), "-> %d", (int)j + 1);
                TextOutA(hDC, drawPos.left + (int)(5 * g_zoom), drawPos.top - (int)(35 * g_zoom), seeBuffer, (int)strlen(seeBuffer));
            }
        }
        */
    }

    // Harita sınırlarını kalın kırmızı çizgiyle ve zoom ile çiz
    {
        RECT mapRect = { 0, 0, MAP_WIDTH, MAP_HEIGHT };
        RECT mapDrawRect;
        mapDrawRect.left = (LONG)((mapRect.left - g_cameraPos.x) * g_zoom);
        mapDrawRect.top = (LONG)((mapRect.top - g_cameraPos.y) * g_zoom);
        mapDrawRect.right = (LONG)((mapRect.right - g_cameraPos.x) * g_zoom);
        mapDrawRect.bottom = (LONG)((mapRect.bottom - g_cameraPos.y) * g_zoom);

        HPEN hPen = CreatePen(PS_SOLID, (int)(6 * g_zoom), RGB(255, 0, 0));
        HPEN hOldPen = (HPEN)SelectObject(_hOffscreenDC, hPen);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(_hOffscreenDC, GetStockObject(HOLLOW_BRUSH));

        Rectangle(_hOffscreenDC, mapDrawRect.left, mapDrawRect.top, mapDrawRect.right, mapDrawRect.bottom);

        SelectObject(_hOffscreenDC, hOldPen);
        SelectObject(_hOffscreenDC, hOldBrush);
        DeleteObject(hPen);
    }

    // Kill message
    if (g_killMessageTimer > 0 && g_killMessage[0] != '\0') {
        SetTextColor(hDC, RGB(255, 255, 0));
        SetBkMode(hDC, TRANSPARENT);

        SIZE textSize;
        GetTextExtentPoint32A(hDC, g_killMessage, (int)strlen(g_killMessage), &textSize);

        //int x = g_screenWidth - textSize.cx - 20;
        int x = 20;
        int y = 20;
        TextOutA(hDC, x, y, g_killMessage, (int)strlen(g_killMessage));
    }

    // --- Minimap çizimi ---
    const int minimapWidth = 200;
    const int minimapHeight = 200;
    const int minimapMargin = 20; // Sağ üstten boşluk

    // Minimap arka planı
    RECT minimapRect;
    minimapRect.left = g_screenWidth - minimapWidth - minimapMargin;
    minimapRect.top = minimapMargin;
    minimapRect.right = g_screenWidth - minimapMargin;
    minimapRect.bottom = minimapMargin + minimapHeight;

    HBRUSH minimapBg = CreateSolidBrush(RGB(30, 30, 30));
    FillRect(hDC, &minimapRect, minimapBg);
    DeleteObject(minimapBg);

    // Harita oranı
    float scaleX = (float)minimapWidth / MAP_WIDTH;
    float scaleY = (float)minimapHeight / MAP_HEIGHT;

    // Karakterin konumu (beyaz nokta)
    RECT charRect = _pCharacterSprite->GetPosition();
    int charX = (charRect.left + charRect.right) / 2;
    int charY = (charRect.top + charRect.bottom) / 2;
    int miniCharX = minimapRect.left + (int)(charX * scaleX);
    int miniCharY = minimapRect.top + (int)(charY * scaleY);

    HBRUSH charBrush = CreateSolidBrush(RGB(255, 255, 255));
    HPEN charPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    HGDIOBJ oldBrush = SelectObject(hDC, charBrush);
    HGDIOBJ oldPen = SelectObject(hDC, charPen);
    Ellipse(hDC, miniCharX - 4, miniCharY - 4, miniCharX + 4, miniCharY + 4);
    SelectObject(hDC, oldBrush);
    SelectObject(hDC, oldPen);
    DeleteObject(charBrush);
    DeleteObject(charPen);

    // Katilin öldürdüğü yerler (kırmızı nokta)
    HBRUSH killBrush = CreateSolidBrush(RGB(255, 0, 0));
    HPEN killPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
    oldBrush = SelectObject(hDC, killBrush);
    oldPen = SelectObject(hDC, killPen);
    for (const POINT& pt : g_killPositions) {
        int miniX = minimapRect.left + (int)(pt.x * scaleX);
        int miniY = minimapRect.top + (int)(pt.y * scaleY);
        Ellipse(hDC, miniX - 3, miniY - 3, miniX + 3, miniY + 3);
    }
    SelectObject(hDC, oldBrush);
    SelectObject(hDC, oldPen);
    DeleteObject(killBrush);
    DeleteObject(killPen);

    // Minimap çerçevesi
    HPEN borderPen = CreatePen(PS_SOLID, 2, RGB(200, 200, 200));
    oldPen = SelectObject(hDC, borderPen);
    SelectObject(hDC, GetStockObject(HOLLOW_BRUSH));
    Rectangle(hDC, minimapRect.left, minimapRect.top, minimapRect.right, minimapRect.bottom);
    SelectObject(hDC, oldPen);
    DeleteObject(borderPen);
}
void GameCycle()
{

    if (g_gameOver) {
        HWND hWindow = _pGame->GetWindow();
        HDC hDC = GetDC(hWindow);
        SetBkMode(hDC, TRANSPARENT);
        SetTextColor(hDC, RGB(255, 0, 0));
        SIZE textSize;
        GetTextExtentPoint32A(hDC, g_gameOverMessage, strlen(g_gameOverMessage), &textSize);

        // Calculate centered position
        int x = (g_screenWidth - textSize.cx) / 2;
        int y = (g_screenHeight - textSize.cy) / 2;

        TextOutA(hDC, x, y, g_gameOverMessage, strlen(g_gameOverMessage));
        ReleaseDC(hWindow, hDC);


        // Buton yoksa oluştur
        if (!g_hNewGameButton) {
            g_hNewGameButton = CreateWindowA(
                "BUTTON", "New Game",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                (g_screenWidth - 200) / 2, y + textSize.cy + 40, 200, 40,
                hWindow, (HMENU)1001, GetModuleHandle(NULL), NULL);
        }
        return;
    }
    else {
        // Oyun devam ediyorsa butonu gizle
        if (g_hNewGameButton) {
            DestroyWindow(g_hNewGameButton);
            g_hNewGameButton = NULL;
        }

    }

    // SheriffGame.cpp
    static int focusCooldown = 0;
    if (--focusCooldown <= 0) {
        focusCooldown = 600; // 20 saniyede bir (30 FPS için)
        if (_pBackground) {
            const auto& houses = _pBackground->GetHouses();
            if (!houses.empty() && villagers.size() >= 5) {
                std::vector<int> villagerIndices(villagers.size());
                std::iota(villagerIndices.begin(), villagerIndices.end(), 0);
                std::random_shuffle(villagerIndices.begin(), villagerIndices.end());
                for (int i = 0; i < 2; ++i) {
                    int vIdx = villagerIndices[i];
                    int houseId = rand() % houses.size();
                    villagers[vIdx]->SetTargetHouse(houses[houseId].id, 300 + rand() % 200); // 10-16 sn
                }
            }
        }
    }

    //Katil hariç tüm köylüler öldüyse oyun bitsin ---
    int killerAlive = 0;
    int otherAlive = 0;
    for (Villager* v : villagers)
    {
        if (v->GetRageLevel() >= 40 && v->GetState() != DYING)
            killerAlive++;
        else if (v->GetState() != DYING)
            otherAlive++;
    }
    
    // Eğer yaşayan herkes katil olduysa oyun bitsin
    int aliveVillagerCount = 0;
    int killerCount = 0;
    for (Villager* v : villagers)
    {
        if (v->GetState() != DYING)
        {
            aliveVillagerCount++;
            if (v->GetRageLevel() >= 40)
                killerCount++;
        }
    }

    //Eğer total öldürme sayısına ulaşıldıysa
    if ((total_villager - g_maxKillsAllowed) >= aliveVillagerCount)
    {
        HWND hWindow = _pGame->GetWindow();
        HDC hDC = GetDC(hWindow);
        SetBkMode(hDC, TRANSPARENT);
        SetTextColor(hDC, RGB(255, 0, 0));
        strcpy_s(g_gameOverMessage, "Too many people died! Game Over.");
        g_gameOver = true;
        ReleaseDC(hWindow, hDC);
        return;
    }

    if (aliveVillagerCount > 0 && aliveVillagerCount == killerCount)
    {
        HWND hWindow = _pGame->GetWindow();
        HDC hDC = GetDC(hWindow);
        SetBkMode(hDC, TRANSPARENT);
        SetTextColor(hDC, RGB(255, 0, 0));
        strcpy_s(g_gameOverMessage, "Everyone became killer! Game Over.");
        g_gameOver = true;
        ReleaseDC(hWindow, hDC);
        return;
    }

    if (killerAlive > 0 && otherAlive == 0)
    {
        HWND hWindow = _pGame->GetWindow();
        HDC hDC = GetDC(hWindow);
        SetBkMode(hDC, TRANSPARENT);
        SetTextColor(hDC, RGB(255, 0, 0));
        strcpy_s(g_gameOverMessage, "Killer killed everyone! Game Over.");
        g_gameOver = true;
        ReleaseDC(hWindow, hDC);
        return;
    }

    for (size_t i = 0; i < villagers.size(); ++i)
    {
        char buf[64];
        sprintf(buf, "GameCycle: InfectIfClose i=%zu\n", i);

        for (size_t j = 0; j < villagers.size(); ++j)
        {
            if (i != j)
                villagers[i]->InfectIfClose(villagers[j]);
        }

        villagers[i]->Update();
    }


    for (Villager* v : villagers)
    {
        v->TryKillNearby(villagers);
        v->Update();
    }


    // Update the background
    _pBackground->Update();

    // Update the sprites
    _pGame->UpdateSprites();

    // Karakter sprite'ını güncelle
    _pCharacterSprite->Update();

    // Karakter sprite'ını çiz
    _pCharacterSprite->Draw(_hOffscreenDC);
    for (Villager* v : villagers)
    {
        v->Update();
        v->SpreadToNearby(villagers);
        v->Draw(_hOffscreenDC);
    }

    static bool g_graphicsError = false;
    HWND hWindow = _pGame->GetWindow();

    if (g_graphicsError) {
        if (IsWindow(hWindow) && !IsIconic(hWindow)) {
            HDC hWindowDC = GetDC(hWindow);
            if (hWindowDC) {
                if (_hOffscreenBitmap) DeleteObject(_hOffscreenBitmap);
                if (_hOffscreenDC) DeleteDC(_hOffscreenDC);
                _hOffscreenDC = CreateCompatibleDC(hWindowDC);
                _hOffscreenBitmap = CreateCompatibleBitmap(hWindowDC, g_screenWidth, g_screenHeight);
                if (_hOffscreenDC && _hOffscreenBitmap)
                    SelectObject(_hOffscreenDC, _hOffscreenBitmap);
                ReleaseDC(hWindow, hWindowDC);
                if (_hOffscreenDC && _hOffscreenBitmap)
                    g_graphicsError = false;
            }
        }
    }
    if (!g_graphicsError && _hOffscreenDC && _hOffscreenBitmap) {
        HDC hDC = GetDC(hWindow);
        if (hDC) {
            GamePaint(_hOffscreenDC);
            BOOL blitResult = BitBlt(hDC, 0, 0, g_screenWidth, g_screenHeight, _hOffscreenDC, 0, 0, SRCCOPY);
            if (!blitResult) {
                g_graphicsError = true;
            }
            ReleaseDC(hWindow, hDC);
        }
        else {
            g_graphicsError = true;
        }
    }

    UpdateCamera();

    // Kill message timer'ı azalt
    if (g_killMessageTimer > 0)
        g_killMessageTimer--;

    // Cleanup
    //ReleaseDC(hWindow, hDC);
   /* villagers.erase(
        std::remove_if(villagers.begin(), villagers.end(),
            [](Villager* v) {
                if (v && v->GetState() == DYING) {
                    delete v;
                    return true;
                }
                return false;
            }),
        villagers.end());*/

}
bool IsPrisonOccupied()
{
    for (Villager* v : villagers)
    {
        if (v->GetState() == ARRESTED)
            return true;
    }
    return false;
}


void HandleKeys()
{
    extern VillageBackground* _pBackground;

    // Sayaçlar (static olduğu için değerleri korunur)
    static int upIndex = 0;
    static int downIndex = 0;
    static int leftIndex = 0;
    static int rightIndex = 0;
    static int rCooldownTimer = 0;
    const int totalStep = 20;
    const int subStep = 5;
    if (rCooldownTimer > 0)
        rCooldownTimer--;
    if (GetAsyncKeyState(VK_UP) & 0x8000)
    {
        for (int moved = 0; moved < totalStep; moved += subStep)
        {
            RECT origRect = _pCharacterSprite->GetPosition();
            RECT newRect = origRect;
            OffsetRect(&newRect, 0, -subStep);

            bool collision = false;
            for (const RECT& obs : _pBackground->GetObstacles()) {
                RECT shrunkNewRect = newRect, shrunkObs = obs;
                InflateRect(&shrunkNewRect, -6, -6);
                InflateRect(&shrunkObs, -6, -6);
                RECT intersect;
                if (IntersectRect(&intersect, &shrunkNewRect, &shrunkObs)) {
                    collision = true;
                    break;
                }
            }
            if (collision) break;
            _pCharacterSprite->OffsetPosition(0, -subStep);
        }

        _pCharacterSprite->SetFrame(walkUpFrames[upIndex]);
        int length = sizeof(walkUpFrames) / sizeof(walkUpFrames[0]);
        upIndex = (upIndex + 1) % length;
        _lastPlayerDir = UP;
    }

    else if (GetAsyncKeyState(VK_DOWN) & 0x8000)
    {
        for (int moved = 0; moved < totalStep; moved += subStep)
        {
            RECT origRect = _pCharacterSprite->GetPosition();
            RECT newRect = origRect;
            OffsetRect(&newRect, 0, subStep);

            bool collision = false;
            for (const RECT& obs : _pBackground->GetObstacles()) {
                RECT shrunkNewRect = newRect, shrunkObs = obs;
                InflateRect(&shrunkNewRect, -6, -6);
                InflateRect(&shrunkObs, -6, -6);
                RECT intersect;
                if (IntersectRect(&intersect, &shrunkNewRect, &shrunkObs)) {
                    collision = true;
                    break;
                }
            }
            if (collision) break;
            _pCharacterSprite->OffsetPosition(0, subStep);
        }

        _pCharacterSprite->SetFrame(walkDownFrames[downIndex]);
        int length = sizeof(walkDownFrames) / sizeof(walkDownFrames[0]);
        downIndex = (downIndex + 1) % length;
        _lastPlayerDir = DOWN;
    }

    else if (GetAsyncKeyState(VK_LEFT) & 0x8000)
    {
        for (int moved = 0; moved < totalStep; moved += subStep)
        {
            RECT origRect = _pCharacterSprite->GetPosition();
            RECT newRect = origRect;
            OffsetRect(&newRect, -subStep, 0);

            bool collision = false;
            for (const RECT& obs : _pBackground->GetObstacles()) {
                RECT shrunkNewRect = newRect, shrunkObs = obs;
                InflateRect(&shrunkNewRect, -6, -6);
                InflateRect(&shrunkObs, -6, -6);
                RECT intersect;
                if (IntersectRect(&intersect, &shrunkNewRect, &shrunkObs)) {
                    collision = true;
                    break;
                }
            }
            if (collision) break;
            _pCharacterSprite->OffsetPosition(-subStep, 0);
        }

        _pCharacterSprite->SetFrame(walkLeftFrames[leftIndex]);
        int length = sizeof(walkLeftFrames) / sizeof(walkLeftFrames[0]);
        leftIndex = (leftIndex + 1) % length;
        _lastPlayerDir = LEFT;
    }

    else if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
    {
        for (int moved = 0; moved < totalStep; moved += subStep)
        {
            RECT origRect = _pCharacterSprite->GetPosition();
            RECT newRect = origRect;
            OffsetRect(&newRect, subStep, 0);

            bool collision = false;
            for (const RECT& obs : _pBackground->GetObstacles()) {
                RECT shrunkNewRect = newRect, shrunkObs = obs;
                InflateRect(&shrunkNewRect, -6, -6);
                InflateRect(&shrunkObs, -6, -6);
                RECT intersect;
                if (IntersectRect(&intersect, &shrunkNewRect, &shrunkObs)) {
                    collision = true;
                    break;
                }
            }
            if (collision) break;
            _pCharacterSprite->OffsetPosition(subStep, 0);
        }

        _pCharacterSprite->SetFrame(walkRightFrames[rightIndex]);
        int length = sizeof(walkRightFrames) / sizeof(walkRightFrames[0]);
        rightIndex = (rightIndex + 1) % length;
        _lastPlayerDir = RIGHT;
    }

    else if (GetAsyncKeyState('W') & 0x8000)
    {
        for (Villager* v : villagers)
        {
            if (v->GetState() != WALKING)
                continue;

            // HAPİSTEYSE ÖLDÜRME!
            RECT villagerRect = v->GetSprite()->GetPosition();
            RECT prisonRect = GetPrisonRect();
            if (villagerRect.left >= prisonRect.left && villagerRect.right <= prisonRect.right &&
                villagerRect.top >= prisonRect.top && villagerRect.bottom <= prisonRect.bottom)
                continue;

            RECT playerRect = _pCharacterSprite->GetPosition();
            RECT intersect;

            if (IntersectRect(&intersect, &playerRect, &villagerRect))
            {
                // Yanlış kişiyi öldürme kontrolü. Eğer öldürmeye çalıştığımız kişinin rage level'ı 40'ın altında ise masumdur.
                if (v->GetRageLevel() < 40) {
                    if (g_wrongKillChance > 0) {
                        g_wrongKillChance--;
                        v->SetState(DYING);
                        PlaySound(TEXT("Sounds\\gunshot.wav"), NULL, SND_FILENAME | SND_ASYNC);
                        
                        //Artık aşşağıdaki kısmı kullanmıyoruz.
                        /*if (g_wrongKillChance == 0) {
                            strcpy_s(g_gameOverMessage, "You killed wrong person! Game over.");
                            g_gameOver = true;
                            HWND hWindow = _pGame->GetWindow();
                            HDC hDC = GetDC(hWindow);
                            SetBkMode(hDC, TRANSPARENT);
                            SetTextColor(hDC, RGB(255, 0, 0));
                            ReleaseDC(hWindow, hDC);
                        }*/
                    
                    }
                }
                else {
                    v->SetState(DYING);
                    //g_totalKills++;
                    PlaySound(TEXT("Sounds\\gunshot.wav"), NULL, SND_FILENAME | SND_ASYNC);
                }
                break; // Sadece bir köylüyü öldür
            }
        }
    }

    else if (GetAsyncKeyState('Q') & 0x8000 && rCooldownTimer == 0)
    {
        for (Villager* v : villagers)
        {
            if (v->GetState() != WALKING)
                continue;

            RECT playerRect = _pCharacterSprite->GetPosition();
            RECT villagerRect = v->GetSprite()->GetPosition();
            RECT intersect;

            if (!IntersectRect(&intersect, &playerRect, &villagerRect))
                continue;
            int randomSpeech = rand() % 4;

            // Eğer hiç cinayet görmediyse
            if (!v->KnowsKiller())
            {
                std::wstring responses[] = {
                    L"Bilmiyorum...",
                    L"Hiçbir şey görmedim.",
                    L"O sırada orada değildim.",
                    L"Kimseye dikkat etmedim."
                };
               
                v->_speechText = responses[randomSpeech];
                v->_showSpeech = true;
                v->_speechTimer = 150;
                rCooldownTimer = 15;
                continue;
            }

            // Gördüyse kimin cevabını verecek?
            Villager* answerVillager = nullptr;

            if (v->IsLying())
            {
                answerVillager = v->GetLieAnswer();
            }
            else
            {
                answerVillager = v->GetLastKiller();
            }

            // Confidence hesapla
            float confidence = v->GetMemoryConfidence();
            std::wstringstream ss;
            int style = rand() % 3;

            if (confidence > 0.95f)
            {
                if (style == 0) {
                    ss << L"Katil kesinlikle ID " << answerVillager->GetID() << L" idi!";
                    PlaySound(TEXT("Sounds\\kesinlikle.wav"), NULL, SND_FILENAME | SND_ASYNC);
                }
                   
                else if (style == 1) {
                    ss << L"Eminim, ID " << answerVillager->GetID() << L" yaptı.";
                    PlaySound(TEXT("Sounds\\eminimoydu.wav"), NULL, SND_FILENAME | SND_ASYNC);
                }
                   
                else
                {
                    ss << L"%100 eminim ID " << answerVillager->GetID() << L".";
                    PlaySound(TEXT("Sounds\\yuzdeyuz.wav"), NULL, SND_FILENAME | SND_ASYNC);
                }
            }
            else if (confidence > 0.6f)
            {
                if (style == 0)
                {
                    ss << L"Sanırım ID " << answerVillager->GetID() << L" olabilir.";
                    PlaySound(TEXT("Sounds\\sanirim.wav"), NULL, SND_FILENAME | SND_ASYNC);
                }
                else if (style == 1)
                {
                    ss << L"Tam göremedim ama ID " << answerVillager->GetID() << L" olabilir.";
                    PlaySound(TEXT("Sounds\\goremedimama.wav"), NULL, SND_FILENAME | SND_ASYNC);
                }
                else
                {
                    ss << L"Galiba ID " << answerVillager->GetID() << L" yaptı.";
                    PlaySound(TEXT("Sounds\\galiba.wav"), NULL, SND_FILENAME | SND_ASYNC);
                }
            }
            else
            {
                if (style == 0)
                {
                    ss << L"Bence ID " << answerVillager->GetID() << L" olabilir...";
                    PlaySound(TEXT("Sounds\\bence.wav"), NULL, SND_FILENAME | SND_ASYNC);
                }
                else if (style == 1)
                {
                    ss << L"Tam emin değilim ama ID " << answerVillager->GetID() << L"...";
                    PlaySound(TEXT("Sounds\\emindegilim.wav"), NULL, SND_FILENAME | SND_ASYNC);
                }
                else
                {
                    ss << L"Kararsızım, ID " << answerVillager->GetID() << L" olabilir.";
                    PlaySound(TEXT("Sounds\\kararsiz.wav"), NULL, SND_FILENAME | SND_ASYNC);
                }
            }

            v->_speechText = ss.str();
            PlaySound(TEXT("Sounds\\dikkatetmedim.wav"), NULL, SND_FILENAME | SND_ASYNC);
            v->_showSpeech = true;
            v->_speechTimer = 150;
            rCooldownTimer = 15;
            if (randomSpeech == 0) {
                PlaySound(TEXT("Sounds\\bilmiyorum.wav"), NULL, SND_FILENAME | SND_ASYNC);
            }
            else if (randomSpeech == 1) {
                PlaySound(TEXT("Sounds\\gormedim.wav"), NULL, SND_FILENAME | SND_ASYNC);
            }
            else if (randomSpeech == 2) {
                PlaySound(TEXT("Sounds\\ordayoktum.wav"), NULL, SND_FILENAME | SND_ASYNC);
            }
            else if (randomSpeech == 3) {
                PlaySound(TEXT("Sounds\\dikkatetmedim.wav"), NULL, SND_FILENAME | SND_ASYNC);
            }
        }
    }
    
    else if (GetAsyncKeyState(VK_SPACE) & 0x8000) // hapishaneye gönderme
    {
        if (IsPrisonOccupied())
            return;

        for (Villager* v : villagers)
        {
            if (v->GetState() != WALKING)
                continue;

            RECT playerRect = _pCharacterSprite->GetPosition();
            RECT villagerRect = v->GetSprite()->GetPosition();
            RECT intersect;
            RECT prisonRect = GetPrisonRect();

            if (villagerRect.left >= prisonRect.left && villagerRect.right <= prisonRect.right &&
                villagerRect.top >= prisonRect.top && villagerRect.bottom <= prisonRect.bottom)
                continue;

            if (IntersectRect(&intersect, &playerRect, &villagerRect))
            {
                // Yanlış kişiyi hapse atma kontrolü
                if (v->GetRageLevel() < 40) {
                    // Tüm köylülerin kudurma seviyesini 10 artır
                    for (Villager* vill : villagers) {
                        int newRage = vill->GetRageLevel() + 10;
                        if (newRage > 40) newRage = 40;
                        vill->SetRageLevel(newRage);
                    }
                }

                v->SetState(ARRESTED);
                // Hapishanenin ortasına yerleştir
                RECT prisonRect = GetPrisonRect();
                PlaySound(TEXT("Sounds\\prison.wav"), NULL, SND_FILENAME | SND_ASYNC);

                int prisonW = prisonRect.right - prisonRect.left;
                int prisonH = prisonRect.bottom - prisonRect.top;
                RECT rc;
                SetRect(&rc,
                    prisonRect.left + (prisonW - 64) / 2,
                    prisonRect.top + (prisonH - 64) / 2,
                    prisonRect.left + (prisonW + 64) / 2,
                    prisonRect.top + (prisonH + 64) / 2
                );
                v->GetSprite()->SetPosition(rc);
                break; // Sadece bir köylüyü yakala
            }
        }
        }
    
    else if (GetAsyncKeyState('K') & 0x8000) // hapishaneye gönderme
    {
        godmode = !godmode;
    }
    
    else
    {
        switch (_lastPlayerDir)
        {
        case UP:    _pCharacterSprite->SetFrame(walkUpFrames[0]); break;
        case DOWN:  _pCharacterSprite->SetFrame(walkDownFrames[0]); break;
        case LEFT:  _pCharacterSprite->SetFrame(walkLeftFrames[0]); break;
        case RIGHT: _pCharacterSprite->SetFrame(walkRightFrames[0]); break;
        }

        _pCharacterSprite->SetFrameDelay(-1);  // animasyon oynatmasın
    }
    
    RECT charRect = _pCharacterSprite->GetPosition();
    int dx = 0, dy = 0;
    if (charRect.left < 0) dx = -charRect.left;
    if (charRect.top < 0) dy = -charRect.top;
    if (charRect.right > MAP_WIDTH) dx = MAP_WIDTH - charRect.right;
    if (charRect.bottom > MAP_HEIGHT) dy = MAP_HEIGHT - charRect.bottom;
    if (dx != 0 || dy != 0)
        _pCharacterSprite->OffsetPosition(dx, dy);

    UpdateCamera();
}


void MouseButtonDown(int x, int y, BOOL bLeft)
{
}

void MouseButtonUp(int x, int y, BOOL bLeft)
{
}

void MouseMove(int x, int y)
{
}

BOOL SpriteCollision(Sprite* pSpriteHitter, Sprite* pSpriteHittee)
{
    return FALSE;
}
