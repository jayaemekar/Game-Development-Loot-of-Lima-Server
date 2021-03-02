#ifndef GAMEPLAY_H
#define GAMEPLAY_H
#include<unordered_set>
#include<vector>
#include<string>
#include "LoLProtocol.h"
#include "Player.h"

class Gameplay {
  public:
    Gameplay(int num_players);
    void announce_player_count() const;
    void allocate_terrain_tokens() const;
    void share_partial_information() const;
    void share_leftovers() const;
    void play_game();
  private:
    int num_players;
    LoLProtocol lol;
    std::unordered_set<std::string> treasures;
    std::unordered_set<std::string> leftover;
    //std::unordered_set<int> disqualified;
    std::vector<Player> players;
    Gameplay(const Gameplay&) = delete;
    Gameplay& operator=(const Gameplay&) = delete;
};
#endif
