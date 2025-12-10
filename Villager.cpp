#define NOMINMAX

#include "Villager.h"
#include <windows.h>
#include <algorithm> // std::min için
#include <string>
#include <sstream>
#include "SheriffGame.h"
#include "VillageBackground.h"

// Frame dizileri (dardan alnabilir)
extern int g_screenWidth;
extern int g_screenHeight;
extern int walkUp[], walkDown[], walkLeft[], walkRight[], villagerDie[];
extern VillageBackground* _pBackground;
int walkUp[] = { 73,74, 75, 76, 77, 78, 79, 80 };
int walkLeft[] = { 81, 82,83, 84, 85, 86, 87, 88, 89 };
int walkDown[] = { 90, 91,92, 93, 94, 95, 96, 97,98 };
int walkRight[] = { 99, 100,101,102,103,104,105,106,107 };
int villagerFrameIndex = 0;
int villagerDie[] = { 181,182,183,184,185 };
extern Sprite* _pCharacterSprite;
std::vector<Villager*> g_witnesses;

Villager::Villager(Sprite* sprite)
    : _sprite(sprite), _dir(static_cast<Direction>(rand() % 4)), _state(WALKING),
    _frameIndex(0), _rageLevel(0), _cooldown(0), _isInfected(false),
    _infectionTimer(0), _isKiller(false), _suspicionLevel(0),
    _lastKiller(nullptr), _fleeTimer(0), _memoryTimer(0), _isScared(false),
    _moveTimer(210 + rand() % 91), // 7-10 saniye arası
    _pauseTimer(0),
    _isPaused(false),
    _hunger(0), _hungerTick(0)
{
    _sprite->SetFrameDelay(-1);
    
    int roll = rand() % 100;
    if (roll < 30)
        _resistance = 30;      // Zayıf → çabuk kudurur
    else if (roll < 70)
        _resistance = 60;     // Orta
    else
        _resistance = 90;     // Güçlü → zor kudurur

}

Direction Villager::GetDirection() const {
    return _dir;
}

void Villager::SetTargetHouse(int houseId, int focusTime) {
    _targetHouseId = houseId;
    _focusTimer = focusTime;
}
void Villager::ClearTargetHouse() {
    _targetHouseId = -1;
    _focusTimer = 0;
}
int Villager::GetTargetHouse() const {
    return _targetHouseId;
}

