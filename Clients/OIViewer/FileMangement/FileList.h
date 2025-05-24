
#include <LLUtils/StringDefs.h>
#include <LLUtils/FileSystemHelper.h>
#include <limits>
#include <iterator>
#include <defs.h>
#include "IFileListProvider.h"

namespace OIV
{
    /// @brief A class to manage file browsing with embedded file watcher
    class FileList
    {
      public:

        using string_type = FileListStringType;
        using list_string_type = LLUtils::ListString<string_type>;
        using index_type = list_string_type::difference_type;
        using size_type = list_string_type::size_type;

        static constexpr auto IndexEnd = std::numeric_limits<index_type>::max();
        static constexpr auto IndexStart = std::numeric_limits<index_type>::min();

        FileList(IFileListProvider* fileListProvider) : fFileListProvider(fileListProvider)
        {
            fileListProvider->GetFileWatcher()->FileChangedEvent.Add(
                std::bind(&FileList::OnFileChanged, this, std::placeholders::_1));
        }

        void SetFolder(const string_type& folder)
        {
            using namespace std::filesystem;

            const std::wstring absoluteFolderPath = LLUtils::FileSystemHelper::ResolveFullPath(folder);

            if (absoluteFolderPath != fCurrentFolder)
            {
                if (fCurrentFolder.empty() == false)
                    fFileListProvider->GetFileWatcher()->RemoveFolder(fCurrentFolder);

                fCurrentFolder = absoluteFolderPath;

                LoadFileInFolder();
                // Watch current folder
                fFolderID = fFileListProvider->GetFileWatcher()->AddFolder(fCurrentFolder);

                fCurrentEntryIndex = IndexStart;
            }
        }

        const string_type& GetFolder() const { return fCurrentFolder; }

        size_t GetSize() const { return fListFiles.size(); }

        index_type SetCurrentIndexByElementName(const string_type& element)
        {
            list_string_type::iterator it = std::find(fListFiles.begin(), fListFiles.end(), element);

            if (it != fListFiles.end())
                fCurrentEntryIndex = std::distance(fListFiles.begin(), it);
        }

        void Sort() { std::sort(fListFiles.begin(), fListFiles.end(), fFileListProvider->GetFileSorter()); }

        ResultCode JumpDelta(index_type step)
        {
            if (fListFiles.empty())
                return ResultCode::RC_InvalidState;

            size_type totalFiles = fListFiles.size();
            index_type entryIndex = fCurrentEntryIndex;

            switch (step)
            {
                case IndexEnd:
                    entryIndex = static_cast<index_type>(fListFiles.size());
                    break;
                case IndexStart:
                    entryIndex = static_cast<index_type>(0);
                    break;
                default:
                    entryIndex += step;
                    break;
            }

            if (entryIndex < 0 || entryIndex >= fListFiles.size())
                return ResultCode::RC_OutOfRange;

            fCurrentEntryIndex = entryIndex;
            return ResultCode::RC_Success;
        }

      private:

        void LoadFileInFolder()
        {
            using namespace std::filesystem;

            auto fileList = GetSupportedFileListInFolder(fCurrentFolder, fFileListProvider->GetknownFileTypes(),
                                                         *fFileListProvider->GetFileSorter());

            // File is loaded from a different folder then the active one.
            std::swap(fListFiles, fileList);
            // UpdateOpenedFileIndex();
        }

        list_string_type GetSupportedFileListInFolder(const string_type& folderPath, const string_type& knownFileTypes,
                                                      const FileSorter& fileSorter)
        {
            list_string_type fileList;
            if (std::filesystem::is_directory(folderPath))
            {
                LLUtils::FileSystemHelper::FindFiles(fileList, folderPath, fFileListProvider->GetknownFileTypes(),
                                                     false, false);
                std::sort(fileList.begin(), fileList.end(), fileSorter);
            }
            else
            {
                LL_EXCEPTION(LLUtils::Exception::ErrorCode::InvalidState, "Not a folder");
            }

            return fileList;
        }

        string_type GetCurrentEntry()
        {
            return fCurrentEntryIndex != IndexStart ? fListFiles.at(fCurrentEntryIndex) : string_type{};
        }

        // void TestApp::OnFileChangedImpl(FileWatcher::FileChangedEventArgs* fileChangedEventArgsPtr)
        // {
        //     auto fileChangedEventArgs = *fileChangedEventArgsPtr;

