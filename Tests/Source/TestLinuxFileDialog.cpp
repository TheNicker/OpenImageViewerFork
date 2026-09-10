#ifdef OIV_TEST_PLATFORM_LINUX

    #include "PlatformFileDialog.h"

    #include <catch2/catch_test_macros.hpp>
    #include <catch2/generators/catch_generators.hpp>
    #include <LWS/Platform.hpp>
    #include <gdk/gdkwayland.h>
    #include <gtk/gtk.h>
    #include <wayland-client.h>

    #include <chrono>
    #include <condition_variable>
    #include <cstdio>
    #include <cstdlib>
    #include <filesystem>
    #include <mutex>
    #include <thread>

namespace
{
    struct DialogResponse
    {
        std::string fileName;
        int response{};
        unsigned attempts{};
        bool clicked{};

        static gboolean Respond(gpointer data)
        {
            auto& state = *static_cast<DialogResponse*>(data);
            ++state.attempts;
            GList* windows = gtk_window_list_toplevels();
            for (GList* item = windows; item != nullptr; item = item->next)
            {
                if (!GTK_IS_FILE_CHOOSER_DIALOG(item->data) || !gtk_widget_get_visible(GTK_WIDGET(item->data)))
                    continue;

                auto* dialog     = GTK_DIALOG(item->data);
                char* selected   = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
                const bool ready = selected != nullptr && state.fileName == selected;
                g_free(selected);
                if (!state.clicked && ready)
                {
                    state.clicked = true;
                    if (state.response == GTK_RESPONSE_DELETE_EVENT)
                        gtk_window_close(GTK_WINDOW(dialog));
                    else
                        gtk_button_clicked(GTK_BUTTON(gtk_dialog_get_widget_for_response(dialog, state.response)));
                }
                else if (state.attempts >= 100)
                {
                    gtk_dialog_response(dialog, GTK_RESPONSE_CANCEL);
                }
            }
            g_list_free(windows);
            return G_SOURCE_CONTINUE;
        }
    };
}  // namespace

// This opens real GTK dialogs. Run explicitly with GDK_BACKEND=wayland GTK_USE_PORTAL=0.
TEST_CASE("Linux file dialogs flush closure after acceptance and cancellation", "[.][oiviewer][gtk]")
{
    if (g_strcmp0(g_getenv("GTK_USE_PORTAL"), "0") != 0)
        SKIP("Set GTK_USE_PORTAL=0 to exercise GTK's local file chooser");
    const auto flatpakInfo = std::filesystem::path(g_get_user_runtime_dir()) / "flatpak-info";
    if (g_file_test(flatpakInfo.c_str(), G_FILE_TEST_EXISTS))
        SKIP("Flatpak forces a portal instead of GTK's local file chooser");
    if (!gtk_init_check(nullptr, nullptr) || !GDK_IS_WAYLAND_DISPLAY(gdk_display_get_default()))
        SKIP("A GTK Wayland display is required");

    const auto dialogType = GENERATE(LWS::FileDialogType::OpenFile, LWS::FileDialogType::SaveFile);
    const int response    = GENERATE(GTK_RESPONSE_ACCEPT, GTK_RESPONSE_CANCEL, GTK_RESPONSE_DELETE_EVENT);
    const bool multiple   = dialogType == LWS::FileDialogType::OpenFile && GENERATE(false, true);
    CAPTURE(dialogType, response, multiple);

    const auto fileName = dialogType == LWS::FileDialogType::OpenFile
                              ? std::filesystem::path(OIV_TEST_SOURCE_DIR) / "../External/ImageCodec/Example/cat.jpg"
                              : std::filesystem::temp_directory_path() /
                                    ("oiv-dialog-selection-" + std::to_string(g_get_monotonic_time()));
    DialogResponse pending{.fileName = fileName.lexically_normal().string(), .response = response};
    LWS::PlatformContext platform;
    REQUIRE(platform.Init({.backend = LWS::BackendId::Wayland}) == LWS::Result::Success);
    LWS::Window owner(platform);
    // A chooser or response may never appear; the deadline must not depend on GTK dispatch or a local widget.
    std::mutex deadlineMutex;
    std::condition_variable_any deadlineCondition;
    std::jthread deadline(
        [&](std::stop_token stop)
        {
            std::unique_lock lock(deadlineMutex);
            deadlineCondition.wait_for(lock, stop, std::chrono::seconds(15), [] { return false; });
            if (!stop.stop_requested())
            {
                std::fputs("GTK file-dialog regression timed out after 15 seconds\n", stderr);
                std::fflush(stderr);
                std::_Exit(EXIT_FAILURE);
            }
        });
    const guint timer  = g_timeout_add(100, DialogResponse::Respond, &pending);
    std::string output = "unchanged";
    LWS::ListFileDialogFileNames outputs{"unchanged"};
    const auto result = multiple ? OIV::PlatformFileDialog::Show(dialogType, {}, "OIV dialog regression", owner,
                                                                 "*.png", 0, pending.fileName, outputs)
                                 : OIV::PlatformFileDialog::Show(dialogType, {}, "OIV dialog regression", owner,
                                                                 "*.png", 0, pending.fileName, output);
    g_source_remove(timer);
    deadline.request_stop();

    // Check before another GTK iteration can hide the bug by sending the buffered unmap/destroy requests.
    const int bufferedBytes = wl_display_flush(gdk_wayland_display_get_wl_display(gdk_display_get_default()));
    REQUIRE(bufferedBytes == 0);
    REQUIRE(pending.clicked);
    REQUIRE(pending.attempts < 100);
    if (response == GTK_RESPONSE_ACCEPT)
    {
        REQUIRE(result == LWS::FileDialogResult::Success);
        const auto expected = dialogType == LWS::FileDialogType::SaveFile ? pending.fileName + ".png"
                                                                          : pending.fileName;
        if (multiple)
            REQUIRE(outputs == LWS::ListFileDialogFileNames{expected});
        else
            REQUIRE(output == expected);
    }
    else
    {
        REQUIRE(result == LWS::FileDialogResult::UserCanceled);
        REQUIRE(output == "unchanged");
        REQUIRE(outputs == LWS::ListFileDialogFileNames{"unchanged"});
    }
}

#endif
