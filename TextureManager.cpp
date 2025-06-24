#include "TextureManager.h"
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <iostream>
#include <map>

// Static members
SDL_Renderer *TextureManager::gRenderer = nullptr;
std::map<std::string, SDL_Texture *> TextureManager::textures;
std::map<std::string, TTF_Font *> TextureManager::fonts;

bool TextureManager::loadTextures(SDL_Renderer *renderer) {
  gRenderer = renderer;

  // List of textures to load
  std::map<std::string, std::string> texturePaths = {
      {"background", "res/background3.png"},
      {"ball", "res/ball.png"},
      {"arrow", "res/arrow.png"},
      {"startScreen", "res/flash_screen.png"},
      {"object1", "res/tile100x150_light.png"},
      {"object2", "res/tile100x150_light.png"},
      {"object3", "res/tile100x150_light.png"},
      {"object4", "res/tile100_light.png"},
      {"object5", "res/tile100_light.png"},
      {"object6", "res/tile100_light.png"},
      {"object7", "res/tile100_light.png"},
      {"object8", "res/tile100_light.png"},
      {"object9", "res/tile100_light.png"},
      {"object10", "res/tile100x150_light.png"},
      {"object11", "res/tile100_light.png"},
      {"object12", "res/tile100_light.png"},
      {"hole", "res/hole.png"},
      {"comScreen", "res/com_background.png"}};

  // Load all textures
  for (const auto &pair : texturePaths) {
    textures[pair.first] = loadTexture(pair.second);
    if (!textures[pair.first]) {
      std::cerr << "Failed to load texture: " << pair.first << std::endl;
      return false;
    }
  }

  return true;
}

void TextureManager::freeTextures() {
  for (auto &pair : textures) {
    SDL_DestroyTexture(pair.second);
    pair.second = nullptr;
  }
  textures.clear();
}
void TextureManager::freeFonts() {
  for (auto &pair : fonts) {
    TTF_CloseFont(pair.second);
    pair.second = nullptr;
  }
  fonts.clear();
}

SDL_Texture *TextureManager::getTexture(const std::string &name) {
  if (textures.find(name) != textures.end()) {
    return textures[name];
  }
  std::cerr << "Texture not found: " << name << std::endl;
  return nullptr;
}

SDL_Texture *TextureManager::loadTexture(const std::string &path) {
  SDL_Surface *loadedSurface = IMG_Load(path.c_str());
  if (!loadedSurface) {
    std::cerr << "Unable to load image " << path
              << "! SDL_image Error: " << IMG_GetError() << std::endl;
    return nullptr;
  }

  SDL_Texture *newTexture =
      SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
  SDL_FreeSurface(loadedSurface);

  if (!newTexture) {
    std::cerr << "Unable to create texture from " << path
              << "! SDL Error: " << SDL_GetError() << std::endl;
  }

  return newTexture;
}

TTF_Font *TextureManager::getFont(const std::string &name) {
  if (fonts.find(name) != fonts.end()) {
    return fonts[name];
  }
  std::cerr << "Font not found: " << name << std::endl;
  return nullptr;
}

bool TextureManager::loadFonts() {
  // Adjust the path as needed
  fonts["font"] = TTF_OpenFont("res/font.ttf", 30);
  if (!fonts["font"]) {
    std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
    return false;
  }
  return true;
}
