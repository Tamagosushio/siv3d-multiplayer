# pragma once
# include <Siv3D.hpp>
# include "Multiplayer_Photon.hpp"
# include "IGame.hpp"

class OnlineManager : public Multiplayer_Photon {
private:
  IGame* m_game_handler = nullptr;
  Array<LocalPlayer> m_local_players;
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
  /// @brief このネットワークマネージャが通知を送る先のゲームハンドラを設定
  void set_game_handler(IGame* handler) {
    m_game_handler = handler;
  }
  /// @brief ゲームイベントを送信する
  template<class T>
  void send_game_event(const uint8 event_code, const T& data) {
    sendEvent(event_code, Serializer<MemoryWriter>{}(data));
  }
  void debug(void) const {};
};

void OnlineManager::connectReturn(
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

void OnlineManager::disconnectReturn() {
  if (m_verbose) Print << U"OnlineManager::disconnectReturn()";
  m_local_players.clear();
}

void OnlineManager::joinRandomRoomReturn(
  [[maybe_unused]] const LocalPlayerID playerID, const int32 errorCode, const String& errorString) {

  if (m_verbose) Print << U"OnlineManager::joinRandomRoomReturn()";
  if (errorCode == NoRandomMatchFound) {
    if (m_verbose) Print << U"[参加可能なルームが見つからず、新規作成します]";
    // ゲームハンドラが設定されていなければルームは作れない
    if (not m_game_handler) {
      if (m_verbose) Print << U"[エラー] GameHandlerが未設定のため、ルームを作成できません。";
      return;
    }
    const RoomName roomName = (getUserName() + U"'s room-" + ToHex(RandomUint32()));
    // ゲームハンドラから最大プレイヤー数を取得してルームを作成
    createRoom(roomName, m_game_handler->get_max_players()); // << 修正箇所
    return;
  }
  if (errorCode) {
    if (m_verbose) Print << U"[ルーム参加でエラー] " << errorString;
  }
}

void OnlineManager::createRoomReturn(
  [[maybe_unused]] const LocalPlayerID playerID, const int32 errorCode, const String& errorString) {

  if (m_verbose) Print << U"OnlineManager::createRoomReturn()";
  if (errorCode) {
    if (m_verbose) Print << U"[ルーム作成でエラー] " << errorString;
  }
}

void OnlineManager::joinRoomEventAction(
  const LocalPlayer& newPlayer, [[maybe_unused]] const Array<LocalPlayerID>& playerIDs, const bool isSelf) {

  if (m_verbose) Print << U"OnlineManager::joinRoomEventAction()";
  m_local_players = getLocalPlayers();
  // プレイヤーが揃ったら、ゲームハンドラにゲーム開始を通知
  if (m_local_players.size() == m_game_handler->get_max_players() && m_game_handler) {
    m_game_handler->on_game_start(m_local_players, isHost());
  }
}

void OnlineManager::leaveRoomEventAction(const LocalPlayerID playerID, [[maybe_unused]] const bool isInactive) {
  if (m_verbose) Print << U"OnlineManager::leaveRoomEventAction()";
  m_local_players = getLocalPlayers();
  // ゲームハンドラにプレイヤーの退出を通知
  if (m_game_handler) {
    m_game_handler->on_player_left(playerID);
  }
}

void OnlineManager::leaveRoomReturn(
  [[maybe_unused]] int32 errorCode, [[maybe_unused]] const String& errorString) {

  if (m_verbose) Print << U"OnlineManager::leaveRoomReturn()";
  m_local_players.clear();
  // ゲームハンドラに自身が退出したことを通知
  if (m_game_handler) {
    m_game_handler->on_leave_room();
  }
}

void OnlineManager::customEventAction(
  const LocalPlayerID playerID, const uint8 eventCode, Deserializer<MemoryViewReader>& reader) {

  // ゲームハンドラにイベント受信をそのまま通知
  if (m_game_handler) {
    m_game_handler->on_event_received(playerID, eventCode, reader);
  }
}





