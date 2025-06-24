#ifndef TEXTUREMANAGER_H
#define TEXTUREMANAGER_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <map>
#include <string>

class TextureManager {
public:
  static bool loadTextures(SDL_Renderer *renderer);
  static bool loadFonts();
  static SDL_Texture *getTexture(const std::string &name);
  static TTF_Font *getFont(const std::string &name);
  static void freeTextures();
  static void freeFonts();
  static SDL_Texture *loadTexture(const std::string &path);

private:
  static SDL_Renderer *gRenderer;
  static std::map<std::string, SDL_Texture *> textures;
  static std::map<std::string, TTF_Font *> fonts;
};

#endif
