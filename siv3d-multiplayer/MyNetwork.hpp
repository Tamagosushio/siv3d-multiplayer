# pragma once

# include "Multiplayer_Photon.hpp"
# include "PHOTON_APP_ID.SECRET"

class MyNetwork : public Multiplayer_Photon {
public:
  static constexpr int32 MaxPlayers = 3;
  using Multiplayer_Photon::Multiplayer_Photon;
private:
  Array<LocalPlayer> m_localPlayers;

  void connectReturn(
    [[maybe_unused]] const int32 errorCode,
    const String& errorString,
    const String& region,
    [[maybe_unused]] const String& cluster
  ) override {
    if (m_verbose) {
      Print << U"MyNetwork::connectReturn() [サーバへの接続を試みた結果を処理する]";
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

  void disconnectReturn() override {
    if (m_verbose) {
      Print << U"MyNetwork::disconnectReturn() [サーバから切断したときに呼ばれる]";
    }
    m_localPlayers.clear();
    Scene::SetBackground(Palette::DefaultBackground);
  }

  void joinRandomRoomReturn(
    [[maybe_unused]] const LocalPlayerID playerID,
    const int32 errorCode,
    const String& errorString
  ) override {
    if (m_verbose) {
      Print << U"MyNetwork::joinRandomRoomReturn() [既存のランダムなルームに参加を試みた結果を処理する]";
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

  void createRoomReturn(
    [[maybe_unused]] const LocalPlayerID playerID,
    const int32 errorCode,
    const String& errorString
  ) override {
    if (m_verbose) {
      Print << U"MyNetwork::createRoomReturn() [ルームを新規作成した結果を処理する]";
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

  void joinRoomEventAction(
    const LocalPlayer& newPlayer,
    [[maybe_unused]] const Array<LocalPlayerID>& playerIDs,
    const bool isSelf
  ) override {
    if (m_verbose) {
      Print << U"MyNetwork::joinRoomEventAction() [誰か（自分を含む）が現在のルームに参加したときに呼ばれる]";
    }
    m_localPlayers = getLocalPlayers();
    if (m_verbose) {
      Print << U"[{} (ID: {}) がルームに参加した。ローカル ID: {}] {}"_fmt(newPlayer.userName, newPlayer.userID, newPlayer.localID, (isSelf ? U"(自分自身)" : U""));
      Print << U"現在の " << getCurrentRoomName() << U" のルームメンバー";
      for (const auto& player : m_localPlayers) {
        Print << U"- [{}] {} (id: {}) {}"_fmt(player.localID, player.userName, player.userID, player.isHost ? U"(host)" : U"");
      }
    }
  }

  void leaveRoomEventAction(
    const LocalPlayerID playerID,
    [[maybe_unused]] const bool isInactive
  ) override {
    if (m_verbose) {
      Print << U"MyNetwork::leaveRoomEventAction() [誰かがルームから退出したら呼ばれる]";
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
  }

  void leaveRoomReturn(
    int32 errorCode,
    const String& errorString
  ) override {
    if (m_verbose) {
      Print << U"MyNetwork::leaveRoomReturn() [ルームから退出したときに呼ばれる]";
    }
    m_localPlayers.clear();
    if (errorCode) {
      if (m_verbose) {
        Print << U"[ルームからの退出でエラーが発生] " << errorString;
      }
      return;
    }
  }

  // int32 を受信したときに呼ばれる関数をオーバーライドしてカスタマイズする
  void customEventAction(
    const LocalPlayerID playerID,
    const uint8 eventCode,
    const int32 data
  ) override {
    Print << U"<<< [" << playerID << U"] からの eventCode: " << eventCode << U", data: int32(" << data << U") を受信";
  }

  // String を受信したときに呼ばれる関数をオーバーライドしてカスタマイズする
  void customEventAction(
    const LocalPlayerID playerID,
    const uint8 eventCode,
    const String& data
  ) override {
    Print << U"<<< [" << playerID << U"] からの eventCode: " << eventCode << U", data: String(" << data << U") を受信";
  }

  // Point を受信したときに呼ばれる関数をオーバーライドしてカスタマイズする
  void customEventAction(
    const LocalPlayerID playerID,
    const uint8 eventCode,
    const Point& data
  ) override {
    Print << U"<<< [" << playerID << U"] からの eventCode: " << eventCode << U", data: Point" << data << U" を受信";
  }

  // Array<int32> を受信したときに呼ばれる関数をオーバーライドしてカスタマイズする
  void customEventAction(
    const LocalPlayerID playerID,
    const uint8 eventCode,
    const Array<int32>& data
  ) override {
    Print << U"<<< [" << playerID << U"] からの eventCode: " << eventCode << U", data: Array<int32>" << data << U" を受信";
  }

  // Array<String> を受信したときに呼ばれる関数をオーバーライドしてカスタマイズする
  void customEventAction(
    const LocalPlayerID playerID,
    const uint8 eventCode,
    const Array<String>& data
  ) override {
    Print << U"<<< [" << playerID << U"] からの eventCode: " << eventCode << U", data: Array<String>" << data << U" を受信";
  }
};
