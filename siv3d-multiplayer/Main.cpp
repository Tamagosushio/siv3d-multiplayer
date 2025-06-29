# include <Siv3D.hpp>
# include "OnlineManager.hpp"
# include "TicTacToe.hpp"
# include "PHOTON_APP_ID.SECRET"
# include "SushiGUI.hpp"

const std::string secretAppID{ SIV3D_OBFUSCATE(PHOTON_APP_ID) };

// インスタンス生成のファクトリ型定義
using GameFactory = std::function<std::unique_ptr<IGame>()>;

struct GameInfo {
  GameFactory factory;
  uint8 max_players;
};

struct GameData {
  // チュートリアルに沿った新しいコンストラクタでインスタンスを生成
  OnlineManager online_manager{ secretAppID, U"1.0", Verbose::Yes };
  // ゲームロジックのインスタンスを保持するポインタ
  std::unique_ptr<IGame> current_game = nullptr;
  // 利用するゲームIDと、そのインスタンスを生成するファクトリの対応
  HashTable<String, GameInfo> game_infos;
  // ロビーでプレイヤーが選択しているゲームID
  Optional<String> selected_game_id;
  // コンストラクタ
  GameData() {
    game_infos[U"TicTacToe"] = {
      []() { return std::make_unique<TicTacToe::Game>(); },
      std::make_unique<TicTacToe::Game>()->get_max_players()
    };
  }
  void create_game_instance(const String& game_id) {
    if (not game_infos.contains(game_id)) return;
    current_game = game_infos[game_id].factory();
    online_manager.set_game_handler(current_game.get());
    current_game->set_network(&online_manager);
  }
  void reset_game_instance(void) {
    current_game.reset();
    online_manager.set_game_handler(nullptr);
    selected_game_id.reset();
  }
};

using App = SceneManager<String, GameData>;

class NetworkScene : public App::Scene {
private:
  TextEditState user_name{ U"UserName" };
  TextEditState room_name{ U"RoomName" };
  Font font{ FontMethod::MSDF, 32, Typeface::Bold };
  Font font_small{ FontMethod::MSDF, 24, Typeface::Bold };
  Font font_title{ FontMethod::MSDF, 48, Typeface::Bold };
public:
  NetworkScene(const InitData& init) : IScene{ init } {}

