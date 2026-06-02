#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <ctime>

using namespace std;

class Card {
public:
    int value;

    Card(int v) : value(v) {}
};

class Deck {
private:
    vector<Card> cards;

public:
    Deck() {
        for (int i = 1; i <= 13; i++) {
            for (int j = 0; j < 4; j++) {
                int value = i;

                if (value > 10)
                    value = 10;

                cards.push_back(Card(value));
            }
        }

        shuffle();
    }

    void shuffle() {
        random_device rd;
        mt19937 g(rd());

        std::shuffle(cards.begin(), cards.end(), g);
    }

    Card draw() {
        Card c = cards.back();
        cards.pop_back();
        return c;
    }
};

class Player {
protected:
    vector<Card> hand;

public:
    void addCard(Card c) {
        hand.push_back(c);
    }

    int getScore() {
        int score = 0;
        int aces = 0;

        for (auto &card : hand) {
            if (card.value == 1) {
                score += 11;
                aces++;
            } else {
                score += card.value;
            }
        }

        while (score > 21 && aces > 0) {
            score -= 10;
            aces--;
        }

        return score;
    }

    void showHand() {
        for (auto &card : hand)
            cout << card.value << " ";

        cout << "\nScore: " << getScore() << endl;
    }
};

class Dealer : public Player {
};

class Game {
private:
    Deck deck;
    Player player;
    Dealer dealer;

public:
    void start() {
        player.addCard(deck.draw());
        player.addCard(deck.draw());

        dealer.addCard(deck.draw());
        dealer.addCard(deck.draw());

        cout << "Dealer score: "
             << dealer.getScore() << endl;

        while (true) {
            cout << "\nPlayer cards:\n";
            player.showHand();

            if (player.getScore() > 21) {
                cout << "Bust! Dealer wins.\n";
                return;
            }

            int choice;

            cout << "\n1. Hit\n2. Stand\nChoice: ";
            cin >> choice;

            if (choice == 1) {
                player.addCard(deck.draw());
            }
            else {
                break;
            }
        }

        while (dealer.getScore() < 17)
            dealer.addCard(deck.draw());

        cout << "\nFinal Score\n";
        cout << "Player: " << player.getScore() << endl;
        cout << "Dealer: " << dealer.getScore() << endl;

        if (dealer.getScore() > 21 ||
            player.getScore() > dealer.getScore())
            cout << "Player wins!\n";
        else if (player.getScore() < dealer.getScore())
            cout << "Dealer wins!\n";
        else
            cout << "Draw!\n";
    }
};

int main() {
    Game game;
    game.start();

    return 0;
}