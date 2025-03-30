#include "minzip.h"

void minzip::getFileData(LPCSTR filePath, std::vector<unsigned char>* data) {
    std::ifstream fs(filePath, std::ios::binary);
    if (!fs.is_open()) {
        printf("[-] failed to open file for reading :%s\n", filePath);
        return;
    }

    *data = std::vector<unsigned char>(std::istreambuf_iterator<char>(fs), std::istreambuf_iterator<char>());
}
FILE* minzip::openFileFromMemory(std::vector<unsigned char>& data) {
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD bytesWritten = 0;

    // Step 1: Create a temporary file in memory
    hFile = CreateFileW(
        L"memory_temp.dat",              // Temporary file name
        GENERIC_READ | GENERIC_WRITE,    // Read/write access
        0,                               // No sharing
        NULL,                            // Default security attributes
        CREATE_ALWAYS,                   // Create new file always
        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, // Temporary and delete on close
        NULL                             // No template file
    );
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Failed to create temporary file in memory\n");
        return nullptr;
    }

    // Step 2: Write data into the file
    if (!WriteFile(hFile, data.data(), (DWORD)data.size(), &bytesWritten, NULL)) {
        printf("Failed to write data to temporary file in memory\n");
        CloseHandle(hFile);
        return nullptr;
    }

    // Step 3: Open FILE* stream from file descriptor
    int fd = _open_osfhandle((intptr_t)hFile, _O_RDONLY);
    if (fd == -1) {
        printf("Failed to open file descriptor from handle\n");
        CloseHandle(hFile);
        return nullptr;
    }

    FILE* fileStream = _fdopen(fd, "rb");
    if (!fileStream) {
        printf("Failed to open FILE* stream from file descriptor\n");
        _close(fd);
        CloseHandle(hFile);
        return nullptr;
    }

    // Note: CloseHandle(hFile) is not called here as it's managed by fileStream

    return fileStream;
}


// ==================================================
// =====================minizip======================
// ==================================================
int minizip::check_exist_file(const char* filename) {
    FILE* ftestexist;
    int ret = 1;
    fopen_s(&ftestexist, filename, "rb");
    if (ftestexist == NULL)
        ret = 0;
    else
        fclose(ftestexist);
    return ret;
}
int minizip::filetime(const char* f, tm_zip* tmzip, uLong* dt) {
    int ret = 0;
    {
        FILETIME ftLocal;
        HANDLE hFind;
        WIN32_FIND_DATAA ff32;

        hFind = FindFirstFileA(f, &ff32);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            FileTimeToLocalFileTime(&(ff32.ftLastWriteTime), &ftLocal);
            FileTimeToDosDateTime(&ftLocal, ((LPWORD)dt) + 1, ((LPWORD)dt) + 0);
            FindClose(hFind);
            ret = 1;
        }
    }
    return ret;
}
int minizip::getFileCrc(FILE* fin, void* buf, unsigned long size_buf, unsigned long* result_crc) {
    //FILE* fin;
    unsigned long calculate_crc = 0;
    int err = ZIP_OK;
    //FILE* fin = FOPEN_FUNC(filenameinzip, "rb");
    //fopen_s(&fin, fileNameOnDisk, "rb");

    unsigned long size_read = 0;
    /* unsigned long total_read = 0; */
    if (fin == NULL)
    {
        err = ZIP_ERRNO;
    }

    if (err == ZIP_OK)
        do
        {
            err = ZIP_OK;
            size_read = (unsigned long)fread(buf, 1, (size_t)size_buf, fin);
            if (size_read < size_buf)
                if (feof(fin) == 0)
                {
                    printf("error in reading file in stream\n");
                    err = ZIP_ERRNO;
                }

            if (size_read > 0)
                calculate_crc = crc32_z(calculate_crc, reinterpret_cast<Bytef*>(buf), size_read);
            /* total_read += size_read; */

        } while ((err == ZIP_OK) && (size_read > 0));

        if (fin)
            fclose(fin);

        *result_crc = calculate_crc;
        //printf("file %s crc %lx\n", filenameinzip, calculate_crc);
        return err;
}
int minizip::isLargeFile(FILE* pFile) {
    int largeFile = 0;
    ZPOS64_T pos = 0;

    if (pFile != NULL)
    {
        fseeko64(pFile, 0, SEEK_END);
        pos = (ZPOS64_T)ftello64(pFile);

        //printf("File : %s is %llu bytes\n", filename, pos);

        if (pos >= 0xffffffff)
            largeFile = 1;

        fclose(pFile);
    }

    return largeFile;
}


