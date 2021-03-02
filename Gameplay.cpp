#include<iostream>
#include<iomanip>
#include<cstring>
#include "Gameplay.h"
#include "LootOfLima.h"

Gameplay::Gameplay(int num_players) : num_players{num_players}, lol{num_players}
{
  if(num_players < 2 || num_players > 5)
    throw std::out_of_range{"In Gameplay::Gameplay() Illegal number of players provided: " + std::to_string(num_players)};
  std::string treasure1, treasure2;
  players=LootOfLima::instance().divide_locations(num_players, leftover, treasure1, treasure2);
  treasures.insert(treasure1);
  treasures.insert(treasure2);
  /***/std::clog << "There are " << num_players << " playing Loot of Lima.\n";
  /***/std::clog << "The two treasures are located at " << treasure1 << " and " << treasure2 << "\n\n";
}

void Gameplay::announce_player_count() const
{
  LoLProtocol::MessageFrame msg;
  msg.messageID=1;
  msg.num_players=num_players;
  for(int plyr=1; plyr<=num_players; ++plyr)
    lol.sendMessage(plyr, msg);
}

void Gameplay::allocate_terrain_tokens() const
{
  LoLProtocol::MessageFrame msg;
  msg.messageID=2;
  /***/std::clog << "The players have been allocated the following location tokens:\n";
  for(int plyr=1; plyr<=num_players; ++plyr)
  {
    /***/std::clog << "  Player " << plyr << ": ";
    msg.playerA=plyr;
    std::vector<std::string> locations=players[plyr-1].all_locations();
    msg.num_locations=locations.size();
    int idx=0;
    for(std::string& loc : locations)
    {
      /***/std::clog << loc << " "; 
      strncpy(msg.location[idx++],loc.c_str(),3);
    }
    /***/std::clog << '\n';
    lol.sendMessage(plyr, msg);
  }
  /***/std::clog << '\n';
}

void Gameplay::share_partial_information() const
{
  if(num_players<4) return;

  LoLProtocol::MessageFrame msg;
  msg.messageID=10;

  for(int i=0; i<num_players; ++i)
  {
    msg.playerA=i+1;//token owner
    msg.playerB=(i+1)%num_players+1;//token receiver
    std::string loc{players[i].one_random_location()};
    strncpy(msg.location[0], loc.c_str(),3);
    /***/std::clog << "Player " << msg.playerB << " knows about Player " << msg.playerA << "'s location " << loc << '\n';
    lol.sendMessage(msg.playerA, msg);
    lol.sendMessage(msg.playerB, msg);
  }
  /***/std::clog << '\n';
}

void Gameplay::share_leftovers() const
{
  if(leftover.size()==0)
    return;
  LoLProtocol::MessageFrame msg;
  msg.messageID=3;
  msg.num_locations=leftover.size();
  /***/std::clog << "All players know the treasure is not at the following locations: ";
  int idx=0;
  for(const std::string& left_loc : leftover)
  {
    /***/std::clog << left_loc << " ";
    strncpy(msg.location[idx++],left_loc.c_str(),3);
  }
  /***/std::clog << "\n\n";
  for(int plyr=1; plyr<=num_players; ++plyr)
    lol.sendMessage(plyr, msg);
  msg.num_players=num_players;
}

