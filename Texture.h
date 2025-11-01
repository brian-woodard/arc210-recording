#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

class CTexture
{
public:

   // Hash map to hold textures
   static std::unordered_map<std::string, std::shared_ptr<CTexture>> TextureMap;
   static std::vector<std::string> AvailableTextures;

   CTexture(const char* Filename);
   ~CTexture();

   //! \fn const std::string& GetTextureFilename()
   //! \details Returns the texture filename
   const std::string& GetTextureFilename() const { return mFilename; };

   //! \fn unsigned int GetTexture()
   //! \details Returns the texture identifier
   unsigned int GetTexture() const { return mTextureId; };

private:

   std::string  mFilename;
   unsigned int mTextureId;
   int          mWidth;
   int          mHeight;
   int          mChannels;
};

std::shared_ptr<CTexture> GetOrCreateTexture(const char* Filename);
