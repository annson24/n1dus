#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>

#define READSIZE 0x4000000 // 4 MB buffer.
uint8_t buffer[READSIZE];

int main()
{
	printf("Size of ptr: %d\n", (int)sizeof(void*));

	uint64_t fileOff = 0;
	size_t readSize  = READSIZE; 

	FILE* infile;
	if (!(infile = fopen("test.bin", "rb")))
	{
		printf("Error opening file\n");
	}
	fseeko(infile, 0, SEEK_END);
	uint64_t inFileSize = ftello(infile);
	fseeko(infile, 0, SEEK_SET);

	printf("Filesize %" PRIu64 "\n", inFileSize);
		
	while (fileOff < inFileSize)
	{
		if (fileOff + readSize >= inFileSize)
			readSize = (size_t)(inFileSize - fileOff);

		fread(buffer, readSize, 1, infile);
		fileOff += readSize;
	}
	fclose(infile);

    return 0;
}

