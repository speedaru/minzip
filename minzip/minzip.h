#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <Windows.h>
#include <io.h>
#include <fcntl.h>

#include <minizip/zip.h>
#include <minizip/ioapi.h>
#include <minizip/unzip.h>

#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "minizip.lib")

namespace fs = std::filesystem;

#define MAXFILENAME (256)
#define WRITEBUFFERSIZE (8192)

//#define FOPEN_FUNC(filename, mode) fopen64(filename, mode)
//#define FTELLO_FUNC(stream) ftello64(stream)
//#define FSEEKO_FUNC(stream, offset, origin) fseeko64(stream, offset, origin)

namespace minizip {
    int check_exist_file(const char* filename);
    int filetime(const char* f, tm_zip* tmzip, uLong* dt);
    /* calculate the CRC32 of a file,
       because to encrypt a file, we need known the CRC32 of the file before */
	int getFileCrc(FILE* fin, void* buf, unsigned long size_buf, unsigned long* result_crc);
	int isLargeFile(FILE* pFile);
}

namespace minzip {
	// read file from disk
	void getFileData(LPCSTR filePath, std::vector<unsigned char>* data);
	FILE* openFileFromMemory(std::vector<unsigned char>& data);

	// ==================================================
	// =====================zipping======================
	// ==================================================
	// zip file
	int addFileToArchive(zipFile& zf, LPCSTR filePathInZip, const std::vector<unsigned char>& data);
	// zip file wrapper
	int archiveFile(LPCSTR outputZipPath, LPCSTR inputFile, LPCSTR filePathInZip, int appendMode = APPEND_STATUS_CREATE);
	// zip folder
	int addFolderToArchive(zipFile& zf, LPCSTR folderPathInZip, LPCSTR folderPathOnDisk);
	// zip folder wrapper
	int archiveFolder(LPCSTR outputZipPath, LPCSTR inputFolder, LPCSTR filePathInZip, int appendMode = APPEND_STATUS_CREATE);
	// add password protected file to archive
	int addProtectedFileToArchive(zipFile& zf, zip_fileinfo zip, LPCSTR filePathOnDisk, LPCSTR filePathInZip, LPCSTR password);
	// Function to add a password-protected file to an archive
	int zipFileWithPassword(LPCSTR archiveName, LPCSTR filePathOnDisk, LPCSTR filePathInZip, LPCSTR password, LPCSTR comment);


	// ==================================================
	// ====================unzipping=====================
	// ==================================================
	// unzip file
	int unzipFileFromArchive(unzFile& uz, LPCSTR filePathInZip, LPCSTR outputFilePath);
	// unzip file wrapper
	int unzipFile(LPCSTR archivePath, LPCSTR filePathInZip, LPCSTR outputFilePath);
	// unzip first file inside a zip file
	int unzipFirstFileFromArchive(unzFile& uz, LPCSTR outputFilePath);
	// unzip first file wrapper
	int unzipFistFile(LPCSTR archivePath, LPCSTR outputFilePath);
	// unzip folder
	int unzipFolderFromArchive(unzFile& uz, LPCSTR folderPathInZip, LPCSTR outputFolderPath);
	// unzip folder wrapper
	int unzipFolder(LPCSTR archivePath, LPCSTR folderPathInZip, LPCSTR outputFolderPath);
	// Function to unzip a single file with password protection
	int unzipFileFromArchiveWithPassword(unzFile& uz, LPCSTR filePathInZip, LPCSTR outputFilePath, LPCSTR password);
	// Function to unzip an entire archive with password protection
	int unzipArchiveWithPassword(LPCSTR archivePath, LPCSTR outputFolderPath, LPCSTR password);
}