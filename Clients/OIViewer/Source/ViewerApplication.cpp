#include <iomanip>
#include <filesystem>
#include <thread>
#include <future>
#include <cassert>

#include "ViewerApplication.h"

#include <Version.h>

#include <Functions.h>
#include <ApiGlobal.h>
#include <LWS/Platform.hpp>

#include <LLUtils/Exception.h>
#include <LLUtils/FileHelper.h>
#include <LLUtils/PlatformUtility.h>
#include <LLUtils/StringUtility.h>
#include <LLUtils/UniqueIDProvider.h>
#include <LLUtils/FileSystemHelper.h>
#include <LLUtils/Rect.h>

#include <OIVAppCore/OIVHelper.h>
#include "Helpers/ClipboardSetup.h"
#include <OIVAppCore/MessageHelper.h>
#include <OIVAppCore/ShellIntegrationHelper.h>
#include "Helpers/ShellCommandHandler.h"

#include "OIVCommands.h"

#include "OIVImage/OIVFileImage.h"
#include "OIVImage/OIVRawImage.h"
#include "VirtualStatusBar.h"
#include "MonitorProvider.h"

#include "ContextMenu.h"
#include "Globals.h"
#include <OIVAppCore/ConfigurationLoader.h>
#include "CommandRegistry.h"
#include <OIVAppCore/ColorCountPolicy.h>
#include <OIVAppCore/ColorCorrectionCommandPolicy.h>
#include <OIVAppCore/FileChangePolicy.h>
#include <OIVAppCore/FrameLimiterPolicy.h>
#include <OIVAppCore/ImageEditPolicy.h>
#include <OIVAppCore/ImageFormatCatalogPolicy.h>
#include <OIVAppCore/ImageLoadPresentationPolicy.h>
#include <OIVAppCore/ImageTransformCommandPolicy.h>
#include <OIVAppCore/InputGesturePolicy.h>
#include <OIVAppCore/OIVImageHelper.h>
#include <OIVAppCore/SelectionWorkflowPolicy.h>
#include <OIVAppCore/SequencerPolicy.h>
#include <OIVAppCore/SortCommandPolicy.h>
#include <OIVAppCore/SubImagePolicy.h>
#include <OIVAppCore/ViewActionController.h>
#include <OIVAppCore/ViewCommandPolicy.h>
#include <OIVAppCore/ViewerPresentationPolicy.h>
#include <OIVShared/PixelHelper.h>
#include <ImageUtil/ImageUtil.h>
#include "InterThreadMessages.h"

