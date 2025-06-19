# pragma once

# include <Siv3D.hpp>

namespace TikTakToe {

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
  public:
    Grid<Cell> grid_;
    size_t cell_size_ = 100;
    Point cell_offset_{ 100, 100 };
    Font font_symbol_{ FontMethod::MSDF, static_cast<int32>(cell_size_), Typeface::Bold };
    Cell player_symbol_ = Cell::None;
    Point get_cell_point_(const size_t y, const size_t x) const;
    Rect get_cell_rect_(const Point& pos) const;
    Rect get_cell_rect_(const size_t y, const size_t x) const;
  public:
    Game() {}
    Game(const size_t grid_size, Cell player_simbol);
    void initialize(const size_t grid_size, Cell player_simbol);
    Optional<Cell> operate(const Operation& op);
    Optional<Cell> is_finished(void) const;
    Cell get_symbol(void) const;
    void draw(void) const;
    void update(void);
    Optional<Operation> get_operation(void) const;
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
  }

  Optional<Cell> Game::operate(const Operation& op) {
    grid_[op.pos] = op.cell_type;
    return is_finished();
  }

  Optional<Cell> Game::is_finished(void) const {
    Cell cell_type;
    // 行走査
    for (size_t col : step(grid_.height())) {
      // 行が全てNone以外の同じ要素か判定
      cell_type = grid_[col][0];
      if (cell_type == Cell::None) continue;
      bool row_ok = true;
      for (size_t row : step(grid_.width())) {
        if (cell_type != grid_[col][row]) {
          row_ok = false;
          break;
        }
      }
      if (row_ok) {
        return cell_type;
      }
    }
    // 列走査
    for (size_t row : step(grid_.width())) {
      // 列が全てNone以外の同じ要素か判定
      cell_type = grid_[0][row];
      if (cell_type == Cell::None) continue;
      bool col_ok = true;
      for (size_t col : step(grid_.height())) {
        if (cell_type != grid_[col][row]) {
          col_ok = false;
          break;
        }
      }
      if (col_ok) {
        return cell_type;
      }
    }
    // 対角線走査
    cell_type = grid_[0][0];
    if (cell_type != Cell::None) {
      bool diagonal_ok = true;
      for (size_t idx = 0; idx < grid_.height(); idx++) {
        if (cell_type != grid_[idx][idx]) {
          diagonal_ok = false;
          break;
        }
      }
      if (diagonal_ok) {
        return cell_type;
      }
    }
    cell_type = grid_[grid_.height()-1][0];
    if (cell_type != Cell::None) {
      bool diagonal_ok = true;
      for (size_t idx = 0; idx < grid_.height(); idx++) {
        if (cell_type != grid_[grid_.height()-1-idx][idx]) {
          diagonal_ok = false;
          break;
        }
      }
      if (diagonal_ok) {
        return cell_type;
      }
    }
    // 引き分け判定
    for (size_t col : step(grid_.height())) {
      for (size_t row : step(grid_.width())) {
        if (grid_.at(col, row) == Cell::None) return none;
      }
    }
    return Cell::None;
  }

  Cell Game::get_symbol(void) const {
    return player_symbol_;
  }

  void Game::update(void) {}

  Optional<Operation> Game::get_operation(void) const { 
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
  }

};

