# include <Siv3D.hpp>
# include "TicTacToe.hpp"
# include "MyNetwork.hpp"

void Main() {
  Window::Resize(1280, 720);
  const std::string secretAppID{ SIV3D_OBFUSCATE(PHOTON_APP_ID) };
  MyNetwork network{ secretAppID, U"1.0", Verbose::Yes };

  while (System::Update()) {
    network.update();    
    network.update_game();
    network.draw();
    //network.debug();

    if (SimpleGUI::Button(U"Connect", Vec2{ 1000, 20 }, 160, (not network.isActive()))) {
      const String userName = U"Siv";
      network.connect(userName);
    }
    if (SimpleGUI::Button(U"Disconnect", Vec2{ 1000, 60 }, 160, network.isActive())) {
      network.disconnect();
    }
    if (SimpleGUI::Button(U"Join Room", Vec2{ 1000, 100 }, 160, network.isInLobby())) {
      network.joinRandomRoom(MyNetwork::MaxPlayers);
    }
    if (SimpleGUI::Button(U"Leave Room", Vec2{ 1000, 140 }, 160, network.isInRoom())) {
      network.leaveRoom();
    }

  }
}
