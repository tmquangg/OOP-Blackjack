#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <iostream>
#include <vector>
#include <ctime>
#include <algorithm>
#include <string>
using namespace std;

// ===== LOAD TEXTURE =====
SDL_Texture* loadTexture(const char* path, SDL_Renderer* renderer) {
    SDL_Surface* surface = IMG_Load(path);

    if (!surface) {
        cout << "Load fail: " << SDL_GetError() << endl;
        return nullptr;
    }
    SDL_SetSurfaceColorKey(surface, true, SDL_MapRGB(SDL_GetPixelFormatDetails(surface->format), nullptr, 255, 255, 255));

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);

    return texture;
}

// =====AUDIO=====

// bgm
SDL_AudioSpec musicSpec;
Uint8* musicBuffer = nullptr;
Uint32 musicLength = 0;

SDL_AudioDeviceID musicDevice = 0;

//sfx
SDL_AudioSpec sfxSpec;
Uint8* sfxBuffer = nullptr;
Uint32 sfxLength = 0;

SDL_AudioStream* sfxStream = nullptr;

Uint8* winBuffer = nullptr;
Uint32 winLength = 0;

Uint8* loseBuffer = nullptr;
Uint32 loseLength = 0;

// Render Card
void renderCard(SDL_Renderer* renderer, SDL_Texture* sheet,
    int col, int row,
    float x, float y, float w, float h) {

    int spriteW = 72;   // chỉnh theo ảnh bạn
    int spriteH = 88;


    int srcY = row * spriteH;

    if (row == 4) {
        srcY = row * 88;
        spriteH = 104;    
    }

    SDL_FRect src = {
        col * spriteW,
        srcY,
        spriteW,
        spriteH
    };

    SDL_FRect dst = { x, y, w, h };

    SDL_RenderTexture(renderer, sheet, &src, &dst);
}

struct Card {
    int value; // 1 → 13 (A → K)
    int suit;  // 0 → 3 (♥ ♦ ♣ ♠)
};

struct CardAnimation {
    Card card;

    float x, y;
    float targetX, targetY;

    bool done = false;
    bool dealerCard = false;
};

// DRAW RANDOM CARD
Card drawCard() {
    return { rand() % 13, rand() % 4 };
}

// TOTAL SCORE
int getTotal(vector<Card>& hand)
{
    int total = 0;
    int aceCount = 0;

    for (auto& c : hand)
    {
        int v = c.value + 1;

        // Ace
        if (v == 1)
        {
            total += 11;
            aceCount++;
        }
        // J Q K
        else if (v > 10)
        {
            total += 10;
        }
        else
        {
            total += v;
        }
    }

    while (total > 21 && aceCount > 0)
    {
        total -= 10;
        aceCount--;
    }

    return total;
}

void printHand(vector<Card>& hand) {
    cout << "Cards: ";
    for (auto& c : hand) {
        cout << "(" << c.value << ") ";
    }
    cout << endl;
}

// Card animation
bool animating = false;

float animX, animY;
float targetX, targetY;

Card animCard;

void addAnimatedCard(
    vector<CardAnimation>& animations,
    int deckX,
    int deckY,
    int targetX,
    int targetY,
    bool dealer = false
) {
    CardAnimation anim;

    anim.card = drawCard();

    anim.x = deckX;
    anim.y = deckY;

    anim.targetX = targetX;
    anim.targetY = targetY;

    anim.dealerCard = dealer;
    animations.push_back(anim);
}

int w = 800;
int h = 600;

int cardW = 90;
int cardH = 110;
int spacing = cardW * 0.5f;

SDL_FRect deck = {
    w / 2.0f - cardW / 2.0f,
    h / 2.0f - cardH / 2.0f,
    (float)cardW,
    (float)cardH
};