void Villager::Update()
{



    if (_sprite == nullptr)
        return;


    if (_speechTimer > 0)
    {
        _speechTimer--;
        if (_speechTimer == 0)
        {
            _showSpeech = false;
            _speechText = L"";
        }
    }

    // Açlık sistemini güncelle
    if (_hunger < 10 && _state != DYING && _state != ARRESTED) {
        if (++_hungerTick >= 300) {
            _hunger++;
            _hungerTick = 0;
        }
    }


    int offsetX = 0, offsetY = 0;
    // Hareket/durma döngüsü
    //if (_isPaused)
    //{
    //    if (_pauseTimer > 0)
    //    {
    //        _pauseTimer--;
    //        // Duruyorsa animasyon oynatmasın
    //        _sprite->SetFrameDelay(-1);
    //        return;
    //    }
    //    else
    //    {
    //        // Tekrar hareket etmeye başla
    //        _isPaused = false;
    //        _moveTimer = 300 + rand() % 91; // 10-13 saniye
    //    }
    //}
    //else
    //{
    //    if (_moveTimer > 0)
    //    {
    //        _moveTimer--;
    //    }
    //    else
    //    {
    //        // Durma moduna geç
    //        _isPaused = true;
    //        _pauseTimer = 60 + rand() % 91; // 2-5 saniye
    //        _sprite->SetFrameDelay(-1); // Animasyon dursun
    //        return;
    //    }
    //}
        // Hareket/durma döngüsü
    if (_isPaused)
    {
        if (_pauseTimer > 0)
        {
            _pauseTimer--;
            _sprite->SetFrameDelay(-1);
            return;
        }
        else
        {
            // Tekrar hareket etmeye başla ve yön değiştir
            _isPaused = false;
            _dir = static_cast<Direction>(rand() % 4);
        }
    }



    if (_state == DYING)
    {
        int length = sizeof(villagerDie) / sizeof(int);
        if (_frameIndex < length)
        {
            _sprite->SetFrame(villagerDie[_frameIndex++]);
            _sprite->SetFrameDelay(6); // Animasyon dursun

        }
        return;
    }

    // Hafıza süresini güncelle
    if (_lastKiller != nullptr)
        _memoryTimer++;  // Süreyi saydırıyoruz

    // Kaçma durumunu güncelle
    if (_state == FLEEING)
    {
        _fleeTimer--;
        if (_fleeTimer <= 0)
        {
            _state = WALKING;
            _isScared = false;
        }
        else if (_lastKiller)
        {
            RECT myPos = _sprite->GetPosition();
            RECT killerPos = _lastKiller->GetSprite()->GetPosition();

            int dx = myPos.left - killerPos.left;
            int dy = myPos.top - killerPos.top;

            int primaryX = (dx > 0) ? 1 : -1;
            int primaryY = (dy > 0) ? 1 : -1;

            RECT pos = _sprite->GetPosition();
            bool moved = false;

            if (abs(dx) > abs(dy))

                // Tehlikeden uzaklaş 
                if (_lastKiller != nullptr)

                {
                    // Önce X yönünü dene
                    RECT testX = pos;
                    OffsetRect(&testX, primaryX, 0);
                    if (testX.left >= 25 && testX.top >= 25 &&
                        testX.right <= 475 && testX.bottom <= 375)
                    {
                        _sprite->OffsetPosition(primaryX, 0);
                        _dir = (primaryX > 0) ? RIGHT : LEFT;
                        moved = true;
                    }
                    else
                    {
                        // X olmadıysa Y yönünü dene
                        RECT testY = pos;
                        OffsetRect(&testY, 0, primaryY);
                        if (testY.left >= 25 && testY.top >= 25 &&
                            testY.right <= 475 && testY.bottom <= 375)
                        {
                            _sprite->OffsetPosition(0, primaryY);
                            _dir = (primaryY > 0) ? DOWN : UP;
                            moved = true;
                        }
                    }
                }
                else
                {
                    // Önce Y yönünü dene
                    RECT testY = pos;
                    OffsetRect(&testY, 0, primaryY);
                    if (testY.left >= 25 && testY.top >= 25 &&
                        testY.right <= 475 && testY.bottom <= 375)
                    {
                        _sprite->OffsetPosition(0, primaryY);
                        _dir = (primaryY > 0) ? DOWN : UP;
                        moved = true;
                    }
                    else
                    {
                        // Y olmadıysa X yönünü dene
                        RECT testX = pos;
                        OffsetRect(&testX, primaryX, 0);
                        if (testX.left >= 25 && testX.top >= 25 &&
                            testX.right <= 475 && testX.bottom <= 375)
                        {
                            _sprite->OffsetPosition(primaryX, 0);
                            _dir = (primaryX > 0) ? RIGHT : LEFT;
                            moved = true;
                        }
                    }
                }

            if (!moved)
            {
                _dir = static_cast<Direction>(rand() % 4); // Çaresizse rastgele dön
            }

            // Animasyonu güncelle
            int* currentFrames = nullptr;
            int length = 0;
            switch (_dir)
            {
            case UP:    currentFrames = walkUp;    length = sizeof(walkUp) / sizeof(int); break;
            case DOWN:  currentFrames = walkDown;  length = sizeof(walkDown) / sizeof(int); break;
            case LEFT:  currentFrames = walkLeft;  length = sizeof(walkLeft) / sizeof(int); break;
            case RIGHT: currentFrames = walkRight; length = sizeof(walkRight) / sizeof(int); break;
            }

            _sprite->SetFrame(currentFrames[_frameIndex]);
            _frameIndex = (_frameIndex + 1) % length;
        }
    }


    // --- Hedef eve odaklanma mantığı ---
    if (_targetHouseId >= 0 && _focusTimer > 0) {
        // Evin merkezine git
        const VillageHouse* targetHouse = nullptr;
        for (const auto& house : _pBackground->GetHouses()) {
            if (house.id == _targetHouseId) {
                targetHouse = &house;
                break;
            }
        }
        if (targetHouse) {
            RECT myRect = _sprite->GetPosition();
            int myX = (myRect.left + myRect.right) / 2;
            int myY = (myRect.top + myRect.bottom) / 2;
            int targetX = targetHouse->x + targetHouse->w / 2;
            int targetY = targetHouse->y + targetHouse->h / 2;

            int dx = targetX - myX;
            int dy = targetY - myY;

            // Basit yön seçimi
            if (abs(dx) > abs(dy))
                _dir = (dx > 0) ? RIGHT : LEFT;
            else
                _dir = (dy > 0) ? DOWN : UP;

            // Eğer çok yaklaştıysa hedefi bırak
            if (abs(dx) < 8 && abs(dy) < 8) {
                ClearTargetHouse();
            }
        }
        _focusTimer--;
        if (_focusTimer <= 0)
            ClearTargetHouse();
    }

    // Normal hareket
    if (_state == WALKING)
    {
        int* currentFrames = nullptr;
        int length = 0;

        // Katil ise, şerifin ekranında mı kontrol et
        bool isKiller = (_rageLevel >= 40);
        bool isSheriffScreen = false;
        if (isKiller)
        {
            // Şerifin ekran dikdörtgenini al
            extern int g_screenWidth, g_screenHeight;
            //const float g_zoom;
            extern POINT g_cameraPos;
            RECT sheriffScreen;
            sheriffScreen.left = g_cameraPos.x;
            sheriffScreen.top = g_cameraPos.y;
            /*sheriffScreen.right = g_cameraPos.x + (int)(g_screenWidth / g_zoom);
            sheriffScreen.bottom = g_cameraPos.y + (int)(g_screenHeight / g_zoom);*/
            sheriffScreen.right = g_cameraPos.x + (int)(g_screenWidth );
            sheriffScreen.bottom = g_cameraPos.y + (int)(g_screenHeight);
            RECT myRect = _sprite->GetPosition();
            int myX = (myRect.left + myRect.right) / 2;
            int myY = (myRect.top + myRect.bottom) / 2;

            if (myX >= sheriffScreen.left && myX < sheriffScreen.right &&
                myY >= sheriffScreen.top && myY < sheriffScreen.bottom)
            {
                isSheriffScreen = true;
            }
        }

        if (isKiller)
        {
            // Katil ve şerifin ekranında değilse hedefe odaklan
            Villager* nearest = nullptr;
            int minDistSq = INT_MAX;
            RECT myRect = _sprite->GetPosition();
            int myX = (myRect.left + myRect.right) / 2;
            int myY = (myRect.top + myRect.bottom) / 2;

            RECT playerRect = _pCharacterSprite->GetPosition();
            int playerX = (playerRect.left + playerRect.right) / 2;
            int playerY = (playerRect.top + playerRect.bottom) / 2;
            int playerRadius = 80; // Oyuncunun çevresi (piksel cinsinden)

            for (Villager* v : villagers)
            {
                if (v == this || v->GetState() == DYING || v->GetState() == ARRESTED)
                    continue;

                RECT vRect = v->GetSprite()->GetPosition();
                int vX = (vRect.left + vRect.right) / 2;
                int vY = (vRect.top + vRect.bottom) / 2;

                // Eğer bu köylü oyuncunun çevresindeyse, hedef olarak alma
                int dxPlayer = vX - playerX;
                int dyPlayer = vY - playerY;
                int distSqPlayer = dxPlayer * dxPlayer + dyPlayer * dyPlayer;
                if (distSqPlayer < playerRadius * playerRadius)
                    continue;

                int dx = vX - myX;
                int dy = vY - myY;
                int distSq = dx * dx + dy * dy;
                if (distSq < minDistSq)
                {
                    minDistSq = distSq;
                    nearest = v;
                }
            }

            if (nearest)
            {
                RECT targetRect = nearest->GetSprite()->GetPosition();
                int targetX = (targetRect.left + targetRect.right) / 2;
                int targetY = (targetRect.top + targetRect.bottom) / 2;
                int dx = targetX - myX;
                int dy = targetY - myY;

                // Hangi eksende daha uzaksa o yöne git
                if (abs(dx) > abs(dy))
                    _dir = (dx > 0) ? RIGHT : LEFT;
                else
                    _dir = (dy > 0) ? DOWN : UP;
            }
        }
        else if (isKiller && isSheriffScreen)
        {
            // Katil ve şerifin ekranındaysa, rastgele hareket etsin
            if (rand() % 100 < 2)
                _dir = static_cast<Direction>(rand() % 4);
        }



        int moveX = 0, moveY = 0;
        switch (_dir)
        {
        case UP:    currentFrames = walkUp;    length = sizeof(walkUp) / sizeof(int);    moveY = -1; break;
        case DOWN:  currentFrames = walkDown;  length = sizeof(walkDown) / sizeof(int);  moveY = 1;  break;
        case LEFT:  currentFrames = walkLeft;  length = sizeof(walkLeft) / sizeof(int);  moveX = -1; break;
        case RIGHT: currentFrames = walkRight; length = sizeof(walkRight) / sizeof(int); moveX = 1;  break;
        }

        // Hız katsayısı
        /*int speed = 2;
        if (_hunger <= 5) speed = 1;*/
        int speed = 1;

        moveX *= speed;
        moveY *= speed;


        // Collision kontrolü
        RECT origRect = _sprite->GetPosition();
        RECT newRect = origRect;
        OffsetRect(&newRect, moveX, moveY);

        bool obstacleCollision = false;
        if (_pBackground) {
            for (const RECT& obs : _pBackground->GetObstacles()) {
                RECT intersect;
                if (IntersectRect(&intersect, &newRect, &obs)) {
                    obstacleCollision = true;
                    break;
                }
            }
        }
        // Prison collision check
        RECT prisonRect = GetPrisonRect();
        bool prisonCollision = false;
        if (!obstacleCollision) {
            RECT intersect;
            if (IntersectRect(&intersect, &newRect, &prisonRect)) {
                prisonCollision = true;
            }
        }

        if (!obstacleCollision && !prisonCollision) {
            _sprite->OffsetPosition(moveX, moveY);
        }
        else if (obstacleCollision) {
            if (_hunger >= 2) {
                _hunger-=2;
            }
			else if (_hunger == 1) {
				_hunger--;
			}
            // Sadece normal engelde pause moduna geç
            if (!_isPaused) {
                _isPaused = true;
                _pauseTimer = 60 + rand() % 91; // 2-5 saniye (60 FPS için)
                _sprite->SetFrameDelay(-1); // Animasyon dursun
            }
            return;
        }
        else if (prisonCollision) {
            _sprite->OffsetPosition(-moveX, -moveY);
            _dir = static_cast<Direction>(rand() % 4);
            return;
        }




        RECT pos = _sprite->GetPosition();

        switch (_dir)
        {
        case UP:    offsetY = -1; break;
        case DOWN:  offsetY = 1; break;
        case LEFT:  offsetX = -1; break;
        case RIGHT: offsetX = 1; break;
        }

        // Villager.cpp, inside Villager::Update()

// Use global screen size for movement boundaries
        if (pos.left < 0 || pos.top < 0 || pos.right > MAP_WIDTH || pos.bottom > MAP_HEIGHT)
        {
            _sprite->OffsetPosition(-offsetX, -offsetY);
            // Only non-killers change direction randomly
            if (_rageLevel < 40)
                _dir = static_cast<Direction>(rand() % 4);
            return;
        }



        if (_frameIndex >= length) _frameIndex = 0;
        _sprite->SetFrame(currentFrames[_frameIndex]);
        _frameIndex = (_frameIndex + 1) % length;


        // Sadece katil olmayanlar rastgele yön değiştirsin
        if (_rageLevel < 40)
        {
            if (rand() % 100 < 2)
                _dir = static_cast<Direction>(rand() % 4);
        }
    }


    // Kudurma seviyesi güncelleme
    if (_isInfected)
    {
        if (_infectionTimer <= 0)
        {
            _rageLevel = std::min(_rageLevel + 10, 40);
            _isInfected = false;
        }
        else
        {
            _infectionTimer--;

            if (_infectionTimer == 0)
            {
                _rageLevel = (_rageLevel + 5 > 40) ? 40 : _rageLevel + 5;
                _isInfected = false;
            }
        }
    }

    // Katil olma kontrolü
    if (_rageLevel >= 40 && !_isKiller)
    {
        _isKiller = true;
    }

    if (_state == ARRESTED)
    {
        if (_prisonTimer > 0)
        {
            _prisonTimer--;
            return;
        }
        _state = WALKING;
        _dir = static_cast<Direction>(rand() % 4);
        RECT rc = _sprite->GetPosition();
        int offsetX = 0, offsetY = 0;
        switch (_dir)
        {
        case UP:    offsetY = -70; break;
        case DOWN:  offsetY = 70; break;
        case LEFT:  offsetX = -70; break;
        case RIGHT: offsetX = 70; break;
        }
        RECT prisonRect = GetPrisonRect();
        do {
            OffsetRect(&rc, offsetX, offsetY);
        } while (rc.left < prisonRect.right && rc.right > prisonRect.left &&
            rc.top < prisonRect.bottom && rc.bottom > prisonRect.top);
        _sprite->SetPosition(rc);
        return;

    }


    if (_cooldown > 0)
        _cooldown--;

}


