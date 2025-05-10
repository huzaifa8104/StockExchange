#include "Trader.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <algorithm>
#include <cmath>
#include <queue>
#include <string>

using namespace std;

void Trader::showError(const string &message) {
    cout << "Error: " << message << " ✗\n";
}

void Trader::initializeDatabase() {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS Users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT UNIQUE NOT NULL,"
        "password TEXT NOT NULL,"
        "balance REAL NOT NULL DEFAULT 10000.0);"
        
        "CREATE TABLE IF NOT EXISTS Stocks ("
        "symbol TEXT PRIMARY KEY,"
        "price REAL NOT NULL);"
        
        "CREATE TABLE IF NOT EXISTS Portfolios ("
        "user_id INTEGER NOT NULL,"
        "stock_symbol TEXT NOT NULL,"
        "quantity INTEGER NOT NULL,"
        "initial_price REAL NOT NULL,"
        "PRIMARY KEY (user_id, stock_symbol));"
        
        "CREATE TABLE IF NOT EXISTS Transactions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "type TEXT NOT NULL,"
        "symbol TEXT,"
        "price REAL,"
        "quantity INTEGER,"
        "date TEXT);"
        
        "CREATE TABLE IF NOT EXISTS Orders ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "stock_symbol TEXT NOT NULL,"
        "price REAL NOT NULL,"
        "quantity INTEGER NOT NULL,"
        "type TEXT NOT NULL);";

    char* errMsg = 0;
    int rc = sqlite3_exec(db, sql, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        cerr << "SQL error: " << errMsg << endl;
        sqlite3_free(errMsg);
    }
    populateInitialData();
}

void Trader::populateInitialData() {
    const char* stocks[] = {"AAPL", "GOOGL", "AMZN", "MSFT", "TSLA"};
    double prices[] = {150.0, 2800.0, 3400.0, 299.0, 700.0};

    for (int i = 0; i < 5; i++) {
        string sql = "INSERT OR IGNORE INTO Stocks VALUES ('" +
                     string(stocks[i]) + "', " + to_string(prices[i]) + ");";
        sqlite3_exec(db, sql.c_str(), 0, 0, 0);
    }

    for (int i = 1; i <= 10; i++) {
        string username = "user" + to_string(i);
        string password = "pass" + to_string(i);

        string sql = "INSERT OR IGNORE INTO Users (username, password) VALUES ('" +
                     username + "', '" + password + "');";
        sqlite3_exec(db, sql.c_str(), 0, 0, 0);

        sqlite3_int64 userId = sqlite3_last_insert_rowid(db);

        for (int j = 0; j < 5; j++) {
            string stock = stocks[j];
            string portfolioSql = "INSERT OR IGNORE INTO Portfolios VALUES (" +
                                  to_string(userId) + ", '" + stock + "', 10, " +
                                  to_string(prices[j]) + ");";
            sqlite3_exec(db, portfolioSql.c_str(), 0, 0, 0);
        }
    }
}

void Trader::updateStockPrices() {
    const char* sql = "SELECT symbol, price FROM Stocks;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

    stockPrices.clear();
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        string symbol = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        double price = sqlite3_column_double(stmt, 1);
        stockPrices[symbol] = price;
    }
    sqlite3_finalize(stmt);
}

void Trader::loadOrdersFromDatabase() {
    const char* sql = "SELECT Orders.id, Users.username, Orders.stock_symbol, "
                    "Orders.price, Orders.quantity, Orders.type FROM Orders "
                    "JOIN Users ON Orders.user_id = Users.id WHERE Orders.quantity > 0;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Order order;
        order.id = sqlite3_column_int(stmt, 0);
        order.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        order.symbol = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        order.price = sqlite3_column_double(stmt, 3);
        order.quantity = sqlite3_column_int(stmt, 4);
        order.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        
        if (order.type == "BUY") buyOrders.push_back(order);
        else sellOrders.push_back(order);
    }
    sqlite3_finalize(stmt);
}