// ==================================================
// =====================zipping======================
// ==================================================
int minzip::addFileToArchive(zipFile& zf, LPCSTR filePathInZip, const std::vector<unsigned char>& data) {
    zip_fileinfo zi = { 0 };
    if (zipOpenNewFileInZip(zf, filePathInZip, &zi, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION) != ZIP_OK) {
        printf("[-] failed to open zip file: %s\n", filePathInZip);
        return -1;
    }

    if (zipWriteInFileInZip(zf, data.data(), static_cast<unsigned int>(data.size())) != ZIP_OK) {
        printf("[-] failed to write data to zip file: %s\n", filePathInZip);
        zipCloseFileInZip(zf);
        return -2;
    }

    zipCloseFileInZip(zf);

    return ZIP_OK;
}

int minzip::archiveFile(LPCSTR outputZipPath, LPCSTR inputFile, LPCSTR filePathInZip, int appendMode) {
    zipFile zf = zipOpen(outputZipPath, APPEND_STATUS_CREATE);
    if (!zf) {
        printf("[-] failed to open zip file: %s\n", outputZipPath);
        return -1;
    }

    std::vector<unsigned char> fileData;
    getFileData(inputFile, &fileData);

    if (fileData.size() <= 0) {
        printf("[-] failed to get file data from: %s\n", inputFile);
        return -2;
    }

    int result = addFileToArchive(zf, filePathInZip, fileData);
    if (result != ZIP_OK) {
        printf("[-] failed to add: %s to zip file\n", inputFile);
        return -3;
    }

    zipClose(zf, nullptr);
    return result;
}

int minzip::addFolderToArchive(zipFile& zf, LPCSTR folderPathInZip, LPCSTR folderPathOnDisk) {
    std::string folderPath = std::string(folderPathOnDisk);

    for (const auto& entry : fs::recursive_directory_iterator(folderPathOnDisk)) {
        // if entry is a folder then skip it, it will be added automatically
        if (!entry.is_regular_file()) {
            continue;
        }

        std::string entryFilePath = entry.path().string();
        std::string relativePath = entryFilePath.substr(folderPath.length() + 1); /* +1 to account for \ */
        fs::path filePathInZip(folderPathInZip);
        filePathInZip /= relativePath;

        std::vector<unsigned char> data;
        getFileData(entryFilePath.c_str(), &data);

        if (data.size() <= 0) {
            printf("[-] failed to get file data from: %s\n", entryFilePath.c_str());
            return -2;
        }

        int result = addFileToArchive(zf, filePathInZip.string().c_str(), data);
        if (result != ZIP_OK) {
            printf("[-] failed to add file to zip: %s\n", entryFilePath.c_str());
            return result;
        }
    }

    return ZIP_OK;
}

int minzip::archiveFolder(LPCSTR outputZipPath, LPCSTR inputFolder, LPCSTR filePathInZip, int appendMode) {
    zipFile zf = zipOpen(outputZipPath, appendMode);
    if (!zf) {
        printf("[-] failed to open zip file: %s\n", outputZipPath);
        return -1;
    }

    int result = addFolderToArchive(zf, filePathInZip, inputFolder);
    if (result != ZIP_OK) {
        printf("[-] failed to zip folder: %s\n", inputFolder);
        return -1;
    }

    zipClose(zf, nullptr);
    return result;
}

