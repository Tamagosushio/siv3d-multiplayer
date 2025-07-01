# pragma once
# include <Siv3D.hpp>
# include "IGame.hpp"
# include "OnlineManager.hpp"

namespace DotsAndBoxes {
  
  enum class LineColor{
    None = 0,
    Red = 1,
    Blue = 2,
  };

  enum class LineDirection{
    Top,
    Left,
  };

  struct Operation {
    constexpr static uint8 code = 42;
    Point pos;
    LineDirection dir;
    LineColor line_color;
    Operation() = default;
    Operation(const Point& pos, const LineDirection& dir, const LineColor& line_color)
      : pos(pos), dir(dir), line_color(line_color) {}
    template<class Archive>
    void SIV3D_SERIALIZE(Archive& archive) {
      archive(pos, dir, line_color);
    }
  };

  class Game : public IGame {
  private:
    OnlineManager* network_ = nullptr;
    Grid<LineColor> horizontal_lines_; // 水平線
    Grid<LineColor> vertical_lines_; // 垂直線
    Grid<LineColor> box_owners_; // 各ボックスはどのプレイヤーのものか管理
    Size grid_size_{ 6,4 }; // 盤面サイズ（セル数）
    int32 cell_size_ = 100; // セルの描画サイズ
    int32 dot_radius_ = 8; // ドットの半径
    int32 line_thickness_ = 10; // セル周りの線の太さ
    Point board_offset_; // 描画のオフセット
    HashTable<LineColor, int32> scores_; // 各プレイヤーの得点
    LineColor player_color_ = LineColor::None; // このプレイヤーの色
    bool is_started_ = false;
    bool is_turn_ = false;
    bool is_finished_ = false;
    Optional<LineColor> winner_ = none; // ゲームの勝者、引き分け時はNone
    Font font_ui_{ FontMethod::MSDF, 30, Typeface::Bold };
    Font font_result_{ FontMethod::MSDF, 80, Typeface::Bold };
    Point get_dot_pos_(int32 y, int32 x) const;
    Optional<Operation> get_operation_(void) const;
    void operate_(const Operation& op);
    void calc_result_(void);
  public:
    Game() {};
    void set_network(OnlineManager* network) override {network_ = network;}
    String get_game_id(void) const override {return U"DotsAndBoxes";}
    uint8 get_max_players(void) const override {return 2;}
    bool is_started(void) const override {return is_started_;}
    bool is_finished(void) const override {return is_finished_;}
    void update(void) override;
    void draw(void) const override;
    void debug(void) override {}
    void on_game_start(const Array<LocalPlayer>& players, bool is_host) override;
    void on_player_left(LocalPlayerID player_id) override;
    void on_leave_room(void) override;
    void on_event_received(LocalPlayerID player_id, uint8 event_code, Deserializer<MemoryViewReader>& reader) override;
    void initialize(const Size& grid_size, const LineColor player_color);
    void reset(void);
  };

  inline Point Game::get_dot_pos_(const int32 y, const int32 x) const {
    return Point{ x * cell_size_, y * cell_size_ } + board_offset_;
  }

  inline void Game::initialize(const Size& grid_size, const LineColor player_color) {
    grid_size_ = grid_size;
    horizontal_lines_.assign(grid_size_.x, grid_size_.y + 1, LineColor::None);
    vertical_lines_.assign(grid_size_.x + 1, grid_size_.y, LineColor::None);
    box_owners_.assign(grid_size_, LineColor::None);
    scores_[LineColor::Red] = 0;
    scores_[LineColor::Blue] = 0;
    player_color_ = player_color;
    is_turn_ = (player_color == LineColor::Red);
    is_started_ = true;
    is_finished_ = false;
    winner_ = none;
    const int32 board_width = grid_size_.x * cell_size_;
    const int32 board_height = grid_size_.y * cell_size_;
    board_offset_ = Scene::Center() - Point{ board_width / 2, board_height / 2 };
  }
  inline void Game::reset(void) {
    horizontal_lines_.clear();
    vertical_lines_.clear();
    box_owners_.clear();
    scores_.clear();
    player_color_ = LineColor::None;
    is_started_ = false;
    is_turn_ = false;
    is_finished_ = false;
    winner_ = none;
  }

  inline void Game::on_game_start(const Array<LocalPlayer>& players, bool is_host) {
    initialize(grid_size_, is_host ? LineColor::Red : LineColor::Blue);
    if (is_host and network_) network_->set_room_visible(false);
  }
  inline void Game::on_player_left(LocalPlayerID player_id) {
    if (is_finished_) return;
    reset();
  }
  inline void Game::on_leave_room(void) {
    reset();
  }
  inline void Game::on_event_received(LocalPlayerID player_id, uint8 event_code, Deserializer<MemoryViewReader>& reader) {
    if (event_code == Operation::code) {
      Operation op;
      reader(op);
      operate_(op);
    }
  }

