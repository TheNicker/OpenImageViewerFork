
#include <LLUtils/StringDefs.h>
#include <LLUtils/FileSystemHelper.h>
#include <limits>
#include <iterator>
#include <defs.h>
#include "FileWatcher.h"
#include "FileSorter.h"

namespace OIV
{
    /// @brief A class to manage file browsing with embedded file watcher
    class FileList
    {
      public:

        using string_type = LLUtils::native_string_type;
        using list_string_type = LLUtils::ListString<string_type>;
        using index_type = list_string_type::difference_type;
        using size_type = list_string_type::size_type;

        static constexpr auto IndexEnd = std::numeric_limits<index_type>::max();
        static constexpr auto IndexStart = std::numeric_limits<index_type>::min();

        FileList(FileWatcher* fileWatcher, FileSorter* fileSorter) : fFileWatcher(fileWatcher), fFileSorter(fileSorter)
        {
            fFileWatcher->FileChangedEvent.Add(std::bind(&FileList::OnFileChanged, this, std::placeholders::_1));
        }

        void SetFolder(const string_type& folder, const string_type& knownFileTypes)
        {
            using namespace std::filesystem;
            const std::wstring absoluteFolderPath = LLUtils::FileSystemHelper::ResolveFullPath(folder);

            if (absoluteFolderPath != fCurrentFolder)
            {
                if (fCurrentFolder.empty() == false)
                    fFileWatcher->RemoveFolder(fCurrentFolder);

                fCurrentFolder = absoluteFolderPath;

                LoadFileInFolder(knownFileTypes);
                // Watch current folder
                fFolderID = fFileWatcher->AddFolder(fCurrentFolder);

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

        void Sort() { std::sort(fListFiles.begin(), fListFiles.end(), *fFileSorter); }

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

        void LoadFileInFolder(const string_type& knownFileTypes)
        {
            using namespace std::filesystem;

            auto fileList = GetSupportedFileListInFolder(fCurrentFolder, knownFileTypes, *fFileSorter);

            // File is loaded from a different folder then the active one.
            std::swap(fListFiles, fileList);
            // UpdateOpenedFileIndex();
        }

        static list_string_type GetSupportedFileListInFolder(const string_type& folderPath,
                                                             const string_type& knownFileTypes,
                                                             const FileSorter& fileSorter)
        {
            list_string_type fileList;
            if (std::filesystem::is_directory(folderPath))
            {
                LLUtils::FileSystemHelper::FindFiles(fileList, folderPath, knownFileTypes, false, false);
                std::sort(fileList.begin(), fileList.end(), fileSorter);
            }
            else
            {
                LL_EXCEPTION(LLUtils::Exception::ErrorCode::InvalidState, "Not a folder");
            }

            return fileList;
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

        void OnFileChanged(FileWatcher::FileChangedEventArgs fileChangedEventArgs)  // callback from file watcher
        {
        }

        string_type GetCurrentEntry()
        {
            return fCurrentEntryIndex != IndexStart ? fListFiles.at(fCurrentEntryIndex) : string_type{};
        }

      private:

        list_string_type fListFiles;
        index_type fCurrentEntryIndex = IndexStart;
        string_type fCurrentFolder;
        FileWatcher* fFileWatcher;
        FileSorter* fFileSorter;
        FileWatcher::FolderID fFolderID{};
    };
}  // namespace OIV
