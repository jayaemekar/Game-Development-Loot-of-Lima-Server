#ifndef LOLPROTOCOL 
#define LOLPROTOCOL 

class LoLProtocol {
  //
  //Structure for messages
  //
  public:
    struct MessageFrame {
      int messageID;
      int playerA;
      int playerB;
      union {
        int num_players;
        int num_locations;
      };
      char dice[3][4];//includes null character
      char location[24][3];//includes null character
      char terrain_type;
      char text[200];
    };
  public:
    LoLProtocol(int num_players);
    void receiveMessage(int playernum, LoLProtocol::MessageFrame& frame) const;
    void sendMessage(int playernum, const LoLProtocol::MessageFrame& frame) const;
    ~LoLProtocol();
  private:
    int from_player(int playernum) const { return fds[2*(playernum-1)];}
    int to_player(int playernum) const { return fds[2*(playernum-1)+1];}
    int num_players;
    int fds[10];

};
#endif
