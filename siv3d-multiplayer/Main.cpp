# include <Siv3D.hpp>
# include "TicTacToe.hpp"
# include "SushiGUI.hpp"


const std::string secretAppID{ SIV3D_OBFUSCATE(PHOTON_APP_ID) };
struct SceneData {
  TicTacToe::Network network{ secretAppID, U"1.0", Verbose::Yes };
};

using App = SceneManager<String, SceneData>;

class NetworkScene : public App::Scene {
private:
  TextEditState user_name{ U"UserName" };
  TextEditState room_name{ U"RoomName" };
  const Font font_room_name{ FontMethod::MSDF, 35, Typeface::Bold };
  const Font font{ FontMethod::MSDF, 64, Typeface::Bold };
public:
  NetworkScene(const InitData& init) : IScene{ init } {}

  void update(void) override {
    getData().network.update();
    if (getData().network.is_started() and not getData().network.is_finished()) changeScene(U"GameScene");
    if (not getData().network.isActive()) SimpleGUI::TextBoxAt(user_name, Vec2{ Scene::CenterF().x, 50 }, 200);
    if (getData().network.isInLobby()) SimpleGUI::TextBoxAt(room_name, Vec2{ Scene::CenterF().x, 50 }, 200);
    if (getData().network.isInLobby() and not getData().network.isInRoom()) {
      for (size_t idx = 0; RoomName room_name : getData().network.getRoomNameList()) {
        Vec2 rect_leftup{ 980, 50 * idx };
        Size rect_size{ 250, 50 };
        RectF room_name_rect{ rect_leftup, rect_size };
        room_name_rect.draw(Palette::White);
        room_name_rect.drawFrame(2.5, Palette::Black);
        font_room_name(room_name).draw(room_name_rect.stretched({ -10, 0 }), Palette::Black);
        if (room_name_rect.mouseOver()) {
          room_name_rect.draw(ColorF{ 0.0, 0.5 });
          Cursor::RequestStyle(CursorStyle::Hand);
        }
        if (room_name_rect.leftClicked()) getData().network.joinRoom(room_name);
        idx++;
      }
    }
    // 接続/切断ボタン
    if (getData().network.isActive() and SushiGUI::button1(font, U"Disconnect", Vec2{ 800, 20 }, Vec2{ 150, 50 })) {
      Print << U"disconnect";
      getData().network.disconnect();
    }
    if (not getData().network.isActive() and SushiGUI::button3(font, U"Connect", Vec2{ 800, 20 }, Vec2{ 150, 50 })) {
      Print << U"connect";
      getData().network.connect(user_name.text, U"jp");
    }
    // ルーム作成/退出ボタン
    if (getData().network.isInLobby() and SushiGUI::button3(font, U"Create Room", Vec2{ 800, 80 }, Vec2{ 150, 50 })) {
      getData().network.createRoom(room_name.text, TicTacToe::Network::MaxPlayers);
    }
    if (getData().network.isInRoom() and SushiGUI::button1(font, U"Leave Room", Vec2{ 800, 80 }, Vec2{ 150, 50 })) {
      getData().network.leaveRoom();
    }
  }

  void draw(void) const override {
    getData().network.draw();
  }

};

class GameScene : public App::Scene {
public:
  GameScene(const InitData& init) : IScene{ init } {}
  void update(void) override {
    getData().network.update_game();
    if (getData().network.is_finished()) changeScene(U"NetworkScene");
  }
  void draw(void) const override {
    getData().network.draw();
  }
};

void Main() {
  Window::Resize(1280, 720);

  App manager;
  manager.add<NetworkScene>(U"NetworkScene");
  manager.add<GameScene>(U"GameScene");

  manager.init(U"NetworkScene");

  while (System::Update()) {
    if (not manager.update()) {
      break;
    }
  }
}
