#include<fcntl.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<cassert>
#include<cstring>
#include<stdexcept>
#include<string>
#include"LoLProtocol.h"
#include"LootOfLima.h"

using std::string;

LoLProtocol::LoLProtocol(int num_players) : num_players{num_players}
{
  if(num_players < 2 || num_players > 5)
    throw std::out_of_range{"In LoLProtocol::LoLProtocol() Illegal number of players provided: " + std::to_string(num_players)};
  // Create named pipes (FIFOs)
  umask(0);//stop the masking of permissions bits
  for(int i=0; i<num_players; ++i)
  {
    string fromfile{"/tmp/alphafromP" + std::to_string(i+1)};
    string tofile{"/tmp/alphatoP" + std::to_string(i+1)};
    //Just in case these files were left hanging around from a previous run.
    unlink(tofile.c_str());
    unlink(fromfile.c_str());
    //Set user permissions to O_RDWR to prevent blocking on O_RDONLY/O_WRONLY
    if(mkfifo(fromfile.c_str(), S_IRUSR|S_IWUSR|S_IWGRP|S_IWOTH)==-1 || (fds[2*i]  =open(fromfile.c_str(),O_RDWR))==-1 ||
       mkfifo(tofile.c_str(),   S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH)==-1 || (fds[2*i+1]=open(tofile.c_str(),O_RDWR))==-1)
      throw std::runtime_error{"In LoLProtocol::LoLProtocol() Cannot create named pipes"};
  }
}