  void update() override {
    auto& manager = getData().online_manager;
    auto& game_data = getData();
    // ネットワークの状態を常に更新
    manager.update();
    // ゲームが始まったらGameSceneに遷移
    if (game_data.current_game and game_data.current_game->is_started()) {
      changeScene(U"GameScene");
      return;
    }
    if (manager.isInRoom() and not game_data.current_game) {
      const RoomName current_room_name = manager.getCurrentRoomName();
      if (const Optional<String> game_id = RoomNameHelper::get_game_id(current_room_name)) {
        if (game_data.game_infos.contains(*game_id)) {
          if (manager.getPlayerCountInCurrentRoom() == game_data.game_infos[*game_id].max_players) {
            game_data.create_game_instance(*game_id);
            if (game_data.current_game) {
              game_data.current_game->on_game_start(manager.getLocalPlayers(), manager.isHost());
            }
          }
        }
      }
    }
    // [接続前] ユーザ名入力と接続ボタン
    if (not manager.isActive()) {
      SimpleGUI::TextBoxAt(user_name, Scene::CenterF().movedBy(0, -100), 250);
      if (SushiGUI::button3(font_title, U"Connect", Arg::center( Scene::CenterF() ), Vec2{ 200, 50 })) {
        if (not user_name.text.isEmpty()) {
          manager.connect(user_name.text, U"jp");
        }
      }
      return; // 接続前は以降の処理を行わない
    }
    // [接続後] 切断ボタン
    if (manager.isActive() and SushiGUI::button1(font, U"Disconnect", { Scene::Width() - 170, 20 }, Vec2{150, 40})) {
      manager.disconnect();
      game_data.reset_game_instance();
    }
    // [ロビー内]
    if (manager.isInLobby()) {
      font_title(U"Select Game").drawAt(Scene::CenterF().x, 60);
      Vec2 game_select_pos{ Scene::CenterF().x - 250, 100 };
      for (const auto& pair : game_data.game_infos) {
        const String& game_id = pair.first;
        const RectF region{ game_select_pos, 500, 40 };
        if (game_data.selected_game_id and *game_data.selected_game_id == game_id) {
          region.draw(Palette::Orange);
        }
        if (region.mouseOver()) {
          Cursor::RequestStyle(CursorStyle::Hand);
          region.draw(ColorF{ 0.9 });
        }
        if (region.leftClicked()) {
          game_data.selected_game_id = game_id;
        }
        region.drawFrame(2);
        font(game_id).drawAt(region.center(), Palette::Black);
        game_select_pos.y += 45;
      }
      if (not game_data.selected_game_id) return;
      // ルーム作成
      const Vec2 lobby_action_pos{ 20, Scene::Height() - 60 };
      SimpleGUI::TextBoxAt(room_name, lobby_action_pos.movedBy(180, 0), 250);
      if (SushiGUI::button3(font, U"Create Room", lobby_action_pos.movedBy(400, 0), Vec2{ 200, 50 })) {
        if (not room_name.text.isEmpty()) {
          game_data.create_game_instance(*game_data.selected_game_id);
          if (game_data.current_game) {
            manager.create_game_room(room_name.text, game_data.current_game->get_max_players(), *game_data.selected_game_id);
          }
        }
      }
      // ランダムマッチ
      if (SushiGUI::button4(font, U"Join Random", lobby_action_pos.movedBy(640, 0), Vec2{200, 50})) {
        game_data.create_game_instance(*game_data.selected_game_id);
        if (game_data.current_game) {
          manager.join_random_game_room(*game_data.selected_game_id);
        }
      }
      // ルーム一覧表示と参加
      font_title(U"Room List").drawAt(Scene::CenterF().x, 200);
      Vec2 room_list_pos{ Scene::CenterF().x - 300, 250 };
      for (const auto& room_name : manager.getRoomNameList()) {
        const RectF region{ room_list_pos, 600, 40 };
        if (SushiGUI::button4(font_title, room_name, region)) {
          if (auto game_id = RoomNameHelper::get_game_id(room_name)) {
            game_data.create_game_instance(*game_id);
            if (game_data.current_game) manager.joinRoom(room_name);
          }
        }
        room_list_pos.y += 40;
      }
    }
  }
  void draw() const override {}
};

// ゲーム本体のシーン
class GameScene : public App::Scene {
public:
  GameScene(const InitData& init) : IScene{ init } {}
  void update() override {
    auto& manager = getData().online_manager;
    auto& game = getData().current_game;
    if (not game) {
      changeScene(U"NetworkScene");
      return;
    }
    // ネットワークの状態を常に更新
    manager.update();
    // ゲームロジックを更新
    game->update();
    // ゲームが終了したらNetworkSceneに戻る
    if (game->is_finished()) {
      if (SimpleGUI::Button(U"Back to Lobby", Scene::CenterF().movedBy(0, 200))){
        manager.leaveRoom();
        getData().reset_game_instance();
        changeScene(U"NetworkScene");
      }
    }
  }
  void draw() const override {
    // ゲームの描画処理を呼び出す
    if (getData().current_game) {
      getData().current_game->draw();
    }
  }
};


void Main() {
  Window::Resize(1280, 720);

  // SceneManagerを生成
  App manager;

  // シーンを登録
  manager.add<NetworkScene>(U"NetworkScene");
  manager.add<GameScene>(U"GameScene");
  manager.init(U"NetworkScene");

  while (System::Update()) {
    if (not manager.update()) {
      break;
    }
  }
}
