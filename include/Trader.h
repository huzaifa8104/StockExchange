#ifndef TRADER_H
#define TRADER_H

#include "Models.h"
#include <map>
#include <vector>
#include <sqlite3.h>
#include <string>

class Trader {
private:
    sqlite3* db;
    User* currentUser;
    map<string, double> stockPrices;
    vector<Order> buyOrders;
    vector<Order> sellOrders;

    void showError(const std::string &message);
    void initializeDatabase();
    void populateInitialData();
    void updateStockPrices();
    void loadOrdersFromDatabase();
    void executeTrade(const std::string& buyer, const std::string& seller, 
                      const std::string& symbol, double price, int quantity);
    void matchOrders(Order& newOrder);

public:
    Trader();
    ~Trader();

    void registerUser();
    void login();
    void placeOrder();
    void viewPrices();
    void checkPortfolio();
    void depositMoney();
    void withdrawMoney();
    void viewUserOrders();
    void cancelOrder(int orderId);
};

#endif 