void Villager::SpreadToNearby(std::vector<Villager*>& others)
{
    for (Villager* other : others)
    {
        if (other == this) continue;
        InfectIfClose(other);
    }
}
VillagerState Villager::GetState() const {
    return _state;
}
void Villager::Draw(HDC hDC)
{
    if (_sprite == nullptr)
        return;

    _sprite->Draw(hDC);

    if (_showSpeech)
    {
        RECT rect = _sprite->GetPosition();
        int centerX = (rect.left + rect.right) / 2;
        int topY = rect.top - 30;
        RECT speechRect = { centerX - 80, topY - 30, centerX + 80, topY + 30 };
        DrawTextW(hDC, _speechText.c_str(), -1, &speechRect, DT_CENTER | DT_WORDBREAK);
    }
    // Açlık seviyesini yaz
    RECT rect = _sprite->GetPosition();
    int centerX = (rect.left + rect.right) / 2;
    int topY = rect.top - 10;
    char hungerText[32];
    sprintf(hungerText, "%d", _hunger);
    SetTextColor(hDC, RGB(255, 255, 0));
    SetBkMode(hDC, TRANSPARENT);
    TextOutA(hDC, centerX +10, topY, hungerText, (int)strlen(hungerText));

    //if (_sprite == nullptr)
    //    return;

    //_sprite->Draw(hDC);

    //RECT rect = _sprite->GetPosition();

    //// Çerçeve rengi
    //HPEN hPen = nullptr;
    //if (_state == ARRESTED)
    //    hPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 255)); // Mavi
    //else
    //    hPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0)); // Kırmızı

    //HPEN hOldPen = (HPEN)SelectObject(hDC, hPen);
    //HBRUSH hOldBrush = (HBRUSH)SelectObject(hDC, GetStockObject(HOLLOW_BRUSH));

    //Rectangle(hDC, rect.left, rect.top, rect.right, rect.bottom);

    //// Görüş açısı çizgileri
    //int x = (rect.left + rect.right) / 2;
    //int y = (rect.top + rect.bottom) / 2;

    //int radius = 200; // Görüş mesafesi
    //float angleRad = 3.14159f / 2; // 90 derece

    //float directionAngle = 0;

    //switch (_dir)
    //{
    //case UP:    directionAngle = -3.14159f / 2; break;
    //case DOWN:  directionAngle = 3.14159f / 2; break;
    //case LEFT:  directionAngle = 3.14159f; break;
    //case RIGHT: directionAngle = 0; break;
    //}

    //HPEN hVisionPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
    //HPEN hOldVisionPen = (HPEN)SelectObject(hDC, hVisionPen);

    //int x1 = x + (int)(radius * cos(directionAngle - angleRad));
    //int y1 = y + (int)(radius * sin(directionAngle - angleRad));
    //int x2 = x + (int)(radius * cos(directionAngle + angleRad));
    //int y2 = y + (int)(radius * sin(directionAngle + angleRad));

    //MoveToEx(hDC, x, y, NULL);
    //LineTo(hDC, x1, y1);
    //MoveToEx(hDC, x, y, NULL);
    //LineTo(hDC, x2, y2);

    //if (!(x1 == x2 && y1 == y2))
    //    Arc(hDC, x - radius, y - radius, x + radius, y + radius, x1, y1, x2, y2);

    //if (_showSpeech)
    //{
    //    RECT rect = _sprite->GetPosition();
    //    int centerX = (rect.left + rect.right) / 2;
    //    int topY = rect.top - 30;
    //    RECT speechRect = { centerX - 80, topY - 30, centerX + 80, topY + 30 };
    //    DrawTextW(hDC, _speechText.c_str(), -1, &speechRect, DT_CENTER | DT_WORDBREAK);
    //}


    //// Temizle
    //SelectObject(hDC, hOldVisionPen);
    //DeleteObject(hVisionPen);

    //SelectObject(hDC, hOldPen);
    //SelectObject(hDC, hOldBrush);
    //DeleteObject(hPen);
}

