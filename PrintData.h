
#pragma once

class CPrintData
{
public:

   static constexpr int BYTES_PER_LINE = 16;
   static constexpr int CHARS_PER_LINE = 80;

   CPrintData() {}
   ~CPrintData() = default;

   static const char* GetDataAsString(char* Data, int Length, const char* NewLine = nullptr, unsigned long Address = 0);
   static const char* GetTimeAsString();

private:

   static char* mDataString;
   static int   mDataLength;

};
