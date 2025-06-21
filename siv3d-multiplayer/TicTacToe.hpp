# pragma once

# include <Siv3D.hpp>
# include "Multiplayer_Photon.hpp"
# include "PHOTON_APP_ID.SECRET"

namespace TicTacToe {

  enum class Cell{
    None = 0, // 何も置かれていない
    Circle = 1, // 〇が置かれている
    Cross = 2 // ×が置かれている
  };

  struct Operation {
    constexpr static uint8 code = 67; // データ送受信時のイベントコード
    Point pos; // 操作される位置
    Cell cell_type; // 配置するセルの種類
    Operation() {}
    Operation(const Point& pos, const Cell cell_type)
      : pos(pos), cell_type(cell_type) {}
    template<class Archive>
    void SIV3D_SERIALIZE(Archive& archive) {
      archive(pos, cell_type);
    }
  };

  class Game {
  private:
    Grid<Cell> grid_; // 盤面情報
    size_t cell_size_ = 100; // 1つのセルの1辺の長さ
    Point cell_offset_{ 100, 100 }; // 盤面描画時のオフセット
    Cell player_symbol_ = Cell::None; // このプレイヤーの記号
    bool is_started_ = false; // ゲームが開始されているか
    bool is_turn_ = false; // 自分のターンであるか
    bool is_finished_ = false; // ゲームが終了しているか
    Optional<Cell> winner_ = none; // ゲームの勝者
    Font font_detail_{ FontMethod::MSDF, 256, Typeface::Bold }; // ゲームに関わるテキストのフォント
    Font font_symbol_{ FontMethod::MSDF, static_cast<int32>(cell_size_), Typeface::Bold }; // 盤面記号描画のフォント
    Point get_cell_point_(const size_t y, const size_t x) const; // セルの左上座標
    Rect get_cell_rect_(const Point& pos) const; // セルの四角形
    Rect get_cell_rect_(const size_t y, const size_t x) const;
  public:
    Game() {}
    Game(const size_t grid_size, Cell player_simbol); // コンストラクタ(ゲーム初期化)
    void initialize(const size_t grid_size, Cell player_simbol); // ゲーム初期化
    void reset(void); // ゲームリセット
    Optional<Cell> operate(const Operation& op); // 盤面操作
    void calc_result(void); // 勝者の計算
    Cell get_symbol(void) const; // プレイヤーシンボルのゲッター
    void draw(void) const; // 盤面やテキストの描画
    void update(void); // 盤面の更新
    Optional<Operation> get_operation(void) const; // 操作情報を取得
    void debug(void); // デバッグ表示
  };
  Point Game::get_cell_point_(const size_t y, const size_t x) const {
    return Point(x * cell_size_, y * cell_size_) + cell_offset_;
  }
  Rect Game::get_cell_rect_(const Point& pos) const {
    return get_cell_rect_(pos.y, pos.x);
  }
  Rect Game::get_cell_rect_(const size_t y, const size_t x) const {
    return Rect{get_cell_point_(y, x), cell_size_};
  }
  Game::Game(const size_t grid_size, Cell player_symbol) {
    initialize(grid_size, player_symbol);
  }
  void Game::initialize(const size_t grid_size, Cell player_symbol) {
    grid_.clear();
    grid_.resize(grid_size, grid_size, Cell::None);
    player_symbol_ = player_symbol;
    is_started_ = true;
    is_turn_ = (player_symbol_ == Cell::Circle);
    is_finished_ = false;
    winner_ = none;
  }
  void Game::reset(void) {
    grid_.clear();
    player_symbol_ = Cell::None;
    is_started_ = false;
    is_turn_ = false;
    is_finished_ = false;
    winner_ = none;
  }
  Optional<Cell> Game::operate(const Operation& op) {
    grid_[op.pos] = op.cell_type;
    is_turn_ = (player_symbol_ != op.cell_type);
    calc_result();
    if (winner_) is_finished_ = true;
    return winner_;
  }
  void Game::calc_result(void) {
    std::function<Optional<Cell>(std::function<Cell(size_t)>, size_t)> check_line
      = [](std::function<Cell(size_t)> get_cell, size_t count) -> Optional<Cell> {
        Cell first = get_cell(0);
        if (first == Cell::None) return none;
        for (size_t i = 1; i < count; i++) {
          if (get_cell(i) != first) return none;
        }
        return first;
      };
    // 行チェック
    for (size_t col : step(grid_.height())) {
      Optional<Cell> result
        = check_line([&](size_t i) { return grid_[col][i]; }, grid_.width());
      if (result) {
        winner_ = *result;
        return;
      }
    }
    // 列チェック
    for (size_t row : step(grid_.width())) {
      Optional<Cell> result
        = check_line([&](size_t i) {return grid_[i][row]; }, grid_.height());
      if (result) {
        winner_ = *result;
        return;
      }
    }
    // 左上から右下
    Optional<Cell> result1
      = check_line([&](size_t i) { return grid_[i][i]; }, grid_.height());
    if (result1) {
      winner_ = *result1;
      return;
    }
    // 右上から左下
    Optional<Cell> result2
      = check_line([&](size_t i) { return grid_[grid_.height() -1 - i][i]; }, grid_.height());
    if (result2) {
      winner_ = *result2;
      return;
    }
    // 引き分けチェック（None が残っていないか）
    for (const Point& p : Step2D(Point{ 0,0 }, grid_.size(), Point{1,1})) {
      if (grid_[p] == Cell::None) return;
    }
    winner_ = Cell::None; // 引き分け
  }
  Cell Game::get_symbol(void) const {
    return player_symbol_;
  }
  void Game::update(void) {}
  Optional<Operation> Game::get_operation(void) const {
    if (not (is_started_ and is_turn_ and not is_finished_)) return none;
    for (size_t h = 0; h < grid_.height(); h++) {
      for (size_t w = 0; w < grid_.width(); w++) {
        if (get_cell_rect_(h, w).leftClicked() and grid_.at(h,w) == Cell::None) {
          return Operation{ Point(w,h), player_symbol_ };
        }
      }
    }
    return none;
  }
  void Game::draw(void) const {
    if (not is_started_) return;
    for (size_t h = 0; h < grid_.height(); h++) {
      for (size_t w = 0; w < grid_.width(); w++) {
        // グリッドを描画
        get_cell_rect_(h, w).draw(Palette::Black).drawFrame(5, Palette::White);
        // セルの種類に応じた描画
        Cell cell_type = grid_[h][w];
        if (cell_type != Cell::None) {
          font_symbol_(cell_type == Cell::Circle ? U"O" : U"X").drawAt(
            cell_size_ * 0.8,
            get_cell_point_(h, w) + Point(cell_size_, cell_size_)/2
          );
        }
      }
    }
    Vec2 draw_text_center{ Vec2{ Scene::Center().x, Scene::Center().y * 1.5 } };
    if (is_finished_) {
      font_detail_(
        winner_ == Cell::None
          ? U"Draw"
          : winner_ == player_symbol_
            ? U"You Win!"
            : U"You Lose"
      ).drawAt(
        128,
        draw_text_center,
        winner_ == Cell::None
          ? Palette::Black
          : winner_ == player_symbol_
            ? Palette::Red
            : Palette::Blue
      );
    } else {
      font_detail_(
        is_turn_ ? U"Your Turn!" : U"Not Your Turn"
      ).drawAt(
        128,
        draw_text_center,
        is_turn_ ? Palette::Red : Palette::Blue
      );
    }
  }
  void Game::debug(void) {
    Console << U"--------------------";
    for (size_t h = 0; h < grid_.height(); h++) {
      String str = U"";
      for (size_t w = 0; w < grid_.width(); w++) {
        str += (grid_[h][w] == Cell::None ? U"." : grid_[h][w] == Cell::Circle ? U"O" : U"X");
      }
      Console << str;
    }
  }

