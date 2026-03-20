#include <gtest/gtest.h>
#include "Persistence/Database.hpp"
#include "TestHelpers.hpp"

namespace whot::persistence {

using namespace whot::test;

TEST(TestDatabase, InMemory_ConnectDisconnect) {
    auto db = createInMemoryDatabase();
    ASSERT_NE(db, nullptr);
    EXPECT_TRUE(db->isConnected());
    db->disconnect();
    EXPECT_FALSE(db->isConnected());
}

TEST(TestDatabase, InMemory_Execute) {
    auto db = createInMemoryDatabase();
    ASSERT_NE(db, nullptr);
    EXPECT_TRUE(db->execute("CREATE TABLE IF NOT EXISTS t1 (id INTEGER)"));
    EXPECT_TRUE(db->execute("INSERT INTO t1 (id) VALUES (1)"));
}

TEST(TestDatabase, QueryOne) {
    auto db = createInMemoryDatabase();
    ASSERT_NE(db, nullptr);
    db->execute("CREATE TABLE IF NOT EXISTS t2 (id INTEGER)");
    db->execute("INSERT INTO t2 (id) VALUES (42)");
    auto r = db->queryOne("SELECT id FROM t2 LIMIT 1");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, "42");
}

TEST(TestDatabase, QueryMany) {
    auto db = createInMemoryDatabase();
    ASSERT_NE(db, nullptr);
    db->execute("CREATE TABLE IF NOT EXISTS t3 (id INTEGER)");
    db->execute("INSERT INTO t3 (id) VALUES (1)");
    db->execute("INSERT INTO t3 (id) VALUES (2)");
    auto rows = db->queryMany("SELECT id FROM t3 ORDER BY id");
    EXPECT_EQ(rows.size(), 2u);
}

TEST(TestDatabase, Transaction) {
    auto db = createInMemoryDatabase();
    ASSERT_NE(db, nullptr);
    db->beginTransaction();
    db->execute("CREATE TABLE IF NOT EXISTS t4 (id INTEGER)");
    db->commit();
    db->beginTransaction();
    db->rollback();
}

TEST(TestDatabase, GetCurrentSchemaVersion) {
    auto db = createInMemoryDatabase();
    ASSERT_NE(db, nullptr);
    int v = db->getCurrentSchemaVersion();
    EXPECT_GE(v, 0);
}

TEST(TestDatabase, Execute_InvalidSql_NoCrash) {
    auto db = createInMemoryDatabase();
    ASSERT_NE(db, nullptr);
    bool ok = db->execute("INVALID SQL ???");
    EXPECT_FALSE(ok);
}

} // namespace whot::persistence
