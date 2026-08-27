#include "ImageControl.h"

#include <LWS/Win32/EventWin32.hpp>

#include <Windows.h>
#include <windowsx.h>

#include <algorithm>

namespace OIV
{
    void ImageControl::RefreshScrollInfo()
    {
        const HWND window          = reinterpret_cast<HWND>(GetHandle());
        const size_t deltaElements = fImageList.GetNumberOfElements() - fImageList.GetNumberOfDisplayedElements();
        SCROLLINFO info{};
        info.cbSize = sizeof(info);
        info.fMask  = SIF_ALL;
        info.nMin   = 0;
        info.nMax   = static_cast<int>(deltaElements);
        info.nPage  = 1;
        info.nPos   = std::min(GetScrollPos(window, SB_VERT), info.nMax);
        SetScrollInfo(window, SB_VERT, &info, TRUE);
        SetImagePos(info.nPos);
        InvalidateRect(window, nullptr, TRUE);
    }

    std::intptr_t ImageControl::SendMessage(std::uint32_t message, std::uintptr_t wParam, std::intptr_t lParam)
    {
        return ::SendMessage(reinterpret_cast<HWND>(GetHandle()), message, wParam, lParam);
    }

    bool ImageControl::HandleWindowEvent(const LWS::AnyEvent& eventData)
    {
        const auto* raw = std::get_if<LWS::EventRawPlatform>(&eventData);
        if (raw == nullptr || raw->platformType != std::to_underlying(LWS::BackendId::Win32) ||
            raw->platformData == nullptr)
            return false;

        const auto& message = *reinterpret_cast<const LWS::Win32::WinMessage*>(raw->platformData);
        switch (message.message)
        {
            case WM_LBUTTONDOWN:
                fImageList.MouseClick(GET_X_LPARAM(message.lParam), GET_Y_LPARAM(message.lParam));
                return true;
            case WM_VSCROLL:
            {
                int minRange      = 0;
                int maxRange      = 0;
                const HWND window = reinterpret_cast<HWND>(message.hWnd);
                GetScrollRange(window, SB_VERT, &minRange, &maxRange);
                const int oldPos = GetScrollPos(window, SB_VERT);
                int newPos       = oldPos;
                switch (LOWORD(message.wParam))
                {
                    case SB_PAGEDOWN:
                        ++newPos;
                        break;
                    case SB_PAGEUP:
                        --newPos;
                        break;
                    case SB_THUMBTRACK:
                    case SB_THUMBPOSITION:
                        newPos = HIWORD(message.wParam);
                        break;
                    default:
                        break;
                }

                newPos = std::clamp(newPos, minRange, maxRange);
                if (newPos != oldPos)
                {
                    SetImagePos(newPos);
                    SetScrollPos(window, SB_VERT, newPos, TRUE);
                    InvalidateRect(window, nullptr, TRUE);
                }
                return true;
            }
            case WM_CREATE:
                fImageList.SetTarget(GetHandle());
                return true;
            case WM_PAINT:
                fImageList.Draw();
                return true;
            case WM_MOUSEWHEEL:
            {
                const int wheelDelta = GET_WHEEL_DELTA_WPARAM(message.wParam) / WHEEL_DELTA;
                SendMessage(WM_VSCROLL, wheelDelta < 0 ? SB_PAGEDOWN : SB_PAGEUP, 0);
                return true;
            }
            case WM_SIZE:
                RefreshScrollInfo();
                return true;
            default:
                return false;
        }
    }
}  // namespace OIV
