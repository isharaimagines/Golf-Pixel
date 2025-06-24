#include "TextureManager.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <stdlib.h>

// Window dimensions
const int SCREEN_WIDTH = 960;
const int SCREEN_HEIGHT = 540;
const int BALL_SIZE = 16;
const int HOLE_SIZE = 16;
const float BALL_SPEED = 5.0f;
const float FRICTION = 0.9f;

int bounceCount = 0.0;

// Game states
enum GameState { START_SCREEN, GAME_RUNNING, GAME_COMPLETED, GAME_EXIT };

// Function prototypes
bool init();
void close();
void handleEvents(SDL_Event &e, bool &quit, bool &moveBall, SDL_Rect &ballRect,
                  float &ballVelX, float &ballVelY, bool &mousePressed,
                  Uint32 &pressStartTime, bool &showArrow, SDL_Rect &arrowRect,
                  float &arrowAngle, GameState &currentState);
void updateBallPosition(SDL_Rect &ballRect, float &ballVelX, float &ballVelY,
                        bool moveBall, const SDL_Rect objects[],
                        size_t numObjects, GameState &currentState,
                        const SDL_Rect &holeRect);
void render(SDL_Texture *backgroundTexture, SDL_Texture *ballTexture,
            SDL_Texture *arrowTexture, SDL_Texture *objectTextures[],
            SDL_Texture *holeTexture, const SDL_Rect &ballRect,
            const SDL_Rect &arrowRect, const SDL_Rect objects[],
            const SDL_Rect &holeRect, bool &showArrow, float arrowAngle,
            GameState &currentState, size_t numObjectTextures);
void renderStartScreen(SDL_Texture *startScreenTexture);

SDL_Window *gWindow = nullptr;
SDL_Renderer *gRenderer = nullptr;
Mix_Chunk *clickSound = nullptr;
Mix_Chunk *holeSound = nullptr;

bool init() {
  // Initialize SDL, SDL_image, and SDL_ttf
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0 ||
      !IMG_Init(IMG_INIT_PNG) || TTF_Init() == -1) {
    std::cerr << "Initialization error: " << SDL_GetError() << " | "
              << IMG_GetError() << " | " << TTF_GetError() << std::endl;
    return false;
  }

  // Initialize SDL2_mixer
  if (Mix_Init(MIX_INIT_MP3) == 0) {
    std::cerr << "SDL_mixer initialization error: " << Mix_GetError()
              << std::endl;
    return false;
  }

  if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096) == -1) {
    std::cerr << "SDL_mixer open audio error: " << Mix_GetError() << std::endl;
    return false;
  }

  // Create window and renderer
  gWindow = SDL_CreateWindow("Golf Pixel", SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                             SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);

  if (!gWindow || !gRenderer) {
    std::cerr << "Window/Renderer creation error: " << SDL_GetError()
              << std::endl;
    return false;
  }

  // Load textures and fonts
  if (!TextureManager::loadTextures(gRenderer) ||
      !TextureManager::loadFonts()) {
    return false;
  }

  // Load sound effects
  clickSound = Mix_LoadWAV("res/ball_hit.mp3");
  if (!clickSound) {
    std::cerr << "Failed to load click sound: " << Mix_GetError() << std::endl;
    return false;
  }
  holeSound = Mix_LoadWAV("res/hole_0.mp3");
  if (!holeSound) {
    std::cerr << "Failed to load click sound: " << Mix_GetError() << std::endl;
    return false;
  }
  return true;
}

void close() {
  Mix_FreeChunk(clickSound);
  clickSound = nullptr;
  Mix_FreeChunk(holeSound);
  holeSound = nullptr;
  Mix_Quit();
  TextureManager::freeTextures();
  TextureManager::freeFonts();
  SDL_DestroyRenderer(gRenderer);
  SDL_DestroyWindow(gWindow);
  TTF_Quit();
  IMG_Quit();
  SDL_Quit();
  exit(0);
}

