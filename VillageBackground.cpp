#include "VillageBackground.h"
#include <cstdlib>
#include "SheriffGame.h" 

// İki RECT çakışıyor mu?
static bool RectsOverlap(const RECT& a, const RECT& b) {
    return !(a.right <= b.left || a.left >= b.right || a.bottom <= b.top || a.top >= b.bottom);
}

static int Clamp(int value, int min, int max) {
    return (value < min) ? min : (value > max) ? max : value;
}

VillageBackground::VillageBackground(int width, int height)
    : m_width(width), m_height(height)
{
    //Haritadaki yolun dikdörtgensel büyüklüðü
    RECT roadRect;
    int roadWidth = 80;
    roadRect.left = m_width / 2 - roadWidth / 2;
    roadRect.right = m_width / 2 + roadWidth / 2;
    roadRect.top = 0;
    roadRect.bottom = m_height;

    // Hapishane ve başlangıç alanı
    RECT prisonRect = GetPrisonRect();
    RECT expandedPrisonRect = prisonRect;
    const int prisonMargin = 100; // You can adjust this value for a wider area
    expandedPrisonRect.left -= prisonMargin;
    expandedPrisonRect.top -= prisonMargin;
    expandedPrisonRect.right += prisonMargin;
    expandedPrisonRect.bottom += prisonMargin;

    // Karakterin doğduğu alan: hapishanenin ortasının 120px sağında 64x64'lük bir kutu
    RECT startRect;
    int startW = 64, startH = 64;
    int startX = (prisonRect.left + prisonRect.right) / 2 + 120;
    int startY = (prisonRect.top + prisonRect.bottom) / 2 - startH / 2;
    startRect.left = startX;
    startRect.top = startY;
    startRect.right = startX + startW;
    startRect.bottom = startY + startH;

    // Ağaçlar
    for (int i = 0; i < 10; ++i) {
        int tryCount = 0;
        while (true) {
            int treeType = 0;     // Artık tüm ağaçlar "normal" ağaç
            int size = 75;        // Ortalama boyutta sabit bir değer

            int margin = 50;
            int treeX = Clamp(rand() % width, margin, width - margin);
            int treeY = Clamp(rand() % height, margin, height - margin);
            VillageTree tree = { treeX, treeY, size, treeType };

            RECT treeRect;
            int trunkWidth = size / 4;
            int trunkHeight = size / 2;
            treeRect.left = tree.x - trunkWidth / 2;
            treeRect.right = tree.x + trunkWidth / 2;
            treeRect.top = tree.y + size / 2;
            treeRect.bottom = tree.y + size;

            RECT leafRect;
            leafRect.left = tree.x - size / 2;
            leafRect.right = tree.x + size / 2;
            leafRect.top = tree.y;
            leafRect.bottom = tree.y + size / 2;

            // Harita sınırlarını aşmasın
            leafRect.left = Clamp(leafRect.left, 0, width);
            leafRect.right = Clamp(leafRect.right, 0, width);
            leafRect.top = Clamp(leafRect.top, 0, height);
            leafRect.bottom = Clamp(leafRect.bottom, 0, height);

            treeRect.left = Clamp(treeRect.left, 0, width);
            treeRect.right = Clamp(treeRect.right, 0, width);
            treeRect.top = Clamp(treeRect.top, 0, height);
            treeRect.bottom = Clamp(treeRect.bottom, 0, height);

            // Çakışma kontrolü
            bool overlap = false;
            for (const RECT& obs : m_obstacles) {
                if (RectsOverlap(treeRect, obs)) {
                    overlap = true;
                    break;
                }
            }

            if (!overlap && !RectsOverlap(treeRect, expandedPrisonRect) && !RectsOverlap(treeRect, startRect) && !RectsOverlap(treeRect, roadRect)) {
                m_trees.push_back(tree);
                m_obstacles.push_back(leafRect);
                m_obstacles.push_back(treeRect);
                break;
            }

            if (++tryCount > 100) break;
        }
    }


    // Rastgele evler
    for (int i = 0; i < 10; ++i) {
        int tryCount = 0;
        while (true) {
            int houseType;
            if (i < 4) {
                houseType = 0; // 2x2
            }
            else if (i >= 4 && i < 8) {
                houseType = 1; // 2x4
            }
            else {
                houseType = 2; // 2x6
            }

            int w, h;
            switch (houseType) {
            case 0: w = 128; h = 128; break;
            case 1: w = 256; h = 128; break;
            default: w = 384; h = 128; break;
            }

            int roofHeight = h / 2; // çatı yüksekliği

            int margin = 50;
            int maxX = width - w - margin;
            int maxY = height - h - margin;
            int houseX = Clamp(rand() % width, margin, maxX);
            int houseY = Clamp(rand() % height, margin + roofHeight, maxY); // yukarı çatı eklendiği için margin arttı

            VillageHouse house = { houseX, houseY, w, h, houseType, i };

            // Çatı + ev birleşik collision dikdörtgeni
            RECT houseRect = {
                house.x,
                house.y - roofHeight,               // çatıdan başlar
                house.x + house.w,
                house.y + house.h                   // evin altına kadar
            };

            // Clamp'le sınırlandır
            houseRect.left = Clamp(houseRect.left, 0, width);
            houseRect.right = Clamp(houseRect.right, 0, width);
            houseRect.top = Clamp(houseRect.top, 0, height);
            houseRect.bottom = Clamp(houseRect.bottom, 0, height);

            // Çakışma kontrolü
            bool overlap = false;
            for (const RECT& obs : m_obstacles) {
                if (RectsOverlap(houseRect, obs)) {
                    overlap = true;
                    break;
                }
            }

            if (!overlap && !RectsOverlap(houseRect, expandedPrisonRect) && !RectsOverlap(houseRect, startRect) && !RectsOverlap(houseRect, roadRect)) {
                m_houses.push_back(house);
                m_obstacles.push_back(houseRect); // tek dikdörtgen collision
                break;
            }

            if (++tryCount > 100) break;
        }
    }





}