int minzip::addProtectedFileToArchive(zipFile& zf, zip_fileinfo zip, LPCSTR filePathOnDisk, LPCSTR filePathInZip, LPCSTR password) {
    // Initialize variables
    zip_fileinfo zi = { 0 };
    int err = ZIP_OK;
    unsigned long crcFile = 0;
    void* buf = NULL;
    size_t size_buf = WRITEBUFFERSIZE;
    int zip64 = 0;

    // allocate memory
    buf = (void*)malloc(size_buf);
    if (buf == NULL) {
        printf("Error allocating memory\n");
        return ZIP_INTERNALERROR;
    }

    // Set file time
    minizip::filetime(filePathInZip, &zi.tmz_date, &zi.dosDate);

    if (password != NULL) {
        FILE* fin;
        fopen_s(&fin, filePathOnDisk, "rb");
        err = minizip::getFileCrc(fin, buf, (unsigned long)size_buf, &crcFile);
        if (err != ZIP_OK) {
            printf("Error getting CRC for %s\n", filePathInZip);
            free(buf);
            zipClose(zf, NULL);
            return err;
        }
    }

    FILE* fi;
    fopen_s(&fi, filePathOnDisk, "rb");

    // Determine if zip64 is required
    zip64 = minizip::isLargeFile(fi);

    // Open new file in zip
    err = zipOpenNewFileInZip3_64(
        zf,                         // zip file
        filePathInZip,              // file name
        &zi,                        // zipfile info
        NULL, 0, NULL, 0, NULL,     // extrafield_local, size_ef_local, ef_global, size_ef_global, comment
        Z_DEFLATED,                 // method
        Z_DEFAULT_COMPRESSION,      // level
        0,                          // raw
        -MAX_WBITS,                 // windowBits
        DEF_MEM_LEVEL,              // memLevel
        Z_DEFAULT_STRATEGY,         // strategy
        password, crcFile, zip64);  // password, crcFile, zip64

    if (err != ZIP_OK) {
        printf("Error opening %s in zipfile\n", filePathInZip);
        free(buf);
        zipClose(zf, NULL);
        return err;
    }

    // get file data
    std::vector<unsigned char> data;
    getFileData(filePathOnDisk, &data);

    // Write data from 'data' vector to the zip file
    if (!data.empty()) {
        err = zipWriteInFileInZip(zf, data.data(), (unsigned)data.size());
        if (err < 0) {
            printf("Error writing %s in the zipfile\n", filePathInZip);
            zipCloseFileInZip(zf);
            zipClose(zf, NULL);
            return err;
        }
    }

    // Close the file in the zip
    err = zipCloseFileInZip(zf);
    if (err != ZIP_OK) {
        printf("Error closing %s in the zipfile\n", filePathInZip);
        zipClose(zf, NULL);
        return err;
    }

    printf("Successfully added %s to the zipfile\n", filePathInZip);
    return ZIP_OK;
}

int minzip::zipFileWithPassword(LPCSTR archiveName, LPCSTR filePathOnDisk, LPCSTR filePathInZip, LPCSTR password, LPCSTR comment) {
    // open zip
    zipFile zf = zipOpen64(archiveName, 0);
    if (zf == NULL) {
        printf("Error opening %s\n", archiveName);
        return ZIP_ERRNO;
    }

    // Set file time
    zip_fileinfo zi{ 0 };
    minizip::filetime(filePathOnDisk, &zi.tmz_date, &zi.dosDate);

    int err = addProtectedFileToArchive(zf, zi, filePathInZip, filePathOnDisk, password);

    if (err == ZIP_OK) {
        zipClose(zf, comment);
    }

    return err;
}

