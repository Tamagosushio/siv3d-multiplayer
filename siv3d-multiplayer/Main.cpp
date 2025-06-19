# include <Siv3D.hpp>
# include "TicTacToe.hpp"
# include "MyNetwork.hpp"

void Main() {
  Window::Resize(1280, 720);
  const std::string secretAppID{ SIV3D_OBFUSCATE(PHOTON_APP_ID) };
  MyNetwork network{ secretAppID, U"1.0", Verbose::Yes };

  TikTakToe::Game game{ 3, TikTakToe::Cell::Circle };
  TikTakToe::Cell n = TikTakToe::Cell::None;
  TikTakToe::Cell o = TikTakToe::Cell::Circle;
  TikTakToe::Cell x = TikTakToe::Cell::Cross;
  game.grid_ = {
    {o, o, x},
    {x, x, o},
    {n, o, o}
  };

  while (System::Update()) {
    game.update();
    game.draw();
    network.update();
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
    if (SimpleGUI::Button(U"Send int32", Vec2{ 1000, 180 }, 200, network.isInRoom())) {
      const int32 n = Random(0, 10000);
      Print << U"eventCode: 0, int32(" << n << U") を送信 >>>";
      network.sendEvent(0, n);
    }
    if (SimpleGUI::Button(U"Send String", Vec2{ 1000, 220 }, 200, network.isInRoom())) {
      const String s = Sample({ U"Hello!", U"Thank you!", U"Nice!" });
      Print << U"eventCode: 0, String(" << s << U") を送信 >>>";
      network.sendEvent(0, s);
    }
    if (SimpleGUI::Button(U"Send Point", Vec2{ 1000, 260 }, 200, network.isInRoom())) {
      const Point pos = RandomPoint(Rect{ 1280, 720 });
      Print << U"eventCode: 0, Point" << pos << U" を送信 >>>";
      network.sendEvent(0, pos);
    }
    if (SimpleGUI::Button(U"Send Array<int32>", Vec2{ 1000, 300 }, 200, network.isInRoom())) {
      Array<int32> v(3);
      for (auto& n : v) {
        n = Random(0, 1000);
      }
      Print << U"eventCode: 0, Array<int32>" << v << U" を送信 >>>";
      network.sendEvent(0, v);
    }
    if (SimpleGUI::Button(U"Send Array<String>", Vec2{ 1000, 340 }, 200, network.isInRoom())) {
      Array<String> words(3);
      for (auto& word : words) {
        word = Sample({ U"apple", U"bird", U"cat", U"dog" });
      }
      Print << U"eventCode: 0, Array<String>" << words << U" を送信 >>>";
      network.sendEvent(0, words);
    }
  }
}
