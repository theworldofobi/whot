#include <gtest/gtest.h>
#include "Network/WebSocketServer.hpp"

namespace whot::network {

TEST(TestWebSocketServer, Construct) {
    WebSocketServer ws(9090);
    EXPECT_FALSE(ws.isRunning());
}

TEST(TestWebSocketServer, SendMessage_NonexistentSession_NoCrash) {
    WebSocketServer ws(9091);
    ws.start();
    Message m;
    m.type = MessageType::GAME_STATE_UPDATE;
    m.payload = "{}";
    ws.sendMessage("nonexistent-session-id", m);
    ws.stop();
}

TEST(TestWebSocketServer, GetActiveConnectionCount_InitiallyZero) {
    WebSocketServer ws(9092);
    EXPECT_EQ(ws.getActiveConnectionCount(), 0u);
}

TEST(TestWebSocketServer, SetMessageHandler_DeserializeEmpty_NoCrash) {
    WebSocketServer ws(9093);
    ws.setMessageHandler([](const std::string&, const Message&) {});
    ws.start();
    ws.stop();
}

} // namespace whot::network
