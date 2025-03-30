# minzip

Minzip is a simple C++ wrapper for working with zip archives. It supports zipping and unzipping files and folders, with optional password protection for zip archives. This library is built on top of [minizip](https://github.com/nmoinvaz/minizip) and [zlib](https://github.com/madler/zlib).

## Features

- **Zip file operations**:
  - Add files and folders to a zip archive.
  - Support for password-protected files.
  - Append or create new archives.
  
- **Unzip file operations**:
  - Extract files and folders from a zip archive.
  - Support for password-protected zip archives.

## Project Structure

- `minzip.cpp` contains the implementation of the zip/unzip functionality.
- `minzip.h` provides the interface for the zip/unzip functions.
- `include/` contains the necessary header files for zlib and minizip.
- `lib/` contains the static libraries for zlib and minizip.

## Dependencies

- [minizip](https://github.com/nmoinvaz/minizip)
- [zlib](https://github.com/madler/zlib)

## Functions

### Zipping

- **addFileToArchive**: Adds a single file to a zip archive.
- **archiveFile**: Creates or appends a file to a zip archive.
- **addFolderToArchive**: Adds a folder and its contents to a zip archive.
- **archiveFolder**: Creates or appends a folder to a zip archive.
- **addProtectedFileToArchive**: Adds a file with password protection to a zip archive.
- **zipFileWithPassword**: Creates a password-protected zip archive from a file.

### Unzipping

- **unzipFileFromArchive**: Extracts a file from a zip archive.
- **unzipFile**: Extracts a file from a zip archive using a wrapper.
- **unzipFirstFileFromArchive**: Extracts the first file from a zip archive.
- **unzipFistFile**: Extracts the first file using a wrapper.
- **unzipFolderFromArchive**: Extracts a folder from a zip archive.
- **unzipFolder**: Extracts a folder using a wrapper.
- **unzipFileFromArchiveWithPassword**: Extracts a password-protected file from a zip archive.
- **unzipArchiveWithPassword**: Extracts an entire archive with password protection.

## Example Usage

### Zipping a File

```cpp
#include "minzip.h"

int main() {
    const char* archiveName = "example.zip";
    const char* filePathOnDisk = "example.txt";
    const char* filePathInZip = "example.txt";

    int result = minizip::archiveFile(archiveName, filePathOnDisk, filePathInZip);
    if (result == 0) {
        std::cout << "File successfully added to the archive." << std::endl;
    } else {
        std::cerr << "Failed to add file to the archive." << std::endl;
    }
    return 0;
}
```

### Unzipping a File

```cpp
#include "minzip.h"

int main() {
    const char* archivePath = "example.zip";
    const char* filePathInZip = "example.txt";
    const char* outputFilePath = "output_example.txt";

    int result = minizip::unzipFile(archivePath, filePathInZip, outputFilePath);
    if (result == 0) {
        std::cout << "File successfully extracted." << std::endl;
    } else {
        std::cerr << "Failed to extract file." << std::endl;
    }
    return 0;
}
```

### Password-Protected Zip File

```cpp
#include "minzip.h"

int main() {
    const char* archiveName = "protected.zip";
    const char* filePathOnDisk = "example.txt";
    const char* filePathInZip = "example.txt";
    const char* password = "mypassword";

    int result = minizip::zipFileWithPassword(archiveName, filePathOnDisk, filePathInZip, password, "This is a protected zip file.");
    if (result == 0) {
        std::cout << "File successfully added to the password-protected archive." << std::endl;
    } else {
        std::cerr << "Failed to add file to the archive." << std::endl;
    }
    return 0;
}
```

### Building the Project
- Ensure that zlib.lib and minizip.lib are in the lib/ folder.
- Include the header files in the include/ folder in your project.
- Link the zlib.lib and minizip.lib static libraries in your build settings.

Example for Visual Studio
Add zlib.lib and minizip.lib under Project Properties > Linker > Input > Additional Dependencies.

Add the include/ folder under Project Properties > C/C++ > General > Additional Include Directories.