void randomizeObjectPositions(SDL_Rect objects[], size_t numObjects,
                              SDL_Rect &ballRect, SDL_Rect &holeRect) {
  const int minD = 100; // Minimum distance between screen boundaries
  int BOUNDARY_WIDTH = 700;
  int BOUNDARY_HEIGHT = 300;

  srand(time(0));
  // Helper function to check if two rectangles overlap
  auto checkOverlap = [](const SDL_Rect &rect1, const SDL_Rect &rect2) {
    return !(rect1.x + rect1.w <= rect2.x || rect1.y + rect1.h <= rect2.y ||
             rect1.x >= rect2.x + rect2.w || rect1.y >= rect2.y + rect2.h);
  };

  // Randomize positions for objects
  for (size_t i = 0; i < numObjects; ++i) {
    bool positionFound = false;

    while (!positionFound) {
      // Random x and y positions within the screen boundaries
      objects[i].x = minD + rand() % (BOUNDARY_WIDTH);
      objects[i].y = minD + rand() % (BOUNDARY_HEIGHT);

      // Check for overlap with other objects
      bool overlap = false;
      for (size_t j = 0; j < numObjects; ++j) {
        if (i != j && checkOverlap(objects[i], objects[j])) {
          overlap = true;
          break;
        }
      }

      // Ensure the position is not overlapping with any other objects
      if (!overlap) {
        positionFound = true;
      }
    }
  }

  // Randomize position for the ball
  bool positionFound = false;
  while (!positionFound) {
    ballRect.x = minD + rand() % (BOUNDARY_WIDTH);
    ballRect.y = minD + rand() % (BOUNDARY_HEIGHT);

    // Check for overlap with other objects
    bool overlap = false;
    for (size_t j = 0; j < numObjects; ++j) {
      if (checkOverlap(ballRect, objects[j])) {
        overlap = true;
        break;
      }
    }

    // Ensure the position is not overlapping with any other objects
    if (!overlap) {
      positionFound = true;
    }
  }

  // Randomize position for the hole
  positionFound = false;
  while (!positionFound) {
    holeRect.x = minD + rand() % (BOUNDARY_WIDTH);
    holeRect.y = minD + rand() % (BOUNDARY_HEIGHT);

    // Check for overlap with other objects and ball
    bool overlap = false;
    for (size_t j = 0; j < numObjects; ++j) {
      if (checkOverlap(holeRect, objects[j])) {
        overlap = true;
        break;
      }
    }
    if (checkOverlap(holeRect, ballRect)) {
      overlap = true;
    }

    // Ensure the position is not overlapping with any other objects or the ball
    if (!overlap) {
      positionFound = true;
    }
  }
}

