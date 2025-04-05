#include "DatabaseManager.h"
#include <iostream>

DatabaseManager::DatabaseManager(const std::string &databaseName) : db(nullptr) {
    if (sqlite3_open(databaseName.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        db = nullptr;
    }
}

DatabaseManager::~DatabaseManager() {
    if (db) {
        sqlite3_close(db);
    }
}

bool DatabaseManager::executeSQL(const std::string &sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << (errMsg ? errMsg : "unknown error") << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

sqlite3_stmt* DatabaseManager::prepareStatement(const std::string &sql) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sql << std::endl;
        return nullptr;
    }
    return stmt;
}

sqlite3* DatabaseManager::getDB() {
    return db;
}