  class Network : public Multiplayer_Photon {
  public:
    static constexpr int32 MaxPlayers = 2;
    using Multiplayer_Photon::Multiplayer_Photon;
    void update_game(void); // ゲーム情報を更新
    void draw(void) const; // 描画
    void debug(void); // デバッグ表示
  private:
    Array<LocalPlayer> m_localPlayers;
    Game game; // ゲーム情報
    // サーバー接続を試みた結果を処理する関数
    void connectReturn(
      [[maybe_unused]] const int32 errorCode,
      const String& errorString,
      const String& region,
      [[maybe_unused]] const String& cluster
    ) override;
    // サーバー切断時呼ばれる関数
    void disconnectReturn() override;
    // 既存のランダムルームに参加を試みた結果を処理する関数
    void joinRandomRoomReturn(
      [[maybe_unused]] const LocalPlayerID playerID,
      const int32 errorCode,
      const String& errorString
    ) override;
    // ルーム作成を試みた結果を処理する関数
    void createRoomReturn(
      [[maybe_unused]] const LocalPlayerID playerID,
      const int32 errorCode,
      const String& errorString
    ) override;
    // 誰かが現在のルームに参加したときに呼ばれる関数
    void joinRoomEventAction(
      const LocalPlayer& newPlayer,
      [[maybe_unused]] const Array<LocalPlayerID>& playerIDs,
      const bool isSelf
    ) override;
    // 誰かが現在のルームから退出したときに呼ばれる関数
    void leaveRoomEventAction(
      const LocalPlayerID playerID,
      [[maybe_unused]] const bool isInactive
    ) override;
    // 自身が現在のルームから退出したときに呼ばれる関数
    void leaveRoomReturn(
      int32 errorCode,
      const String& errorString
    ) override;
    // シリアライズデータを受信したときに呼ばれる関数
    void customEventAction(
      const LocalPlayerID playerID,
      const uint8 eventCode,
      Deserializer<MemoryViewReader>& reader
    ) override;

  };

