
#include "include.h"




CRBuff::CRBuff(int _w,int _h,int _bpp,BOOL _clear)
{
  ClearVars();

  width = _w;
  height = _h;  // can be negative
  bpp = _bpp;

  ZeroMemory(&bi.bmiHeader,sizeof(bi.bmiHeader));
  bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
  bi.bmiHeader.biWidth = width;
  bi.bmiHeader.biHeight = height;
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = bpp;

  for ( int n = 0; n < 256; n++ )
      {
        bi.palette[n].rgbBlue = bi.palette[n].rgbGreen = bi.palette[n].rgbRed = n;
        bi.palette[n].rgbReserved = 0;
      }
  
  buff = NULL;
  bitmap = CreateDIBSection(NULL,(BITMAPINFO*)&bi,DIB_RGB_COLORS,(void**)&buff,NULL,0);

  hdc = CreateCompatibleDC(NULL);
  old_bitmap = (HBITMAP)SelectObject(hdc,bitmap);

  if ( _clear )
     {
       Clear();
     }
}


CRBuff::~CRBuff()
{
  if ( hdc )
     SelectObject(hdc,old_bitmap);
  if ( hdc )
     DeleteDC(hdc);  // multithread ok
  if ( bitmap )
     DeleteObject(bitmap);  // multithread ok

  ClearVars();
}


void CRBuff::ClearVars()
{
  hdc = NULL;
  bitmap = NULL;
  old_bitmap = NULL;
  buff = NULL;
  width = 0;
  height = 0;
  bpp = 0;
}


void CRBuff::Clear(int _color)
{
  if ( hdc )
     {
       HBRUSH brush = CreateSolidBrush(_color);
       RECT r;
       SetRect(&r,0,0,GetWidth(),GetHeight());
       FillRect(hdc,&r,brush);
       DeleteObject(brush);
     }
}


BOOL CRBuff::PaintTo(HDC d_hdc,int d_x,int d_y) const
{
  BOOL rc = FALSE;
  
  if ( hdc && d_hdc )
     {
       if ( hdc == d_hdc )
          {
            rc = TRUE;
          }
       else
          {
            rc = BitBlt(d_hdc,d_x,d_y,GetWidth(),GetHeight(),hdc,0,0,SRCCOPY);
          }
     }

  return rc;
}


CRBuff* CRBuff::CreateCopy() const
{
  CRBuff *d = new CRBuff(width,height,bpp,TRUE);

  PaintTo(d->GetHDC(),0,0);

  return d;
}


HBITMAP CRBuff::CreateDDB(HDC dev_hdc) const
{
  return IsValid() ? CreateDIBitmap(dev_hdc,&bi.bmiHeader,CBM_INIT,buff,(BITMAPINFO*)&bi,DIB_RGB_COLORS) : NULL;
}


BOOL CRBuff::IsBlack4Overlay() const
{
  BOOL rc = TRUE;

  if ( IsValid() )
     {
       for ( int m = 0; m < GetHeight(); m++ )
           {
             const BYTE *row = (BYTE*)buff + m * GetRowStride();

             int cnt_bytes = width*(bpp/8);
             
             for ( int n = 0; n < cnt_bytes; n++ )
                 {
                   BYTE c = row[n];
                   
                   if ( c != 0 && c != 16 )   // not good solution, but it's working
                      {
                        rc = FALSE;
                        break;
                      }
                 }

             if ( !rc )
                break;
           }
     }

  return rc;
}


void CRBuff::SetPixel24(int x,int y,int color)
{
  if ( IsValid() && GetBPP() == 24 )
     {
       if ( (x >= 0 && x < GetWidth()) && (y >= 0 && y < GetHeight()) )
          {
            BYTE c[4];
            *(int*)c = color;

            if ( IsDownTop() )
               {
                 y = GetHeight()-1-y;
               }
            
            unsigned byte_offset = y*GetRowStride()+x*3;

            BYTE *dst = (BYTE*)buff + byte_offset;

            dst[0] = c[2];
            dst[1] = c[1];
            dst[2] = c[0];
          }
     }
}
