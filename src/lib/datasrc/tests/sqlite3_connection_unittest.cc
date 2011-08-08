// Copyright (C) 2011  Internet Systems Consortium, Inc. ("ISC")
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
// OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <datasrc/sqlite3_connection.h>
#include <datasrc/data_source.h>

#include <dns/rrclass.h>

#include <gtest/gtest.h>

using namespace isc::datasrc;
using isc::data::ConstElementPtr;
using isc::data::Element;
using isc::dns::RRClass;
using isc::dns::Name;

namespace {
// Some test data
ConstElementPtr SQLITE_DBFILE_EXAMPLE = Element::fromJSON(
    "{ \"database_file\": \"" TEST_DATA_DIR "/test.sqlite3\"}");
ConstElementPtr SQLITE_DBFILE_EXAMPLE2 = Element::fromJSON(
    "{ \"database_file\": \"" TEST_DATA_DIR "/example2.com.sqlite3\"}");
ConstElementPtr SQLITE_DBFILE_EXAMPLE_ROOT = Element::fromJSON(
    "{ \"database_file\": \"" TEST_DATA_DIR "/test-root.sqlite3\"}");
ConstElementPtr SQLITE_DBFILE_BROKENDB = Element::fromJSON(
    "{ \"database_file\": \"" TEST_DATA_DIR "/brokendb.sqlite3\"}");
ConstElementPtr SQLITE_DBFILE_MEMORY = Element::fromJSON(
    "{ \"database_file\": \":memory:\"}");

// The following file must be non existent and must be non"creatable";
// the sqlite3 library will try to create a new DB file if it doesn't exist,
// so to test a failure case the create operation should also fail.
// The "nodir", a non existent directory, is inserted for this purpose.
ConstElementPtr SQLITE_DBFILE_NOTEXIST = Element::fromJSON(
    "{ \"database_file\": \"" TEST_DATA_DIR "/nodir/notexist\"}");

// Opening works (the content is tested in different tests)
TEST(SQLite3Open, common) {
    EXPECT_NO_THROW(SQLite3Connection conn(SQLITE_DBFILE_EXAMPLE,
                                           RRClass::IN()));
}

// Missing config
TEST(SQLite3Open, noConfig) {
    EXPECT_THROW(SQLite3Connection conn(Element::fromJSON("{}"),
                                                          RRClass::IN()),
                 DataSourceError);
}

// The file can't be opened
TEST(SQLite3Open, notExist) {
    EXPECT_THROW(SQLite3Connection conn(SQLITE_DBFILE_NOTEXIST,
                                        RRClass::IN()), SQLite3Error);
}

// It rejects broken DB
TEST(SQLite3Open, brokenDB) {
    EXPECT_THROW(SQLite3Connection conn(SQLITE_DBFILE_BROKENDB,
                                        RRClass::IN()), SQLite3Error);
}

// Test we can create the schema on the fly
TEST(SQLite3Open, memoryDB) {
    EXPECT_NO_THROW(SQLite3Connection conn(SQLITE_DBFILE_MEMORY,
                                           RRClass::IN()));
}

// Test fixture for querying the connection
class SQLite3Conn : public ::testing::Test {
public:
    SQLite3Conn() {
        initConn(SQLITE_DBFILE_EXAMPLE, RRClass::IN());
    }
    // So it can be re-created with different data
    void initConn(const ConstElementPtr& config, const RRClass& rrclass) {
        conn.reset(new SQLite3Connection(config, rrclass));
    }
    // The tested connection
    std::auto_ptr<SQLite3Connection> conn;
};

// This zone exists in the data, so it should be found
TEST_F(SQLite3Conn, getZone) {
    std::pair<bool, int> result(conn->getZone(Name("example.com")));
    EXPECT_TRUE(result.first);
    EXPECT_EQ(1, result.second);
}

// But it should find only the zone, nothing below it
TEST_F(SQLite3Conn, subZone) {
    EXPECT_FALSE(conn->getZone(Name("sub.example.com")).first);
}

// This zone is not there at all
TEST_F(SQLite3Conn, noZone) {
    EXPECT_FALSE(conn->getZone(Name("example.org")).first);
}

// This zone is there, but in different class
TEST_F(SQLite3Conn, noClass) {
    initConn(SQLITE_DBFILE_EXAMPLE, RRClass::CH());
    EXPECT_FALSE(conn->getZone(Name("example.com")).first);
}

// This tests the iterator context
TEST_F(SQLite3Conn, iterator) {
    // Our test zone is conveniently small, but not empty
    initConn(SQLITE_DBFILE_EXAMPLE2, RRClass::IN());

    // Get the iterator context
    DatabaseConnection::IteratorContextPtr
        context(conn->getIteratorContext(Name("example2.com"), 1));
    ASSERT_NE(DatabaseConnection::IteratorContextPtr(),
              context);

    std::string name, rtype, data;
    int ttl;
    // Get and check the first and only record
    EXPECT_TRUE(context->getNext(name, rtype, ttl, data));
    EXPECT_EQ("example2.com.", name);
    EXPECT_EQ("SOA", rtype);
    EXPECT_EQ("master.example2.com. admin.example2.com. "
              "1234 3600 1800 2419200 7200", data);
    EXPECT_EQ(3600, ttl);
    // Check there's no other
    EXPECT_FALSE(context->getNext(name, rtype, ttl, data));
}

}
