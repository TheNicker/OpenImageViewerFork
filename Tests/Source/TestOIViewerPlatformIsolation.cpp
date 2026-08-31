#include <catch2/catch_all.hpp>

#include <ContextMenu.h>
#include <ImageList.h>
#include <LLUtils/Exception.h>

#include <type_traits>

static_assert(!std::is_copy_constructible_v<ImageList>);
static_assert(!std::is_copy_assignable_v<ImageList>);
static_assert(!std::is_move_constructible_v<ImageList>);
static_assert(!std::is_move_assignable_v<ImageList>);

TEST_CASE("ImageList keeps portable selection state", "[oiviewer][platform]")
{
    ImageList imageList;
    int selectedIndex        = -1;
    auto selectionConnection = imageList.ImageSelectionChanged.Connect(
        [&](const ImageList::ImageSelectionChangeArgs& args) { selectedIndex = args.imageIndex; });

    imageList.SetImage({.index = 0, .title = LLUTILS_TEXT("first")});
    imageList.SetImage({.index = 1, .title = LLUTILS_TEXT("second")});
    imageList.SetViewportHeight(100);
    imageList.SetSelected(1);

    REQUIRE(imageList.GetNumberOfElements() == 2);
    REQUIRE(imageList.GetSelected() == 1);
    REQUIRE(imageList.GetScrollPosition() == 1);
    REQUIRE(selectedIndex == 1);
}

TEST_CASE("ImageList scrolls selection coordinates and clears state", "[oiviewer][platform]")
{
    ImageList imageList;
    imageList.SetImage({.index = 0, .title = LLUTILS_TEXT("first")});
    imageList.SetImage({.index = 1, .title = LLUTILS_TEXT("second")});
    imageList.SetImage({.index = 2, .title = LLUTILS_TEXT("third")});
    imageList.SetViewportHeight(100);
    bool scrollPositionChanged = false;
    imageList.Changed.Add([&](ImageList::ChangeType change)
                          { scrollPositionChanged = change == ImageList::ChangeType::ScrollPosition; });

    imageList.Scroll(1);
    REQUIRE(imageList.GetScrollPosition() == 1);
    REQUIRE(scrollPositionChanged);
    const auto clickedIndex = imageList.GetImageIndexAt(0);
    REQUIRE(clickedIndex == 1);
    imageList.SetSelected(static_cast<int>(*clickedIndex));
    REQUIRE(imageList.GetSelected() == 1);

    imageList.Clear();
    REQUIRE(imageList.GetNumberOfElements() == 0);
    REQUIRE(imageList.GetSelected() == -1);
}

TEST_CASE("ImageList owns visible row layout and hit testing", "[oiviewer][platform]")
{
    ImageList imageList;
    imageList.SetImage({.index = 0, .title = LLUTILS_TEXT("first")});
    imageList.SetImage({.index = 1, .title = LLUTILS_TEXT("second")});
    imageList.SetImage({.index = 2, .title = LLUTILS_TEXT("third")});
    imageList.SetViewportHeight(150);
    imageList.Scroll(1);

    const auto visible = imageList.GetVisibleRange();
    REQUIRE(visible.first == 1);
    REQUIRE(visible.last == 3);
    REQUIRE(imageList.GetImageIndexAt(0) == 1);
    REQUIRE(imageList.GetImageIndexAt(149) == 2);
    REQUIRE_FALSE(imageList.GetImageIndexAt(150));

    const LWS::Rect secondRow = imageList.GetRowRect(2, 240);
    REQUIRE(secondRow.GetCorner(LLUtils::Corner::TopLeft) == LLUtils::PointI32{0, 100});
    REQUIRE(secondRow.GetCorner(LLUtils::Corner::BottomRight) == LLUtils::PointI32{240, 200});
}

TEST_CASE("ContextMenu supports both viewer item models", "[oiviewer][platform]")
{
    struct CommandItem
    {
        std::string command;
        std::string arguments;
    };

    OIV::ContextMenu<int> notificationMenu(0);
    OIV::ContextMenu<CommandItem> commandMenu(0);
    notificationMenu.AddItem(LLUTILS_TEXT("Quit"), 0);
    commandMenu.AddItem(LLUTILS_TEXT("Open"), {.command = "open", .arguments = {}});

    REQUIRE_FALSE(notificationMenu.IsVisible());
    REQUIRE_FALSE(commandMenu.IsVisible());
}

#ifdef OIV_TEST_PLATFORM_LINUX
TEST_CASE("Linux context-menu presentation reports NotImplemented", "[oiviewer][platform]")
{
    LLUtils::Exception::ErrorCode errorCode = LLUtils::Exception::ErrorCode::Unspecified;
    auto exceptionConnection = LLUtils::Exception::OnException.Connect([&](const LLUtils::Exception::EventArgs& args)
                                                                       { errorCode = args.errorCode; });
    OIV::ContextMenu<int> menu(0);

    REQUIRE_THROWS_AS(menu.Show(0, 0, OIV::AlignmentHorizontal::None, OIV::AlignmentVertical::None),
                      LLUtils::Exception);
    REQUIRE(errorCode == LLUtils::Exception::ErrorCode::NotImplemented);
}
#endif