void handleEvents(SDL_Event &e, bool &quit, bool &moveBall, SDL_Rect &ballRect,
                  float &ballVelX, float &ballVelY, bool &mousePressed,
                  Uint32 &pressStartTime, bool &showArrow, SDL_Rect &arrowRect,
                  float &arrowAngle, GameState &currentState,
                  SDL_Rect objects[], size_t numObjects, SDL_Rect &holeRect) {
  const Uint32 maxPressDuration = 400;

  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_QUIT) {
      quit = true;
    } else if (currentState == START_SCREEN && e.type == SDL_KEYDOWN) {
      currentState = GAME_RUNNING;
    } else if (currentState == GAME_RUNNING) {
      if (e.type == SDL_MOUSEBUTTONDOWN && ballVelX == 0 && ballVelY == 0) {
        moveBall = true;
        mousePressed = true;
        pressStartTime = SDL_GetTicks();

        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        float directionX = mouseX - (ballRect.x + ballRect.w / 2);
        float directionY = mouseY - (ballRect.y + ballRect.h / 2);
        float length = sqrt(directionX * directionX + directionY * directionY);

        if (length) {
          directionX /= length;
          directionY /= length;
        }

        arrowAngle = atan2f(directionY, directionX) * 180.0f / M_PI - 90;
        float arrowDistance = BALL_SIZE * 1.7f;
        arrowRect = {static_cast<int>(ballRect.x + (ballRect.w / 2) -
                                      (arrowDistance * directionX) - 25),
                     static_cast<int>(ballRect.y + (ballRect.h / 2) -
                                      (arrowDistance * directionY) - 25),
                     50, 50};

        showArrow = true;
      } else if (e.type == SDL_MOUSEBUTTONUP && ballVelX == 0 &&
                 ballVelY == 0) {
        moveBall = false;
        mousePressed = false;

        Uint32 pressDuration = SDL_GetTicks() - pressStartTime;
        if (pressDuration > maxPressDuration) {
          pressDuration = maxPressDuration;
        }

        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        float directionX = mouseX - (ballRect.x + ballRect.w / 2);
        float directionY = mouseY - (ballRect.y + ballRect.h / 2);
        float length = sqrt(directionX * directionX + directionY * directionY);

        // Play the click sound effect
        Mix_PlayChannel(-1, clickSound, 0);

        if (length) {
          directionX /= length;
          directionY /= length;
        }

        ballVelX = -directionX * (pressDuration / 10.0f);
        ballVelY = -directionY * (pressDuration / 10.0f);

        showArrow = false;
      }
    } else if (currentState == GAME_COMPLETED &&
               e.key.keysym.sym == SDLK_RETURN) {

      // Reset the ball position and velocity
      ballRect = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, BALL_SIZE, BALL_SIZE};
      ballVelX = ballVelY = 0;

      // Randomize object positions including ball and hole
      randomizeObjectPositions(objects, numObjects, ballRect, holeRect);

      // Reset bounce count
      bounceCount = 0.0;

      // Next level
      currentState = GameState::GAME_RUNNING;
    } else if (e.key.keysym.sym == SDLK_ESCAPE) {
      close();
    }
  }
}

void reflectBallOffObject(SDL_Rect &ballRect, float &ballVelX, float &ballVelY,
                          const SDL_Rect &objectRect) {
  int deltaX =
      (ballRect.x + ballRect.w / 2) - (objectRect.x + objectRect.w / 2);
  int deltaY =
      (ballRect.y + ballRect.h / 2) - (objectRect.y + objectRect.h / 2);

  if (std::abs(deltaX) > std::abs(deltaY)) {
    if (deltaX > 0) {
      // Ball is on the right side of the object
      ballRect.x =
          objectRect.x + objectRect.w; // Move ball to the right of the object
    } else {
      // Ball is on the left side of the object
      ballRect.x =
          objectRect.x - ballRect.w; // Move ball to the left of the object
    }
    ballVelX = -ballVelX;
  } else {
    if (deltaY > 0) {
      // Ball is below the object
      ballRect.y = objectRect.y + objectRect.h; // Move ball below the object
    } else {
      // Ball is above the object
      ballRect.y = objectRect.y - ballRect.h; // Move ball above the object
    }
    ballVelY = -ballVelY;
  }
}

