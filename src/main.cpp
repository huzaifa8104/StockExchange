#include "Trader.h"
#include <iostream>
#include <limits>
using namespace std;

int main() {
    Trader trader;
    int choice;

    do {
        cout << "\nStock Exchange System\n";
        cout << "1. Register\n";
        cout << "2. Login\n";
        cout << "3. Place Order\n";
        cout << "4. View Prices\n";
        cout << "5. Check Portfolio\n";
        cout << "6. Deposit Money\n";
        cout << "7. Withdraw Money\n";
        cout << "8. Exit\n";
        cout << "Choice: ";
        cin >> choice;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a number.\n";
            continue;
        }

        switch (choice) {
            case 1:
                trader.registerUser();
                break;
            case 2:
                trader.login();
                break;
            case 3:
                trader.placeOrder();
                break;
            case 4:
                trader.viewPrices();
                break;
            case 5:
                trader.checkPortfolio();
                break;
            case 6:
                trader.depositMoney();
                break;
            case 7:
                trader.withdrawMoney();
                break;
            case 8:
                cout << "Exiting...\n";
                break;
            default:
                cout << "Invalid choice\n";
        }
    } while (choice != 8);

    return 0;
}