int zipFileWithPassword(const char* zipfilename, const char* filenameinzip, const char* password) {
    // init vars
    zipFile zf;
    zip_fileinfo zi = { 0 };
    FILE* fin = NULL;
    size_t size_read;
    unsigned long crcFile = 0;
    int err;
    int zip64 = 0;
    void* buf = NULL;
    size_t size_buf = WRITEBUFFERSIZE;

    // allocate memory
    buf = (void*)malloc(size_buf);
    if (buf == NULL) {
        printf("Error allocating memory\n");
        return ZIP_INTERNALERROR;
    }

    // open zip
    zf = zipOpen64(zipfilename, 0);
    if (zf == NULL) {
        printf("Error opening %s\n", zipfilename);
        free(buf);
        return ZIP_ERRNO;
    }

    printf("Creating %s\n", zipfilename);

    minizip::filetime(filenameinzip, &zi.tmz_date, &zi.dosDate);

    if (password != NULL) {
        FILE* fin;
        fopen_s(&fin, filenameinzip, "rb");
        err = minizip::getFileCrc(fin, buf, (unsigned long)size_buf, &crcFile);
        if (err != ZIP_OK) {
            printf("Error getting CRC for %s\n", filenameinzip);
            free(buf);
            zipClose(zf, NULL);
            return err;
        }
    }

    FILE* pFile;
    fopen_s(&pFile, filenameinzip, "rb");
    zip64 = minizip::isLargeFile(pFile);

    err = zipOpenNewFileInZip3_64(zf, filenameinzip, &zi,
        NULL, 0, NULL, 0, NULL,
        Z_DEFLATED,
        Z_DEFAULT_COMPRESSION, 0,
        -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
        password, crcFile, zip64);

    if (err != ZIP_OK) {
        printf("Error opening %s in zipfile\n", filenameinzip);
        free(buf);
        zipClose(zf, NULL);
        return err;
    }

    fopen_s(&fin, filenameinzip, "rb");
    if (fin == NULL) {
        printf("Error opening %s for reading\n", filenameinzip);
        free(buf);
        zipCloseFileInZip(zf);
        zipClose(zf, NULL);
        return ZIP_ERRNO;
    }

    do {
        size_read = fread(buf, 1, size_buf, fin);
        if (size_read < size_buf && ferror(fin)) {
            printf("Error reading %s\n", filenameinzip);
            free(buf);
            fclose(fin);
            zipCloseFileInZip(zf);
            zipClose(zf, NULL);
            return ZIP_ERRNO;
        }

        if (size_read > 0) {
            err = zipWriteInFileInZip(zf, buf, (unsigned)size_read);
            if (err < 0) {
                printf("Error writing %s in the zipfile\n", filenameinzip);
                free(buf);
                fclose(fin);
                zipCloseFileInZip(zf);
                zipClose(zf, NULL);
                return ZIP_ERRNO;
            }
        }
    } while (size_read > 0);

    fclose(fin);
    free(buf);

    err = zipCloseFileInZip(zf);
    if (err != ZIP_OK) {
        printf("Error closing %s in the zipfile\n", filenameinzip);
        zipClose(zf, NULL);
        return err;
    }

    err = zipClose(zf, NULL);
    if (err != ZIP_OK) {
        printf("Error closing %s\n", zipfilename);
        return err;
    }

    printf("Successfully created %s with password protection\n", zipfilename);
    return ZIP_OK;
}


// ==================================================
// ====================unzipping=====================
// ==================================================

