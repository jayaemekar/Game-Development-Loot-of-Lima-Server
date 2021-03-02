#ifndef LOOTOFLIMA_H
#define LOOTOFLIMA_H
#include<unordered_set>
#include<vector>
#include<string>
#include"LoLProtocol.h"
#include"Player.h"
class LootOfLima {
  public:
    static LootOfLima& instance();
    bool has_location(const std::string& location) const;
    void roll_dice(LoLProtocol::MessageFrame& frame) const;
    bool invalid_roll(const std::string& die_roll) const;
    std::vector<Player> divide_locations(int num_players, std::unordered_set<std::string>& leftover, std::string& treasure1, std::string& treasure2) const;

  private:
    LootOfLima();
    std::unordered_set<std::string> all_locations;
    //made these vectors instead of sets to ease random selection
    std::vector<std::string> die1;
    std::vector<std::string> die2;
    std::vector<std::string> die3;

    LootOfLima(const LootOfLima&) = delete;
    LootOfLima& operator=(const LootOfLima&) = delete;
};
#endif
