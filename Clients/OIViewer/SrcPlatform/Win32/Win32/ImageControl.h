#pragma once
#include <Windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <LWS/Win32/EventWin32.hpp>
#include <LWS/Win32/WindowWin32.hpp>
#include "ImageList.h"
namespace OIV
{
    namespace Win32

    {
        class ImageControl : public LWS::WindowWin32
        {
        public:

            ImageControl()
            {
                AddEventListener([this](const LWS::AnyEvent& eventData) { return HandleWindwMessage(eventData); });
            }

            void SetImagePos(int pos)
            {
                fImageList.SetPos(pos);
            }


            ImageList& GetImageList() { return fImageList; }

            void RefreshScrollInfo()
            {
                HWND hwnd = reinterpret_cast<HWND>(GetHandle());
                const size_t deltaElements = fImageList.GetNumberOfElements() - fImageList.GetNumberOfDisplayedElements();
                SCROLLINFO si{};
                si.cbSize = sizeof(SCROLLINFO);
                si.fMask = SIF_ALL;
                si.nMin = 0;
                si.nMax = static_cast<int>((std::max)(static_cast<size_t>(0), deltaElements));
                si.nPage = 1;
                si.nPos = (std::min)(GetScrollPos(hwnd, SB_VERT), si.nMax);
                SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
                SetImagePos(si.nPos);
                InvalidateRect(hwnd, nullptr, TRUE);
            }

            LRESULT SendMessage(UINT message, WPARAM wParam, LPARAM lParam)
            {
                return ::SendMessage(reinterpret_cast<HWND>(GetHandle()), message, wParam, lParam);
            }

            bool HandleWindwMessage(const LWS::AnyEvent& eventData)
            {
                bool handled = true;

                const auto* raw = std::get_if<LWS::EventRawPlatform>(&eventData);
                if (raw == nullptr || raw->platformType != std::to_underlying(LWS::BackendId::Win32) ||
                    raw->platformData == nullptr)
                    return false;
            
                const LWS::Win32::WinMessage& msg = *reinterpret_cast<const LWS::Win32::WinMessage*>(raw->platformData);
                switch (msg.message)
                {
                case WM_LBUTTONDOWN:
                {
                    int xPos = GET_X_LPARAM(msg.lParam);
                    int yPos = GET_Y_LPARAM(msg.lParam);
                    fImageList.MouseClick(xPos, yPos);
                }

                break;
                case WM_VSCROLL:
                {
                    INT minRange, maxRange;
                    HWND hwnd = reinterpret_cast<HWND>(msg.hWnd);
                    GetScrollRange(hwnd, SB_VERT, &minRange, &maxRange);
                    int oldPos = GetScrollPos(hwnd, SB_VERT);
                    int newPos = oldPos;
                    switch (LOWORD(msg.wParam))
                    {
                    case SB_PAGEDOWN:
                        newPos++;
                        break;
                    case SB_PAGEUP:
                        newPos--;
                        break;
                    case SB_THUMBTRACK:
                    case SB_THUMBPOSITION:
                    {
                        newPos = HIWORD(msg.wParam);
                    }

                    break;
                    case SB_ENDSCROLL:
                        break;
                    }
                    newPos = std::clamp(newPos, minRange, maxRange);

                    if (newPos != oldPos)
                    {
                        SetImagePos(newPos);
                        SetScrollPos(hwnd, SB_VERT, newPos, TRUE);
                        InvalidateRect(hwnd, nullptr, TRUE);
                    }


                }
                break;
                case WM_CREATE:
                {
                    fImageList.SetTarget(reinterpret_cast<HWND>(GetHandle()));
                }
                break;
                case WM_PAINT:
                    fImageList.Draw();
                    return true;
                    break;
                case WM_MOUSEWHEEL:
                    //case WM_MOUSEHWHEEL:
                {
                    //MessageBox(0, LLUTILS_TEXT("aa"), LLUTILS_TEXT("aa"), MB_OK);
                    int zDelta = GET_WHEEL_DELTA_WPARAM(msg.wParam) / WHEEL_DELTA;
                    DWORD direction = zDelta < 0 ? SB_PAGEDOWN : SB_PAGEUP;
                    SendMessage(WM_VSCROLL, direction, 0);

                }
                break;

                case WM_SIZE:
                {
                    RefreshScrollInfo();
                }
                break;
                default:
                    handled = false;

                }
                
                return handled;

            }
          
        private:
            ImageList fImageList;
        };
    }
}