void Gameplay::play_game()
{
  LoLProtocol::MessageFrame msg;
  bool winner=false;
  int iteration=0;
  do {
    /***/if(iteration%num_players==0)
    /***/  std::clog << "========\nRound " << std::setw(2) << iteration/num_players+1 << "\n========\n";
    //roll dice
    msg.messageID=4;
    msg.playerA=iteration%num_players+1;
    //skip disqualified players
    if(players[msg.playerA-1].is_disqualified())
    {
      ++iteration; 
      continue;
    }
    LootOfLima::instance().roll_dice(msg);
    /***/std::clog << "Player " << msg.playerA << " has rolled: " << msg.dice[0] << ", " << msg.dice[1] << ", " << msg.dice[2] << '\n';

    //Let EVERY player know what was rolled
    for(int plyr=1; plyr<=num_players; ++plyr)
      lol.sendMessage(plyr, msg);

    //Now get the current player's reply
    lol.receiveMessage(msg.playerA, msg);
    switch(msg.messageID)
    {
      case 5: //interrogate another player with compass range and terrain
        {
          //get rid of the terrain character
          std::string die1{msg.dice[0]}; die1.pop_back();
          std::string die2{msg.dice[1]}; die2.pop_back();

          /***/std::clog << "  Player " << iteration%num_players+1 << " asks Player " << msg.playerA << " how many locations they've searched between " << die1 << " and " << die2;
          /***/switch(msg.terrain_type){ 
          /***/  case 'M': std::clog << " on the Mountain.\n"; break;
          /***/  case 'B': std::clog << " on the Beach.\n"; break;
          /***/  case 'F': std::clog << " in the Forest.\n"; break;
          /***/  case 'A': std::clog << " in all terrain.\n"; break;
          /***/}

          //ask the player
          msg.num_locations=players[msg.playerA-1].locations_owned(die1, die2, msg.terrain_type);

          /***/std::clog << "  Player " << msg.playerA << " responds: " << msg.num_locations << ".\n\n";

          //
          // Send out report of interrogation
          //
          msg.messageID=6;
          //PlayerB is current player. PlayerA is subject of interrogation. 
          msg.playerB=iteration%num_players+1; 
          for(int plyr=1; plyr<=num_players; ++plyr)
            lol.sendMessage(plyr, msg);
        }
        break;
      case 7: //Player guesses treasure location
        {
          /***/std::clog << "Player " << iteration%num_players+1 << " is submitting a guess at the treasure locations!\n";
          bool first_correct=treasures.count(msg.location[0]);
          bool second_correct=treasures.count(msg.location[1]);
          if(first_correct && second_correct) //if player guessed correctly
          {
           /***/std::clog << "Player " << iteration%num_players+1 << " is correct! They have won the game!!!!\n\n";
            msg.messageID=8;
            winner=true;
          }
          else
          {
            /***/std::clog << "Player " << iteration%num_players+1 << " was wrong. They are now disqualified from winning.\n\n";
            msg.messageID=9;//assume incorrect guess
            players[msg.playerA-1].disqualify();
          }
          //Send message 8 (or 9)
          for(int plyr=1; plyr<=num_players; ++plyr)
            lol.sendMessage(plyr, msg);
          
          //If there is only a single player not disqualified, send the default winner message
          int players_still_standing=num_players;
          for(int i=0; i<num_players; ++i)
            if(players[i].is_disqualified())
              --players_still_standing;
            else
              msg.playerA=i+1;//if there is only one player, this will be properly initialized

          if(players_still_standing==1)
          {
            /***/std::clog << "Player " << msg.playerA << " is the last player left. They win by default.\n\n";
            msg.messageID=11;
            winner=true;
            int idx=0;
            for(auto& treasure : treasures)
              strncpy(msg.location[idx++], treasure.c_str(),3); 
            //Send message 11
            for(int plyr=1; plyr<=num_players; ++plyr)
              lol.sendMessage(plyr, msg);
          }
        }
        break;
      default:
        {
          int badID=msg.messageID;
          msg.messageID=99;
          std::string error_text{"In Gameplay::play_game() Unkown message ID " + std::to_string(badID) + " from Player " + std::to_string(iteration % num_players + 1)};
          std::strncpy(msg.text,error_text.c_str(),200);
          for(int plyr=1; plyr<=num_players; ++plyr)
            lol.sendMessage(plyr, msg);
          throw std::invalid_argument{error_text};
        }
    }
    ++iteration;
  }while(winner==false);
}
