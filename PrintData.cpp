
#include "PrintData.h"
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <ctype.h>

char* CPrintData::mDataString = nullptr;
int   CPrintData::mDataLength = 0;

const char* CPrintData::GetDataAsString(char* Data, int Length, const char* NewLine, unsigned long Address)
{
   const char* newline = "\n";

   if (NewLine)
      newline = NewLine;

   // format data received
   if (Length > 0)
   {
      char element[BYTES_PER_LINE+3] = { 0 };

      // allocate string long enough to hold data
      int length_required = ((Length + 8) / 8) * CHARS_PER_LINE;

      if (!mDataString)
      {
         mDataString = new char[length_required];
         mDataLength = length_required;
      }
      else if (mDataString && length_required > mDataLength)
      {
         delete [] mDataString;
         mDataString = new char[length_required];
         mDataLength = length_required;
      }

      memset(mDataString, 0, mDataLength);

      for (int i = 0; i < Length; i++)
      {
         if (i % BYTES_PER_LINE == 0)
         {
            if (i != 0)
            {
               // print data as string
               strcat(mDataString, "   |");
               memcpy(element, &Data[i-BYTES_PER_LINE], BYTES_PER_LINE);
               for (int j = 0; j < BYTES_PER_LINE; j++)
               {
                  // replace non-printable characters with '.'
                  if ((element[j] >= 0 && element[j] <= 31) || element[j] >= 127 || element[j] < 0)
                     element[j] = '.';
               }
               strcat(mDataString, element);
               strcat(mDataString, "|");
               strcat(mDataString, newline);
               memset(element, 0, sizeof(element));
            }
            sprintf(element, "%08x:", Address + i);
            strcat(mDataString, element);
            memset(element, 0, sizeof(element));
         }

         if (i % (BYTES_PER_LINE / 2) == 0)
         {
            // add a space after halfway through the bytes on a line
            strcat(mDataString, " ");
         }
         // add a space every byte
         strcat(mDataString, " ");

         // print data
         sprintf(element, "%02x", (unsigned char)Data[i]);
         strcat(mDataString, element);
         memset(element, 0, sizeof(element));
      }

      if (Length % BYTES_PER_LINE != 0)
      {
         // pad with spaces and print data as string
         for (int j = 0; j < ((BYTES_PER_LINE - (Length % BYTES_PER_LINE)) * 3); j++)
            strcat(mDataString, " ");
         
         if (Length % BYTES_PER_LINE <= 8)
            strcat(mDataString, " ");

         strcat(mDataString, "   |");
         memcpy(element, &Data[Length-(Length % BYTES_PER_LINE)], Length % BYTES_PER_LINE);
         for (int j = 0; j < BYTES_PER_LINE; j++)
         {
            // replace non-printable characters with '.'
            if ((element[j] >= 0 && element[j] <= 31) || element[j] >= 127 || element[j] < 0)
               element[j] = '.';
         }
         strcat(mDataString, element);
         strcat(mDataString, "|");
         memset(element, 0, sizeof(element));
      }
      else
      {
         // print data as string
         strcat(mDataString, "   |");
         memcpy(element, &Data[Length-BYTES_PER_LINE], BYTES_PER_LINE);
         for (int j = 0; j < BYTES_PER_LINE; j++)
         {
            // replace non-printable characters with '.'
            if ((element[j] >= 0 && element[j] <= 31) || element[j] >= 127 || element[j] < 0)
               element[j] = '.';
         }
         strcat(mDataString, element);
         strcat(mDataString, "|");
         memset(element, 0, sizeof(element));
      }
   }

   return mDataString;
}

const char* CPrintData::GetTimeAsString()
{
   struct timeval tv;
   struct tm*     tm;

   if (!mDataString)
   {
      mDataString = new char[20];
      mDataLength = 20;
   }
   else if (mDataString && 20 > mDataLength)
   {
      delete [] mDataString;
      mDataString = new char[20];
      mDataLength = 20;
   }

   gettimeofday(&tv, NULL);
   tm = localtime(&tv.tv_sec);

   sprintf(mDataString, "%02d:%02d:%02d.%06ld - ", tm->tm_hour, tm->tm_min, tm->tm_sec, tv.tv_usec);

   return mDataString;
}
