// CIS7 blackjack project
// Matt Dean
// single 52-card deck
// prints probabilities each turn (stand now vs hit once then stand)
// dealer hole card is hidden until dealer plays

#include <iostream>
#include <vector>
#include <array>
#include <map>
#include <random>
#include <iomanip>
#include <string>
#include <cctype>

using std::array;
using std::cout;
using std::cin;
using std::endl;
using std::string;

struct DeckCounts {
    // card buckets:
    // 0 = A, 1 = 2, ... , 8 = 9, 9 = 10/J/Q/K
    array<int, 10> c{};

    static DeckCounts singleDeck() {
        DeckCounts d;
        // standard single deck
        d.c = {4,4,4,4,4,4,4,4,4,16};
        return d;
    }

    int total() const {
        int s = 0;
        for (int x : c) s += x;
        return s;
    }

    bool canDraw(int idx) const { return c[idx] > 0; }
    void remove(int idx) { c[idx]--; }
    void add(int idx) { c[idx]++; }

    // used for memoization keys
    string key() const {
        string k;
        for (int i = 0; i < 10; i++) {
            k += std::to_string(c[i]);
            if (i != 9) k += ",";
        }
        return k;
    }
};

static int cardValueFromIdx(int idx) {
    if (idx == 0) return 11;        // ace starts as 11
    if (idx <= 8) return idx + 1;  // 2..9
    return 10;                     // 10/J/Q/K
}

static string cardNameFromIdx(int idx) {
    if (idx == 0) return "A";
    if (idx <= 8) return std::to_string(idx + 1);
    return "10";
}

struct HandValue {
    int total;
    bool soft; // true if ace still counted as 11
};

static HandValue evaluateHand(const std::vector<int>& cards) {
    int total = 0;
    int aces = 0;

    for (int idx : cards) {
        if (idx == 0) aces++;
        total += cardValueFromIdx(idx);
    }

    // downgrade aces if bust
    int t = total;
    int acesAs11 = aces;
    while (t > 21 && acesAs11 > 0) {
        t -= 10;
        acesAs11--;
    }

    bool soft = (aces > 0 && acesAs11 > 0 && t <= 21);
    return {t, soft};
}

static bool isBlackjack(const std::vector<int>& cards) {
    return cards.size() == 2 && evaluateHand(cards).total == 21;
}

struct OutcomeProbs {
    double win;
    double lose;
    double push;
};

static OutcomeProbs normalize(const OutcomeProbs& p) {
    double s = p.win + p.lose + p.push;
    if (s == 0) return {0,0,0};
    return {p.win/s, p.lose/s, p.push/s};
}

static void printProbs(const string& label, const OutcomeProbs& p) {
    auto n = normalize(p);
    cout << label << "\n";
    cout << "  P(win)  = " << std::fixed << std::setprecision(4) << n.win  << "\n";
    cout << "  P(lose) = " << std::fixed << std::setprecision(4) << n.lose << "\n";
    cout << "  P(push) = " << std::fixed << std::setprecision(4) << n.push << "\n";
    cout << "  Advantage (win-lose) = "
         << std::fixed << std::setprecision(4)
         << (n.win - n.lose) << "\n";
}

// dealer rule: hit 16 or less, stand 17+
static bool dealerShouldHit(const std::vector<int>& dealerCards) {
    return evaluateHand(dealerCards).total <= 16;
}

struct DealerDist {
    // p[0..4] = dealer ends on 17..21
    // p[5]    = dealer bust
    array<double, 6> p{};
};

static string dealerMemoKey(const std::vector<int>& dealerCards,
                            const DeckCounts& deck) {
    string k = deck.key();
    k += "|";
    for (size_t i = 0; i < dealerCards.size(); i++) {
        k += std::to_string(dealerCards[i]);
        if (i + 1 != dealerCards.size()) k += ",";
    }
    return k;
}

static DealerDist dealerOutcomeDistRec(std::vector<int>& dealerCards,
                                      DeckCounts& deck,
                                      std::map<string, DealerDist>& memo) {
    string key = dealerMemoKey(dealerCards, deck);
    auto it = memo.find(key);
    if (it != memo.end()) return it->second;

    DealerDist dist{};
    HandValue hv = evaluateHand(dealerCards);

    if (hv.total > 21) {
        dist.p[5] = 1.0;
        memo[key] = dist;
        return dist;
    }

    if (!dealerShouldHit(dealerCards)) {
        if (hv.total >= 17 && hv.total <= 21)
            dist.p[hv.total - 17] = 1.0;
        else
            dist.p[5] = 1.0;
        memo[key] = dist;
        return dist;
    }

    int totalCards = deck.total();
    for (int idx = 0; idx < 10; idx++) {
        if (!deck.canDraw(idx)) continue;
        double pr = (double)deck.c[idx] / totalCards;

        deck.remove(idx);
        dealerCards.push_back(idx);

        DealerDist sub = dealerOutcomeDistRec(dealerCards, deck, memo);
        for (int i = 0; i < 6; i++)
            dist.p[i] += pr * sub.p[i];

        dealerCards.pop_back();
        deck.add(idx);
    }

    memo[key] = dist;
    return dist;
}

static DealerDist dealerOutcomeDistribution(const std::vector<int>& dealerCards,
                                           const DeckCounts& deck) {
    std::vector<int> dc = dealerCards;
    DeckCounts d = deck;
    std::map<string, DealerDist> memo;
    return dealerOutcomeDistRec(dc, d, memo);
}

