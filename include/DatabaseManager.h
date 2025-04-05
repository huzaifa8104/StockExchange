#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <sqlite3.h>
#include <string>

class DatabaseManager {
public:
    DatabaseManager(const std::string &databaseName);
    ~DatabaseManager();
    bool executeSQL(const std::string &sql);
    sqlite3_stmt* prepareStatement(const std::string &sql);
    sqlite3* getDB();

private:
    sqlite3* db;
};

#endif