void Villager::InfectIfClose(Villager* other)
{
    if (_sprite == nullptr)
        return;

    // Eğer köylü ölüyse veya yürümüyorsa enfekte edemez
    if (_state != WALKING || _cooldown > 0 || _rageLevel < 10)
        return;

    // Hedef köylü ölüyse veya yürümüyorsa enfekte olamaz
    if (other->_state != WALKING)
        return;

    RECT r1 = _sprite->GetPosition();
    RECT r2 = other->GetSprite()->GetPosition();
    RECT intersect;

    if (IntersectRect(&intersect, &r1, &r2))
    {
        if (other->_rageLevel < _rageLevel && other->_rageLevel < 40)
        {
            other->_isInfected = true;
            other->_infectionTimer = other->_resistance;
            _cooldown = 60;
        }
    }
}
void Villager::TryKillNearby(std::vector<Villager*>& others)
{
    if (_sprite == nullptr)
        return;

    if (_rageLevel != 40 || _state != WALKING)
        return;

    RECT myRect = _sprite->GetPosition();

    // Oyuncunun ekran dikdörtgenini al
    extern int g_screenWidth, g_screenHeight;
    RECT sheriffScreen;
    sheriffScreen.left = g_cameraPos.x;
    sheriffScreen.top = g_cameraPos.y;
    sheriffScreen.right = g_cameraPos.x + g_screenWidth / g_zoom;
    sheriffScreen.bottom = g_cameraPos.y + g_screenHeight / g_zoom;


    for (Villager* other : others)
    {
        if (other == this || other->GetState() != WALKING)
            continue;

        // HAPİSTEYSE ÖLDÜRME!
        RECT otherRect = other->GetSprite()->GetPosition();
        if (otherRect.left >= 200 && otherRect.right <= 200 &&
            otherRect.top >= 200 && otherRect.bottom <= 200)
            continue;

        // Diğer köylünün merkezi noktası
        int vX = (otherRect.left + otherRect.right) / 2;
        int vY = (otherRect.top + otherRect.bottom) / 2;

        
        // Eğer köylü şerifin ekranındaysa öldürme!
        if (vX >= sheriffScreen.left && vX < sheriffScreen.right &&
            vY >= sheriffScreen.top && vY < sheriffScreen.bottom)
        {
            if (!godmode)
                continue;
        }
        

        int dx = ((myRect.left + myRect.right) / 2) - vX;
        int dy = ((myRect.top + myRect.bottom) / 2) - vY;
        int distSq = dx * dx + dy * dy;

        if (distSq <= 40 * 40)  // Mesafe uygunsa (40px)
        {
            other->SetState(DYING);


            if (_rageLevel >= 40 && _hunger > 0) {
                _hunger--;
            }

            AssignWitnessesByDistance(villagers, this, _pCharacterSprite);

            for (Villager* witness : g_witnesses)
            {
                if (witness != nullptr && witness->GetState() == WALKING)
                    witness->WitnessMurder(this);
            }



            // Show kill message if this villager is a killer
            if (_rageLevel >= 40) {
                extern char g_killMessage[];
                extern int g_killMessageTimer;
                strcpy_s(g_killMessage, 30, "A villager was killed!");
                PlaySound(TEXT("Sounds\\gunshot.wav"), NULL, SND_FILENAME | SND_ASYNC);
                g_killMessageTimer = 60; // 3 seconds at 30 FPS
            }
            // KATİLİN ÖLDÜRDÜĞÜ YERİ KAYDET
            extern std::vector<POINT> g_killPositions;
            POINT killPos;
            killPos.x = (otherRect.left + otherRect.right) / 2;
            killPos.y = (otherRect.top + otherRect.bottom) / 2;
            g_killPositions.push_back(killPos);
            


            // Katil ışınlansın: collision olmayan ve sheriff ekranı dışında bir yere
            extern VillageBackground* _pBackground;
            extern int g_screenWidth, g_screenHeight;
            extern POINT g_cameraPos;

            const int maxTries = 100;
            RECT newRect;
            bool found = false;
            int frameW = 64, frameH = 64;

            for (int tryCount = 0; tryCount < maxTries; ++tryCount) {
                int x = rand() % (MAP_WIDTH - frameW);
                int y = rand() % (MAP_HEIGHT - frameH);
                SetRect(&newRect, x, y, x + frameW, y + frameH);

                // 1. Obstacle collision check
                bool collision = false;
                for (const RECT& obs : _pBackground->GetObstacles()) {
                    RECT intersect;
                    if (IntersectRect(&intersect, &newRect, &obs)) {
                        collision = true;
                        break;
                    }
                }
                // 2. Prison collision check
                if (!collision) {
                    RECT prisonRect = GetPrisonRect();
                    RECT intersect;
                    if (IntersectRect(&intersect, &newRect, &prisonRect)) {
                        collision = true;
                    }
                }
                // 3. Sheriff screen check
                if (!collision) {
                    RECT sheriffScreen;
                    sheriffScreen.left = g_cameraPos.x;
                    sheriffScreen.top = g_cameraPos.y;
                    sheriffScreen.right = g_cameraPos.x + int(g_screenWidth / g_zoom);
                    sheriffScreen.bottom = g_cameraPos.y + int(g_screenHeight / g_zoom);

                    if (!(newRect.right < sheriffScreen.left || newRect.left > sheriffScreen.right ||
                        newRect.bottom < sheriffScreen.top || newRect.top > sheriffScreen.bottom)) {
                        collision = true;
                    }
                }
                if (!collision) {
                    found = true;
                    break;
                }
            }

            if (found) {
                _sprite->SetPosition(newRect);
            }

            break; // Sadece birini öldür

        }

    }
}