namespace OIV
{
    void ViewerApplication::Init(LLUtils::native_string_type relativeFilePath)
    {
        using namespace std;
        using namespace placeholders;

        LLUtils::native_string_type filePath = LLUtils::FileSystemHelper::ResolveFullPath(relativeFilePath);
        filePath                             = std::filesystem::path(filePath).lexically_normal();

        const bool isDirectory = std::filesystem::is_directory(filePath);

        const bool isInitialFileProvided = filePath.empty() == false && isDirectory == false;
        const bool isInitialFileExists   = isInitialFileProvided && filesystem::exists(filePath);

        if (isDirectory)
            fPendingFolderLoad = filePath;

        future<bool> asyncResult;

        if (isInitialFileExists == true)
        {
            fIsTryToLoadInitialFile = true;

            // if initial file is provided, load asynchronously.
            asyncResult = async(launch::async,
                                [&]() -> bool
                                {
                                    fInitialFile = std::make_shared<OIVFileImage>(filePath);
                                    return fInitialFile->Load(&fImageLoader,
                                                              IMCodec::PluginTraverseMode::AnyPlugin |
                                                                  IMCodec::PluginTraverseMode::AnyFileType) ==
                                           RC_Success;
                                });
        }

        // initialize the windowing system of the window
        const LWS::WindowConfig windowConfig{.size = {1200, 800}};
        if (fWindow.Create(windowConfig) != LWS::Result::Success)
            LL_EXCEPTION(LLUtils::Exception::ErrorCode::InvalidState, "Unable to create the main window");
        fWindow.SetMenuChar(false);
        fWindow.ShowStatusBar(false);
        fWindow.SetDestoryOnClose(false);
        if (LWS::Platform::supports(LWS::Platform::Feature::DragAndDrop) &&
            fWindow.EnableDragAndDrop(true) != LWS::Result::Success)
            LL_EXCEPTION(LLUtils::Exception::ErrorCode::InvalidState, "Unable to enable window drag and drop");
        // Set canvas background the same color as in the renderer for flicker free startup.
        // TODO: fix resize and disable background erasure of top level windows.
        fWindow.SetBackgroundColor(LLUtils::Color(45, 45, 48));
        fWindow.GetCanvasWindow().SetBackgroundColor(LLUtils::Color(45, 45, 48));

        fWindow.SetDoubleClickMode(LWS::DoubleClickMode::Default);
        fWindow.SetWindowStyles(LWS::WindowStyle::ResizableBorder | LWS::WindowStyle::MaximizeButton |
                                    LWS::WindowStyle::MinimizeButton,
                                true);

        AutoScroll::CreateParams params = {&fWindow,
                                           std::bind(&ViewerApplication::OnScroll, this, std::placeholders::_1)};
        fAutoScroll                     = std::make_unique<AutoScroll>(params);

        std::ignore = fWindow.AddEventListener(
            [this](const LWS::AnyEvent& eventData)
            { return HandleEventCallback([&]() { return HandleMessages(eventData); }); });
        std::ignore = fWindow.GetCanvasWindow().AddEventListener(
            [this](const LWS::AnyEvent& eventData)
            { return HandleEventCallback([&]() { return HandleClientWindowMessages(eventData); }); });

        fRefreshOperation.Begin();

        fTimerNoActiveZoom.SetTargetWindow(fWindow.GetHandle());

        fTimerNoActiveZoom.SetCallback(MakeSafeCallback([this]() { DelayResamplingCallback(); }));

        fTimerNavigation.SetTargetWindow(fWindow.GetHandle());
        fTimerNavigation.SetCallback(MakeSafeCallback(
            [this]()
            {
                const int jump = GetRawNavigationDirection();

                if (jump != 0 &&
                    fLastImageLoadTimeStamp.GetElapsedTimeInteger(LLUtils::StopWatch::Milliseconds) > fQuickBrowseDelay)
                {
                    fLastImageLoadTimeStamp.Start();
                    fLastImageLoadTimeStamp.Stop();

                    if (JumpFiles(jump) == false)
                    {
                        fLastImageLoadTimeStamp.Start();
                    }
                }
            }));

        // TODO: move sequencer initialiaztion to PostInitOperations.
        fSequencerTimer.SetTargetWindow(fWindow.GetHandle());
        fSequencerTimer.SetCallback(MakeSafeCallback(
            [this]()
            {
                auto currentImage = fImageState.GetOpenedImage()->GetImage()->GetSubImage(fCurrentFrame);
                fImageState.SetImageChainRoot(
                    std::make_shared<OIVBaseImage>(ImageSource::GeneratedByLib, currentImage));

                fSequencerTimer.SetInterval(SequencerPolicy::FrameIntervalMs(
                    currentImage->GetAnimationData().delayMilliseconds, fCurrentSequencerSpeed));
                fCurrentFrame = SequencerPolicy::NextFrame(fCurrentFrame,
                                                           fImageState.GetOpenedImage()->GetImage()->GetNumSubImages());
                RefreshImage();
            }));

        fMessageManager = std::make_unique<MessageManager>(fWindow.GetHandle(), &fLabelManager, 5,
                                                           [&]() -> void { fRefreshOperation.Queue(); });

        InitializeRenderer();

        // Update oiv lib client size
        UpdateWindowSize();

        // Wait for initial file to finish loading
        bool isInitialFileLoadedSuccesfuly = false;
        if (asyncResult.valid())
        {
            asyncResult.wait();
            isInitialFileLoadedSuccesfuly = asyncResult.get();
        }

        // If there is no initial file or the file has failed to load, show the window now, otherwise show the window
        // after the image has rendered completely at the method FinalizeImageLoad.
        fWindow.SetVisible(!isInitialFileLoadedSuccesfuly);

        // If initial file is provided but doesn't exist
        if (isInitialFileProvided && !isInitialFileExists)
        {
            using namespace std::string_literals;
            SetUserMessage(LLUTILS_TEXT("Can not load the file: "s) + filePath + LLUTILS_TEXT(", it doesn't exist"s),
                           static_cast<GroupID>(UserMessageGroups::FailedFileLoad), MessageFlags::Persistent);
        }

        fRefreshOperation.End(!isInitialFileLoadedSuccesfuly);

        if (isInitialFileLoadedSuccesfuly)
        {
            LoadOivImage(fInitialFile);
            fInitialFile.reset();
        }
    }

