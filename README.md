# CIS7-Blackjack
Discrete Structures Blackjack probability project

Auther: Matthew Dean
Course: CIS7 - Discrete Structures
Project: Case 4 - Blackjack
Date: Fall 2025

This project simulates Blackjack hands using a 52 card deck. The program computes the player's probabilities of winning, losing, or pushing based upon the deck's remaining cards, without any hard-coding.

The program displays probabilities for both standing immediately, AND taking one hit and standing.

The dealer follows standard casino rules (hit on 16 or less, stand on 17 or more).
The dealer's hole card is not shown until the dealer's turn.

The deck is represented as counts of card ranks.
Probabilities are computed by using conditional probability: P(card) = count(card) / remaining cards.
Dealer outcomes are computer recursively until reaching 17-21 or bust.
Memoization is used to reduce repeat probability calculations.
Aces are properly treated as 1/11.

CIS7_Final_Blackjack_MattDean is the main and only C++ file.

You can run this by using a C++ compiler.
You can then follow the prompts to either hit or stand.

NOTES:
This program is limited in its scope and focuses on hit/stand only. More traditional rules and features such as doubling/splitting/surrendering/multiple decks are not present here. The goal of this project is simple probability computation as opposed to a full-on Blackjack Strategy program.