// ===== MAIN =====
int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        cout << "SDL init fail\n";
        return -1;
    }

    Uint32 lastDrawTime = 0;
    Uint32 lastInputTime = 0;
    
    // RENDER
    SDL_Window* window = SDL_CreateWindow(
        "Blackjack SDL",
        800,
        600,
        0
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    if (!SDL_LoadWAV(
        "assets/bgmusic.wav",
        &musicSpec,
        &musicBuffer,
        &musicLength))
    {
        cout << "Music load fail: "
            << SDL_GetError()
            << endl;
    }

    if (!SDL_LoadWAV(
        "assets/card_draw.wav",
        &sfxSpec,
        &sfxBuffer,
        &sfxLength))
    {
        cout << "SFX load fail: "
            << SDL_GetError()
            << endl;
    }

    SDL_LoadWAV(
        "assets/win.wav",
        &sfxSpec,
        &winBuffer,
        &winLength
    );

    SDL_LoadWAV(
        "assets/lose.wav",
        &sfxSpec,
        &loseBuffer,
        &loseLength
    );

    SDL_AudioStream* musicStream = nullptr;

    //bgm
    musicStream =
        SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
            &musicSpec,
            nullptr,
            nullptr
        );

    if (!musicStream)
    {
        cout << "Audio open fail: "
            << SDL_GetError()
            << endl;
    }

    SDL_PutAudioStreamData(
        musicStream,
        musicBuffer,
        musicLength
    );

    SDL_ResumeAudioStreamDevice(
        musicStream
    );

    //sfx
    sfxStream =
        SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
            &sfxSpec,
            nullptr,
            nullptr
        );

    if (!sfxStream)
    {
        cout << "SFX open fail: "
            << SDL_GetError()
            << endl;
    }

    SDL_ResumeAudioStreamDevice(
        sfxStream
    );

    // load ảnh
    SDL_Texture* cardSheet = loadTexture("assets/cards.bmp", renderer);
    SDL_Texture* tableBg = loadTexture("assets/table.bmp", renderer);
    SDL_Texture* controls = loadTexture("assets/controls.bmp", renderer);

    SDL_Texture* winScreen =
        loadTexture("assets/you_win.bmp", renderer);

    SDL_Texture* loseScreen =
        loadTexture("assets/you_lose.bmp", renderer);

    SDL_Texture* mainScreen =
        loadTexture("assets/mainscreen.bmp", renderer);

    if (!cardSheet) {
        cout << "Texture null\n";
        return -1;
    }

    // Player hand
    srand(time(0));
    vector<Card> playerHand;
    vector<CardAnimation> animations;
    bool dealerAnimating = false;
    bool initialDealing = true;
    
    addAnimatedCard(
        animations,
        deck.x,
        deck.y,
        300,
        450
    );

    addAnimatedCard(
        animations,
        deck.x,
        deck.y,
        340,
        450
    );

    bool playerTurn = true;

    // Dealer hand
    vector<Card> dealerHand;
    
    addAnimatedCard(
        animations,
        w / 2.0f - cardW / 2.0f,
        h / 2.0f - cardH / 2.0f,
        300,
        80,
        true
    );

    addAnimatedCard(
        animations,
        w / 2.0f - cardW / 2.0f,
        h / 2.0f - cardH / 2.0f,
        340,
        80,
        true
    );

    bool running = true;
    bool roundEnded = false;
    string resultText = "";
    int winCount = 0;
    int loseCount = 0;
    bool gameWon = false;
    bool gameLost = false;
    bool showMainMenu = true;
    bool menuFading = false;
    float menuAlpha = 255.0f;
    SDL_Event e;

    while (running) {
        // ===== INPUT =====
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }
            // music
            if (SDL_GetAudioStreamAvailable(musicStream) == 0)
            {
                SDL_PutAudioStreamData(
                    musicStream,
                    musicBuffer,
                    musicLength
                );
            }

            // menu
            if (showMainMenu &&
                e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                menuFading = true;
            }

            // Hard Reset
            if ((gameWon || gameLost) &&
                e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                winCount = 0;
                loseCount = 0;
              
                gameWon = false;
                gameLost = false;
                initialDealing = true;

                playerHand.clear();
                dealerHand.clear();
                animations.clear();

                resultText = "";
                playerTurn = true;
                roundEnded = false;
                cout << "\n=== RESTARTED GAME ===\n";
                SDL_ResumeAudioStreamDevice(
                    musicStream
                );

                // chia bài mới

                addAnimatedCard(
                    animations,
                    deck.x,
                    deck.y,
                    300,
                    450
                );

                addAnimatedCard(
                    animations,
                    deck.x,
                    deck.y,
                    340,
                    450
                );

                addAnimatedCard(
                    animations,
                    w / 2.0f - cardW / 2.0f,
                    h / 2.0f - cardH / 2.0f,
                    300,
                    80,
                    true
                );

                addAnimatedCard(
                    animations,
                    w / 2.0f - cardW / 2.0f,
                    h / 2.0f - cardH / 2.0f,
                    340,
                    80,
                    true
                );
            }

            if (e.type == SDL_EVENT_KEY_DOWN) {
                
                Uint32 currentInput = SDL_GetTicks();

                if (currentInput - lastInputTime < 500)
                    continue;

                lastInputTime = currentInput;

                if (showMainMenu)
                {
                    continue;
                }

                if (gameWon || gameLost)
                {
                    continue;
                }

                // Restart Round
                if (roundEnded &&
                    e.key.scancode == SDL_SCANCODE_R) {
                    initialDealing = true;
                    playerHand.clear();
                    dealerHand.clear();
                    animations.clear();

                    //player
                    addAnimatedCard(
                        animations,
                        deck.x,
                        deck.y,
                        300,
                        450
                    );

                    addAnimatedCard(
                        animations,
                        deck.x,
                        deck.y,
                        340,
                        450
                    );

                    //dealer
                    addAnimatedCard(
                        animations,
                        w / 2.0f - cardW / 2.0f,
                        h / 2.0f - cardH / 2.0f,
                        300,
                        80,
                        true
                    );

                    addAnimatedCard(
                        animations,
                        w / 2.0f - cardW / 2.0f,
                        h / 2.0f - cardH / 2.0f,
                        340,
                        80,
                        true
                    );

                    playerTurn = true;
                    roundEnded = false;

                    cout << "\n=== NEW ROUND ===\n";
                    resultText = "";
                }

                // Controls

                if (!roundEnded) {

                    // Hit
                    if (e.key.scancode == SDL_SCANCODE_H) {
                        
                        int targetX = 300 + playerHand.size() * 40;
                        int targetY = h * 0.73f;

                        addAnimatedCard(
                            animations,
                            deck.x,
                            deck.y,
                            targetX,
                            targetY
                        );
                        
                        SDL_ClearAudioStream(sfxStream);

                        SDL_PutAudioStreamData(
                            sfxStream,
                            sfxBuffer,
                            sfxLength
                        );
                    }

                    // Stand
                    if (e.key.scancode == SDL_SCANCODE_S && playerTurn) {
                        playerTurn = false;
                        cout << "Stand!\n";
                    }
                }
            }
        }
        
        for (auto& anim : animations) {

            anim.x += (anim.targetX - anim.x) * 0.1f;
            anim.y += (anim.targetY - anim.y) * 0.1f;

            if (abs(anim.targetX - anim.x) < 5 &&
                abs(anim.targetY - anim.y) < 5 &&
                !anim.done) {

                if (anim.dealerCard)
                {
                    dealerHand.push_back(anim.card);

                    if (!initialDealing)
                    {
                        cout << "Dealer draw:\n";
                        printHand(dealerHand);

                        cout << "Dealer total: "
                            << getTotal(dealerHand)
                            << endl;
                    }

                    dealerAnimating = false;
                }
                else {

                    playerHand.push_back(anim.card);

                    if (!initialDealing)
                    {
                        cout << "Hit! Number of cards: "
                            << playerHand.size()
                            << endl;

                        printHand(playerHand);

                        cout << "Total: "
                            << getTotal(playerHand)
                            << endl;
                    }

                    if (getTotal(playerHand) > 21) {
                        cout << "PLAYER BUST! YOU LOSE!\n";
                        resultText = "YOU LOSE!";
                        loseCount++;

                        if (loseCount >= 5)
                        {
                            gameLost = true;

                            SDL_PauseAudioStreamDevice(
                                musicStream
                            );

                            SDL_ClearAudioStream(sfxStream);

                            SDL_PutAudioStreamData(
                                sfxStream,
                                loseBuffer,
                                loseLength
                            );
                        }

                        roundEnded = true;
                    }
                }

                anim.done = true;
            }
        }

        animations.erase(
            remove_if(
                animations.begin(),
                animations.end(),
                [](CardAnimation& a) {
                    return a.done;
                }
            ),
            animations.end()
        );

        if (playerHand.size() >= 2 &&
            dealerHand.size() >= 2 &&
            initialDealing)
        {
            initialDealing = false;

            cout << "\n=== ROUND START ===\n";
            cout << "(First to 5 Wins)\n";
            cout << "Player: " << winCount
                << " | Dealer: "
                << loseCount
                << "\n\n";

            cout << "Player:\n";
            printHand(playerHand);
            cout << "Total: "
                << getTotal(playerHand)
                << "\n\n";

            cout << "Dealer:\n";
            printHand(dealerHand);
            cout << "Total: "
                << getTotal(dealerHand)
                << "\n\n";
        }

        // Dealer draw

        if (!playerTurn && !roundEnded) {

            Uint32 currentTime = SDL_GetTicks();

            if (currentTime - lastDrawTime > 1500) {

                if (getTotal(dealerHand) < 17 && !dealerAnimating) {
                    int targetXD = 300 + dealerHand.size() * 40;
                    int targetYD = h * 0.08f;

                    addAnimatedCard(
                        animations,
                        w / 2.0f - cardW / 2.0f,
                        h / 2.0f - cardH / 2.0f,
                        targetXD,
                        targetYD,
                        true
                    );

                    SDL_ClearAudioStream(sfxStream);

                    SDL_PutAudioStreamData(
                        sfxStream,
                        sfxBuffer,
                        sfxLength
                    );

                    dealerAnimating = true;

                    lastDrawTime = currentTime;
                }
                else {
                    // dừng và tính kết quả
                    int playerTotal = getTotal(playerHand);
                    int dealerTotal = getTotal(dealerHand);

                    cout << "Dealer: " << dealerTotal << endl;
                    cout << "Player: " << playerTotal << endl;

                    if (dealerTotal > 21 || playerTotal > dealerTotal) {
                        cout << "YOU WIN!\n";
                        resultText = "YOU WIN!";
                        winCount++;

                        if (winCount >= 5)
                        {
                            gameWon = true;

                            SDL_PauseAudioStreamDevice(
                                musicStream
                            );

                            SDL_ClearAudioStream(sfxStream);

                            SDL_PutAudioStreamData(
                                sfxStream,
                                winBuffer,
                                winLength
                            );
                        }
                    }
                    else if (playerTotal == dealerTotal) {
                        cout << "DRAW!\n";
                        resultText = "DRAW!";
                    }
                    else {
                        cout << "YOU LOSE!\n";
                        resultText = "YOU LOSE!";
                        loseCount++;

                        if (loseCount >= 5)
                        {
                            gameLost = true;

                            SDL_PauseAudioStreamDevice(
                                musicStream
                            );

                            SDL_ClearAudioStream(sfxStream);

                            SDL_PutAudioStreamData(
                                sfxStream,
                                loseBuffer,
                                loseLength
                            );
                        }
                    }

                    roundEnded = true;
                }
            }
        }

        //menu
        if (menuFading)
        {
            menuAlpha -= 3.0f;

            if (menuAlpha <= 0)
            {
                menuAlpha = 0;
                showMainMenu = false;
                menuFading = false;
            }
        }

        // RENDER
        
        // lấy size window
        int w, h;
        SDL_GetWindowSize(window, &w, &h);

        // clear nền
        SDL_SetRenderDrawColor(renderer, 0, 120, 0, 255);
        SDL_RenderClear(renderer);

        // BG
        SDL_FRect dst = { 0, 0, (float)w, (float)h };
        SDL_RenderTexture(renderer, tableBg, NULL, &dst);

        // ===== DECK (bộ bài) =====

        int cardW = w * 0.12f;
        int cardH = h * 0.20f;
        int spacing = cardW * 0.5f;

        SDL_FRect deck = {
            w / 2.0f - cardW / 2.0f,  
            h / 2.0f - cardH / 2.0f,  
            (float)cardW,
            (float)cardH
        };

        renderCard(renderer, cardSheet,
            12, 4,
            deck.x,
            deck.y,
            deck.w,
            deck.h);

        // ===== DEALER CARD =====
        int spacingD = cardW * 0.5f;

        int totalWidthD = cardW + (dealerHand.size() - 1) * spacingD;
        int startXD = (w - totalWidthD) / 2;

        for (int i = 0; i < dealerHand.size(); i++) {
            SDL_FRect d = {
                (float)(startXD + i * spacingD),
                h * 0.08f,
                (float)cardW,
                (float)cardH
            };

            renderCard(renderer, cardSheet,
                dealerHand[i].value,
                dealerHand[i].suit,
                (float)(startXD + i * spacingD),
                h * 0.08f,
                (float)cardW,
                (float)cardH);
        }

        // ===== PLAYER =====

        int totalWidth = cardW + (playerHand.size() - 1) * spacing;
        int startX = (w - totalWidth) / 2;

        for (int i = 0; i < playerHand.size(); i++) {
            SDL_FRect player = {
                (float)(startX + i * spacing),
                h * 0.73f,
                (float)cardW,
                (float)cardH
            };

            renderCard(renderer, cardSheet,
                playerHand[i].value,
                playerHand[i].suit,
                (float)(startX + i * spacing),
                h * 0.73f,
                (float)cardW,
                (float)cardH);
        }

        // ===== PRESENT =====
        for (auto& anim : animations) {

            renderCard(renderer, cardSheet,
                anim.card.value,
                anim.card.suit,
                anim.x,
                anim.y,
                (float)cardW,
                (float)cardH);
        }

        SDL_FRect controlsRect = {
            40.0f,
            40.0f,
            180.0f,
            140.0f
        };

        SDL_RenderTexture(
            renderer,
            controls,
            NULL,
            &controlsRect
        );

        // Menu
        if (showMainMenu)
        {
            SDL_SetTextureAlphaMod(
                mainScreen,
                (Uint8)menuAlpha
            );

            SDL_FRect menuRect = {
                0,
                0,
                (float)w,
                (float)h
            };

            SDL_RenderTexture(
                renderer,
                mainScreen,
                NULL,
                &menuRect
            );
        }

        // Win / Lose screen

        if (gameWon)
        {
            SDL_FRect winRect = {
                0,
                0,
                (float)w,
                (float)h
            };

            SDL_RenderTexture(
                renderer,
                winScreen,
                NULL,
                &winRect
            );
        }

        if (gameLost)
        {
            SDL_FRect loseRect = {
                0,
                0,
                (float)w,
                (float)h
            };

            SDL_RenderTexture(
                renderer,
                loseScreen,
                NULL,
                &loseRect
            );
        }

        SDL_RenderPresent(renderer);

        string title =
            "Blackjack (First to 5 Wins) | Wins: " +
            std::to_string(winCount) +
            " | Losses: " +
            std::to_string(loseCount) +
            " | Player: " +
            std::to_string(getTotal(playerHand)) +
            " | Dealer: " +
            std::to_string(getTotal(dealerHand));

        if (!resultText.empty()) {
            title += " | " + resultText;
        }

        SDL_SetWindowTitle(window, title.c_str());

        // chống nháy
        SDL_Delay(16);
    }

    SDL_DestroyTexture(cardSheet);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    
    SDL_DestroyAudioStream(
        musicStream
    );

    SDL_free(
        musicBuffer
    );

    SDL_DestroyAudioStream(sfxStream);

    SDL_free(sfxBuffer);
    SDL_Quit();

    return 0;
}