# pragma once
# include <Siv3D.hpp>
# include "Multiplayer_Photon.hpp"
# include "IGame.hpp"

namespace RoomNameHelper {
  inline String create(const String& base_name, const String& game_id) {
    return U"[{}]"_fmt(game_id) + base_name;
  }
  inline Optional<String> get_game_id(const String& room_name) {
    if(room_name.starts_with(U'[') and room_name.contains(U']')) {
      return room_name.substr(1, room_name.indexOf(U']') - 1);
    }
    return none;
  }
  inline String get_base_name(const String& room_name) {
    if (room_name.starts_with(U'[') and room_name.contains(U']')) {
      return room_name.substr(room_name.indexOf(U']') + 1);
    }
    return room_name;
  }
}

class OnlineManager : public Multiplayer_Photon {
private:
  IGame* game_handler_ = nullptr;
  Array<LocalPlayer> local_players_;
  /* Photonのオーバーライド */
  void connectReturn(int32 errorCode, const String& errorString, const String& region, const String& cluster) override;
  void disconnectReturn() override;
  void joinRandomRoomReturn(LocalPlayerID playerID, int32 errorCode, const String& errorString) override;
  void createRoomReturn(LocalPlayerID playerID, int32 errorCode, const String& errorString) override;
  void joinRoomEventAction(const LocalPlayer& newPlayer, const Array<LocalPlayerID>& playerIDs, bool isSelf) override;
  void leaveRoomEventAction(LocalPlayerID playerID, bool isInactive) override;
  void leaveRoomReturn(int32 errorCode, const String& errorString) override;
  void customEventAction(LocalPlayerID playerID, uint8 eventCode, Deserializer<MemoryViewReader>& reader) override;
public:
  using Multiplayer_Photon::Multiplayer_Photon;
  /// @brief ルームの公開状態を設定するラッパー
  void set_room_visible(bool is_visible) {
    if (isInRoom()) setIsVisibleInCurrentRoom(is_visible);
  }
  /// @brief ルームの参加可否を設定するラッパー
  void set_room_open(bool is_open) {
    if (isInRoom()) setIsOpenInCurrentRoom(is_open);
  }
  /// @brief このネットワークマネージャが通知を送る先のゲームハンドラを設定
  void set_game_handler(IGame* handler) {
    game_handler_ = handler;
  }
  /// @brief ゲームイベントを送信する
  template<class T>
  void send_game_event(const uint8 event_code, const T& data) {
    sendEvent(event_code, Serializer<MemoryWriter>{}(data));
  }
  /// @brief ゲームIDを付与してルームを作成する
  void create_game_room(const String& room_name, uint8 max_players, const String& game_id) {
    const String prefixed_room_name = RoomNameHelper::create(room_name, game_id);
    Multiplayer_Photon::createRoom(prefixed_room_name, max_players);
  }
  /// @brief ゲームIDを指定してランダムなルールに参加、無ければ作成
  void join_random_game_room(const String& game_id) {
    if (not game_handler_) {
      if (m_verbose) Print << U"[エラー] GameHandlerが未設定";
      return;
    }
    const Array<RoomName> room_list = getRoomNameList();
    Array<String> candidate_rooms;
    for (const RoomName& room : room_list) {
      if (RoomNameHelper::get_game_id(room) == game_id) {
        candidate_rooms.push_back(room);
      }
    }
    if (candidate_rooms.empty()) {
      if (m_verbose) Print << U"[参加可能なルームが見つからず、新規作成します]";
      const RoomName new_room_name = (getUserName() + U"'s room-" + ToHex(RandomUint32()));
      create_game_room(new_room_name, game_handler_->get_max_players(), game_id);
    } else {
      joinRoom(candidate_rooms.choice());
    }
  }
  void debug(void) const {};
};

inline void OnlineManager::connectReturn(
  [[maybe_unused]] const int32 errorCode, const String& errorString,
  const String& region, [[maybe_unused]] const String& cluster) {

  if (m_verbose) Print << U"OnlineManager::connectReturn()";
  if (errorCode) {
    if (m_verbose) Print << U"[サーバへの接続に失敗] " << errorString;
    return;
  }
  if (m_verbose) {
    Print << U"[サーバへの接続に成功] region: " << region;
    Print << U"ユーザ名: {}, ユーザ ID: {}"_fmt(getUserName(), getUserID());
  }
}

inline void OnlineManager::disconnectReturn() {
  if (m_verbose) Print << U"OnlineManager::disconnectReturn()";
  local_players_.clear();
}

inline void OnlineManager::joinRandomRoomReturn(
  [[maybe_unused]] const LocalPlayerID playerID, const int32 errorCode, const String& errorString) {

  if (m_verbose) Print << U"OnlineManager::joinRandomRoomReturn()";
  if (errorCode == NoRandomMatchFound) {
    if (m_verbose) Print << U"[参加可能なルームが見つからず、新規作成します]";
    // ゲームハンドラが設定されていなければルームは作れない
    if (not game_handler_) {
      if (m_verbose) Print << U"[エラー] GameHandlerが未設定のため、ルームを作成できません。";
      return;
    }
    const RoomName roomName = (getUserName() + U"'s room-" + ToHex(RandomUint32()));
    // ゲームハンドラから最大プレイヤー数を取得してルームを作成
    createRoom(roomName, game_handler_->get_max_players()); // << 修正箇所
    return;
  }
  if (errorCode) {
    if (m_verbose) Print << U"[ルーム参加でエラー] " << errorString;
  }
}

inline void OnlineManager::createRoomReturn(
  [[maybe_unused]] const LocalPlayerID playerID, const int32 errorCode, const String& errorString) {

  if (m_verbose) Print << U"OnlineManager::createRoomReturn()";
  if (errorCode) {
    if (m_verbose) Print << U"[ルーム作成でエラー] " << errorString;
  }
}

inline void OnlineManager::joinRoomEventAction(
  const LocalPlayer& newPlayer, [[maybe_unused]] const Array<LocalPlayerID>& playerIDs, const bool isSelf) {

  if (m_verbose) Print << U"OnlineManager::joinRoomEventAction()";
  local_players_ = getLocalPlayers();
  // プレイヤーが揃ったら、ゲームハンドラにゲーム開始を通知
  if (game_handler_ and local_players_.size() == game_handler_->get_max_players()) {
    game_handler_->on_game_start(local_players_, isHost());
  }
}

inline void OnlineManager::leaveRoomEventAction(const LocalPlayerID playerID, [[maybe_unused]] const bool isInactive) {
  if (m_verbose) Print << U"OnlineManager::leaveRoomEventAction()";
  local_players_ = getLocalPlayers();
  // ゲームハンドラにプレイヤーの退出を通知
  if (game_handler_) {
    game_handler_->on_player_left(playerID);
  }
}

inline void OnlineManager::leaveRoomReturn(
  [[maybe_unused]] int32 errorCode, [[maybe_unused]] const String& errorString) {

  if (m_verbose) Print << U"OnlineManager::leaveRoomReturn()";
  local_players_.clear();
  // ゲームハンドラに自身が退出したことを通知
  if (game_handler_) {
    game_handler_->on_leave_room();
  }
}

inline void OnlineManager::customEventAction(
  const LocalPlayerID playerID, const uint8 eventCode, Deserializer<MemoryViewReader>& reader) {

  // ゲームハンドラにイベント受信をそのまま通知
  if (game_handler_) {
    game_handler_->on_event_received(playerID, eventCode, reader);
  }
}