        //     if (fileChangedEventArgs.folderID == fOpenedFileFolderID)
        //     {
        //         std::wstring absoluteFilePath = std::filesystem::path(GetOpenedFileName());
        //         std::wstring absoluteFolderPath = std::filesystem::path(GetOpenedFileName()).parent_path();
        //         std::wstring changedFileName =
        //             (std::filesystem::path(fileChangedEventArgs.folder) / fileChangedEventArgs.fileName).wstring();
        //         std::wstring changedFileName2 =
        //             (std::filesystem::path(fileChangedEventArgs.folder) / fileChangedEventArgs.fileName2).wstring();

        //         switch (fileChangedEventArgs.fileOp)
        //         {
        //             case FileWatcher::FileChangedOp::None:
        //                 break;
        //             case FileWatcher::FileChangedOp::Add:
        //                 UpdateFileList(fileChangedEventArgs.fileOp, changedFileName, std::wstring());
        //                 break;
        //             case FileWatcher::FileChangedOp::Remove:
        //                 UpdateFileList(fileChangedEventArgs.fileOp, changedFileName, std::wstring());
        //                 break;
        //             case FileWatcher::FileChangedOp::Modified:
        //                 if (absoluteFilePath == changedFileName)
        //                     ProcessCurrentFileChanged();
        //                 break;
        //             case FileWatcher::FileChangedOp::Rename:
        //                 UpdateFileList(FileWatcher::FileChangedOp::Rename, changedFileName, changedFileName2);
        //                 if (absoluteFilePath == changedFileName2)
        //                     ProcessCurrentFileChanged();
        //                 break;

        //             case FileWatcher::FileChangedOp::WatchedFolderRemoved:
        //                 fCurrentFolderWatched.clear();
        //                 break;
        //         }
        //     }
        //     else if (fileChangedEventArgs.folderID == fCOnfigurationFolderID)
        //     {
        //         if (fileChangedEventArgs.fileName == L"Settings.json")
        //         {
        //             LoadSettings();
        //         }
        //     }
        //     else
        //     {
        //         LL_EXCEPTION_UNEXPECTED_VALUE;
        //     }
        // }

        void UpdateFileList(FileWatcher::FileChangedOp fileOp, const std::wstring& filePath,
                            const std::wstring& filePath2)
        {
            switch (fileOp)
            {
                case FileWatcher::FileChangedOp::Add:
                {
                    // Add file to list only if it's a known file type
                    std::wstring extension = LLUtils::StringUtility::ToLower(
                        std::filesystem::path(filePath).extension().wstring());

                    std::wstring_view sv(extension);
                    if (sv.empty() == false)
                        sv = sv.substr(1);

                    if (fKnownFileTypesSet.contains(sv.data()))
                    {
                        // TODO: add file sorted
                        auto itAddedFile = std::lower_bound(fListFiles.begin(), fListFiles.end(), filePath,
                                                            fFileSorter);

                        if (itAddedFile != fListFiles.end() && *itAddedFile == filePath)
                            LL_EXCEPTION(LLUtils::Exception::ErrorCode::InvalidState, "Trying to add an existing file");

                        fListFiles.insert(itAddedFile, filePath);

                        // File has been added to the current folder, indices have changed - update current file index
                        auto itCurrentFile = std::lower_bound(fListFiles.begin(), fListFiles.end(), GetOpenedFileName(),
                                                              fFileSorter);
                        fCurrentFileIndex = std::distance(fListFiles.begin(), itCurrentFile);

                        UpdateTitle();
                    }
                }
                break;

                case FileWatcher::FileChangedOp::Remove:
                {
                    auto it = std::find(fListFiles.begin(), fListFiles.end(), filePath);
                    if (it != fListFiles.end())
                    {
                        auto fileNameToRemove = *it;
                        fListFiles.erase(it);
                        ProcessRemovalOfOpenedFile(fileNameToRemove);
                    }
                }
                break;
                case FileWatcher::FileChangedOp::Rename:
                {
                    auto it = std::find(fListFiles.begin(), fListFiles.end(), filePath);
                    if (it != fListFiles.end())
                    {
                        auto fileNameToRemove = *it;
                        fListFiles.erase(it);
                        auto itRenamedFile = std::lower_bound(fListFiles.begin(), fListFiles.end(), filePath2,
                                                              fFileSorter);
                        fListFiles.insert(itRenamedFile, filePath2);

                        if (filePath == GetOpenedFileName())
                        {
                            UnloadOpenedImaged();
                            LoadFile(filePath2, IMCodec::PluginTraverseMode::NoTraverse);
                        }
                        else
                        {
                            // File has been added to the current folder, indices have changed - update current file index
                            auto itCurrentFile = std::lower_bound(fListFiles.begin(), fListFiles.end(),
                                                                  GetOpenedFileName(), fFileSorter);
                            fCurrentFileIndex = std::distance(fListFiles.begin(), itCurrentFile);
                            UpdateTitle();
                        }
                    }
                    else
                    {
                        LL_EXCEPTION(LLUtils::Exception::ErrorCode::InvalidState, "Invalid file removal request");
                    }
                }

                break;

                case FileWatcher::FileChangedOp::Modified:
                case FileWatcher::FileChangedOp::None:
                case FileWatcher::FileChangedOp::WatchedFolderRemoved:
                    break;
            }
        }

