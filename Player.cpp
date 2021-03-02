#include<cassert>
#include<map>
#include <random>
#include <iterator>
#include "Player.h"

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


Player::Player(const std::unordered_set<std::string>& locs)
  :locations{locs}, disqualified(false)
{}
Player::Player(const Player& arg)
  :locations{arg.locations}, disqualified(false)
{
}

std::vector<std::string> Player::all_locations() const
{
  std::vector<std::string> locs{locations.begin(), locations.end()};
  return locs;
}
void Player::disqualify()
{
  disqualified=true;
}

bool Player::is_disqualified() const
{
  return disqualified;
}

std::string Player::one_random_location() const
{
  std::vector<std::string> locs{locations.begin(), locations.end()};
  return *select_randomly(locs.begin(), locs.end());
}

int Player::locations_owned(const std::string& die1, const std::string& die2, char terrain) const
{
  std::map<std::string, int> compass {
    { "NN", 0}, { "NE", 1},
    { "EE", 2}, { "SE", 3},
    { "SS", 4}, { "SW", 5},
    { "WW", 6}, { "NW", 7} };
  if(compass.count(die1) != 1)
    throw std::out_of_range{"In Player::locations_owned() Illegal first compass direction provided: " + die1};
  if(compass.count(die2) != 1)
    throw std::out_of_range{"In Player::locations_owned() Illegal second compass direction provided: " + die1};
  std::unordered_set<std::string> locations_to_check;
  
  //all locations to check are placed into an unordered_set
  int needle=compass.at(die1);
  do {
    int number_to_check=needle%8+1;
    switch(terrain)
    {
      case 'A':
      case 'M':
        locations_to_check.emplace(std::to_string(number_to_check)+'M');
        if(terrain!='A') break;
      case 'F':
        locations_to_check.emplace(std::to_string(number_to_check)+'F');
        if(terrain!='A') break;
      case 'B':
        locations_to_check.emplace(std::to_string(number_to_check)+'B');
        break;
      default:
        assert(false);//shouldn't be possible
    }
    ++needle;
  }while(needle%8 != compass.at(die2));

  //enumerate the number of these locations the player has
  int location_matches=0;
  for( const std::string& potential_location : locations_to_check)
    if(locations.count(potential_location)==1)
      ++location_matches;
  return location_matches;
}

std::ostream& operator<<(std::ostream& os, const Player& rhs)
{
  int counter=0;
  for(const auto& loc : rhs.locations)
    os << " " << ++counter << ") Location: " << loc << '\n';
  return os;
}