bool checkCollision(SDL_Rect &ballRect, const SDL_Rect &objectRect) {
  return SDL_HasIntersection(&ballRect, &objectRect);
}
void updateBallPosition(SDL_Rect &ballRect, float &ballVelX, float &ballVelY,
                        bool moveBall, const SDL_Rect objects[],
                        size_t numObjects, GameState &currentState,
                        const SDL_Rect &holeRect, bool &ballInHole,
                        float &animationProgress) {

  if (ballInHole) {
    // Animate the ball falling into the hole
    animationProgress += 0.05f; // Adjust speed of animation
    if (ballRect.w >= 12) {
      // Ball falls and shrinks
      ballRect.w = static_cast<float>(
          BALL_SIZE * (0.8f - animationProgress)); // Shrink width
      ballRect.h = static_cast<float>(
          BALL_SIZE * (0.8f - animationProgress)); // Shrink height
    } else {
      SDL_Delay(1000);
      currentState = GameState::GAME_COMPLETED;
      ballInHole = false;
    }

    return;
  }

  if (!moveBall) {
    ballVelX *= FRICTION;
    ballVelY *= FRICTION;
    if (std::fabs(ballVelX) < 0.1f)
      ballVelX = 0;
    if (std::fabs(ballVelY) < 0.1f)
      ballVelY = 0;
  }

  ballRect.x += static_cast<int>(ballVelX);
  ballRect.y += static_cast<int>(ballVelY);

  if (ballRect.x < 0 || ballRect.x > SCREEN_WIDTH - BALL_SIZE ||
      ballRect.y < 0 || ballRect.y > SCREEN_HEIGHT - BALL_SIZE) {
    ballRect = {3 * SCREEN_WIDTH / 4, 3 * SCREEN_HEIGHT / 4, BALL_SIZE,
                BALL_SIZE};
    ballVelX = ballVelY = 0;
  }

  float newX = ballRect.x + ballVelX;
  float newY = ballRect.y + ballVelY;

  SDL_Rect futureBallRect = {static_cast<int>(newX), static_cast<int>(newY),
                             ballRect.w, ballRect.h};

  // Check if the ball falls into the hole
  if (checkCollision(futureBallRect, holeRect)) {
    ballRect.x = holeRect.x + holeRect.w / 4;
    ballRect.y = holeRect.y + holeRect.h / 4;

    ballInHole = true;
    ballVelX = ballVelY = 0;
    Mix_PlayChannel(-1, holeSound, 0);
    animationProgress = 0.0f; // Reset animation progress
    return;                   // Exit the function to start animation
  }

  // Use a standard for loop to iterate over the objects array
  for (size_t i = 0; i < numObjects; ++i) {
    if (checkCollision(futureBallRect, objects[i])) {
      reflectBallOffObject(ballRect, ballVelX, ballVelY, objects[i]);
      bounceCount++; // Increment bounce count on collision
      return;        // Stop further checks once a collision is handled
    }
  }

  // Update ball position if no collision
  ballRect.x = newX;
  ballRect.y = newY;
}

void render(SDL_Texture *startScreenTexture, SDL_Texture *backgroundTexture,
            SDL_Texture *ballTexture, SDL_Texture *arrowTexture,
            SDL_Texture *objectTextures[], SDL_Texture *holeTexture,
            const SDL_Rect &ballRect, const SDL_Rect &arrowRect,
            const SDL_Rect objects[], const SDL_Rect &holeRect, bool showArrow,
            float arrowAngle, GameState currentState, size_t numObjectTextures,
            SDL_Texture *flashScreenTexture) {
  SDL_RenderClear(gRenderer);

  if (currentState == START_SCREEN) {
    if (startScreenTexture) {
      SDL_RenderCopy(gRenderer, startScreenTexture, nullptr, nullptr);
    } else {
      std::cerr << "Failed to load start screen texture." << std::endl;
    }
  } else if (currentState == GAME_RUNNING) {
    if (backgroundTexture) {
      SDL_RenderCopy(gRenderer, backgroundTexture, nullptr, nullptr);
    }
    for (size_t i = 0; i < numObjectTextures; ++i) {
      if (objectTextures[i]) {
        SDL_RenderCopy(gRenderer, objectTextures[i], nullptr, &objects[i]);
      }
    }
    if (holeTexture) {
      SDL_RenderCopy(gRenderer, holeTexture, nullptr, &holeRect);
    }
    if (ballTexture) {
      SDL_RenderCopy(gRenderer, ballTexture, nullptr, &ballRect);
    }
    if (showArrow && arrowTexture) {
      SDL_RenderCopyEx(gRenderer, arrowTexture, nullptr, &arrowRect, arrowAngle,
                       nullptr, SDL_FLIP_NONE);
    }

    // Render the bounce count
    TTF_Font *font = TextureManager::getFont("font"); // Ensure 'font' is loaded
    if (font) {
      SDL_Color textColor = {255, 255, 255, 255}; // White color
      std::string bounceText = "Bounce #" + std::to_string(bounceCount);

      // Create surface from text
      SDL_Surface *textSurface =
          TTF_RenderText_Solid(font, bounceText.c_str(), textColor);
      if (textSurface) {
        // Create texture from surface
        SDL_Texture *textTexture =
            SDL_CreateTextureFromSurface(gRenderer, textSurface);
        SDL_FreeSurface(textSurface); // Free the surface

        // Get text texture dimensions
        int textWidth = 0, textHeight = 0;
        SDL_QueryTexture(textTexture, nullptr, nullptr, &textWidth,
                         &textHeight);

        // Set text position (top-left corner)
        SDL_Rect textRect = {40, 30, textWidth, textHeight};

        // Render the text
        SDL_RenderCopy(gRenderer, textTexture, nullptr, &textRect);

        // Clean up
        SDL_DestroyTexture(textTexture);
      } else {
        std::cerr << "Unable to render text surface! SDL_ttf Error: "
                  << TTF_GetError() << std::endl;
      }
    } else {
      std::cerr << "Font not loaded." << std::endl;
    }
  } else if (currentState == GAME_COMPLETED) {
    if (flashScreenTexture) {
      SDL_RenderCopy(gRenderer, flashScreenTexture, nullptr, nullptr);
    } else {
      std::cerr << "Failed to load flash screen texture." << std::endl;
    }
  }

  SDL_RenderPresent(gRenderer);
}