void Trader::executeTrade(const string& buyer, const string& seller,
                          const string& symbol, double price, int quantity) {
    string updateSql = "UPDATE Stocks SET price = ? WHERE symbol = ?;";
    sqlite3_stmt* updateStmt;
    sqlite3_prepare_v2(db, updateSql.c_str(), -1, &updateStmt, 0);
    sqlite3_bind_double(updateStmt, 1, price);
    sqlite3_bind_text(updateStmt, 2, symbol.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(updateStmt);
    sqlite3_finalize(updateStmt);
    stockPrices[symbol] = price;

    string sql = "UPDATE Users SET balance = balance - ? WHERE username = ?;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
    sqlite3_bind_double(stmt, 1, price * quantity);
    sqlite3_bind_text(stmt, 2, buyer.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    sql = "UPDATE Users SET balance = balance + ? WHERE username = ?;";
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
    sqlite3_bind_double(stmt, 1, price * quantity);
    sqlite3_bind_text(stmt, 2, seller.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    sql = "UPDATE Portfolios SET quantity = quantity + ? "
            "WHERE user_id = (SELECT id FROM Users WHERE username = ?) "
            "AND stock_symbol = ?;";
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
    sqlite3_bind_int(stmt, 1, quantity);
    sqlite3_bind_text(stmt, 2, buyer.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, symbol.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    sql = "UPDATE Portfolios SET quantity = quantity - ? "
            "WHERE user_id = (SELECT id FROM Users WHERE username = ?) "
            "AND stock_symbol = ?;";
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
    sqlite3_bind_int(stmt, 1, quantity);
    sqlite3_bind_text(stmt, 2, seller.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, symbol.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    cout << "Trade executed: " << quantity << " shares of " << symbol
            << " at $" << price << " each.\n";
}

void Trader::matchOrders(Order& newOrder) {
    sqlite3_int64 newOrderId = -1;
    sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0);

    auto execute_sql = [&](const char* sql, auto&& binder) {
        sqlite3_stmt* stmt;
        if(sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) return false;
        binder(stmt);
        bool result = sqlite3_step(stmt) == SQLITE_DONE;
        sqlite3_finalize(stmt);
        return result;
    };

    if(newOrder.quantity > 0) {
        const char* insertSql = "INSERT INTO Orders (user_id, stock_symbol, price, quantity, type) "
                        "VALUES ((SELECT id FROM Users WHERE username = ?), ?, ?, ?, ?);";
        execute_sql(insertSql, [&](sqlite3_stmt* stmt) {
            sqlite3_bind_text(stmt, 1, newOrder.username.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, newOrder.symbol.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_double(stmt, 3, newOrder.price);
            sqlite3_bind_int(stmt, 4, newOrder.quantity);
            sqlite3_bind_text(stmt, 5, newOrder.type.c_str(), -1, SQLITE_STATIC);
        });
        newOrderId = sqlite3_last_insert_rowid(db);
        newOrder.id = newOrderId;
    }

    auto processOrders = [&](auto& orders, const char* orderType, auto&& tradeExecutor) {
        for(auto it = orders.begin(); it != orders.end();) {
            if(it->symbol == newOrder.symbol && 
            ((newOrder.type == "BUY" && it->price <= newOrder.price) ||
            (newOrder.type == "SELL" && it->price >= newOrder.price))) {
                
                int tradeQty = min(newOrder.quantity, it->quantity);
                if(tradeQty <= 0) {
                    ++it;
                    continue;
                }

                tradeExecutor(*it, tradeQty);
                newOrder.quantity -= tradeQty;
                it->quantity -= tradeQty;

                if(it->quantity <= 0) {
                    const char* deleteSql = "DELETE FROM Orders WHERE rowid = ("
                        "SELECT rowid FROM Orders WHERE user_id = "
                        "(SELECT id FROM Users WHERE username = ?) "
                        "AND stock_symbol = ? AND price = ? AND type = ? LIMIT 1);";
                    
                    execute_sql(deleteSql, [&](sqlite3_stmt* stmt) {
                        sqlite3_bind_text(stmt, 1, it->username.c_str(), -1, SQLITE_STATIC);
                        sqlite3_bind_text(stmt, 2, it->symbol.c_str(), -1, SQLITE_STATIC);
                        sqlite3_bind_double(stmt, 3, it->price);
                        sqlite3_bind_text(stmt, 4, orderType, -1, SQLITE_STATIC);
                    });
                    
                    it = orders.erase(it);
                } else {
                    const char* updateSql = "UPDATE Orders SET quantity = ? WHERE rowid = ("
                        "SELECT rowid FROM Orders WHERE user_id = "
                        "(SELECT id FROM Users WHERE username = ?) "
                        "AND stock_symbol = ? AND price = ? AND type = ? LIMIT 1);";
                    
                    execute_sql(updateSql, [&](sqlite3_stmt* stmt) {
                        sqlite3_bind_int(stmt, 1, it->quantity);
                        sqlite3_bind_text(stmt, 2, it->username.c_str(), -1, SQLITE_STATIC);
                        sqlite3_bind_text(stmt, 3, it->symbol.c_str(), -1, SQLITE_STATIC);
                        sqlite3_bind_double(stmt, 4, it->price);
                        sqlite3_bind_text(stmt, 5, orderType, -1, SQLITE_STATIC);
                    });
                    ++it;
                }
            } else {
                ++it;
            }
        }
    };

    if(newOrder.type == "BUY") {
        processOrders(sellOrders, "SELL", [&](Order& sellOrder, int qty) {
            executeTrade(newOrder.username, sellOrder.username, 
                        newOrder.symbol, sellOrder.price, qty);
        });
        if(newOrder.quantity > 0) buyOrders.push_back(newOrder);
    } else {
        processOrders(buyOrders, "BUY", [&](Order& buyOrder, int qty) {
            executeTrade(buyOrder.username, newOrder.username, 
                        newOrder.symbol, newOrder.price, qty);
        });
        if(newOrder.quantity > 0) sellOrders.push_back(newOrder);
    }

    if(newOrderId != -1) {
        if(newOrder.quantity <= 0) {
            execute_sql("DELETE FROM Orders WHERE rowid = ?;", [&](sqlite3_stmt* stmt) {
                sqlite3_bind_int64(stmt, 1, newOrderId);
            });
        } else {
            execute_sql("UPDATE Orders SET quantity = ? WHERE rowid = ?;", [&](sqlite3_stmt* stmt) {
                sqlite3_bind_int(stmt, 1, newOrder.quantity);
                sqlite3_bind_int64(stmt, 2, newOrderId);
            });
        }
    }

    sqlite3_exec(db, "COMMIT;", 0, 0, 0);

    cout << "\nUpdated Market Prices:\n";
    for(const auto& [sym, price] : stockPrices) {
        cout << sym << ": $" << price << "\n";
    }
}

Trader::Trader() : currentUser(nullptr) {
    int rc = sqlite3_open("stock_exchange.db", &db);
    if (rc != SQLITE_OK) {
        cerr << "Cannot open database: " << sqlite3_errmsg(db) << endl;
        exit(1);
    }
    initializeDatabase();
    updateStockPrices();
    loadOrdersFromDatabase();
}

Trader::~Trader() {
    sqlite3_close(db);
    delete currentUser;
}

void Trader::registerUser() {
    string username, password;
    cout << "Enter username: ";
    cin >> username;
    cout << "Enter password: ";
    cin >> password;

    string sql = "INSERT INTO Users (username, password) VALUES (?, ?);";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        cout << "Registration successful!\n";
    } else {
        showError("Username already exists");
    }
    sqlite3_finalize(stmt);
}

void Trader::login() {
    string username, password;
    cout << "Enter username: ";
    cin >> username;
    cout << "Enter password: ";
    cin >> password;

    string sql = "SELECT * FROM Users WHERE username = ? AND password = ?;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        currentUser = new User();
        currentUser->username = username;
        currentUser->password = password;
        currentUser->balance = sqlite3_column_double(stmt, 3);

        string portfolioSql = "SELECT stock_symbol, quantity, initial_price "
                              "FROM Portfolios WHERE user_id = ?;";
        sqlite3_stmt* pStmt;
        sqlite3_prepare_v2(db, portfolioSql.c_str(), -1, &pStmt, 0);
        sqlite3_bind_int(pStmt, 1, sqlite3_column_int(stmt, 0));

        while (sqlite3_step(pStmt) == SQLITE_ROW) {
            Stock stock;
            stock.symbol = reinterpret_cast<const char*>(sqlite3_column_text(pStmt, 0));
            stock.quantity = sqlite3_column_int(pStmt, 1);
            stock.initialPrice = sqlite3_column_double(pStmt, 2);
            currentUser->portfolio.push_back(stock);
        }
        sqlite3_finalize(pStmt);
        cout << "Login successful!\n";
    } else {
        showError("Invalid credentials");
    }
    sqlite3_finalize(stmt);
}

void Trader::placeOrder() {
    if (!currentUser) {
        showError("No user logged in");
        return;
    }

    Order newOrder;
    cout << "Enter symbol: ";
    cin >> newOrder.symbol;
    
    bool validSymbol = false;
    {
        const char* checkSql = "SELECT 1 FROM Stocks WHERE symbol = ?;";
        sqlite3_stmt* checkStmt;
        sqlite3_prepare_v2(db, checkSql, -1, &checkStmt, 0);
        sqlite3_bind_text(checkStmt, 1, newOrder.symbol.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(checkStmt) == SQLITE_ROW) {
            validSymbol = true;
        }
        sqlite3_finalize(checkStmt);
    }
    
    if (!validSymbol) {
        showError("Invalid stock symbol");
        return;
    }

    string orderType;
    bool validOrderType = false;
    do {
        cout << "Type (BUY/SELL): ";
        cin >> orderType;
        transform(orderType.begin(), orderType.end(), orderType.begin(), ::toupper);
        
        if (orderType == "BUY" || orderType == "SELL") {
            validOrderType = true;
            newOrder.type = orderType;
        } else {
            showError("Invalid order type. Please enter BUY or SELL");
        }
    } while (!validOrderType);

    while (true) {
        cout << "Price: ";
        if (cin >> newOrder.price) {
            if (newOrder.price > 0) {
                break;
            } else {
                showError("Price must be positive");
            }
        } else {
            showError("Invalid price. Please enter a number");
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }
    
    while (true) {
        cout << "Quantity: ";
        if (cin >> newOrder.quantity) {
            if (newOrder.quantity > 0) {
                break;
            } else {
                showError("Quantity must be positive");
            }
        } else {
            showError("Invalid quantity. Please enter a whole number");
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }

    newOrder.username = currentUser->username;

    if (newOrder.type == "SELL") {
        int userId = -1;
        {
            const char* idSql = "SELECT id FROM Users WHERE username = ?;";
            sqlite3_stmt* idStmt;
            sqlite3_prepare_v2(db, idSql, -1, &idStmt, 0);
            sqlite3_bind_text(idStmt, 1, currentUser->username.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(idStmt) == SQLITE_ROW) {
                userId = sqlite3_column_int(idStmt, 0);
            }
            sqlite3_finalize(idStmt);
        }
        int ownedQty = 0;
        {
            const char* qtySql = 
                "SELECT quantity FROM Portfolios "
                " WHERE user_id = ? AND stock_symbol = ?;";
            sqlite3_stmt* qtyStmt;
            sqlite3_prepare_v2(db, qtySql, -1, &qtyStmt, 0);
            sqlite3_bind_int(qtyStmt, 1, userId);
            sqlite3_bind_text(qtyStmt, 2, newOrder.symbol.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(qtyStmt) == SQLITE_ROW) {
                ownedQty = sqlite3_column_int(qtyStmt, 0);
            }
            sqlite3_finalize(qtyStmt);
        }
        if (newOrder.quantity > ownedQty) {
            showError("Insufficient shares to sell");
            return;
        }
    }

    if (newOrder.type == "BUY" && currentUser->balance < newOrder.price * newOrder.quantity) {
        showError("Insufficient funds");
        return;
    }

    matchOrders(newOrder);
}

void Trader::viewPrices() {
    cout << "\nCurrent Market Prices:\n";
    for (const auto& [sym, price] : stockPrices) {
        cout << sym << ": $" << price << "\n";
    }
}

void Trader::checkPortfolio() {
    if (!currentUser) {
        showError("No user logged in");
        return;
    }

    string sql = "SELECT p.stock_symbol, p.quantity, p.initial_price, s.price "
                 "FROM Portfolios p "
                 "JOIN Stocks s ON p.stock_symbol = s.symbol "
                 "WHERE p.user_id = (SELECT id FROM Users WHERE username = ?);";

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, currentUser->username.c_str(), -1, SQLITE_STATIC);

    cout << "\nPortfolio for " << currentUser->username << "\n";
    cout << "----------------------------------------\n";
    cout << left << setw(8) << "Symbol" << setw(10) << "Quantity"
         << setw(12) << "Avg Cost" << setw(10) << "Curr Price"
         << setw(15) << "Current Value" << "\n";

    double totalValue = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        string symbol = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        int quantity = sqlite3_column_int(stmt, 1);
        double avgCost = sqlite3_column_double(stmt, 2);
        double currPrice = sqlite3_column_double(stmt, 3);
        double value = quantity * currPrice;

        cout << left << setw(8) << symbol
             << setw(10) << quantity
             << "$" << setw(11) << fixed << setprecision(2) << avgCost
             << "$" << setw(9) << currPrice
             << "$" << value << "\n";

        totalValue += value;
    }

    if (totalValue == 0) {
        cout << "No holdings found\n";
    } else {
        cout << "----------------------------------------\n";
        cout << "Total Portfolio Value: $" << fixed << setprecision(2) << totalValue << "\n";
        cout << "Account Balance: $" << currentUser->balance << "\n";
        cout << "Net Worth: $" << (totalValue + currentUser->balance) << "\n";
    }
    sqlite3_finalize(stmt);
}

void Trader::depositMoney() {
    if (!currentUser) {
        showError("No user logged in");
        return;
    }

    double amount;
    cout << "Enter deposit amount: $";
    cin >> amount;

    if (amount <= 0) {
        showError("Invalid amount");
        return;
    }

    string sql = "UPDATE Users SET balance = balance + ? WHERE username = ?;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
    sqlite3_bind_double(stmt, 1, amount);
    sqlite3_bind_text(stmt, 2, currentUser->username.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        currentUser->balance += amount;

        string transSql = "INSERT INTO Transactions (user_id, type, price, date) "
                          "VALUES ((SELECT id FROM Users WHERE username = ?), 'DEPOSIT', ?, date('now'));";
        sqlite3_stmt* transStmt;
        sqlite3_prepare_v2(db, transSql.c_str(), -1, &transStmt, 0);
        sqlite3_bind_text(transStmt, 1, currentUser->username.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(transStmt, 2, amount);
        sqlite3_step(transStmt);
        sqlite3_finalize(transStmt);

        cout << "Deposit successful! New balance: $"
             << fixed << setprecision(2) << currentUser->balance << "\n";
    } else {
        showError("Deposit failed");
    }
    sqlite3_finalize(stmt);
}

void Trader::withdrawMoney() {
    if (!currentUser) {
        showError("No user logged in");
        return;
    }

    double amount;
    cout << "Enter withdrawal amount: $";
    cin >> amount;

    if (amount <= 0 || amount > currentUser->balance) {
        showError(amount <= 0 ? "Invalid amount" : "Insufficient funds");
        return;
    }

    string sql = "UPDATE Users SET balance = balance - ? WHERE username = ?;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
    sqlite3_bind_double(stmt, 1, amount);
    sqlite3_bind_text(stmt, 2, currentUser->username.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        currentUser->balance -= amount;

        string transSql = "INSERT INTO Transactions (user_id, type, price, date) "
                          "VALUES ((SELECT id FROM Users WHERE username = ?), 'WITHDRAW', ?, date('now'));";
        sqlite3_stmt* transStmt;
        sqlite3_prepare_v2(db, transSql.c_str(), -1, &transStmt, 0);
        sqlite3_bind_text(transStmt, 1, currentUser->username.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(transStmt, 2, amount);
        sqlite3_step(transStmt);
        sqlite3_finalize(transStmt);

        cout << "Withdrawal successful! New balance: $"
             << fixed << setprecision(2) << currentUser->balance << "\n";
    } else {
        showError("Withdrawal failed");
    }
    sqlite3_finalize(stmt);
}

void Trader::viewUserOrders() {
    if (!currentUser) {
        showError("No user logged in");
        return;
    }

    vector<Order> userOrders;
    for (const auto& order : buyOrders) {
        if (order.username == currentUser->username) userOrders.push_back(order);
    }
    for (const auto& order : sellOrders) {
        if (order.username == currentUser->username) userOrders.push_back(order);
    }

    if (userOrders.empty()) {
        cout << "No active orders.\n";
        return;
    }

    cout << "\nYour Active Orders:\n";
    cout << "--------------------------------------------------\n";
    cout << left << setw(8) << "ID" << setw(8) << "Type" << setw(8) << "Symbol"
         << setw(12) << "Price" << setw(10) << "Quantity" << "\n";
    for (const auto& order : userOrders) {
        cout << setw(8) << order.id 
             << setw(8) << order.type 
             << setw(8) << order.symbol
             << "$" << setw(11) << fixed << setprecision(2) << order.price
             << setw(10) << order.quantity << "\n";
    }
    cout << "--------------------------------------------------\n";
}

void Trader::cancelOrder(int orderId) {
    if (!currentUser) {
        showError("No user logged in");
        return;
    }

    auto cancelOrderInVector = [&](auto& orders, const string& type) {
        for (auto it = orders.begin(); it != orders.end(); ++it) {
            if (it->id == orderId && it->username == currentUser->username) {
                const char* sql = "DELETE FROM Orders WHERE id = ? AND user_id = "
                                "(SELECT id FROM Users WHERE username = ?);";
                sqlite3_stmt* stmt;
                sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
                sqlite3_bind_int(stmt, 1, orderId);
                sqlite3_bind_text(stmt, 2, currentUser->username.c_str(), -1, SQLITE_STATIC);
                
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    orders.erase(it);
                    cout << type << " order #" << orderId << " cancelled successfully.\n";
                } else {
                    showError("Failed to cancel order");
                }
                sqlite3_finalize(stmt);
                return true;
            }
        }
        return false;
    };

    if (!cancelOrderInVector(buyOrders, "Buy") && 
        !cancelOrderInVector(sellOrders, "Sell")) {
        showError("Order not found or not owned by you");
    }
}
