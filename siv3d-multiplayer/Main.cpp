# include <Siv3D.hpp>
# include "TicTacToe.hpp"

void Main() {
  Window::Resize(1280, 720);
  const std::string secretAppID{ SIV3D_OBFUSCATE(PHOTON_APP_ID) };
  TicTacToe::Network network{ secretAppID, U"1.0", Verbose::Yes };

  TextEditState user_name{ U"UserName" };

  while (System::Update()) {
    network.update();    
    network.update_game();
    network.draw();
    //network.debug();

    SimpleGUI::TextBoxAt(user_name, Vec2{ Scene::CenterF().x, 50 }, 200, unspecified, (not network.isActive()));

    if (SimpleGUI::Button(U"Connect", Vec2{ 1000, 20 }, 160, (not network.isActive()))) {
      network.connect(user_name.text, U"jp");
    }
    if (SimpleGUI::Button(U"Disconnect", Vec2{ 1000, 60 }, 160, network.isActive())) {
      network.disconnect();
    }
    if (SimpleGUI::Button(U"Join Room", Vec2{ 1000, 100 }, 160, network.isInLobby())) {
      network.joinRandomRoom(TicTacToe::Network::MaxPlayers);
    }
    if (SimpleGUI::Button(U"Leave Room", Vec2{ 1000, 140 }, 160, network.isInRoom())) {
      network.leaveRoom();
    }

  }
}
