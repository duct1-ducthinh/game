#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <algorithm>

const int SCREEN_WIDTH = 400;
const int SCREEN_HEIGHT = 600;
const int LANE_WIDTH = 100;

struct EnemyCar {
    SDL_Rect rect;
    bool active = true;
};

struct Star {
    SDL_Rect rect;
    bool active = true;
};

enum GameState { MENU, PLAYING, GAME_OVER };

int SDL_main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);

    SDL_Window* window = SDL_CreateWindow("Car Dodging",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    srand(static_cast<unsigned int>(time(nullptr)));

    // Load resources
    TTF_Font* font = TTF_OpenFont("font.ttf", 24);
    Mix_Music* backgroundMusic = Mix_LoadMUS("engine.wav");
    Mix_Chunk* starSound = Mix_LoadWAV("star.wav");
    Mix_Chunk* crashSound = Mix_LoadWAV("crash.wav");
    SDL_Texture* playerTex = IMG_LoadTexture(renderer, "player.png");
    SDL_Texture* enemyTex = IMG_LoadTexture(renderer, "enemy.png");

    if (!font || !backgroundMusic || !starSound || !crashSound || !playerTex || !enemyTex ) {
        SDL_Log("Failed to load resources.");
        return 1;
    }

    Mix_VolumeMusic(MIX_MAX_VOLUME);
    Mix_PlayMusic(backgroundMusic, -1);

    GameState gameState = MENU;
    SDL_Event e;
    bool running = true;

    SDL_Rect player = { SCREEN_WIDTH / 2 - 25, SCREEN_HEIGHT - 100, 50, 80 };
    int playerSpeed = 10;

    std::vector<EnemyCar> enemies;
    std::vector<Star> stars;
    Uint32 lastSpawnTime = 0;
    Uint32 lastStarTime = 0;
    Uint32 gameStartTime = 0;

    int score = 0;
    int timePlayed = 0;
    int highScore = 0;

    int baseSpeed = 6;
    int enemySpeed = baseSpeed;
    int starSpeed = 4;

    while (running) {
        // --- Input ---
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                running = false;

            if (e.type == SDL_KEYDOWN) {
                if (gameState == MENU && e.key.keysym.sym == SDLK_RETURN) {
                    player.x = SCREEN_WIDTH / 2 - 25;
                    enemies.clear();
                    stars.clear();
                    lastSpawnTime = SDL_GetTicks();
                    lastStarTime = SDL_GetTicks();
                    gameStartTime = SDL_GetTicks();
                    score = 0;
                    timePlayed = 0;
                    gameState = PLAYING;
                }
                if (gameState == GAME_OVER && e.key.keysym.sym == SDLK_RETURN) {
                    gameState = MENU;
                }
            }
        }

        const Uint8* keys = SDL_GetKeyboardState(NULL);
        if (gameState == PLAYING) {
            if (keys[SDL_SCANCODE_LEFT] && player.x > 0)
                player.x -= playerSpeed;
            if (keys[SDL_SCANCODE_RIGHT] && player.x + player.w < SCREEN_WIDTH)
                player.x += playerSpeed;
        }

        // --- Update ---
        if (gameState == PLAYING) {
            Uint32 now = SDL_GetTicks();
            timePlayed = (now - gameStartTime) / 1000;
            enemySpeed = baseSpeed + timePlayed / 5;
            starSpeed = 2 + timePlayed / 5;

            if (now - lastSpawnTime > 1000) {
                int lane = rand() % 4;
                enemies.push_back({ { lane * LANE_WIDTH + 25, -80, 50, 80 } });
                lastSpawnTime = now;
            }

            if (now - lastStarTime > 3000) {
                int lane = rand() % 4;
                stars.push_back({ { lane * LANE_WIDTH + 35, -20, 30, 30 } });
                lastStarTime = now;
            }

            for (auto& e : enemies) {
                e.rect.y += enemySpeed;
                if (e.rect.y > SCREEN_HEIGHT)
                    e.active = false;
            }

            for (auto& s : stars) {
                s.rect.y += starSpeed;
                if (s.rect.y > SCREEN_HEIGHT)
                    s.active = false;
            }

            for (auto& e : enemies) {
                if (SDL_HasIntersection(&player, &e.rect)) {
                    Mix_PlayChannel(-1, crashSound, 0);
                    gameState = GAME_OVER;
                    if (score > highScore) highScore = score;
                }
            }

            for (auto& s : stars) {
                if (SDL_HasIntersection(&player, &s.rect)) {
                    s.active = false;
                    score += 5;
                    Mix_PlayChannel(-1, starSound, 0);
                }
            }

            enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
                [](EnemyCar& e) { return !e.active; }), enemies.end());
            stars.erase(std::remove_if(stars.begin(), stars.end(),
                [](Star& s) { return !s.active; }), stars.end());
        }

        // --- Render ---
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255); // Màu nền
        SDL_RenderClear(renderer);


        if (gameState == MENU) {
            SDL_Color white = { 255, 255, 255, 255 };
            std::string msg = "Press ENTER to Start";
            std::string hs = "High Score: " + std::to_string(highScore);

            SDL_Surface* surf = TTF_RenderText_Solid(font, msg.c_str(), white);
            SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = { 80, 250, surf->w, surf->h };
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_FreeSurface(surf);
            SDL_DestroyTexture(tex);

            surf = TTF_RenderText_Solid(font, hs.c_str(), white);
            tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect hsRect = { 110, 300, surf->w, surf->h };
            SDL_RenderCopy(renderer, tex, NULL, &hsRect);
            SDL_FreeSurface(surf);
            SDL_DestroyTexture(tex);
        }

        if (gameState == PLAYING) {
                        // Vẽ làn đường
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        for (int i = 1; i < 4; ++i) {
        SDL_Rect line = { i * LANE_WIDTH - 2, 0, 4, SCREEN_HEIGHT };
        SDL_RenderFillRect(renderer, &line);
        }
            SDL_RenderCopy(renderer, playerTex, NULL, &player);
            for (auto& e : enemies)
                SDL_RenderCopy(renderer, enemyTex, NULL, &e.rect);

            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            for (auto& s : stars)
                SDL_RenderFillRect(renderer, &s.rect);

            SDL_Color white = { 255, 255, 255, 255 };
            std::string scoreText = "Score: " + std::to_string(score);
            std::string timeText = "Time: " + std::to_string(timePlayed) + "s";

            SDL_Surface* surf = TTF_RenderText_Solid(font, scoreText.c_str(), white);
            SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = { 10, 10, surf->w, surf->h };
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_FreeSurface(surf);
            SDL_DestroyTexture(tex);

            surf = TTF_RenderText_Solid(font, timeText.c_str(), white);
            tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect timeRect = { 10, 40, surf->w, surf->h };
            SDL_RenderCopy(renderer, tex, NULL, &timeRect);
            SDL_FreeSurface(surf);
            SDL_DestroyTexture(tex);
        }

        if (gameState == GAME_OVER) {
            SDL_Color red = { 255, 50, 50, 255 };
            std::string msg = "Game Over!";
            std::string msg2 = "Score: " + std::to_string(score);
            std::string msg3 = "Press ENTER";

            SDL_Surface* surf = TTF_RenderText_Solid(font, msg.c_str(), red);
            SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect rect = { 120, 230, surf->w, surf->h };
            SDL_RenderCopy(renderer, tex, NULL, &rect);
            SDL_FreeSurface(surf);
            SDL_DestroyTexture(tex);

            surf = TTF_RenderText_Solid(font, msg2.c_str(), red);
            tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect r2 = { 130, 270, surf->w, surf->h };
            SDL_RenderCopy(renderer, tex, NULL, &r2);
            SDL_FreeSurface(surf);
            SDL_DestroyTexture(tex);

            surf = TTF_RenderText_Solid(font, msg3.c_str(), red);
            tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect r3 = { 110, 310, surf->w, surf->h };
            SDL_RenderCopy(renderer, tex, NULL, &r3);
            SDL_FreeSurface(surf);
            SDL_DestroyTexture(tex);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    Mix_FreeMusic(backgroundMusic);
    Mix_FreeChunk(starSound);
    Mix_FreeChunk(crashSound);
    SDL_DestroyTexture(playerTex);
    SDL_DestroyTexture(enemyTex);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_Quit();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}
