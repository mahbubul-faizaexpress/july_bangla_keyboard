// Shared VERSIONINFO block. Include from a .rc file after defining
// JULY_FILE_DESCRIPTION, JULY_ORIGINAL_FILENAME and JULY_FILE_TYPE.
#include <winver.h>
#include "JulyVersionRc.h"

VS_VERSION_INFO VERSIONINFO
 FILEVERSION    JULY_VERSION_MAJOR, JULY_VERSION_MINOR, JULY_VERSION_PATCH, 0
 PRODUCTVERSION JULY_VERSION_MAJOR, JULY_VERSION_MINOR, JULY_VERSION_PATCH, 0
 FILEFLAGSMASK  VS_FFI_FILEFLAGSMASK
 FILEFLAGS      0
 FILEOS         VOS_NT_WINDOWS32
 FILETYPE       JULY_FILE_TYPE
 FILESUBTYPE    VFT2_UNKNOWN
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904B0"
        BEGIN
            VALUE "CompanyName",      "July Bangla Keyboard project"
            VALUE "FileDescription",  JULY_FILE_DESCRIPTION
            VALUE "FileVersion",      JULY_VERSION_STRING
            VALUE "InternalName",     JULY_ORIGINAL_FILENAME
            VALUE "OriginalFilename", JULY_ORIGINAL_FILENAME
            VALUE "ProductName",      "July Bangla Keyboard"
            VALUE "ProductVersion",   JULY_VERSION_STRING
            VALUE "Comments",         "Engine " JULY_ENGINE_VERSION_STRING ", Bijoy layout " JULY_LAYOUT_VERSION_STRING
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x0409, 1200
    END
END
