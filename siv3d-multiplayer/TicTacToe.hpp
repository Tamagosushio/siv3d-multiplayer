# pragma once

# include <Siv3D.hpp>

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
    Grid<Cell> grid_;
    size_t cell_size_ = 100;
    Point cell_offset_{ 100, 100 };
    Cell player_symbol_ = Cell::None;
    bool is_started_ = false;
    bool is_turn_ = false;
    bool is_finished_ = false;
    Optional<Cell> winner_ = none;
    Font font_detail_{ FontMethod::MSDF, 256, Typeface::Bold };
    Font font_symbol_{ FontMethod::MSDF, static_cast<int32>(cell_size_), Typeface::Bold };
    Point get_cell_point_(const size_t y, const size_t x) const;
    Rect get_cell_rect_(const Point& pos) const;
    Rect get_cell_rect_(const size_t y, const size_t x) const;
  public:
    Game() {}
    Game(const size_t grid_size, Cell player_simbol);
    void initialize(const size_t grid_size, Cell player_simbol);
    void reset(void);
    Optional<Cell> operate(const Operation& op);
    void calc_result(void);
    Cell get_symbol(void) const;
    void draw(void) const;
    void update(void);
    Optional<Operation> get_operation(void) const;
    void debug(void);
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
};