const std::vector<RECT>& VillageBackground::GetObstacles() const {
    return m_obstacles;
}
const std::vector<VillageHouse>& VillageBackground::GetHouses() const {
    return m_houses;
}


VillageBackground::~VillageBackground() {}

void VillageBackground::Update() {
    // Þimdilik animasyon yok, istersek ekleyebiliriz.
}

void VillageBackground::Draw(HDC hdc, int cameraX, int cameraY, int screenW, int screenH, float zoom)
{
    // Görünen alanı hesapla
    int viewLeft = cameraX;
    int viewTop = cameraY;
    int viewRight = cameraX + (int)(screenW / zoom);
    int viewBottom = cameraY + (int)(screenH / zoom);

    // 1. Çimen zemin (sadece görünen alaný doldur)
    HBRUSH grassBrush = CreateSolidBrush(RGB(80, 180, 80));
    RECT rc = { 0, 0, screenW, screenH };
    FillRect(hdc, &rc, grassBrush);
    DeleteObject(grassBrush);

    // 2. Yol (ortadan geçen bir yol)
    HBRUSH roadBrush = CreateSolidBrush(RGB(180, 140, 80));
    int roadLeft = m_width / 2 - 40 - viewLeft;
    int roadRight = m_width / 2 + 40 - viewLeft;
    RECT road = {
        (LONG)(roadLeft * zoom),
        0,
        (LONG)(roadRight * zoom),
        (LONG)(screenH)
    };
    FillRect(hdc, &road, roadBrush);
    DeleteObject(roadBrush);

    // 3. Evler
    for (const auto& house : m_houses) {

        // ... ekranda mı kontrolü ...
        COLORREF color;
        if (house.type == 0)
            color = RGB(200, 180, 120); // Tip 0: Sarýmsý
        else if (house.type == 1)
            color = RGB(180, 120, 200); // Tip 1: Morumsu
        else if (house.type == 2)
            color = RGB(120, 180, 200); // Tip 2: Mavimsi
        else
            color = RGB(150, 150, 150); // Bilinmeyen tipler için gri

        HBRUSH houseBrush = CreateSolidBrush(color);
        RECT h = {
            (LONG)((house.x - viewLeft) * zoom),
            (LONG)((house.y - viewTop) * zoom),
            (LONG)((house.x + house.w - viewLeft) * zoom),
            (LONG)((house.y + house.h - viewTop) * zoom)
        };
        FillRect(hdc, &h, houseBrush);
        DeleteObject(houseBrush);

        // --- Kapı ---
        int doorWidth = 32;
        int doorHeight = 64;
        int doorX = house.x + house.w / 2 - doorWidth / 2;
        int doorY = house.y + house.h - doorHeight;

        RECT doorRect = {
            (LONG)((doorX - viewLeft) * zoom),
            (LONG)((doorY - viewTop) * zoom),
            (LONG)((doorX + doorWidth - viewLeft) * zoom),
            (LONG)((doorY + doorHeight - viewTop) * zoom)
        };

        HBRUSH doorBrush = CreateSolidBrush(RGB(80, 40, 0)); // koyu kahverengi
        FillRect(hdc, &doorRect, doorBrush);
        DeleteObject(doorBrush);

        // --- Kapı Kolu ---
        HBRUSH knobBrush = CreateSolidBrush(RGB(0, 0, 0)); // siyah
        int knobRadius = 4;
        int knobCenterX = doorX + doorWidth - 8;
        int knobCenterY = doorY + doorHeight / 2;
        Ellipse(
            hdc,
            (LONG)((knobCenterX - knobRadius - viewLeft) * zoom),
            (LONG)((knobCenterY - knobRadius - viewTop) * zoom),
            (LONG)((knobCenterX + knobRadius - viewLeft) * zoom),
            (LONG)((knobCenterY + knobRadius - viewTop) * zoom)
        );
        DeleteObject(knobBrush);

        // --- Pencereler (2 tane, sol ve sað üstte) ---
        int winSize = 32;
        int winMargin = 10;

        // Sol pencere
        int winX1 = house.x + winMargin;
        int winY1 = house.y + winMargin;

        // Sağ pencere
        int winX2 = house.x + house.w - winMargin - winSize;
        int winY2 = house.y + winMargin;

        // Her pencere için çizim fonksiyonu (2 kez tekrarlanýyor)
        for (int i = 0; i < 2; ++i) {
            int winX = (i == 0) ? winX1 : winX2;
            int winY = (i == 0) ? winY1 : winY2;

            RECT winRect = {
                (LONG)((winX - viewLeft) * zoom),
                (LONG)((winY - viewTop) * zoom),
                (LONG)((winX + winSize - viewLeft) * zoom),
                (LONG)((winY + winSize - viewTop) * zoom)
            };

            HBRUSH winBrush = CreateSolidBrush(RGB(180, 220, 255)); // açık mavi
            FillRect(hdc, &winRect, winBrush);
            DeleteObject(winBrush);

            // Bölme çizgileri
            HPEN blackPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
            HPEN oldPen = (HPEN)SelectObject(hdc, blackPen);

            // Dikey çizgi
            MoveToEx(hdc, (int)((winX + winSize / 2 - viewLeft) * zoom), (int)((winY - viewTop) * zoom), NULL);
            LineTo(hdc, (int)((winX + winSize / 2 - viewLeft) * zoom), (int)((winY + winSize - viewTop) * zoom));

            // Yatay çizgi
            MoveToEx(hdc, (int)((winX - viewLeft) * zoom), (int)((winY + winSize / 2 - viewTop) * zoom), NULL);
            LineTo(hdc, (int)((winX + winSize - viewLeft) * zoom), (int)((winY + winSize / 2 - viewTop) * zoom));

            SelectObject(hdc, oldPen);
            DeleteObject(blackPen);
        }


        
        // Çatı
        HBRUSH roofBrush = CreateSolidBrush(RGB(150, 75, 0)); // koyu kahverengi iç
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, roofBrush);

        HPEN roofPen = CreatePen(PS_SOLID, 2, RGB(120, 60, 30)); // kenar çizgisi
        HPEN oldPen = (HPEN)SelectObject(hdc, roofPen);

        POINT roof[3] = {
            { (LONG)((house.x - viewLeft) * zoom), (LONG)((house.y - viewTop) * zoom) },
            { (LONG)((house.x + house.w / 2 - viewLeft) * zoom), (LONG)((house.y - house.h / 2 - viewTop) * zoom) },
            { (LONG)((house.x + house.w - viewLeft) * zoom), (LONG)((house.y - viewTop) * zoom) }
        };

        Polygon(hdc, roof, 3);

        // Eski fırça ve kalemi geri yükle
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(roofPen);
        DeleteObject(roofBrush);


        // --- Bacalar (çatı uçlarına) ---
        const int chimneyH = 55;
        const int chimneyW = 16;
        const int chimneyOffset = 10;

        int chimneyBaseY = house.y - house.h + 120;

        HBRUSH chimneyBrush = CreateSolidBrush(RGB(100, 100, 0)); // sarımtırak baca

        // Sol baca
        RECT chimney1 = {
            (LONG)((house.x + chimneyOffset - viewLeft) * zoom),
            (LONG)((chimneyBaseY - chimneyH - viewTop) * zoom),
            (LONG)((house.x + chimneyOffset + chimneyW - viewLeft) * zoom),
            (LONG)((chimneyBaseY - viewTop) * zoom)
        };
        FillRect(hdc, &chimney1, chimneyBrush);

        // Sağ baca
        RECT chimney2 = {
            (LONG)((house.x + house.w - chimneyOffset - chimneyW - viewLeft) * zoom),
            (LONG)((chimneyBaseY - chimneyH - viewTop) * zoom),
            (LONG)((house.x + house.w - chimneyOffset - viewLeft) * zoom),
            (LONG)((chimneyBaseY - viewTop) * zoom)
        };
        FillRect(hdc, &chimney2, chimneyBrush);

        DeleteObject(chimneyBrush);


        // Draw house ID above the house
        char idBuffer[16];
        snprintf(idBuffer, sizeof(idBuffer), "%d", house.id);
        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, TRANSPARENT);

        // Calculate the center of the house roof
        int textX = (int)((house.x + house.w / 2 - viewLeft) * zoom);
        int textY = (int)((house.y - house.h / 2 - viewTop) * zoom) + 80; // 18px above the roof

        TextOutA(hdc, textX - 8, textY, idBuffer, (int)strlen(idBuffer));
    }

    // 4. Aðaçlar
    for (const auto& tree : m_trees) {
        if (tree.x + tree.size < viewLeft || tree.x > viewRight ||
            tree.y + tree.size * (tree.type == 1 ? 3 : 1) < viewTop || tree.y > viewBottom)
            continue;

        // Gövde
        HBRUSH trunkBrush = CreateSolidBrush(RGB(120, 80, 40));
        int trunkHeight = tree.size * (tree.type == 1 ? 3 : 1);
        RECT trunk = {
            (LONG)((tree.x - tree.size / 8 - viewLeft) * zoom),
            (LONG)((tree.y + trunkHeight / 2 - viewTop) * zoom),
            (LONG)((tree.x + tree.size / 8 - viewLeft) * zoom),
            (LONG)((tree.y + trunkHeight - viewTop) * zoom)
        };
        FillRect(hdc, &trunk, trunkBrush);
        DeleteObject(trunkBrush);

        // Yaprak
        HBRUSH leafBrush = CreateSolidBrush(RGB(40, 120, 40));
        HPEN leafPen = CreatePen(PS_SOLID, 1, RGB(40, 120, 40));
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, leafBrush);
        HPEN oldPen = (HPEN)SelectObject(hdc, leafPen);
        if (tree.type == 1) {
            // Kavak: uzun oval
            Ellipse(
                hdc,
                (LONG)((tree.x - tree.size / 2 - viewLeft) * zoom),
                (LONG)((tree.y - trunkHeight / 2 - viewTop) * zoom),
                (LONG)((tree.x + tree.size / 2 - viewLeft) * zoom),
                (LONG)((tree.y + trunkHeight / 2 - viewTop) * zoom)
            );
        }
        else {
            // Normal: yuvarlak
            Ellipse(
                hdc,
                (LONG)((tree.x - tree.size / 2 - viewLeft) * zoom),
                (LONG)((tree.y - viewTop) * zoom),
                (LONG)((tree.x + tree.size / 2 - viewLeft) * zoom),
                (LONG)((tree.y + tree.size / 2 - viewTop) * zoom)
            );
        }
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(leafBrush);
        DeleteObject(leafPen);
    }
}