  inline Optional<Operation> Game::get_operation_(void) const {
    if (not (is_started_ and is_turn_ and not is_finished_)) return none;
    for (int32 y : step(grid_size_.y+1)) {
      for (int32 x : step(grid_size_.x)) {
        if (horizontal_lines_.at(y,x) == LineColor::None) {
          const RectF line_rectf{ get_dot_pos_(y,x), cell_size_, line_thickness_ };
          if (line_rectf.leftClicked()) return Operation{ Point{x,y}, LineDirection::Top, player_color_ };
        }
      }
    }
    for (int32 y : step(grid_size_.y)) {
      for (int32 x : step(grid_size_.x+1)) {
        if (vertical_lines_.at(y, x) == LineColor::None) {
          const RectF line_rectf{ get_dot_pos_(y,x), line_thickness_, cell_size_ };
          if (line_rectf.leftClicked()) return Operation{ Point{x,y}, LineDirection::Left, player_color_ };
        }
      }
    }
    return none;
  }
  inline void Game::operate_(const Operation& op) {
    if (not is_started_ or is_finished_) return;
    bool box_completed_ = false;
    if (op.dir == LineDirection::Top) {
      if (not horizontal_lines_.inBounds(op.pos)) return;
      if (horizontal_lines_.at(op.pos) != LineColor::None) return;
      horizontal_lines_.at(op.pos) = op.line_color;
    } else {
      if (not vertical_lines_.inBounds(op.pos)) return;
      if (vertical_lines_.at(op.pos) != LineColor::None) return;
      vertical_lines_.at(op.pos) = op.line_color;
    }
    for (size_t y : step(grid_size_.y)) {
      for (size_t x : step(grid_size_.x)) {
        if (box_owners_.at(y, x) == LineColor::None) {
          if (horizontal_lines_.at(y, x) != LineColor::None and horizontal_lines_.at(y + 1, x) != LineColor::None
           and vertical_lines_.at(y, x) != LineColor::None and vertical_lines_.at(y, x + 1) != LineColor::None) {
            box_owners_.at(y, x) = op.line_color;
            scores_[op.line_color]++;
            box_completed_ = true;
          }
        }
      }
    }
    if (not box_completed_) is_turn_ = not is_turn_;
    calc_result_();
  }
  inline void Game::calc_result_(void) {
    if ((scores_.at(LineColor::Red) + scores_.at(LineColor::Blue)) == (grid_size_.area())) {
      is_finished_ = true;
      if (scores_.at(LineColor::Red) > scores_.at(LineColor::Blue)) {
        winner_ = LineColor::Red;
      } else if (scores_.at(LineColor::Red) < scores_.at(LineColor::Blue)) {
        winner_ = LineColor::Blue;
      } else {
        winner_ = LineColor::None;
      }
      if (network_ and network_->isHost()) {
        network_->set_room_open(false);
        network_->set_room_visible(false);
      }
    }
  }

  inline void Game::update(void) {
    if (Optional<Operation> op = get_operation_()) {
      if (network_) network_->send_game_event(Operation::code, *op);
      operate_(*op);
    }
  }
  inline void Game::draw(void) const {
    if (not is_started_) return;
    for (int32 y : step(grid_size_.y)) {
      for (int32 x : step(grid_size_.x)) {
        if (box_owners_.at(y, x) != LineColor::None) {
          const ColorF color = (box_owners_[y][x] == LineColor::Red) ? ColorF{ 1.0, 0.5, 0.5, 0.5 } : ColorF{ 0.5, 0.5, 1.0, 0.5 };
          Rect{ get_dot_pos_(y, x), cell_size_ }.draw(color);
        }
      }
    }
    for (int32 y : step(grid_size_.y + 1)) {
      for (int32 x : step(grid_size_.x)) {
        const LineColor color_type = horizontal_lines_.at(y, x);
        if (color_type != LineColor::None) {
          const ColorF line_color = (color_type == LineColor::Red) ? Palette::Red : Palette::Blue;
          Line{ get_dot_pos_(y,x), get_dot_pos_(y, x + 1) }.draw(line_thickness_, line_color.gamma(0.8));
        }
      }
    }
    for (int32 y : step(grid_size_.y)) {
      for (int32 x : step(grid_size_.x + 1)) {
        const LineColor color_type = vertical_lines_.at(y, x);
        if (color_type != LineColor::None) {
          const ColorF line_color = (color_type == LineColor::Red) ? Palette::Red : Palette::Blue;
          Line{ get_dot_pos_(y, x), get_dot_pos_(y + 1, x) }.draw(line_thickness_, line_color.gamma(0.8));
        }
      }
    }
    for (int32 y : step(grid_size_.y + 1)) {
      for (int32 x : step(grid_size_.x + 1)) {
        Circle{ get_dot_pos_(y,x), dot_radius_ }.draw(Palette::Gray);
      }
    }
    font_ui_(U"Red: {}"_fmt(scores_.at(LineColor::Red))).draw(20, Vec2{ 20, 20 }, Palette::Red);
    font_ui_(U"Blue: {}"_fmt(scores_.at(LineColor::Blue))).draw(20, Vec2{ 20, 60 }, Palette::Blue);
    if (not is_finished_) {
      const String turn_text = is_turn_ ? U"Your Turn" : U"Opponent's Turn";
      const ColorF turn_color = is_turn_ ? (player_color_ == LineColor::Red ? Palette::Red : Palette::Blue) : Palette::Dimgray;
      font_ui_(turn_text).draw(Arg::topRight = Scene::Rect().tr().movedBy(-20, 20), turn_color);
    } else {
      const RectF screen_rect = Scene::Rect();
      screen_rect.draw(ColorF{ 0.0, 0.5 });
      if (winner_) {
        if (*winner_ == player_color_) font_result_(U"You Win!").drawAt(screen_rect.center());
        else  font_result_(U"You Lose...").drawAt(screen_rect.center());
      } else font_result_(U"Draw").drawAt(screen_rect.center());
    }
  }

};
