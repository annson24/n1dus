#include <stdio.h>
#include <string.h>

extern FILE*    __real_fopen   (const char *fn, const char *mode);
extern int      __real_fclose  (FILE* f);
extern size_t   __real_fread   (void * ptr, size_t size, size_t count, FILE * stream );
extern size_t   __real_fwrite  (const void * ptr, size_t size, size_t count, FILE * stream );
extern int 	    __real_fseek   (FILE *stream, long int offset, int origin );
extern int 		__real_fseeko  (FILE *stream, off_t offset, int whence);
extern int      __real_fseeko64(FILE *stream, off64_t offset, int whence);
extern long int __real_ftell   (FILE *stream );
extern off_t 	__real_ftello  (FILE *stream);
extern off64_t  __real_ftello64(FILE *stream);

FILE* __wrap_fopen (const char *fn, const char *mode)
{
    FILE* f = __real_fopen(fn, mode);
    printf("HOOK: fopen(\"%s\", \"%s\") == %p\n", fn, mode, f);
    return f;
}

int __wrap_fclose (FILE* f)
{
    printf("HOOK: fclose %p\n", f);
    return __real_fclose(f);
}

size_t __wrap_fread  ( void * ptr, size_t size, size_t count, FILE * stream )
{
    printf("HOOK: fread %p\n", stream);
	return __real_fread(ptr, size, count, stream);
}

size_t __wrap_fwrite ( const void * ptr, size_t size, size_t count, FILE * stream )
{
    printf("HOOK: fwrite %p\n", stream);
	return __real_fwrite(ptr, size, count, stream);
}

int __wrap_fseek  ( FILE * stream, long int offset, int origin )
{
    printf("HOOK: fseek %p\n", stream);
	return __real_fseek(stream, offset, origin);
}

int __wrap_fseeko (FILE *stream, off_t offset, int whence)
{
    printf("HOOK: fseeko %p\n", stream);
	return __real_fseeko(stream, offset, whence);
}

int __wrap_fseeko64(FILE *stream, off64_t offset, int whence)
{
    printf("HOOK: fseeko64 %p\n", stream);
	return 1;//__real_fseeko64(stream, offset, whence);
}

long int __wrap_ftell  ( FILE * stream )
{
    printf("HOOK: ftell %p\n", stream);
	return __real_ftell(stream);
}

off_t __wrap_ftello(FILE *stream)
{
    printf("HOOK: ftello %p\n", stream);
	return __real_ftello(stream);
}

off64_t __wrap_ftello64(FILE *stream)
{
    printf("HOOK: ftello64 %p\n", stream);
	return 1;//__real_ftello64(stream);
}