int minzip::unzipFileFromArchive(unzFile& uz, LPCSTR filePathInZip, LPCSTR outputFilePath) {
    if (unzLocateFile(uz, filePathInZip, 0) != UNZ_OK) {
        printf("[-] failed to locate file in zip: %s\n", filePathInZip);
        return -1;
    }

    if (unzOpenCurrentFile(uz) != UNZ_OK) {
        printf("[-] failed to open current file in zip: %s\n", filePathInZip);
        return -2;
    }
    
    // make sure output file dir
    fs::path outputFileDir = fs::absolute(outputFilePath).parent_path();
    if (!fs::exists(outputFileDir)) {
        fs::create_directories(outputFileDir);
    }

    std::ofstream fs(outputFilePath, std::ios::binary);
    if (!fs.is_open()) {
        printf("[-] failed to open file stream at: %s\n", outputFilePath);
        return -3;
    }

    std::vector<unsigned char> buffer(8192);
    int bytesRead;
    while ((bytesRead = unzReadCurrentFile(uz, buffer.data(), static_cast<int>(buffer.size()))) > 0) {
        fs.write(reinterpret_cast<const char*>(buffer.data()), bytesRead);
    }

    unzCloseCurrentFile(uz);
    fs.close();
    return UNZ_OK;
}

int minzip::unzipFile(LPCSTR archivePath, LPCSTR filePathInZip, LPCSTR outputFilePath) {
    unzFile uz = unzOpen(archivePath);
    if (!uz) {
        printf("[-] failed to open archiv: %s\n", archivePath);
        return -1;
    }

    int result = unzipFileFromArchive(uz, filePathInZip, outputFilePath);

    unzClose(uz);
    return result;
}

int minzip::unzipFirstFileFromArchive(unzFile& uz, LPCSTR outputFilePath) {
    if (unzGoToFirstFile(uz) != UNZ_OK) {
        printf("[-] failed to go to first file in archive\n");
        return -1;
    }

    char szFileName[MAX_PATH];
    if (unzGetCurrentFileInfo(uz, nullptr, szFileName, sizeof(szFileName), nullptr, 0, nullptr, 0)) {
        printf("[-] failed to get first file name\n");
        return -2;
    }

    LPCSTR firstFileName = const_cast<LPCSTR>(szFileName);

    return unzipFileFromArchive(uz, firstFileName, outputFilePath);
}

int minzip::unzipFistFile(LPCSTR archivePath, LPCSTR outputFilePath) {
    unzFile uz = unzOpen(archivePath);
    if (!uz) {
        printf("[-] failed to open archiv: %s\n", archivePath);
        return -1;
    }

    int result = unzipFirstFileFromArchive(uz, outputFilePath);

    unzClose(uz);
    return result;
}

int minzip::unzipFolderFromArchive(unzFile& uz, LPCSTR folderPathInZip, LPCSTR outputFolderPath) {
    if (unzGoToFirstFile(uz) != UNZ_OK) {
        printf("[-] failed to open first file in archive\n");
        return -1;
    }

    // make folderPathInZip into string and replace \ by /
    std::string pathInZip(folderPathInZip);
    std::replace(pathInZip.begin(), pathInZip.end(), '\\', '/');

    if (pathInZip == ".") {
        pathInZip.clear();  // Treat '.' as the root directory
    }

    // make path in zip always end with /
    if (!pathInZip.empty() && !pathInZip.ends_with('/')) {
        pathInZip += '/';
    }

    bool foundDirAlr = false;
    do {
        unz_file_info unzInfo;
        char currentFile[MAX_PATH];
        if (unzGetCurrentFileInfo(uz, &unzInfo, currentFile, sizeof(currentFile), nullptr, 0, nullptr, 0)) {
            printf("[-] failed to get file name\n");
            return -2;
        }

        // skip folders
        if (currentFile[strlen(currentFile) - 1] == '/') {
            continue;
        }

        // get current dir and add trailing /
        std::string currentDir = fs::path(currentFile).parent_path().string();
        if (!currentDir.ends_with("/")) {
            currentDir += '/';
        }

        if (currentDir.find(pathInZip) != std::string::npos) {
            if (!foundDirAlr) {
                foundDirAlr = true;
            }

            // get only last dir
            fs::path outputPath = fs::path(outputFolderPath) / std::string(currentFile).substr(pathInZip.length());

            int res = unzipFileFromArchive(uz, currentFile, outputPath.string().c_str());
            if (res != UNZ_OK) {
                printf("[-] failed to extract file: %d\n", res);
                return -3;
            }
        }
        else {
            // if alr found dir and extracted everything then no need to continune checking
            if (foundDirAlr) {
                break;
            }
        }
    } while (unzGoToNextFile(uz) == UNZ_OK);

    return UNZ_OK;
}