  void Network::update_game(void) {
    game.update();
    Optional<TicTacToe::Operation> op = game.get_operation();
    if (op) {
      sendEvent(
        TicTacToe::Operation::code,
        Serializer<MemoryWriter>{}(*op)
      );
      if (m_verbose) {
        Print << U"<<< TikTakToe::Operation(" << (*op).pos << U", " << static_cast<int>((*op).cell_type) << U") を送信";
      }
      game.operate(*op);
    }
  }
  void Network::draw(void) const {
    game.draw();
  }
  void Network::debug(void) {
    game.debug();
  }

  void Network::connectReturn(
    [[maybe_unused]] const int32 errorCode,
    const String& errorString,
    const String& region,
    [[maybe_unused]] const String& cluster
  ) {
    if (m_verbose) {
      Print << U"Network::connectReturn() [サーバへの接続を試みた結果を処理する]";
    }
    if (errorCode) {
      if (m_verbose) {
        Print << U"[サーバへの接続に失敗] " << errorString;
      }
      return;
    }
    if (m_verbose) {
      Print << U"[サーバへの接続に成功]";
      Print << U"[region: {}]"_fmt(region);
      Print << U"[ユーザ名: {}]"_fmt(getUserName());
      Print << U"[ユーザ ID: {}]"_fmt(getUserID());
    }
    Scene::SetBackground(ColorF{ 0.4, 0.5, 0.6 });
  }
  void Network::disconnectReturn() {
    if (m_verbose) {
      Print << U"Network::disconnectReturn() [サーバから切断したときに呼ばれる]";
    }
    m_localPlayers.clear();
    Scene::SetBackground(Palette::DefaultBackground);
  }
  void Network::joinRandomRoomReturn(
    [[maybe_unused]] const LocalPlayerID playerID,
    const int32 errorCode,
    const String& errorString
  ) {
    if (m_verbose) {
      Print << U"Network::joinRandomRoomReturn() [既存のランダムなルームに参加を試みた結果を処理する]";
    }
    if (errorCode == NoRandomMatchFound) {
      const RoomName roomName = (getUserName() + U"'s room-" + ToHex(RandomUint32()));
      if (m_verbose) {
        Print << U"[参加可能なランダムなルームが見つからなかった]";
        Print << U"[自分でルーム " << roomName << U" を新規作成する]";
      }
      createRoom(roomName, MaxPlayers);
      return;
    }
    else if (errorCode) {
      if (m_verbose) {
        Print << U"[既存のランダムなルームへの参加でエラーが発生] " << errorString;
      }
      return;
    }
    if (m_verbose) {
      Print << U"[既存のランダムなルームに参加できた]";
    }
  }
  void Network::createRoomReturn(
    [[maybe_unused]] const LocalPlayerID playerID,
    const int32 errorCode,
    const String& errorString
  ) {
    if (m_verbose) {
      Print << U"Network::createRoomReturn() [ルームを新規作成した結果を処理する]";
    }
    if (errorCode) {
      if (m_verbose) {
        Print << U"[ルームの新規作成でエラーが発生] " << errorString;
      }
      return;
    }
    if (m_verbose) {
      Print << U"[ルーム " << getCurrentRoomName() << U" の作成に成功]";
    }
  }
  void Network::joinRoomEventAction(
    const LocalPlayer& newPlayer,
    [[maybe_unused]] const Array<LocalPlayerID>& playerIDs,
    const bool isSelf
  ) {
    if (m_verbose) {
      Print << U"Network::joinRoomEventAction() [誰か（自分を含む）が現在のルームに参加したときに呼ばれる]";
    }
    m_localPlayers = getLocalPlayers();
    if (m_verbose) {
      Print << U"[{} (ID: {}) がルームに参加した。ローカル ID: {}] {}"_fmt(newPlayer.userName, newPlayer.userID, newPlayer.localID, (isSelf ? U"(自分自身)" : U""));
      Print << U"現在の " << getCurrentRoomName() << U" のルームメンバー";
      for (const auto& player : m_localPlayers) {
        Print << U"- [{}] {} (id: {}) {}"_fmt(player.localID, player.userName, player.userID, player.isHost ? U"(host)" : U"");
      }
    }
    if (m_localPlayers.size() == MaxPlayers) {
      game.initialize(3, isHost() ? TicTacToe::Cell::Circle : TicTacToe::Cell::Cross);
    }
  }
  void Network::leaveRoomEventAction(
    const LocalPlayerID playerID,
    [[maybe_unused]] const bool isInactive
  ) {
    if (m_verbose) {
      Print << U"Network::leaveRoomEventAction() [誰かがルームから退出したら呼ばれる]";
    }
    m_localPlayers = getLocalPlayers();
    if (m_verbose) {
      for (const auto& player : m_localPlayers) {
        if (player.localID == playerID) {
          Print << U"[{} (ID: {}, ローカル ID: {}) がルームから退出した]"_fmt(player.userName, player.userID, player.localID);
        }
      }
      Print << U"現在の " << getCurrentRoomName() << U" のルームメンバー";
      for (const auto& player : m_localPlayers) {
        Print << U"- [{}] {} (ID: {}) {}"_fmt(player.localID, player.userName, player.userID, player.isHost ? U"(host)" : U"");
      }
    }
    game.reset();
  }
  void Network::leaveRoomReturn(
    int32 errorCode,
    const String& errorString
  ) {
    if (m_verbose) {
      Print << U"Network::leaveRoomReturn() [ルームから退出したときに呼ばれる]";
    }
    m_localPlayers.clear();
    if (errorCode) {
      if (m_verbose) {
        Print << U"[ルームからの退出でエラーが発生] " << errorString;
      }
      return;
    }
    game.reset();
  }
  void Network::customEventAction(
    const LocalPlayerID playerID,
    const uint8 eventCode,
    Deserializer<MemoryViewReader>& reader
  ) {
    if (eventCode == TicTacToe::Operation::code) {
      TicTacToe::Operation op;
      reader(op);
      if (m_verbose) {
        Print << U"<<< [" << playerID << U"] からの TikTakToe::Operation(" << op.pos << U", " << static_cast<int>(op.cell_type) << U") を受信";
      }
      game.operate(op);
    }
  }

};

