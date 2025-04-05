#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <vector>
using namespace std;

const int MAX_STOCKS = 100;

struct Stock {
    string symbol;
    double price;
    double initialPrice;
    int quantity;
};

struct Transaction {
    string type;
    string symbol;
    double price;
    int quantity;
    string date;
};

struct Order {
    string username;
    string symbol;
    double price;
    int quantity;
    string type;
};

struct User {
    string username;
    string password;
    double balance;
    vector<Stock> portfolio;
    vector<Transaction> transactions;
    double overallProfitLoss;
};

#endif