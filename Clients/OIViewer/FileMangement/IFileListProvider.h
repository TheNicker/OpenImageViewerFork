#pragma once
#include <LLUtils/StringDefs.h>
#include "FileWatcher.h"
#include "FileSorter.h"
#include <set>
namespace OIV
{
    using FileListStringType = LLUtils::native_string_type;
    using FileListStringSetType = std::set<FileListStringType>;

    class IFileListProvider
    {
      public:

        virtual FileWatcher* GetFileWatcher() = 0;
        virtual FileSorter* GetFileSorter() = 0;
        virtual FileListStringSetType* GetknownFileTypesSet() = 0;
        virtual FileListStringType GetActiveFileName() = 0;
        virtual FileListStringType GetknownFileTypes() = 0;
    };
}  // namespace OIV