        void OnFileChanged(FileWatcher::FileChangedEventArgs fileChangedEventArgs)  // callback from file watcher
        {
            if (fileChangedEventArgs.folderID == fFolderID)
            {
                auto openedFileName = fFileListProvider->GetOpenedFileName();
                std::wstring absoluteFilePath = std::filesystem::path(openedFileName);
                std::wstring absoluteFolderPath = std::filesystem::path(openedFileName).parent_path();
                std::wstring changedFileName =
                    (std::filesystem::path(fileChangedEventArgs.folder) / fileChangedEventArgs.fileName).wstring();
                std::wstring changedFileName2 =
                    (std::filesystem::path(fileChangedEventArgs.folder) / fileChangedEventArgs.fileName2).wstring();

                switch (fileChangedEventArgs.fileOp)
                {
                    case FileWatcher::FileChangedOp::None:
                        break;
                    case FileWatcher::FileChangedOp::Add:
                        UpdateFileList(fileChangedEventArgs.fileOp, changedFileName, std::wstring());
                        break;
                    case FileWatcher::FileChangedOp::Remove:
                        UpdateFileList(fileChangedEventArgs.fileOp, changedFileName, std::wstring());
                        break;
                    case FileWatcher::FileChangedOp::Modified:
                        if (absoluteFilePath == changedFileName)
                            ProcessCurrentFileChanged();
                        break;
                    case FileWatcher::FileChangedOp::Rename:
                        UpdateFileList(FileWatcher::FileChangedOp::Rename, changedFileName, changedFileName2);
                        if (absoluteFilePath == changedFileName2)
                            ProcessCurrentFileChanged();
                        break;

                    case FileWatcher::FileChangedOp::WatchedFolderRemoved:
                        fCurrentFolderWatched.clear();
                        break;
                }
            }
            else if (fileChangedEventArgs.folderID == fCOnfigurationFolderID)
            {
                if (fileChangedEventArgs.fileName == L"Settings.json")
                {
                    LoadSettings();
                }
            }
            else
            {
                LL_EXCEPTION_UNEXPECTED_VALUE;
            }
        }

        // void TestApp::ProcessCurrentFileChanged()
        // {
        //     switch (fMofifiedFileReloadMode)
        //     {
        //         case MofifiedFileReloadMode::AutoBackground:
        //             LoadFile(GetOpenedFileName(), IMCodec::PluginTraverseMode::NoTraverse);  // Load file immediatly
        //             break;
        //         case MofifiedFileReloadMode::AutoForeground:
        //         case MofifiedFileReloadMode::Confirmation:  // implicitly foreground
        //             if (GetAppActive())
        //                 PerformReloadFile(GetOpenedFileName());
        //             else
        //                 fPendingReloadFileName = GetOpenedFileName();
        //             break;
        //         case MofifiedFileReloadMode::None:  // do nothing
        //             break;
        //     }
        // }

      private:

        IFileListProvider* fFileListProvider;
        list_string_type fListFiles;
        index_type fCurrentEntryIndex = IndexStart;
        string_type fCurrentFolder;
        FileWatcher::FolderID fFolderID{};
    };
}  // namespace OIV