//////////////////////////////////////////////////////////////////////////////////
//////// receiveMessage
//////////////////////////////////////////////////////////////////////////////////
void LoLProtocol::receiveMessage(int playernum, LoLProtocol::MessageFrame& frame) const
{
  char buffer[256];
  int lenread=0;
  lenread=read(from_player(playernum), buffer, 256);
  if(lenread>=256)
  {
    string largestrerror{"LoLProtocol::receiveMessage: Message length is unexpectedly large->"};
    largestrerror+=buffer; largestrerror+="<-";
    throw std::length_error{largestrerror};
  }
  if(lenread<=0)
    throw std::runtime_error{"LoLProtocol::receiveMessage: reading message from named pipe had an issue"};
  lenread=0;
  buffer[2]='\0';//replace colon
  frame.messageID=std::stoi(buffer);
  switch(frame.messageID)
  {
    //*************************
    //*** Number of players
    //*************************
    case 1:
      lenread=sscanf(buffer+3,"%i\n",&frame.num_players);
      break;
    //*************************
    //*** Allocate terrain tokens
    //*** Inform leftover tokens
    //*************************
    case 2:
    case 3:
      {
        char csv_list[201];
        if(frame.messageID==2)
          lenread=sscanf(buffer+3, "P%i,%200s\n",&frame.playerA,csv_list);
        if(frame.messageID==3)
          lenread=sscanf(buffer+3, "%200s\n",csv_list);
        char* next_location{strtok(csv_list,",")};
        int counter=0;
        do {
          strncpy(frame.location[counter],next_location,3);
          frame.location[counter][2]='\0';//just in case provided location isn't 2 characters
          if(LootOfLima::instance().has_location(frame.location[counter])==false)
            throw std::length_error{std::string{"LoLProtocol::receiveMessage 0"} + std::to_string(frame.messageID) + string{": Location does not exist: "}+frame.location[counter]};
          ++counter;
        } while((next_location=strtok(nullptr,","))!=nullptr);
      }
      break;
    //*************************
    //*** Player turn: rolled dice
    //*************************
    case 4:
      lenread=sscanf(buffer+3, "P%i,%3s,%3s,%3s\n",&frame.playerA,frame.dice[0],frame.dice[1],frame.dice[2]);
      //valid number for playerA
      if(frame.playerA < 1 || frame.playerA > num_players)
        throw std::length_error{std::string{"LoLProtocol::receiveMessage 0"} + std::to_string(frame.messageID) + 
                                std::string{": invalid player number "} + std::to_string(frame.playerA)};
      break;
    //*************************
    //*** Player response: dice & player
    //*************************
    case 5:
      {
      lenread=sscanf(buffer+3, "%3s,%3s,%c,P%i\n",frame.dice[0],frame.dice[1],&frame.terrain_type,&frame.playerA);
      //validate number for playerA
      if(frame.playerA < 1 || frame.playerA > num_players)
        throw std::length_error{std::string{"LoLProtocol::receiveMessage 0"} + std::to_string(frame.messageID) + 
                                std::string{": invalid player number "} + std::to_string(frame.playerA)};
      //validate die face for dice[0]
      if(LootOfLima::instance().invalid_roll(frame.dice[0]))
        throw std::invalid_argument{std::string{"LoLProtocol::receiveMessage 0"} + std::to_string(frame.messageID) + 
                                std::string{": 1st- invalid die face returned "} + frame.dice[0]};
      //validate die face for dice[1]
      if(LootOfLima::instance().invalid_roll(frame.dice[1]))
        throw std::invalid_argument{std::string{"LoLProtocol::receiveMessage 0"} + std::to_string(frame.messageID) + 
                                std::string{": 2nd- invalid die face returned "} + frame.dice[0]};
      //validate that chosen terrain type is valid
      char tA=frame.dice[0][2];
      char tB=frame.dice[1][2];
      string possibles;
      if(tA=='W' && tB=='W')//both dice wild
        possibles="MFBA";
      else if(tA=='W')//only first die wild
      { possibles=tB;possibles+='A'; }
      else if(tB=='W')//only second die wild
      { possibles=tA;possibles+='A'; }
      else if(tA==tB)//terrain is same
        possibles=tA;
      else
        possibles='A';
      auto found=possibles.find(frame.terrain_type);
      if(found==std::string::npos)
        throw std::invalid_argument{std::string{"LoLProtocol::receiveMessage 0"} + std::to_string(frame.messageID) + 
                                std::string{": invalid terrain type picked "} + frame.terrain_type};
      }
      break;
    //*************************
    //*** Report of interrogation: dice & found tokens
    //*************************
    case 6:
      {
      lenread=sscanf(buffer+3, "%3s,%3s,%c,%i,P%i,P%i\n",frame.dice[0],frame.dice[1],&frame.terrain_type,&frame.num_locations,&frame.playerA,&frame.playerB);
      //validate number for playerA
      if(frame.playerA < 1 || frame.playerA > num_players)
        throw std::length_error{std::string{"LoLProtocol::receiveMessage 0"} + std::to_string(frame.messageID) + 
                                std::string{": invalid player number "} + std::to_string(frame.playerA)};
      //validate number for playerB
      if(frame.playerB < 1 || frame.playerB > num_players)
        throw std::length_error{std::string{"LoLProtocol::receiveMessage 0"} + std::to_string(frame.messageID) + 
                                std::string{": invalid player number "} + std::to_string(frame.playerB)};
      //validate die face for dice[0]
      if(LootOfLima::instance().invalid_roll(frame.dice[0]))
        throw std::invalid_argument{std::string{"LoLProtocol::receiveMessage 0"} + std::to_string(frame.messageID) + 
                                std::string{": 1st- invalid die face returned "} + frame.dice[0]};
      //validate die face for dice[1]
      if(LootOfLima::instance().invalid_roll(frame.dice[1]))
        throw std::invalid_argument{std::string{"LoLProtocol::receiveMessage 0"} + std::to_string(frame.messageID) + 
                                std::string{": 2nd- invalid die face returned "} + frame.dice[0]};
      //validate that chosen terrain type is valid
      char tA=frame.dice[0][2];
      char tB=frame.dice[1][2];
      string possibles;
      if(tA=='W' && tB=='W')//both dice wild
        possibles="MFBA";
      else if(tA=='W')//only first die wild
      { possibles=tB;possibles+='A'; }
      else if(tB=='W')//only second die wild
      { possibles=tA;possibles+='A'; }
      else if(tA==tB)//terrain is same
        possibles=tA;
      else
        possibles=tB;
      auto found=possibles.find(frame.terrain_type);
      if(found==std::string::npos)
        throw std::invalid_argument{std::string{"LoLProtocol::receiveMessage 0"} + std::to_string(frame.messageID) + 
                                std::string{": invalid terrain type picked "} + frame.terrain_type};
      }
      break;
    //*************************
    //*** Player guesses treasure location
    //*** Correct guess
    //*** Last player standing, win by default
    //*************************
    case 7:
    case 8:
    case 11:
      lenread=sscanf(buffer+3, "P%i,%2s,%2s\n",&frame.playerA,frame.location[0],frame.location[1]);
      //validate number for playerA
      if(frame.playerA != playernum)
        throw std::length_error{std::string{"LoLProtocol::receiveMessage 0"} + std::to_string(frame.messageID) + 
                                std::string{": invalid player number. Player "} + std::to_string(playernum) + 
                                  "'s message indicated that they were Player " + std::to_string(frame.playerA)};
      if(LootOfLima::instance().has_location(frame.location[0])==false)
        throw std::length_error{std::string{"LoLProtocol::receiveMessage 0"} + std::to_string(frame.messageID) + string{": First location does not exist: "}+frame.location[0]};
      if(LootOfLima::instance().has_location(frame.location[1])==false)
        throw std::length_error{std::string{"LoLProtocol::receiveMessage 0"} + std::to_string(frame.messageID) + string{": Second location does not exist: "}+frame.location[1]};
      break;
    //*************************
    //*** Incorrect guess, out of game
    //*************************
    case 9:
      lenread=sscanf(buffer+3, "P%i\n",&frame.playerA);
      //validate number for playerA
      if(frame.playerA < 1 || frame.playerA > num_players)
        throw std::length_error{std::string{"LoLProtocol::receiveMessage 0"} + std::to_string(frame.messageID) + 
                                std::string{": invalid player number "} + std::to_string(frame.playerA)};
      break;
    //*************************
    //*** Provide partial location information to each player
    //*************************
    case 10:
      lenread=sscanf(buffer+3, "P%i,P%i,%2s\n",&frame.playerA,&frame.playerB,frame.location[0]);
      //validate number for playerA
      if(frame.playerA < 1 || frame.playerA > num_players)
        throw std::length_error{std::string{"LoLProtocol::receiveMessage 0"} + std::to_string(frame.messageID) + 
                                std::string{": invalid player number "} + std::to_string(frame.playerA)};
      //validate number for playerB
      if(frame.playerB < 1 || frame.playerB > num_players)
        throw std::length_error{std::string{"LoLProtocol::receiveMessage 0"} + std::to_string(frame.messageID) + 
                                std::string{": invalid player number "} + std::to_string(frame.playerB)};
      if(LootOfLima::instance().has_location(frame.location[0])==false)
        throw std::length_error{std::string{"LoLProtocol::receiveMessage 0"} + std::to_string(frame.messageID) + string{": Location does not exist: "}+frame.location[0]};
      break;
    case 99:
      lenread=sscanf(buffer+3, "%200s\n",frame.text);
      break;
  }
  if(lenread<=0)
    throw std::length_error{std::string{"LoLProtocol::receiveMessage "} + std::to_string(frame.messageID) + std::string{": Error in parsing message from named pipe."}};
}

