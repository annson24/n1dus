#include <string>
#include <stdio.h>
#include <vector>
#include <sys/stat.h>
#include <windows.h>
#include <random>
#include <assert.h>

template <typename T> T randomFrom(const T min, const T max)
{
	static std::random_device rdev;
	static std::default_random_engine re(rdev());
	typedef typename std::conditional<
		std::is_floating_point<T>::value,
		std::uniform_real_distribution<T>,
		std::uniform_int_distribution<T>>::type dist_type;
	dist_type uni(min, max);
	return static_cast<T>(uni(re));
}

bool fileExist(const std::string& filename)
{
	WIN32_FIND_DATAA fd = { 0 };
	HANDLE hFound = FindFirstFileA(filename.c_str(), &fd);
	bool retval = hFound != INVALID_HANDLE_VALUE;
	FindClose(hFound);

	return retval;
}

std::string fixedWidth(int value, int width)
{
	char buffer[10];
	snprintf(buffer, sizeof(buffer), "%.*d", width, value);
	return buffer;
}


// Class to handle virtual files split in 4GB parts
class virtualFile
{
public:
    const uint64_t chunkSize = 0xFFFF0000;// # 4,294,901,760 bytes

    bool open(const char *fn, const char *mode)
    {
		// Create or Open first chunk
        this->mode = mode;
        logicalFilename = fn;
        FileEntry fileEntry;
        fileEntry.filename      = std::string(logicalFilename) + "." + fixedWidth((int)physicalFiles.size()+1,3);
        fileEntry.file          = fopen(fileEntry.filename.c_str(), mode);
        absoluteOffset          = 0;
        currentChunkOffset      = 0;
		physicalFiles.clear();
        physicalFiles.push_back(fileEntry);
        currentChunk = physicalFiles.back();

		// Look for additional chunks
		while (1)
		{
			std::string filename = std::string(logicalFilename) + "." + fixedWidth((int)physicalFiles.size() + 1, 3);
			if (fileExist(filename))
			{
				FileEntry fileEntry;
				fileEntry.filename	= filename;
				fileEntry.file		= nullptr;
				physicalFiles.push_back(fileEntry);
			}
			else
			{
				break;
			}
		}
        return currentChunk.file != nullptr;
    }

    int close()
    {
        for (auto fileEntry : physicalFiles)
        {
            if(fileEntry.file != nullptr)
			{
				fclose(fileEntry.file);
				fileEntry.file = nullptr;
			}
        }
        physicalFiles.clear();
		currentChunk.filename	= std::string();
		currentChunk.file		= nullptr;
		currentChunkOffset		= 0;
        absoluteOffset			= 0;
        return 0;
    }

    uint64_t tell()
    {
        return absoluteOffset;
    }

    int seek(uint64_t offset, int origin)
    {
        if(physicalFiles.size() == 0) return 1;
        if(origin == SEEK_SET)
		{
			absoluteOffset = offset;
		}
        else
		if (origin == SEEK_CUR)
		{
			absoluteOffset += offset;
		}
        else
        if(origin == SEEK_END)
		{
			absoluteOffset = 0;
			uint32_t idx   = 0;
			for (uint32_t i = 0 ; i < physicalFiles.size() - 1 ; ++i)
				absoluteOffset += chunkSize;

			// GetSize of the last chunk
			FILE* lastChunk;
			if (!(lastChunk = fopen(physicalFiles.back().filename.c_str(), "rb")))
			{
				while (0); // error
			}
			fseek(lastChunk, 0, SEEK_END);
			uint64_t lastChunkSize = _ftelli64(lastChunk);
			fclose(lastChunk);
			absoluteOffset += lastChunkSize;			
		}
        currentChunkOffset  =			 absoluteOffset % chunkSize;
        uint32_t fileIdx    = (uint32_t)(absoluteOffset / chunkSize);
		fclose(currentChunk.file);
		currentChunk.file = nullptr;

		if (fileIdx > physicalFiles.size())
		{
			currentChunk.filename   = std::string();
			currentChunk.file		= nullptr;
		}
		else
		{
			physicalFiles[fileIdx].file = fopen(physicalFiles[fileIdx].filename.c_str(), mode.c_str());
			currentChunk = physicalFiles[fileIdx];
			_fseeki64(currentChunk.file, currentChunkOffset, SEEK_SET);
		}
        return 0;
    }

