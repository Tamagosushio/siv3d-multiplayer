# pragma once

# include "Multiplayer_Photon.hpp"
# include "PHOTON_APP_ID.SECRET"
# include "TicTacToe.hpp"

class MyNetwork : public Multiplayer_Photon {
public:
  static constexpr int32 MaxPlayers = 2;
  using Multiplayer_Photon::Multiplayer_Photon;

  void update_game(void);
  void draw(void) const;

private:

  Array<LocalPlayer> m_localPlayers;
  TikTakToe::Game game;
  bool is_started = false;
  bool is_turn = false;
  bool is_finished = false;
  TikTakToe::Cell winner = TikTakToe::Cell::None;
  Font font_detail{FontMethod::MSDF, 256, Typeface::Bold};

  void connectReturn(
    [[maybe_unused]] const int32 errorCode,
    const String& errorString,
    const String& region,
    [[maybe_unused]] const String& cluster
  ) override;
  void disconnectReturn() override;
  void joinRandomRoomReturn(
    [[maybe_unused]] const LocalPlayerID playerID,
    const int32 errorCode,
    const String& errorString
  ) override;
  void createRoomReturn(
    [[maybe_unused]] const LocalPlayerID playerID,
    const int32 errorCode,
    const String& errorString
  ) override;
  void joinRoomEventAction(
    const LocalPlayer& newPlayer,
    [[maybe_unused]] const Array<LocalPlayerID>& playerIDs,
    const bool isSelf
  ) override;
  void leaveRoomEventAction(
    const LocalPlayerID playerID,
    [[maybe_unused]] const bool isInactive
  ) override;
  void leaveRoomReturn(
    int32 errorCode,
    const String& errorString
  ) override;
  void customEventAction(
    const LocalPlayerID playerID,
    const uint8 eventCode,
    Deserializer<MemoryViewReader>& reader
  ) override;

};

void MyNetwork::update_game(void) {
  game.update();
  Optional<TikTakToe::Operation> op = game.get_operation();
  if (is_started and op and is_turn and not is_finished) {
    sendEvent(
      TikTakToe::Operation::code,
      Serializer<MemoryWriter>{}(*op)
    );
    is_turn = false;
    Optional<TikTakToe::Cell> result = game.operate(*op);
    if (result) {
      is_finished = true;
      winner = *result;
    }
  }
}

void MyNetwork::draw(void) const {
  Vec2 draw_text_center{ Vec2{ Scene::Center().x, Scene::Center().y * 1.5 } };
  if (is_started) {
    game.draw();
    if (is_finished) {
      font_detail(
        winner == TikTakToe::Cell::None
          ? U"Draw"
          : winner == game.get_symbol()
            ? U"You Win!"
            : U"You Lose"
      ).drawAt(
        128,
        draw_text_center,
        winner == TikTakToe::Cell::None
          ? Palette::Black
          : winner == game.get_symbol()
            ? Palette::Red
            : Palette::Blue
      );
    }
    else {
      font_detail(
        is_turn ? U"Your Turn!" : U"Not Your Turn"
      ).drawAt(
        128,
        draw_text_center,
        is_turn ? Palette::Red : Palette::Blue
      );
    }
  }
}



void MyNetwork::connectReturn(
  [[maybe_unused]] const int32 errorCode,
  const String& errorString,
  const String& region,
  [[maybe_unused]] const String& cluster
) {
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

void MyNetwork::disconnectReturn() {
  if (m_verbose) {
    Print << U"MyNetwork::disconnectReturn() [サーバから切断したときに呼ばれる]";
  }
  m_localPlayers.clear();
  Scene::SetBackground(Palette::DefaultBackground);
}

void MyNetwork::joinRandomRoomReturn(
  [[maybe_unused]] const LocalPlayerID playerID,
  const int32 errorCode,
  const String& errorString
) {
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

void MyNetwork::createRoomReturn(
  [[maybe_unused]] const LocalPlayerID playerID,
  const int32 errorCode,
  const String& errorString
) {
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

void MyNetwork::joinRoomEventAction(
  const LocalPlayer& newPlayer,
  [[maybe_unused]] const Array<LocalPlayerID>& playerIDs,
  const bool isSelf
) {
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
  if (m_localPlayers.size() == MaxPlayers) {
    game.initialize(3, isHost() ? TikTakToe::Cell::Circle : TikTakToe::Cell::Cross);
    is_turn = isHost();
    is_started = true;
  }
}

void MyNetwork::leaveRoomEventAction(
  const LocalPlayerID playerID,
  [[maybe_unused]] const bool isInactive
) {
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
  is_started = false;
  is_turn = false;
  is_finished = false;
  winner = TikTakToe::Cell::None;
}

void MyNetwork::leaveRoomReturn(
  int32 errorCode,
  const String& errorString
) {
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
  is_started = false;
  is_turn = false;
  is_finished = false;
  winner = TikTakToe::Cell::None;
}


// シリアライズデータを受信したときに呼ばれる関数をオーバーライドしてカスタマイズする
void MyNetwork::customEventAction(
  const LocalPlayerID playerID,
  const uint8 eventCode,
  Deserializer<MemoryViewReader>& reader
) {
  if (eventCode == TikTakToe::Operation::code) {
    TikTakToe::Operation op;
    reader(op);
    if (m_verbose) {
      Print << U"<<< [" << playerID << U"] からの TikTakToe::Operation(" << op.pos << U", " << static_cast<int>(op.cell_type) << U") を受信";
    }
    is_turn = true;
    Optional<TikTakToe::Cell> result = game.operate(op);
    if (result) {
      is_finished = true;
      winner = *result;
    }
  }
}

