#include "Main.h"
#include "CopyDataProtocol.h"
#include "ViewerApplication.h"

#include <LLUtils/FileSystemHelper.h>
#include <Windows.h>
#include <shellapi.h>
#include <cstdlib>

namespace
{
    bool ForwardFile(const LLUtils::native_string_type& input)
    {
        const auto window = OIV::ViewerApplication::FindTrayBarWindow();
        if (window == 0)
            return false;
        const auto path = LLUtils::FileSystemHelper::ResolveFullPath(input);
        COPYDATASTRUCT data{};
        data.dwData = OIV::Win32::LoadFileCopyDataId;
        data.cbData = static_cast<DWORD>((path.size() + 1) * sizeof(wchar_t));
        data.lpData = const_cast<wchar_t*>(path.c_str());
        SendMessageW(reinterpret_cast<HWND>(window), WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&data));
        return true;
    }

    void WriteText(HANDLE stream, const std::string& text)
    {
        if (stream == nullptr || stream == INVALID_HANDLE_VALUE || text.empty())
            return;
        DWORD mode{}, written{};
        if (GetConsoleMode(stream, &mode))
        {
            const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                                                 static_cast<int>(text.size()), nullptr, 0);
            if (size > 0)
            {
                std::wstring wide(static_cast<size_t>(size), L'\0');
                MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()),
                                    wide.data(), size);
                WriteConsoleW(stream, wide.data(), static_cast<DWORD>(wide.size()), &written, nullptr);
            }
        }
        else
        {
            size_t offset = 0;
            while (
                offset < text.size() &&
                WriteFile(stream, text.data() + offset, static_cast<DWORD>(text.size() - offset), &written, nullptr) &&
                written != 0)
                offset += written;
        }
    }

    int PrintExit(const OIV::CommandLineExit& result)
    {
        if (!result.standardOutput.empty() || !result.standardError.empty())
        {
            // Attaching can replace standard handles. Preserve any pipes supplied by the caller.
            HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
            HANDLE err = GetStdHandle(STD_ERROR_HANDLE);
            AttachConsole(ATTACH_PARENT_PROCESS);
            if (out == nullptr || out == INVALID_HANDLE_VALUE)
                out = GetStdHandle(STD_OUTPUT_HANDLE);
            if (err == nullptr || err == INVALID_HANDLE_VALUE)
                err = GetStdHandle(STD_ERROR_HANDLE);
            WriteText(out, result.standardOutput);
            WriteText(err, result.standardError);
        }
        return result.exitCode;
    }
}  // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    int count      = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &count);
    if (argv == nullptr)
        return EXIT_FAILURE;
    OIV::CommandLineExit result;
    try
    {
        auto parsed = OIV::ParseCommandLine(count, argv);
        result      = std::holds_alternative<OIV::CommandLineExit>(parsed)
                          ? std::get<OIV::CommandLineExit>(std::move(parsed))
                          : RunViewer(std::get<OIV::CommandLineParameters>(parsed), ForwardFile);
    }
    catch (const std::exception& error)
    {
        result = {EXIT_FAILURE, {}, std::string("OIViewer: ") + error.what() + "\n"};
    }
    LocalFree(argv);
    return PrintExit(result);
}
