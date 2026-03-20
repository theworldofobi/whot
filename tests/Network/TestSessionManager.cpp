#include <gtest/gtest.h>
#include "Network/SessionManager.hpp"
#include <thread>
#include <vector>

namespace whot::network {

TEST(TestSessionManager, CreateDestroySession) {
    SessionManager mgr;
    std::string id = mgr.createSession("127.0.0.1");
    EXPECT_FALSE(id.empty());
    EXPECT_TRUE(mgr.sessionExists(id));
    mgr.destroySession(id);
    EXPECT_FALSE(mgr.sessionExists(id));
}

TEST(TestSessionManager, GetSession_InvalidId_ReturnsNull) {
    SessionManager mgr;
    EXPECT_EQ(mgr.getSession("nonexistent"), nullptr);
}

TEST(TestSessionManager, SetPlayerIdSetGameId) {
    SessionManager mgr;
    std::string sid = mgr.createSession("1.2.3.4");
    mgr.setPlayerId(sid, "player-1");
    mgr.setGameId(sid, "game-1");
    Session* s = mgr.getSession(sid);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->playerId, "player-1");
    EXPECT_EQ(s->gameId, "game-1");
}

TEST(TestSessionManager, GetSessionsForGame_Empty) {
    SessionManager mgr;
    auto list = mgr.getSessionsForGame("g1");
    EXPECT_TRUE(list.empty());
}

TEST(TestSessionManager, GetSessionsForGame_AfterBind) {
    SessionManager mgr;
    std::string s1 = mgr.createSession("1");
    std::string s2 = mgr.createSession("2");
    mgr.setGameId(s1, "g1");
    mgr.setGameId(s2, "g1");
    auto list = mgr.getSessionsForGame("g1");
    EXPECT_EQ(list.size(), 2u);
}

TEST(TestSessionManager, GetSessionIdForPlayer) {
    SessionManager mgr;
    std::string sid = mgr.createSession("1");
    mgr.setPlayerId(sid, "p1");
    EXPECT_EQ(mgr.getSessionIdForPlayer("p1"), sid);
    EXPECT_TRUE(mgr.getSessionIdForPlayer("p99").empty());
}

TEST(TestSessionManager, RemoveExpiredSessions_Zero_NoOp) {
    SessionManager mgr;
    std::string id = mgr.createSession("1");
    mgr.removeExpiredSessions(0);
    EXPECT_TRUE(mgr.sessionExists(id));
}

TEST(TestSessionManager, RemoveExpiredSessions_Positive_NoCrash) {
    SessionManager mgr;
    mgr.createSession("1");
    mgr.removeExpiredSessions(1);
}

TEST(TestSessionManager, RemoveAllSessionsForGame) {
    SessionManager mgr;
    std::string s1 = mgr.createSession("1");
    mgr.setGameId(s1, "g1");
    mgr.removeAllSessionsForGame("g1");
    EXPECT_EQ(mgr.getSessionsForGame("g1").size(), 0u);
}

TEST(TestSessionManager, GetActiveSessionCount) {
    SessionManager mgr;
    EXPECT_EQ(mgr.getActiveSessionCount(), 0u);
    mgr.createSession("1");
    EXPECT_EQ(mgr.getActiveSessionCount(), 1u);
    mgr.createSession("2");
    EXPECT_EQ(mgr.getActiveSessionCount(), 2u);
}

TEST(TestSessionManager, ConcurrentCreateDestroyAndGetSessionsForGame) {
    SessionManager mgr;
    const int numThreads = 4;
    const int opsPerThread = 50;
    std::vector<std::thread> threads;
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&mgr, t]() {
            for (int i = 0; i < opsPerThread; ++i) {
                std::string sid = mgr.createSession("127.0.0.1");
                mgr.setGameId(sid, "game-1");
                auto list = mgr.getSessionsForGame("game-1");
                (void)list;
                mgr.destroySession(sid);
            }
        });
    }
    for (auto& th : threads) th.join();
}

} // namespace whot::network