void Villager::SetState(VillagerState state)
{
    // Eğer bu bir katilse ve öldürülüyorsa, hayatta başka katil kalmadıysa oyunu bitir
    if (_state == WALKING && state == DYING && _rageLevel >= 40) {
        int aliveKillers = 0;
        for (auto vill : villagers) {
            if (vill->GetRageLevel() >= 40 && vill->GetState() != DYING && vill != this)
                aliveKillers++;
        }
        // Bu son katilse oyunu bitir
        if (aliveKillers == 0) {
            strcpy_s(g_gameOverMessage, "Killer is Dead! Game Over.");
            g_gameOver = true;
        }
    }
    _state = state;
    _frameIndex = 0;
    if (state == ARRESTED)
        _prisonTimer = 900; // 10 saniye (30 FPS * 10)
}

#include <cmath> // atan2, sqrt, cos, etc.


void Villager::SetRageLevel(int level) { _rageLevel = level; }
int Villager::GetRageLevel() const { return _rageLevel; }
Sprite* Villager::GetSprite() { return _sprite; }

void Villager::WitnessMurder(Villager* killer)
{
    if (_state == WALKING )
    {
        _lastKiller = killer;
        _memoryTimer = 0;

        // Yalan mı söyleyecek? Karar ver
        if ((rand() % 100) < (int)(GetTruthChance() * 100))
        {
            _isLying = false;
        }
        else
        {
            _isLying = true;

            // Hemen yalan answer'ını seç
            do {
                _lieAnswer = villagers[rand() % villagers.size()];
            } while (_lieAnswer == killer);
        }

        _isScared = true;
        _state = FLEEING;
        _fleeTimer = 180;
    }
}

