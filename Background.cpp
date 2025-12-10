//-----------------------------------------------------------------
// Background Object
// C++ Source - Background.cpp
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------
#include "Background.h"
extern int g_screenWidth;
extern int g_screenHeight;
//-----------------------------------------------------------------
// Background Constructor(s)/Destructor
//-----------------------------------------------------------------
Background::Background(int iWidth, int iHeight, COLORREF crColor)
{
  // Initialize the member variables
  m_iWidth = iWidth;
  m_iHeight = iHeight;
  m_crColor = crColor;
  m_pBitmap = NULL;
}

Background::Background(Bitmap* pBitmap)
{
  // Initialize the member variables
  m_crColor = 0;
  m_pBitmap = pBitmap;
  m_iWidth = pBitmap->GetWidth();
  m_iHeight = pBitmap->GetHeight();
}

Background::~Background()
{
}

//-----------------------------------------------------------------
// Background General Methods
//-----------------------------------------------------------------
void Background::Update()
{
  // Do nothing since the basic background is not animated
}


//-----------------------------------------------------------------
// StarryBackground Constructor
//-----------------------------------------------------------------
StarryBackground::StarryBackground(int iWidth, int iHeight, int iNumStars,
  int iTwinkleDelay) : Background(iWidth, iHeight, 0)
{
  // Initialize the member variables
  m_iNumStars = min(iNumStars, 100);
  m_iTwinkleDelay = iTwinkleDelay;

  // Create the stars
  for (int i = 0; i < iNumStars; i++)
  {
    m_ptStars[i].x = rand() % iWidth;
    m_ptStars[i].y = rand() % iHeight;
    m_crStarColors[i] = RGB(128, 128, 128);
  }
}

StarryBackground::~StarryBackground()
{
}

//-----------------------------------------------------------------
// StarryBackground General Methods
//-----------------------------------------------------------------
void StarryBackground::Update()
{
  // Randomly change the shade of the stars so that they twinkle
  int iRGB;
  for (int i = 0; i < m_iNumStars; i++)
    if ((rand() % m_iTwinkleDelay) == 0)
    {
      iRGB = rand() % 256;
      m_crStarColors[i] = RGB(iRGB, iRGB, iRGB);
    }
}
void StarryBackground::Draw(HDC hDC)
{
    // Arka planý siyah doldur
    RECT rect = { 0, 0, m_iWidth, m_iHeight };
    HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hDC, &rect, hBrush);
    DeleteObject(hBrush);

    // Yýldýzlarý çiz
    for (int i = 0; i < m_iNumStars; i++)
        SetPixel(hDC, m_ptStars[i].x, m_ptStars[i].y, m_crStarColors[i]);
}

void Background::Draw(HDC hDC)
{
    // Draw the background
    if (m_pBitmap != NULL)
    {
        // Create a memory device context for the bitmap
        HDC hMemDC = CreateCompatibleDC(hDC);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, m_pBitmap->GetHBitmap());

        // Stretch the bitmap to fill the screen
        StretchBlt(
            hDC,
            0, 0, m_iWidth, m_iHeight, // Destination: full screen
            hMemDC,
            0, 0, m_pBitmap->GetWidth(), m_pBitmap->GetHeight(), // Source: full bitmap
            SRCCOPY
        );

        // Cleanup
        SelectObject(hMemDC, hOldBitmap);
        DeleteDC(hMemDC);
    }
    else
    {
        RECT rect = { 0, 0, m_iWidth, m_iHeight };
        HBRUSH hBrush = CreateSolidBrush(m_crColor);
        FillRect(hDC, &rect, hBrush);
        DeleteObject(hBrush);
    }
}
