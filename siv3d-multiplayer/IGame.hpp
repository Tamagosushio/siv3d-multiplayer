# pragma once
# include <Siv3D.hpp>
# include "Multiplayer_Photon.hpp"

class OnlineManager;

class IGame {
public:
  virtual ~IGame() = default;
  /// @brief ネットワークマネージャーのポインタのセッター
  /// @param network ネットワークマネージャーのインスタンス
  virtual void set_network(OnlineManager* network) = 0;
  /// @brief このゲームの識別子を返す 
  /// @return ゲームID
  virtual String get_game_id(void) const = 0;
  /// @brief このゲームの最大プレイヤー数を返す 
  /// @return 最大プレイヤー数
  virtual uint8 get_max_players(void) const = 0;
  /// @brief ゲームの更新処理
  virtual void update(void) = 0;
  /// @brief ゲームの描画処理
  virtual void draw(void) const = 0;
  /// @brief デバッグ表示
  virtual void debug(void) = 0;
  /* Photonイベントに対応するハンドラ */
  /// @brief ゲーム開始条件が満たされたときに呼ばれる
  /// @param players ルームにいるプレイヤーのリスト
  /// @param isHost 自身がホストであるか
  virtual void on_game_start(const Array<LocalPlayer>& players, bool is_host) = 0;
  /// @brief プレイヤーが退出したときに呼ばれる
  /// @param player_id 退出したプレイヤーのローカルID
  virtual void on_player_left(LocalPlayerID player_id) = 0;
  /// @brief 自身がルームから退出したときに呼ばれる
  virtual void on_leave_room(void) = 0;
  /// @brief カスタムイベント受信時に呼ばれる
  /// @param player_id 送信者のローカルID
  /// @param event_code イベントコード
  /// @param reader 受信したデータ
  virtual void on_event_received(LocalPlayerID player_id, uint8 event_code, Deserializer<MemoryViewReader>& reader) = 0;
  /// @brief ゲームが開始されているか 
  virtual bool is_started(void) const = 0;
  /// @brief ゲームが終了状態か 
  virtual bool is_finished(void) const = 0;
};
