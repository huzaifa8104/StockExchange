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
        cout << "8. View Orders\n";
        cout << "9. Cancel Order\n";
        cout << "10. Exit\n";
        cout << "Choice: ";
        cin >> choice;

        switch(choice) {
            case 1: trader.registerUser(); break;
            case 2: trader.login(); break;
            case 3: trader.placeOrder(); break;
            case 4: trader.viewPrices(); break;
            case 5: trader.checkPortfolio(); break;
            case 6: trader.depositMoney(); break;
            case 7: trader.withdrawMoney(); break;
            case 8: trader.viewUserOrders(); break;
            case 9: {
                int orderId;
                cout << "Enter Order ID to cancel: ";
                if (cin >> orderId) {
                    trader.cancelOrder(orderId);
                } else {
                    cout << "Invalid input!\n";
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                }
                break;
            }
            case 10: cout << "Exiting...\n"; break;
            default: cout << "Invalid choice\n";
        }
    } while(choice != 10);

    return 0;
}
