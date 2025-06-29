# pragma once
# include <Siv3D.hpp>
# include "IGame.hpp"
# include "OnlineManager.hpp" // setNetworkでポインタを保持するため

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

  class Game : public IGame {
  private:
    OnlineManager* network_ = nullptr; // ネットワーク層へのポインタ
    Grid<Cell> grid_; // 盤面情報
    size_t cell_size_ = 100; // 1つのセルの1辺の長さ
    Point cell_offset_{ 100, 100 }; // 盤面描画時のオフセット
    Cell player_symbol_ = Cell::None; // このプレイヤーの記号
    bool is_started_ = false; // ゲームが開始されているか
    bool is_turn_ = false; // 自分のターンであるか
    bool is_finished_ = false; // ゲームが終了しているか
    bool is_ready_ = false; // ゲームシーンに遷移可能か
    Optional<Cell> winner_ = none; // ゲームの勝者
    Font font_detail_{ FontMethod::MSDF, 256, Typeface::Bold }; // ゲームに関わるテキストのフォント
    Font font_symbol_{ FontMethod::MSDF, static_cast<int32>(cell_size_), Typeface::Bold }; // 盤面記号描画のフォント
    Point get_cell_point_(const size_t y, const size_t x) const; // セルの左上座標
    Rect get_cell_rect_(const Point& pos) const; // セルの四角形
    Rect get_cell_rect_(const size_t y, const size_t x) const;
    Optional<Operation> get_operation_(void) const;
    void operate_(const Operation& op);
    void calc_result_(void);
  public:
    Game() = default;
    void set_network(OnlineManager* network) override {
      network_ = network;
    }
    String get_game_id(void) const override {
      return U"TicTacToe";
    }
    uint8 get_max_players(void) const override {
      return 2;
    }
    bool is_started(void) const override {
      return is_started_;
    }
    bool is_finished(void) const override {
      return is_finished_;
    }
    void update(void) override;
    void draw(void) const override;
    void debug(void) override;
    void on_game_start(const Array<LocalPlayer>& players, bool is_host) override;
    void on_player_left(LocalPlayerID player_id) override;
    void on_leave_room(void) override;
    void on_event_received(LocalPlayerID player_id, uint8 event_code, Deserializer<MemoryViewReader>& reader) override;
    void initialize(const size_t grid_size, const Cell player_symbol);
    void reset(void);
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
  void Game::operate_(const Operation& op) {
    if (grid_.isEmpty() or is_finished_) return;
    grid_[op.pos] = op.cell_type;
    is_turn_ = (player_symbol_ != op.cell_type);
    const bool is_finished_pre_ = is_finished_;
    calc_result_();
    if (winner_) {
      is_finished_ = true;
    }
    // ゲームが終了状態に切り替わった瞬間を検知
    if (is_finished_ and not is_finished_pre_) {
      // ホストだけがルームを閉じる
      if (network_ and network_->isHost()) {
        network_->set_room_open(false);
        network_->set_room_visible(false);
      }
    }
  }
  Optional<Operation> Game::get_operation_() const {
    if (not (is_started_ and is_turn_ and not is_finished_)) return none;
    for (size_t h : step(grid_.height())) {
      for (size_t w : step(grid_.width())) {
        if (get_cell_rect_(h, w).leftClicked() and grid_.at(h, w) == Cell::None) {
          return Operation{ Point(w,h), player_symbol_ };
        }
      }
    }
    return none;
  }
  void Game::calc_result_(void) {
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
      = check_line([&](size_t i) { return grid_[grid_.height() - 1 - i][i]; }, grid_.height());
    if (result2) {
      winner_ = *result2;
      return;
    }
    // 引き分けチェック（None が残っていないか）
    for (const Point& p : Step2D(Point{ 0,0 }, grid_.size(), Point{ 1,1 })) {
      if (grid_[p] == Cell::None) return;
    }
    winner_ = Cell::None; // 引き分け
  }

  void Game::update() {
    if (Optional<Operation> op = get_operation_()) {
      if (network_) {
        network_->send_game_event(Operation::code, *op);
      }
      operate_(*op);
    }
  }
  void Game::on_game_start(const Array<LocalPlayer>& players, bool is_host) {
    initialize(3, is_host ? Cell::Circle : Cell::Cross);
    if (is_host and network_) network_->set_room_visible(false);
  }
  void Game::on_player_left(LocalPlayerID player_id) {
    // ゲームが既に終了しているなら、状態をリセットしない
    if (is_finished_) return;
    reset();
  }
  void Game::on_leave_room() {
    reset();
  }
  void Game::on_event_received(const LocalPlayerID player_id, const uint8 event_code, Deserializer<MemoryViewReader>& reader) {
    if (event_code == Operation::code) {
      Operation op;
      reader(op);
      operate_(op);
    }
  }
  void Game::initialize(const size_t grid_size, Cell player_symbol) {
    grid_.assign(grid_size, grid_size, Cell::None);
    player_symbol_ = player_symbol;
    is_started_ = true;
    is_turn_ = (player_symbol == Cell::Circle);
    is_finished_ = false;
    winner_ = none;
  }
  void Game::reset() {
    grid_.clear();
    player_symbol_ = Cell::None;
    is_started_ = false;
    is_turn_ = false;
    is_finished_ = false;
    winner_ = none;
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
  void Game::debug(void) {}
};

