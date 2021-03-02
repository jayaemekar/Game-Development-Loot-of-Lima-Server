#include<iostream>
#include<cstring>
#include<cassert>
#include "LoLProtocol.h"
#include "LootOfLima.h"
#include "Gameplay.h"
#include "Player.h"
#include<vector>
using namespace std;

int main(int argc, char* argv[])
{
  if(argc!=2)
  {
    std::cerr << "Usage: " << argv[0] << " number_of_players (between 2 and 5)\n";
    exit(1);
  }

  Gameplay gameplay{std::stoi(argv[1])};
  gameplay.announce_player_count();
  gameplay.allocate_terrain_tokens();
  gameplay.share_leftovers();
  gameplay.share_partial_information();
  gameplay.play_game();

  std::cout << "GAME OVER! Hit <Enter> to exit.\n";
  string pause;
  getline(cin,pause);
  return 0;
}
