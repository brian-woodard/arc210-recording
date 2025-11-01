
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "FileUtils.h"

TBuffer ReadEntireFile(const char* Filename)
{
   TBuffer Result = {};

   FILE* File = fopen(Filename, "rb");

   if (File)
   {
      struct stat Stat;
      stat(Filename, &Stat);

      Result.Size = Stat.st_size;
      Result.Data = new u8[Stat.st_size];

      if (Result.Data)
      {
         if (fread(Result.Data, Result.Size, 1, File) != 1)
         {
            delete [] Result.Data;
            Result.Size = 0;
            Result.Data = nullptr;
         }
      }

      fclose(File);
   }

   return Result;
}
