#include "Helpers/ClipboardSetup.h"

#include <Windows.h>

namespace OIV
{
    void ClipboardSetup::RegisterDefaultFormats(LWS::Clipboard& clipboard)
    {
        clipboard.RegisterFormat(CF_DIBV5);
        clipboard.RegisterFormat(CF_DIB);
        clipboard.RegisterFormat(CF_UNICODETEXT);
        clipboard.RegisterFormat(CF_TEXT);
    }
}  // namespace OIV
