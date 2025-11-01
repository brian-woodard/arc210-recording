
#include <iostream>
#include <algorithm>
#include <glad/glad.h>
#include "Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

std::unordered_map<std::string, std::shared_ptr<CTexture>> CTexture::TextureMap;
std::vector<std::string> CTexture::AvailableTextures;

CTexture::CTexture(const char* Filename)
   : mFilename(Filename),
     mTextureId(0),
     mWidth(0),
     mHeight(0),
     mChannels(0)
{
   if (strlen(Filename) == 0)
      return;

   stbi_set_flip_vertically_on_load(1);
   unsigned char* data = stbi_load(mFilename.c_str(), &mWidth, &mHeight, &mChannels, 0);

   printf("Loaded texture %s (%d, %d) channels %d\n", mFilename.c_str(), mWidth, mHeight, mChannels);

   if (!data)
   {
      printf("Failed to load texture: %s\n", mFilename.c_str());
      return;
   }

   glGenTextures(1, &mTextureId);
   glBindTexture(GL_TEXTURE_2D, mTextureId);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   if (mChannels == 4)
   {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
      glGenerateMipmap(GL_TEXTURE_2D);
   }
   else if (mChannels == 3)
   {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mWidth, mHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
      glGenerateMipmap(GL_TEXTURE_2D);
   }
   else
   {
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Disable byte-alignment restriction
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, mWidth, mHeight, 0, GL_RED, GL_UNSIGNED_BYTE, data);
   }

   stbi_image_free(data);

   glBindTexture(GL_TEXTURE_2D, 0);
}

CTexture::~CTexture()
{
   if (mTextureId)
   {
      glDeleteTextures(1, &mTextureId);
   }
}

std::shared_ptr<CTexture> GetOrCreateTexture(const char* Filename)
{
   std::shared_ptr<CTexture> texture_ptr = nullptr;

   if (CTexture::TextureMap.find(Filename) != CTexture::TextureMap.end())
   {
      texture_ptr = CTexture::TextureMap[Filename];
   }
   else
   {
      // Try loading the texture
      texture_ptr = std::make_shared<CTexture>(Filename);
      if (texture_ptr->GetTexture())
      {
         CTexture::TextureMap[Filename] = texture_ptr;

         CTexture::AvailableTextures.push_back(Filename);

         // Sort alphabetically
         std::sort(CTexture::AvailableTextures.begin(), CTexture::AvailableTextures.end(),
            [](const std::string& a, const std::string& b)
            {
               return a < b;
            });
      }
      else
      {
         texture_ptr = nullptr;
      }
   }

   return texture_ptr;
}
