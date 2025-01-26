
#include <LLUtils/StringDefs.h>
#include <LLUtils/FileSystemHelper.h>

#include <limits>
#include <defs.h>
#include "FileWatcher.h"

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

		FileList( )
		{
			fFileWatcher.FileChangedEvent.Add(std::bind(&FileList::OnFileChanged, this, std::placeholders::_1));
		}

		void OnFileChanged(FileWatcher::FileChangedEventArgs fileChangedEventArgs) // callback from file watcher
		{

		}

		ResultCode SetFolder(LLUtils::native_string_type folder)
		{
            std::wstring absoluteFolderPath = LLUtils::FileSystemHelper::ResolveFullPath(folder);
            if (absoluteFolderPath != fCurrentFolderWatched)
            {
                if (fCurrentFolderWatched.empty() == false)
                    fFileWatcher.RemoveFolder(fCurrentFolderWatched);

                fCurrentFolderWatched = absoluteFolderPath;

				fFolderID = fFileWatcher.AddFolder(absoluteFolderPath);

				return ResultCode::RC_Success;
            }
			else
			{
				return ResultCode::RC_InvalidState;
			}
        }

		string_type GetCurrentEntry()
		{
			return fCurrentEntryIndex != IndexStart ? fListFiles.at(fCurrentEntryIndex) : {} ;
		}


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
			list_string_type fListFiles;
			index_type  fCurrentEntryIndex = IndexStart;
			std::wstring fCurrentFolderWatched;
			FileWatcher fFileWatcher;
			FileWatcher::FolderID fFolderID{};
		
	};
}