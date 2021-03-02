#ifndef PLAYER_H
#define PLAYER_H
#include<string>
#include<unordered_set>
#include<vector>
#include<ostream>
class Player {
  public:
    Player(const std::unordered_set<std::string>& locs);
    Player(const Player& arg);
    bool is_disqualified() const;
    void disqualify();
    int locations_owned(const std::string& die1, const std::string& die2, char terrain) const;
    std::vector<std::string> all_locations() const;
    std::string one_random_location() const;
    friend std::ostream& operator<<(std::ostream& os, const Player& rhs);
  private:
    std::unordered_set<std::string> locations;
    bool disqualified;

    Player& operator=(const Player&) = delete;
};
#endif
