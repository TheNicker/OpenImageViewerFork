#include "PlatformFileDialog.h"

#include <LLUtils/Exception.h>

namespace OIV
{
    LWS::FileDialogResult PlatformFileDialog::Show(
        [[maybe_unused]] LWS::FileDialogType dialogType,
        [[maybe_unused]] const LWS::FileDialogFilterBuilder::ListFileDialogFilters& filters,
        [[maybe_unused]] const LWS::file_dialog_string_type& title, [[maybe_unused]] LWS::Handle ownerWindow,
        [[maybe_unused]] const LWS::file_dialog_string_type& defaultExtension, [[maybe_unused]] uint32_t filterIndex,
        [[maybe_unused]] LWS::file_dialog_string_type defaultFileName,
        [[maybe_unused]] LWS::file_dialog_string_type& outFilename)
    {
        LL_EXCEPTION_NOT_IMPLEMENT("File dialogs are not implemented on Linux");
    }

    LWS::FileDialogResult PlatformFileDialog::Show(
        [[maybe_unused]] LWS::FileDialogType dialogType,
        [[maybe_unused]] const LWS::FileDialogFilterBuilder::ListFileDialogFilters& filters,
        [[maybe_unused]] const LWS::file_dialog_string_type& title, [[maybe_unused]] LWS::Handle ownerWindow,
        [[maybe_unused]] const LWS::file_dialog_string_type& defaultExtension, [[maybe_unused]] uint32_t filterIndex,
        [[maybe_unused]] LWS::file_dialog_string_type defaultFileName,
        [[maybe_unused]] LWS::ListFileDialogFileNames& outFilenames)
    {
        LL_EXCEPTION_NOT_IMPLEMENT("File dialogs are not implemented on Linux");
    }
}  // namespace OIV