    IMCodec::ImageSharedPtr ViewerApplication::GetImageByIndex(int32_t index)
    {
        using namespace IMCodec;
        auto openedImage = fImageState.GetOpenedImage()->GetImage();

        const auto isMainAnActualImage = SubImagePolicy::IncludeMainImage(openedImage->GetItemType());
        const auto actualIndex         = SubImagePolicy::ActualImageIndexFromDisplayIndex(index, isMainAnActualImage);

        if (actualIndex == SubImagePolicy::MainImageIndex)
        {
            return openedImage;
        }
        else
        {
            return openedImage->GetSubImage(actualIndex);
        }
    }

    void ViewerApplication::PostInitOperations()
    {
        mLogFile.Register();

        fTimerTopMostRetention.SetTargetWindow(fWindow.GetHandle());
        fTimerTopMostRetention.SetCallback(MakeSafeCallback([this]() { ProcessTopMost(); }));

        fTimerSlideShow.SetTargetWindow(fWindow.GetHandle());
        fTimerSlideShow.SetCallback(MakeSafeCallback(
            [this]()
            {
                SetSlideShowEnabled(false);

                if (fBrowseSessionController == nullptr)
                    return;

                const auto& fileList = fBrowseSessionController->GetFolderFileList();
                bool foundFile       = JumpFiles(1) ||
                                       (fSlideshowPolicy.ShouldWrap(fileList.GetCurrentIndex(), fileList.GetSize()) &&
                                        JumpFiles(FolderFileList::IndexStart));

                SetSlideShowEnabled(foundFile);
            }));

        fDoubleTap.callback = [this]()
        {
            fWindow.SetAlwaysOnTop(true);
            fTopMostCounter = 3;
            SetTopMostUserMesage();
            fTimerTopMostRetention.SetInterval(1000);
        };

        const ImageFormatCatalog imageFormatCatalog = ImageFormatCatalogPolicy::Build(
            fImageLoader.GetImageCodec().GetPluginsInfo());

        LWS::FileDialogFilterBuilder::ListFileDialogFilters readFilters;
        LWS::FileDialogFilterBuilder::ListFileDialogFilters writeFilters;

        for (const ImageFormatFilter& filter : imageFormatCatalog.readFilters)
            readFilters.push_back({filter.description, filter.extensions});

        for (const ImageFormatFilter& filter : imageFormatCatalog.writeFilters)
            writeFilters.push_back({filter.description, filter.extensions});

        fKnownFileTypesSet          = imageFormatCatalog.knownFileTypesSet;
        fKnownFileTypes             = imageFormatCatalog.knownFileTypes;
        fDefaultSaveFileExtension   = imageFormatCatalog.defaultSaveFileExtension;
        fDefaultSaveFileFormatIndex = imageFormatCatalog.defaultSaveFileFormatIndex;

        fOpenComDlgFilters = LWS::FileDialogFilterBuilder(readFilters);
        fSaveComDlgFilters = LWS::FileDialogFilterBuilder(writeFilters);

        if (fFileWatcher != nullptr)
        {
            fFileWatcher->GetFileChangedEvent().Add(
                std::bind(&ViewerApplication::OnFileChanged, this, std::placeholders::_1));
        }

        fBrowseSessionController = std::make_unique<BrowseSessionController>(
            fFileWatcher.get(), &fFileSorter, fKnownFileTypesSet, fKnownFileTypes, fImageResidencyCache,
            [this](const LLUtils::native_string_type& fileName, IMCodec::ImageSharedPtr image)
            {
                if (!fIsShuttingDown)
                {
                    fEventSync.AddData(static_cast<std::underlying_type_t<InterThreadMessages>>(
                                           InterThreadMessages::FileIndexResidencyReady),
                                       FileIndexResidencyReadyData{fileName, image});
                }
            },
            [this](const BrowseSessionController::BrowseCandidateCompletion& completion)
            {
                if (!fIsShuttingDown)
                {
                    fEventSync.AddData(static_cast<std::underlying_type_t<InterThreadMessages>>(
                                           InterThreadMessages::CandidateResidencyReady),
                                       CandidateResidencyReadyData{completion});
                }
            });
        fImageOpenController->SetBrowseSessionController(fBrowseSessionController.get());

        if (IsOpenedImageIsAFile())
            (void) fBrowseSessionController->CommitCurrentFile(GetOpenedFileName());
        UpdateTitle();

        AddCommandsAndKeyBindings();

        fWindow.GetImageControl().GetImageList().ImageSelectionChanged.Add(
            std::bind(&ViewerApplication::OnImageSelectionChanged, this, std::placeholders::_1));

        // renderer took over on the window, no need to erase background.
        fWindow.GetCanvasWindow().SetEraseBackground(false);

        fContextMenuTimer.SetTargetWindow(fWindow.GetHandle());
        fContextMenuTimer.SetCallback(MakeSafeCallback([this]() { OnContextMenuTimer(); }));
        fContextMenu = std::make_unique<ContextMenu<MenuItemData>>(fWindow.GetHandle());

        fContextMenu->AddItem(LLUTILS_TEXT("Open"), MenuItemData{"cmd_open_file", ""});
        fContextMenu->AddItem(LLUTILS_TEXT("Open containing folder"),
                              MenuItemData{"cmd_shell", "cmd=containingFolder"});
        fContextMenu->AddItem(LLUTILS_TEXT("Open in new window"), MenuItemData{"cmd_shell", "cmd=newWindow"});
        fContextMenu->AddItem(LLUTILS_TEXT("Open in photoshop"), MenuItemData{"cmd_shell", "cmd=openPhotoshop"});
        fContextMenu->AddItem(LLUTILS_TEXT("Quit"), MenuItemData{"cmd_view_state", "type=quit"});

        fContextMenu->EnableItem(LLUTILS_TEXT("Open containing folder"),
                                 fImageState.GetOpenedImage() != nullptr &&
                                     fImageState.GetOpenedImage()->GetImageSource() == ImageSource::File);
        fContextMenu->EnableItem(LLUTILS_TEXT("Open in photoshop"),
                                 fImageState.GetOpenedImage() != nullptr &&
                                     fImageState.GetOpenedImage()->GetImageSource() == ImageSource::File);

        InitializeNotificationIcons();

        fNotificationContextMenu = std::make_unique<ContextMenu<int>>(fWindow.GetHandle());
        fNotificationContextMenu->AddItem(LLUTILS_TEXT("Quit"), int{});

        InitializeRawInput();

        LoadSettings();

        if (fReloadSettingsFileIfChanged && fFileWatcher != nullptr)
            fCOnfigurationFolderID = fFileWatcher->AddFolder(LLUtils::PlatformUtility::GetExeFolder() +
                                                             LLUTILS_TEXT("./Resources/Configuration/."));

        if (fPendingFolderLoad.empty() == false)
        {
            LoadFileOrFolder(fPendingFolderLoad,
                             IMCodec::PluginTraverseMode::AnyPlugin | IMCodec::PluginTraverseMode::AnyFileType);
            fPendingFolderLoad.clear();
        }

        if (IsImageOpen() == false)
        {
            ShowWelcomeMessage();
            UpdateTitle();
        }

        ClipboardSetup::RegisterDefaultFormats(fClipboardHelper);
    }

