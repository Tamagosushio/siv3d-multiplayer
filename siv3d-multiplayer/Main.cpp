# include <Siv3D.hpp>
# include "OnlineManager.hpp"
# include "TicTacToe.hpp"
# include "PHOTON_APP_ID.SECRET"

const std::string secretAppID{ SIV3D_OBFUSCATE(PHOTON_APP_ID) };

struct GameData {
  // チュートリアルに沿った新しいコンストラクタでインスタンスを生成
  OnlineManager onlineManager{ secretAppID, U"1.0", Verbose::Yes };
  // ゲームロジックのインスタンスを作成
  TicTacToe::Game ticTacToeGame;
  // コンストラクタで、ネットワークとゲームロジックを相互に接続する
  GameData() {
    onlineManager.set_game_handler(&ticTacToeGame);
    ticTacToeGame.set_network(&onlineManager);
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
    auto& manager = getData().onlineManager;
    auto& game = getData().ticTacToeGame;
    // ネットワークの状態を常に更新
    manager.update();
    // ゲームが始まったらGameSceneに遷移
    if (game.is_started() and not game.is_finished()) {
      changeScene(U"GameScene");
    }
    // [接続前] ユーザ名入力と接続ボタン
    if (not manager.isActive()) {
      SimpleGUI::TextBoxAt(user_name, Scene::CenterF().movedBy(0, -100), 250);
      if (SimpleGUI::Button(U"Connect", Scene::CenterF().movedBy(0, -50), 200)) {
        manager.connect(user_name.text, U"jp");
      }
      return; // 接続前は以降の処理を行わない
    }
    // [接続後] 切断ボタン
    if (SimpleGUI::Button(U"Disconnect", { Scene::Width() - 170, 20 }, 150)) {
      manager.disconnect();
    }
    // [ロビー内]
    if (manager.isInLobby()) {
      // ルーム作成
      SimpleGUI::TextBoxAt(room_name, { 200, Scene::Height() - 50 }, 250);
      if (SimpleGUI::Button(U"Create Room", { 470, Scene::Height() - 50 }, 200)){
        if (not room_name.text.isEmpty()) {
          manager.createRoom(room_name.text, game.get_max_players());
        }
      }
      // ルーム一覧表示と参加
      font_title(U"Room List").drawAt(Scene::CenterF().x, 40);
      Vec2 roomListPos{ Scene::CenterF().x - 200, 100 };
      for (const auto& room : manager.getRoomNameList()) {
        const RectF region{ roomListPos, 400, 40 };
        region.draw(ColorF{ 0.9, 0.92, 0.94 });
        if (region.mouseOver()) {
          Cursor::RequestStyle(CursorStyle::Hand);
          region.draw(ColorF{ 0.8, 0.85, 0.9 });
        }
        if (region.leftClicked()) {
          manager.joinRoom(room);
        }
        font(room).draw(region.pos.movedBy(10, 0), Palette::Black);
        roomListPos.y += 45;
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
    auto& manager = getData().onlineManager;
    auto& game = getData().ticTacToeGame;
    // ネットワークの状態を常に更新
    manager.update();
    // ゲームロジックを更新
    game.update();
    // ゲームが終了したらNetworkSceneに戻る
    if (game.is_finished()) {
      // 3秒待ってからシーン遷移
      if (SimpleGUI::Button(U"Back to Lobby", Scene::CenterF().movedBy(0, 200))){
        game.reset(); // ゲーム状態をリセット
        changeScene(U"NetworkScene");
      }
    }
  }
  void draw() const override {
    // ゲームの描画処理を呼び出す
    getData().ticTacToeGame.draw();
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