int minzip::unzipFolder(LPCSTR archivePath, LPCSTR folderPathInZip, LPCSTR outputFolderPath) {
    unzFile uz = unzOpen(archivePath);
    if (!uz) {
        printf("[-] failed to open archiv: %s\n", archivePath);
        return -1;
    }

    int result = unzipFolderFromArchive(uz, folderPathInZip, outputFolderPath);

    unzClose(uz);
    return result;
}

int minzip::unzipFileFromArchiveWithPassword(unzFile& uz, LPCSTR filePathInZip, LPCSTR outputFilePath, LPCSTR password) {
    if (unzLocateFile(uz, filePathInZip, 0) != UNZ_OK) {
        printf("[-] failed to locate file in zip: %s\n", filePathInZip);
        return -1;
    }

    if (unzOpenCurrentFilePassword(uz, password) != UNZ_OK) {
        printf("[-] failed to open %s with password: %s\n", filePathInZip, password);
        return -2;
    }

    // Make sure the output file directory exists
    fs::path outputFileDir = fs::absolute(outputFilePath).parent_path();
    if (!fs::exists(outputFileDir)) {
        fs::create_directories(outputFileDir);
    }

    std::ofstream ofs(outputFilePath, std::ios::binary);
    if (!ofs.is_open()) {
        printf("[-] failed to open file stream at: %s\n", outputFilePath);
        unzCloseCurrentFile(uz);
        return -3;
    }

    std::vector<unsigned char> buffer(8192);
    int bytesRead;
    while ((bytesRead = unzReadCurrentFile(uz, buffer.data(), static_cast<int>(buffer.size()))) > 0) {
        ofs.write(reinterpret_cast<const char*>(buffer.data()), bytesRead);
    }

    unzCloseCurrentFile(uz);
    ofs.close();
    return UNZ_OK;
}

int minzip::unzipArchiveWithPassword(LPCSTR archivePath, LPCSTR outputFolderPath, LPCSTR password) {
    unzFile uz = unzOpen(archivePath);
    if (!uz) {
        printf("[-] failed to open archive: %s\n", archivePath);
        return -1;
    }

    if (unzGoToFirstFile(uz) != UNZ_OK) {
        printf("[-] failed to open first file in archive\n");
        unzClose(uz);
        return -2;
    }

    do {
        unz_file_info unzInfo;
        char currentFileName[MAX_PATH];
        if (unzGetCurrentFileInfo(uz, &unzInfo, currentFileName, sizeof(currentFileName), nullptr, 0, nullptr, 0)) {
            printf("[-] failed to get file name\n");
            unzClose(uz);
            return -3;
        }

        // Skip folders
        if (currentFileName[strlen(currentFileName) - 1] == '/') {
            continue;
        }

        // if output folder is . or empty then output path is just current file name
        fs::path outputPath;
        if (!_strcmpi(outputFolderPath, ".") || strlen(outputFolderPath) == 0) {
            outputPath = currentFileName;
        }
        else {
            outputPath = fs::path(outputFolderPath) / currentFileName;
        }

        int res = unzipFileFromArchiveWithPassword(uz, currentFileName, outputPath.string().c_str(), password);
        if (res != UNZ_OK) {
            printf("[-] failed to extract file: %s\n", currentFileName);
            unzClose(uz);
            return res;
        }
    } while (unzGoToNextFile(uz) == UNZ_OK);

    unzClose(uz);
    return UNZ_OK;
}