void Villager::FleeFrom(Villager* threat)
{
    if (_state == WALKING )
    {
        _lastKiller = threat;
        _isScared = true;
        _state = FLEEING;
        _fleeTimer = 120;    // 4 saniye kaçsın
    }
}
bool Villager::KnowsKiller() const {
    return _lastKiller != nullptr;
}

Villager* Villager::GetLastKiller() const {
    return _lastKiller;
}
int Villager::GetID() const {
    return _id;
}
void Villager::SetID(int id) { _id = id; }

int Villager::GetResistance() const {
    return _resistance;
}
float Villager::GetTruthChance() const {
    if (_rageLevel < 20)
        return 1.0f;   // %100 doğru
    else if (_rageLevel == 20)
        return 0.75f;  // %75 doğru
    else if (_rageLevel == 30)
        return 0.60f;  // %60 doğru
    else
        return 0.45f;  // %45 doğru
}
int Villager::GetWitnessedKillerID() const {
    return _witnessedKillerID;
}

int Villager::GetMemoryTimer() const {
    return _memoryTimer;
}
float Villager::GetMemoryConfidence() const
{
    if (_memoryTimer <= 900)  // 30 saniye (60 FPS varsayıldı)
        return 1.0f;           // %100 emin
    else if (_memoryTimer <= 1800)  // 60 saniye
        return 0.75f;
    else if (_memoryTimer <= 2700)  // 90 saniye
        return 0.50f;
    else
        return 0.30f;
}
int Villager::GetRandomWrongID(int correctID)
{
    int fakeID = correctID;
    int attempts = 10;

    while (fakeID == correctID && attempts-- > 0)
    {
        int r = rand() % villagers.size();  // global villagers listesi
        if (villagers[r]->GetID() != correctID)
        {
            fakeID = villagers[r]->GetID();
        }
    }

    return fakeID;
}
void Villager::AssignWitnessesByDistance(std::vector<Villager*>& villagers, Villager* killer, Sprite* sheriffSprite)
{
    std::vector<std::pair<Villager*, float>> distances;

    // Sheriff pozisyonu
    RECT sheriffRect = sheriffSprite->GetPosition();
    int sx = (sheriffRect.left + sheriffRect.right) / 2;
    int sy = (sheriffRect.top + sheriffRect.bottom) / 2;

    for (Villager* v : villagers)
    {
        if (v != killer && v->GetState() == WALKING)
        {
            RECT r = v->GetSprite()->GetPosition();
            int vx = (r.left + r.right) / 2;
            int vy = (r.top + r.bottom) / 2;

            float dist = sqrtf((vx - sx) * (vx - sx) + (vy - sy) * (vy - sy));
            distances.push_back(std::make_pair(v, dist));
        }
    }

    // Uzaklığa göre sırala (küçükten büyüğe)
    std::sort(distances.begin(), distances.end(),
        [](const std::pair<Villager*, float>& a, const std::pair<Villager*, float>& b)
        {
            return a.second < b.second;
        });

    int total = (int)distances.size();
    if (total == 0) return;

    // 1. Tanık → en yakındaki
    g_witnesses.push_back(distances[0].first);

    // 2. Tanık → ortadaki (varsa)
    if (total >= 2)
        g_witnesses.push_back(distances[total / 2].first);

    // 3. Tanık → en uzaktaki (varsa)
    if (total >= 3)
        g_witnesses.push_back(distances.back().first);
}


