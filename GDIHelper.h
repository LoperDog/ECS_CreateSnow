#pragma once

extern ULONG g_CurrentColor;
extern BYTE *g_pBits;

void InitGDI(HWND hWnd);
void ReleaseGDI(HWND hWnd);
void SetColor(BYTE r, BYTE g, BYTE b);
void Clear();
void BufferSwap();

void DrawFrameText();
void DrawSnowText();
void DrawTest();

void Drawjamak(wchar_t jamak[]);