    LLUtils::PointF64 ViewerApplication::GetImageSize(ImageSizeType imageSizeType)
    {
        using namespace LLUtils;
        switch (imageSizeType)
        {
            case ImageSizeType::Original:
                return fImageState.GetImage(ImageChainStage::SourceImage) != nullptr
                           ? PointF64(fImageState.GetImage(ImageChainStage::SourceImage)->GetImage()->GetDimensions())
                           : PointF64(0, 0);
            case ImageSizeType::Transformed:
                return static_cast<PointF64>(
                    fImageState.GetImage(ImageChainStage::Deformed)->GetImage()->GetDimensions());
            case ImageSizeType::Visible:
                return fImageState.GetVisibleSize();

            default:
                LL_EXCEPTION_UNEXPECTED_VALUE;
        }
    }

    LLUtils::PointI32 ViewerApplication::SnapToScreenSpaceImagePixels(LLUtils::PointI32 pointOnScreen)
    {
        return SelectionWorkflowPolicy::SnapToImagePixels(pointOnScreen, GetScale(), GetOffset());
    }

    LLUtils::RectI32 ViewerApplication::ClientToImageRounded(LLUtils::RectI32 clientRect) const
    {
        return static_cast<LLUtils::RectI32>(ClientToImage(clientRect).Round());
    }

