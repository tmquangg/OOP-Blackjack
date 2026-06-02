#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
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
int getTotal(vector<Card>& hand) {
    int total = 0;

    for (auto& c : hand) {
        int v = c.value + 1;

        if (v > 10) v = 10; // J Q K = 10

        total += v;
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
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
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
        SDL_WINDOW_RESIZABLE
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    // load ảnh
    SDL_Texture* cardSheet = loadTexture("assets/cards.bmp", renderer);
    SDL_Texture* tableBg = loadTexture("assets/table.bmp", renderer);

    if (!cardSheet) {
        cout << "Texture null\n";
        return -1;
    }

    // Player hand
    srand(time(0));
    vector<Card> playerHand;
    vector<CardAnimation> animations;
    bool dealerAnimating = false;
    
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

    cout << playerHand.size() << endl;
    printHand(playerHand);
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
    SDL_Event e;

    while (running) {
        // ===== INPUT =====
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }

            if (e.type == SDL_EVENT_KEY_DOWN) {
                
                Uint32 currentInput = SDL_GetTicks();

                if (currentInput - lastInputTime < 500)
                    continue;

                lastInputTime = currentInput;

                // Restart Game
                if (e.key.scancode == SDL_SCANCODE_R) {
                    playerHand.clear();
                    dealerHand.clear();

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

                    cout << "=== NEW GAME ===\n";
                    resultText = "";
                    printHand(playerHand);
                    printHand(dealerHand);
                }

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

                if (anim.dealerCard) {

                    dealerHand.push_back(anim.card);

                    cout << "Dealer draw:\n";
                    printHand(dealerHand);

                    cout << "Dealer total: "
                        << getTotal(dealerHand)
                        << endl;

                    dealerAnimating = false;
                }
                else {

                    playerHand.push_back(anim.card);

                    cout << "Hit! Number of cards: "
                        << playerHand.size()
                        << endl;

                    printHand(playerHand);

                    cout << "Total: "
                        << getTotal(playerHand)
                        << endl;

                    if (getTotal(playerHand) > 21) {
                        cout << "PLAYER BUST! YOU LOSE!\n";
                        resultText = "YOU LOSE!";
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
                    }
                    else if (playerTotal == dealerTotal) {
                        cout << "DRAW!\n";
                        resultText = "DRAW!";
                    }
                    else {
                        cout << "YOU LOSE!\n";
                        resultText = "YOU LOSE!";
                    }

                    roundEnded = true;
                }
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

        SDL_RenderPresent(renderer);

        string title =
            "Blackjack SDL | Player: " +
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
    SDL_Quit();

    return 0;
}