    size_t write(const void * ptr, size_t size, size_t count)
    {
        if(currentChunk.file == nullptr) return 0;
		uint64_t writeSize = size * count;

		uint64_t canWrite = chunkSize - currentChunkOffset;
        if(writeSize < canWrite)
        {
            currentChunkOffset  += writeSize;
            absoluteOffset      += writeSize;
            return fwrite(ptr, size, count, currentChunk.file);
        } 
        else
        {
            // Write what we can write in the current chunk
			uint64_t writtenA = fwrite(ptr, 1, (size_t)canWrite, currentChunk.file);

			uint32_t nextFileIdx = (uint32_t)(absoluteOffset / chunkSize) + 1;
            if(nextFileIdx < physicalFiles.size())
            {
				fclose(currentChunk.file);
				currentChunk.file = nullptr;
				physicalFiles[nextFileIdx].file = fopen(physicalFiles[nextFileIdx].filename.c_str(), mode.c_str());
				currentChunk = physicalFiles[nextFileIdx];
			}
            else
            {
				fclose(currentChunk.file);
				currentChunk.file = nullptr;

				// create new chunk
                FileEntry fileEntry;
                fileEntry.filename      = logicalFilename + "." + fixedWidth((int)physicalFiles.size()+1,3);
                fileEntry.file          = fopen(fileEntry.filename.c_str(), mode.c_str());
                currentChunk			= physicalFiles.back();
				currentChunkOffset		= 0;
				physicalFiles.push_back(fileEntry);
			}
			uint64_t writtenB = fwrite((char*)ptr+canWrite, 1, (size_t)(writeSize-canWrite), currentChunk.file);
			currentChunkOffset  += writtenB;
			absoluteOffset		+= writeSize;
			return size_t(writtenA + writtenB);
        }
        return 0;
    }

    size_t read( void * ptr, size_t size, size_t count)
    {
        if(currentChunk.file == nullptr) return 0;
		uint64_t readSize = size * count;

		uint64_t canRead = chunkSize - currentChunkOffset;
        if(readSize < canRead)
        {
            currentChunkOffset  += readSize;
            absoluteOffset      += readSize;
            return fread(ptr, size, count, currentChunk.file);
        }
        else
        {
            // Read what we can write in the current chunk
			uint64_t readA = fread(ptr, 1, (size_t)canRead, currentChunk.file);

			uint32_t nextFileIdx = (uint32_t)((absoluteOffset+readA) / chunkSize);
            if(nextFileIdx < physicalFiles.size())
            {
				fclose(currentChunk.file);
				currentChunk.file = nullptr;
				
				physicalFiles[nextFileIdx].file = fopen(physicalFiles[nextFileIdx].filename.c_str(), mode.c_str());
				currentChunk = physicalFiles[nextFileIdx];
				
				uint64_t readB = fread((char*)ptr+canRead, 1, (size_t)(readSize-canRead), currentChunk.file);
				
				currentChunkOffset	 = readB;
				absoluteOffset		+= readSize;
                return size_t(readA + readB);
            }
            // end of file
			currentChunkOffset	+= readA;
			absoluteOffset		+= readSize;
			return (size_t)readA;
        }
        return 0;
    }

    struct FileEntry
    {
        std::string filename;
        FILE*       file = nullptr;
    };
    std::vector<FileEntry>  physicalFiles;
    FileEntry               currentChunk;
	uint64_t                currentChunkOffset;
	uint64_t                absoluteOffset;
    std::string             logicalFilename;
    std::string             mode;
};




#define READSIZE 0x400000 // 4 MB buffer.
uint8_t buffer[READSIZE];