static OutcomeProbs winLosePushIfPlayerStands(const std::vector<int>& player,
                                             const std::vector<int>& dealer,
                                             const DeckCounts& deck) {
    OutcomeProbs out{0,0,0};

    if (isBlackjack(dealer)) {
        if (isBlackjack(player)) out.push = 1.0;
        else out.lose = 1.0;
        return out;
    }

    if (isBlackjack(player)) {
        out.win = 1.0;
        return out;
    }

    HandValue phv = evaluateHand(player);
    if (phv.total > 21) {
        out.lose = 1.0;
        return out;
    }

    DealerDist dd = dealerOutcomeDistribution(dealer, deck);

    out.win += dd.p[5]; // dealer bust

    for (int t = 17; t <= 21; t++) {
        double pr = dd.p[t - 17];
        if (phv.total > t) out.win += pr;
        else if (phv.total < t) out.lose += pr;
        else out.push += pr;
    }

    return out;
}

// exactly one hit, then stand
static OutcomeProbs winLosePushIfPlayerHitsOnce(const std::vector<int>& player,
                                               const std::vector<int>& dealer,
                                               const DeckCounts& deck) {
    OutcomeProbs out{0,0,0};

    int totalCards = deck.total();
    for (int idx = 0; idx < 10; idx++) {
        if (!deck.canDraw(idx)) continue;
        double pr = (double)deck.c[idx] / totalCards;

        DeckCounts d2 = deck;
        d2.remove(idx);

        std::vector<int> p2 = player;
        p2.push_back(idx);

        HandValue hv = evaluateHand(p2);
        if (hv.total > 21) {
            out.lose += pr;
        } else {
            OutcomeProbs sub =
                winLosePushIfPlayerStands(p2, dealer, d2);
            auto sn = normalize(sub);
            out.win  += pr * sn.win;
            out.lose += pr * sn.lose;
            out.push += pr * sn.push;
        }
    }

    return out;
}

static int drawRandom(DeckCounts& deck, std::mt19937& rng) {
    int N = deck.total();
    std::uniform_int_distribution<int> dist(1, N);
    int pick = dist(rng);

    int running = 0;
    for (int i = 0; i < 10; i++) {
        running += deck.c[i];
        if (pick <= running) {
            deck.remove(i);
            return i;
        }
    }
    return 9;
}

static void printHand(const string& who, const std::vector<int>& cards) {
    cout << who << ": ";
    for (size_t i = 0; i < cards.size(); i++) {
        cout << cardNameFromIdx(cards[i]);
        if (i + 1 != cards.size()) cout << ", ";
    }
    HandValue hv = evaluateHand(cards);
    cout << "  (total=" << hv.total
         << (hv.soft ? ", soft" : ", hard") << ")\n";
}

static void printDealerUpcardOnly(const std::vector<int>& dealer) {
    cout << "Dealer upcard: "
         << cardNameFromIdx(dealer[0])
         << "  (hole card hidden)\n";
}

static void dealerPlay(std::vector<int>& dealer, DeckCounts& deck,
                       std::mt19937& rng) {
    while (dealerShouldHit(dealer)) {
        dealer.push_back(drawRandom(deck, rng));
    }
}

static void resolveAndPrintResult(const std::vector<int>& player,
                                 const std::vector<int>& dealer) {
    HandValue p = evaluateHand(player);
    HandValue d = evaluateHand(dealer);

    if (p.total > 21) cout << "Result: Player busts -> Player LOSES\n";
    else if (d.total > 21) cout << "Result: Dealer busts -> Player WINS\n";
    else if (p.total > d.total) cout << "Result: Player WINS\n";
    else if (p.total < d.total) cout << "Result: Player LOSES\n";
    else cout << "Result: PUSH\n";
}

int main() {
    std::random_device rd;
    std::mt19937 rng(rd());

    cout << "Blackjack (Single Deck)\n";
    cout << "Showing probabilities before/after one hit\n\n";

    while (true) {
        DeckCounts deck = DeckCounts::singleDeck();
        std::vector<int> player, dealer;

        player.push_back(drawRandom(deck, rng));
        dealer.push_back(drawRandom(deck, rng)); // upcard
        player.push_back(drawRandom(deck, rng));
        dealer.push_back(drawRandom(deck, rng)); // hole

        cout << "-----------------------\n";
        printDealerUpcardOnly(dealer);
        printHand("Player hand", player);

        while (true) {
            cout << "\n";
            printProbs("BEFORE decision (stand now):",
                winLosePushIfPlayerStands(player, dealer, deck));
            cout << "\n";
            printProbs("AFTER one hit (hit once then stand):",
                winLosePushIfPlayerHitsOnce(player, dealer, deck));

            cout << "\nAction? (H)it or (S)tand: ";
            char action;
            cin >> action;
            action = (char)std::toupper(action);

            if (action == 'H') {
                player.push_back(drawRandom(deck, rng));
                printHand("Player hand", player);
                if (evaluateHand(player).total > 21) break;
            } else {
                break;
            }
        }

        dealerPlay(dealer, deck, rng);

        cout << "\nFinal hands:\n";
        printHand("Dealer hand", dealer);
        printHand("Player hand", player);
        resolveAndPrintResult(player, dealer);

        cout << "\nPlay another hand? (Y/N): ";
        char again;
        cin >> again;
        if (std::toupper(again) != 'Y') break;
    }

    cout << "Done.\n";
    return 0;
}