    LLUtils::PointF64 ViewerApplication::GetOffset() const
    {
        return fImageState.GetOffset();
    }

    LLUtils::PointF64 ViewerApplication::ImageToClient(LLUtils::PointF64 imagepos) const
    {
        return ViewTransformController::ImageToClient(imagepos, GetScale(), GetOffset());
    }

    LLUtils::RectF64 ViewerApplication::ImageToClient(LLUtils::RectF64 clientRect) const
    {
        return ViewTransformController::ImageToClient(clientRect, GetScale(), GetOffset());
    }

    LLUtils::PointF64 ViewerApplication::ClientToImage(LLUtils::PointI32 clientPos) const
    {
        return ViewTransformController::ClientToImage(static_cast<LLUtils::PointF64>(clientPos), GetScale(),
                                                      GetOffset());
    }

    LLUtils::RectF64 ViewerApplication::ClientToImage(LLUtils::RectI32 clientRect) const
    {
        return ViewTransformController::ClientToImage(static_cast<LLUtils::RectF64>(clientRect), GetScale(),
                                                      GetOffset());
    }

    LLUtils::PointF64 ViewerApplication::GetCanvasCenter()
    {
        using namespace LLUtils;

        PointF64 canvasCenter;

        if (fWindow.GetFullScreenState() != LWS::FullScreenState::MultiScreen) [[likely]]
        {
            canvasCenter = PointF64(fWindow.GetCanvasSize()) / 2.0;
        }
        else [[unlikely]]
        {
            const auto primaryMonitor   = LWS::Platform::getPrimaryMonitor(false).monitorRect;
            const auto boundingArea     = LWS::Platform::getBoundingMonitorArea();
            const auto primaryMonitorP0 = primaryMonitor.GetCorner(TopLeft);
            const auto boundingAreaP0   = boundingArea.GetCorner(TopLeft);

            using point_type     = PointF64::point_type;
            const auto leftDelta = primaryMonitorP0.x - boundingAreaP0.x;
            const auto topDelta  = primaryMonitorP0.y - boundingAreaP0.y;

            const LLUtils::PointF64 primaryScreenOffset = LLUtils::PointF64(static_cast<point_type>(leftDelta),
                                                                            static_cast<point_type>(topDelta));

            const LLUtils::PointF64 primaryScreenSize = LLUtils::PointF64(
                static_cast<point_type>(primaryMonitor.GetWidth()),
                static_cast<point_type>(primaryMonitor.GetHeight()));

            canvasCenter = primaryScreenOffset + primaryScreenSize / 2.0;
        }
        return canvasCenter;
    }

    LLUtils::PointF64 ViewerApplication::ResolveOffset(const LLUtils::PointF64& point)
    {
        using namespace LLUtils;
        return ViewTransformController::ResolveOffset(point, static_cast<PointF64>(fWindow.GetCanvasSize()),
                                                      GetImageSize(ImageSizeType::Visible), fImageMargins);
    }

}  // namespace OIV
