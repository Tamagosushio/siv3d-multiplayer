# include <Siv3D.hpp>
# include "TicTacToe.hpp"
# include "SushiGUI.hpp"

void Main() {
  Window::Resize(1280, 720);
  const std::string secretAppID{ SIV3D_OBFUSCATE(PHOTON_APP_ID) };
  TicTacToe::Network network{ secretAppID, U"1.0", Verbose::Yes };

  TextEditState user_name{ U"UserName" };
  TextEditState room_name{ U"RoomName" };
  const Font font_room_name{ FontMethod::MSDF, 35, Typeface::Bold };
  const Font font{ FontMethod::MSDF, 64, Typeface::Bold };

  while (System::Update()) {
    network.update();    
    network.update_game();
    network.draw();
    //network.debug();

    if (network.is_started() and not network.is_finished()) continue;

    if (not network.isActive()) SimpleGUI::TextBoxAt(user_name, Vec2{ Scene::CenterF().x, 50 }, 200);
    if (network.isInLobby()) SimpleGUI::TextBoxAt(room_name, Vec2{ Scene::CenterF().x, 50 }, 200);

    if (network.isInLobby() and not network.isInRoom()) {
      for (size_t idx = 0; RoomName room_name : network.getRoomNameList()) {
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
        if (room_name_rect.leftClicked()) network.joinRoom(room_name);
        idx++;
      }
    }

    // 接続/切断ボタン
    if (network.isActive() and SushiGUI::button1(font, U"Disconnect", Vec2{ 800, 20 }, Vec2{ 150, 50 })) {
      Print << U"disconnect";
      network.disconnect();
    }
    if (not network.isActive() and SushiGUI::button3(font, U"Connect", Vec2{ 800, 20 }, Vec2{ 150, 50 })) {
      Print << U"connect";
      network.connect(user_name.text, U"jp");
    }
    // ルーム作成/退出ボタン
    if (network.isInLobby() and SushiGUI::button3(font, U"Create Room", Vec2{ 800, 80 }, Vec2{ 150, 50 })) {
      network.createRoom(room_name.text, TicTacToe::Network::MaxPlayers);
    }
    if (network.isInRoom() and SushiGUI::button1(font, U"Leave Room", Vec2{ 800, 80 }, Vec2{ 150, 50 })) {
      network.leaveRoom();
    }

  }
}