//////////////////////////////////////////////////////////////////////////////////
//////// sendMessage
//////////////////////////////////////////////////////////////////////////////////
void LoLProtocol::sendMessage(int playernum, const LoLProtocol::MessageFrame& frame) const
{
  char buffer[256];
  buffer[0]='\0';
  unsigned int lenwritten=0;
  switch(frame.messageID)
  {
    case 1:
      lenwritten=snprintf(buffer, 256, "01:%02i\n", frame.num_players);
      break;
    case 2:
    case 3:
      {
        if(frame.num_locations < 1 || frame.num_locations > 24)
          throw std::out_of_range{"LoLProtocol::sendMessage(), message 0" + std::to_string(frame.messageID) + ", Illegal number of locations provided: " + std::to_string(frame.num_locations)};
        string location_csv{frame.location[0]};
        for(int i=1; i<frame.num_locations; ++i)
        {
          location_csv+=',';
          location_csv+=frame.location[i];
        }
        if(frame.messageID==2)
          lenwritten=snprintf(buffer, 256, "02:P%i,%s\n", frame.playerA, location_csv.c_str());
        if(frame.messageID==3)
          lenwritten=snprintf(buffer, 256, "03:%s\n", location_csv.c_str());
      }
      break;
    case 4:
      lenwritten=snprintf(buffer, 256, "04:P%i,%3s,%3s,%3s\n",frame.playerA,frame.dice[0],frame.dice[1],frame.dice[2]);
      break;
    case 5:
      lenwritten=snprintf(buffer, 256, "05:%3s,%3s,%c,P%i\n",frame.dice[0],frame.dice[1],frame.terrain_type,frame.playerA);
      break;
    case 6:
      lenwritten=snprintf(buffer, 256, "06:%3s,%3s,%c,%i,P%i,P%i\n",frame.dice[0],frame.dice[1],frame.terrain_type,frame.num_locations,frame.playerA,frame.playerB);
      break;
    case 7:
    case 8:
    case 11:
      lenwritten=snprintf(buffer, 256, "%02i:P%i,%2s,%2s\n",frame.messageID,frame.playerA,frame.location[0],frame.location[1]);
      break;
    case 9:
      lenwritten=snprintf(buffer, 256, "09:P%i\n",frame.playerA);
      break;
    case 10:
      lenwritten=snprintf(buffer, 256, "10:P%i,P%i,%2s\n",frame.playerA,frame.playerB,frame.location[0]);
      break;
    case 99:
      lenwritten=snprintf(buffer, 256, "99:%200s\n",frame.text);
      break;
  }

  if(lenwritten>=256)
  {
    string largestrerror{"LoLProtocol::sendMessage: Message length is unexpectedly large->"};
    largestrerror+=buffer; largestrerror+="<-";
    throw std::length_error{largestrerror};
  }
  if(lenwritten<=0)
    throw std::length_error{"LoLProtocol::sendMessage: Error in composing message to send"};
  lenwritten=write(to_player(playernum), buffer, strlen(buffer));
  if(lenwritten!=strlen(buffer))
    throw std::runtime_error{"LoLProtocol::sendMessage: Error in writing message to named pipe."};
}


LoLProtocol::~LoLProtocol()
{
  for(int i=0; i<num_players; ++i)
  {
    close(fds[2*i]);
    close(fds[2*i+1]);
    string fromfile="/tmp/alphafromP" + std::to_string(i+1);
    string tofile="/tmp/alphatoP" + std::to_string(i+1);
    unlink(tofile.c_str());
    unlink(fromfile.c_str());
  }
}
