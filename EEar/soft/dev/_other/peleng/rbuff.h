
#ifndef ___RBUFF_H___
#define ___RBUFF_H___



class CRBuff
{
          int width;
          int height;  // can be negative
          int bpp;
          HBITMAP bitmap;
          HBITMAP old_bitmap;
          HDC hdc;
          void *buff;

          struct {
           BITMAPINFOHEADER bmiHeader;
           RGBQUAD palette[256];
          } bi;

  public:
          CRBuff(int _w,int _h,int _bpp,BOOL _clear);
          ~CRBuff();
          CRBuff* CreateCopy() const;
          HBITMAP CreateDDB(HDC hdc) const;
          BOOL IsNotEmpty() const { return hdc && buff && bitmap && GetWidth()>0 && GetHeight()>0; }
          BOOL IsEmpty() const { return !IsNotEmpty(); }
          BOOL IsValid() const { return IsNotEmpty(); }
          int GetWidth() const { return width; }
          int GetRowStride() const { return (width*(bpp/8)+3)&~3; }
          int GetHeight() const { return height < 0 ? -height : height; }
          int GetRealHeight() const { return height; }
          BOOL IsSizeMatch(int w,int h) const { return w == GetWidth() && h == GetHeight(); }
          BOOL IsTopDown() const { return height < 0; }
          BOOL IsDownTop() const { return height > 0; }
          int GetDataSize() const { return GetRowStride()*GetHeight(); }
          int GetBPP() const { return bpp; }
          HDC GetHDC() const { return hdc; }
          HBITMAP GetBitmap() const { return bitmap; }
          void* GetBuff() const { return buff; }
          BITMAPINFO* GetBitmapInfo() const { return (BITMAPINFO*)&bi; }
          BOOL PaintTo(HDC d_hdc,int d_x,int d_y) const;
          void Clear(int _color=0);
          BOOL IsBlack4Overlay() const;
          void SetPixel24(int x,int y,int color);

  private:
          void ClearVars();
};



#endif