// TEST1: virtual file creation: success
// FWRITE
bool unitTest1()
{
	uint64_t fileOff = 0;
	size_t readSize  = READSIZE; 
	FILE* infile;
	if (!(infile = fopen("Input.nsp", "rb")))
	{
		while (0); // error
	}
	fseek(infile, 0, SEEK_END);
	uint64_t inFileSize = _ftelli64(infile);
	fseek(infile, 0, SEEK_SET);

	virtualFile outFile;
	outFile.open("C:/devkitPro/msys2/code/n1dus/testSplit/TestSplit/UNIT_TEST1_VirtualFile.nsp", "wb");

	while (fileOff < inFileSize)
	{
		if (fileOff + readSize >= inFileSize)
			readSize = (size_t)(inFileSize - fileOff);

		fread(buffer, readSize, 1, infile);
		outFile.write(buffer, readSize, 1);
		fileOff += readSize;
	}
	fclose(infile);
	outFile.close();
	return true;
}

// TEST2: virtual file read: success
// FREAD
bool unitTest2()
{
	uint64_t fileOff = 0;
	size_t readSize = READSIZE;
	FILE* outfile;
	if (!(outfile = fopen("UNIT_TEST2_VirtualFile_rebuilt.nsp", "wb")))
	{
		while (0); // error
	}

	virtualFile inFile;
	inFile.open("C:/devkitPro/msys2/code/n1dus/testSplit/TestSplit/UNIT_TEST1_VirtualFile.nsp", "rb");
	inFile.seek(0, SEEK_END);
	uint64_t inFileSize = inFile.tell();
	inFile.seek(0, SEEK_SET);

	while (fileOff < inFileSize)
	{
		if (fileOff + readSize >= inFileSize)
			readSize = (size_t)(inFileSize - fileOff);

		inFile.read(buffer, readSize, 1);
		fwrite(buffer, readSize, 1, outfile);
		fileOff += readSize;
	}
	inFile.close();
	fclose(outfile);
	return true;
}

// TEST3: virtual file seek/write
// FSEEK FWRITE
bool unitTest3()
{
	size_t readSize = READSIZE;

	// output files
	FILE* outfileVS;
	if (!(outfileVS = fopen("UNIT_TEST3_VirtualFile_SEEK.nsp", "wb")))
	{
		while (0); // error
	}
	FILE* outfileS;
	if (!(outfileS = fopen("UNIT_TEST3_File_SEEK.nsp", "wb")))
	{
		while (0); // error
	}

	// Input files
	FILE* infile;
	if (!(infile = fopen("Input.nsp", "rb")))
	{
		while (0); // error
	}

	virtualFile inFileVirtual;
	inFileVirtual.open("C:/devkitPro/msys2/code/n1dus/testSplit/TestSplit/UNIT_TEST1_VirtualFile.nsp", "rb");
	inFileVirtual.seek(0, SEEK_END);
	uint64_t inFileSizeVirtual = inFileVirtual.tell();
	inFileVirtual.seek(0, SEEK_SET);

	fseek(infile, 0, SEEK_END);
	uint64_t inFileSize = _ftelli64(infile);
	fseek(infile, 0, SEEK_SET);

	assert(inFileSizeVirtual == inFileSize);

	const int SEEKCOUNT = 100;
	for (int i = 0; i < SEEKCOUNT; ++i)
	{
		uint64_t seek = randomFrom((uint64_t)0, inFileSize);

		inFileVirtual.seek(seek, SEEK_SET);
		inFileVirtual.read(buffer, readSize, 1);
		fwrite(buffer, readSize, 1, outfileVS);


		_fseeki64(infile, seek, SEEK_SET);
		fread(buffer, readSize, 1, infile);
		fwrite(buffer, readSize, 1, outfileS);
	}
	inFileVirtual.close();
	fclose(infile);
	fclose(outfileVS);
	fclose(outfileS);
	return true;
}

int main(int argc, char **argv)
{
//	unitTest1(); // write
//	unitTest2(); // read
	unitTest3(); // seek
	return 0;
}