void renderStartScreen(SDL_Texture *startScreenTexture) {
  SDL_RenderCopy(gRenderer, startScreenTexture, nullptr, nullptr);
}

int main(int argc, char *args[]) {
  if (!init())
    return 1;

  SDL_Rect ballRect = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, BALL_SIZE,
                       BALL_SIZE};
  float ballVelX = 0, ballVelY = 0;
  bool quit = false, moveBall = false, mousePressed = false, showArrow = false;
  Uint32 pressStartTime = 0;
  SDL_Event e;
  SDL_Rect arrowRect = {0, 0, 50, 50};
  float arrowAngle = 0.0f;
  SDL_Rect objects[12] = {
      {300, 200, 70, 40}, {350, 100, 80, 45}, {600, 450, 100, 60},
      {600, 200, 50, 50}, {250, 250, 55, 55}, {300, 400, 50, 50},
      {800, 310, 95, 95}, {750, 100, 95, 95}, {480, 410, 50, 50},
      {150, 180, 80, 45}, {480, 160, 40, 40}, {100, 400, 100, 100},
  };
  SDL_Rect holeRect = {90, 280, HOLE_SIZE, HOLE_SIZE};
  bool ballInHole = false; // Declare a bool variable
  float animationProgress = 0.0f;

  SDL_Texture *objectTextures[12] = {TextureManager::getTexture("object1"),
                                     TextureManager::getTexture("object2"),
                                     TextureManager::getTexture("object3"),
                                     TextureManager::getTexture("object4"),
                                     TextureManager::getTexture("object5"),
                                     TextureManager::getTexture("object6"),
                                     TextureManager::getTexture("object7"),
                                     TextureManager::getTexture("object8"),
                                     TextureManager::getTexture("object9"),
                                     TextureManager::getTexture("object10"),
                                     TextureManager::getTexture("object11"),
                                     TextureManager::getTexture("object12")};

  GameState currentState = START_SCREEN;
  srand(static_cast<unsigned int>(time(nullptr)));

  while (!quit) {
    handleEvents(e, quit, moveBall, ballRect, ballVelX, ballVelY, mousePressed,
                 pressStartTime, showArrow, arrowRect, arrowAngle, currentState,
                 objects, std::size(objects), holeRect);

    if (currentState == GAME_RUNNING) {
      updateBallPosition(ballRect, ballVelX, ballVelY, moveBall, objects,
                         std::size(objects), currentState, holeRect, ballInHole,
                         animationProgress);
    }
    render(TextureManager::getTexture("startScreen"),
           TextureManager::getTexture("background"),
           TextureManager::getTexture("ball"),
           TextureManager::getTexture("arrow"), objectTextures,
           TextureManager::getTexture("hole"), ballRect, arrowRect, objects,
           holeRect, showArrow, arrowAngle, currentState,
           std::size(objectTextures), TextureManager::getTexture("comScreen"));

    SDL_Delay(16); // Approximately 60 FPS
  }

  close();
  return 0;
}
