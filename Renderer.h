#ifndef Renderer_H
#define Renderer_H

#include "stdafx.h"

void UpdateFrame(void);
//
bool IsInRange(int x, int y);
void PutPixel(int x, int y);

bool IsInRange(int x, int y)
{
  return (abs(x) < (g_nClientWidth / 2)) && (abs(y) < (g_nClientHeight / 2));
}

void PutPixel(int x, int y)
{
  if (!IsInRange(x, y)) return;

  ULONG* dest = (ULONG*)g_pBits;
  DWORD offset = g_nClientWidth * g_nClientHeight / 2 + g_nClientWidth / 2 + x + g_nClientWidth * -y;
  *(dest + offset) = g_CurrentColor;
}

void UpdateFrame(void)
{
  // Buffer Clear
  SetColor(0, 0, 0);
  Clear();
  DrawFrameText();

  // Draw
  SetColor(255, 0, 0);
  PutPixel(100, 100);

  // Buffer Swap 
  BufferSwap();
}


#endif // !Renderer_H
