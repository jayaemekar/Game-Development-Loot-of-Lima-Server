#include <algorithm>
#include "LootOfLima.h"
#include "Player.h"
#include <random>
#include <iterator>
#include <algorithm>
#include <ctime>
#include <cstdlib>


template<typename Iter, typename RandomGenerator>
Iter select_randomly(Iter start, Iter end, RandomGenerator& g) {
  std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
  std::advance(start, dis(g));
  return start;
}

template<typename Iter>
Iter select_randomly(Iter start, Iter end) {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  return select_randomly(start, end, gen);
}

LootOfLima& LootOfLima::instance()
{
  static LootOfLima the_instance;
  return the_instance;
}
bool LootOfLima::invalid_roll(const std::string& die_roll) const
{
  //If the die_roll face is not found in any of the dice, the roll is invalid
  auto found1=find(die1.begin(), die1.end(), die_roll);
  auto found2=find(die2.begin(), die2.end(), die_roll);
  auto found3=find(die3.begin(), die3.end(), die_roll);
  if(found1 == die1.end() && found2 == die2.end() && found3 == die3.end())
    return true;//not found, invalid die face
  return false;
}

std::vector<Player> LootOfLima::divide_locations(int num_players, std::unordered_set<std::string>& leftover, std::string& treasure1, std::string& treasure2) const
{
  std::vector<std::string> locs;
  for(auto& entry : all_locations)
    locs.push_back(entry);
  std::random_shuffle(locs.begin(), locs.end());

  treasure1=locs.back(); locs.pop_back();
  treasure2=locs.back(); locs.pop_back();

  int remaining=locs.size()%num_players;
  for(int i=0; i<remaining; ++i)
  {
    leftover.insert(locs.back()); locs.pop_back();
  }

  std::vector<Player> gameplayers;
  int per_player=locs.size()/num_players;
  for(int tn=0; tn<num_players; ++tn)
  {
    std::unordered_set<std::string> plyr_locs;
    for(int p=0; p<per_player; ++p)
    {
      plyr_locs.insert(locs.back()); locs.pop_back();
    }
    gameplayers.emplace_back(plyr_locs);
  }
  return gameplayers;
}

bool LootOfLima::has_location(const std::string& location) const
{
  return all_locations.count(location);
}

void LootOfLima::roll_dice(LoLProtocol::MessageFrame& frame) const
{

  snprintf(frame.dice[0], 4, "%3.3s", select_randomly(die1.begin(), die1.end())->c_str());
  snprintf(frame.dice[1], 4, "%3.3s", select_randomly(die2.begin(), die2.end())->c_str());
  snprintf(frame.dice[2], 4, "%3.3s", select_randomly(die3.begin(), die3.end())->c_str());
}

LootOfLima::LootOfLima()
{
  //seed random number generator
  std::srand(std::time(nullptr));

  char c[9]={"12345678"};
  char mountain[3]={"0M"};
  char forest[3]={"0F"};
  char beach[3]={"0B"};
  for(int i=0; i<8; ++i)
  {
    mountain[0]=c[i];
    forest[0]=c[i];
    beach[0]=c[i];
    all_locations.emplace(mountain);
    all_locations.emplace(forest);
    all_locations.emplace(beach);
  }
  die1.push_back("SWB"); die1.push_back("NWB"); die1.push_back("NEF"); die1.push_back("SEM");
  die1.push_back("SWF"); die1.push_back("NEM"); die1.push_back("SEF"); die1.push_back("NWF");
  die1.push_back("SWM"); die1.push_back("SEB"); die1.push_back("NWM"); die1.push_back("NEB");
  die2.push_back("EEM"); die2.push_back("SSB"); die2.push_back("SSF"); die2.push_back("EEF");
  die2.push_back("NNW"); die2.push_back("EEW"); die2.push_back("WWW"); die2.push_back("NNF");
  die2.push_back("NNB"); die2.push_back("SSM"); die2.push_back("WWM"); die2.push_back("WWB");
  die3.push_back("SWB"); die3.push_back("SEF"); die3.push_back("NNM"); die3.push_back("WWF");
  die3.push_back("NEM"); die3.push_back("NNM"); die3.push_back("SSW"); die3.push_back("SSM");
  die3.push_back("EEB"); die3.push_back("EEB"); die3.push_back("WWF"); die3.push_back("NWB");
}
