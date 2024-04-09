// Project Identifier: 292F24D17A4455C1B5133EDD8C7CEAA0C9570A98
#include "bank.h"

#include <algorithm>
#include <deque>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <ctype.h>
#include <stdio.h>

#include "getopt.h"

using namespace std;

bank::~bank() {
    for (size_t i = 0; i < master_list.size(); i++) {
        delete master_list[i];
    }
}

void bank::get_opt(int argc, char* argv[]) {
    opterr = false;
    string arg;
    int choice = 0;
    int index = 0;
    option long_options[] = {
        {"verbose",       no_argument, nullptr, 'v'},
        {   "file", required_argument, nullptr, 'f'},
        {   "help",       no_argument, nullptr, 'h'},
        {  nullptr,       no_argument, nullptr,   0}
    };

    while ((choice = getopt_long(argc, argv, "vhf:", long_options, &index)) != -1) {
        switch (choice) {
        case 'h': {
            cout << "help \n";
            exit(0);
            break;
        }

        case 'f': {
            arg = optarg;
            break;
        }

        case 'v': {
            verbose = true;
            break;
        }


        default: {
            cerr << "error" << endl;
            exit(1);
            break;
        }
        }
    }
    if (arg.empty()) {
        cerr << "no input file given" << endl;
        exit(1);
    } else {
        file_name = arg;
    }
}


void bank::readFile() {
    // getline and break it into a substring
    ifstream ifs(file_name);
    if (!ifs.is_open()) {
        cerr << "Failed to open file" << endl;
        exit(1);
    }

    string timestamp;
    string pin;
    string balance;
    string username;

    while (getline(ifs, timestamp, '|') && getline(ifs, username, '|') && getline(ifs, pin, '|')
           && getline(ifs, balance)) {
        user_info user;
        user.timestamp = time_converter(timestamp);
        user.pin = pin;
        user.balance = stoi(balance);
        user.username = username;
        registry[username] = user;
    }
    ifs.close();
}
// transaction functions:
void bank::transactions() {
    transaction_info* transaction = new transaction_info;
    string ip;
    string placed_date;
    string exec_date;
    bool trans_date = true;
    bool regist_date_sender = true;
    bool regist_date_receiver = true;
    bool valid = true;
    cin >> placed_date >> ip >> transaction->sender >> transaction->receiver >> transaction->amount >> exec_date
      >> transaction->payment_method;
    auto it = registry.find(transaction->sender);
    auto it2 = registry.find(transaction->receiver);
    if (it == registry.end()) {
        valid = false;
        if (verbose == true) {
            cout << "Sender " << transaction->sender << " does not exist.\n";
        }
        return;

    } else if (it2 == registry.end()) {
        valid = false;
        if (verbose == true) {
            cout << "Recipient " << transaction->receiver << " does not exist.\n";
        }
        return;
    }
    transaction->placed_date = time_converter(placed_date);
    transaction->exec_date = time_converter(exec_date);
    trans_date = trans_date_check(transaction->placed_date, transaction->exec_date);
    regist_date_sender = basic_check(it->second.timestamp, transaction->exec_date);
    regist_date_receiver = basic_check(it2->second.timestamp, transaction->exec_date);
    // Basic Check
    if (transaction->placed_date < current_time) {
        cerr << "Invalid decreasing timestamp in 'place' command.\n";
        exit(1);
    } else {
        current_time = transaction->placed_date;
    }
    if (transaction->exec_date < current_time) {
        cerr << "You cannot have an execution date before the current timestamp.\n";
        exit(1);
    }

    if (trans_date == false) {
        valid = false;
        if (verbose == true) {
            cout << "Select a time less than three days in the future.\n";
        }
        delete transaction;
        return;

    } else if (it == registry.end()) {
        valid = false;
        if (verbose == true) {
            cout << "Sender " << transaction->sender << " does not exist.\n";
        }
        delete transaction;
        return;

    } else if (it2 == registry.end()) {
        valid = false;
        if (verbose == true) {
            cout << "Receiver " << transaction->receiver << " does not exist.\n";
        }
        delete transaction;
        return;

    } else if (regist_date_sender == false || regist_date_receiver == false) {
        valid = false;
        if (verbose == true) {
            cout << "At the time of execution, sender and/or recipient have not registered.\n";
        }
        delete transaction;
        return;

    } else if (it->second.ip_list.size() == 0) {
        valid = false;
        if (verbose == true) {
            cout << "Sender " << transaction->sender << " is not logged in.\n";
        }
        delete transaction;
        return;
    }
    // Fraud Check:
    else if (it->second.ip_list.find(ip) == it->second.ip_list.end()) {
        valid = false;
        if (verbose == true) {
            cout << "Fraudulent transaction detected, aborting request.\n";
        }
        delete transaction;
        return;

    } else if (transaction->placed_date > transaction->exec_date) {
        valid = false;
        if (verbose == true) {
            cout << "You cannot have an execution date before the current timestamp.\n";
        }
        return;
    }

    if (!pq_transaction_list.empty()) {
        execute_by_day(current_time);
    }

    if (valid == true) {
        transaction->transaction_fee
          = transaction_fee(transaction->amount, it->second.timestamp, transaction->exec_date);
        transaction->id = id;
        pq_transaction_list.push(transaction);
        current_time = transaction->placed_date;
        if (verbose) {
            cout << "Transaction placed at " << transaction->placed_date << ": $" << transaction->amount << " from "
                 << transaction->sender << " to " << transaction->receiver << " at " << transaction->exec_date << ".\n";
        }
        id++;
    }
}

bool bank::trans_date_check(uint64_t timestamp, uint64_t set_date) {
    if (set_date - timestamp > 3000000) {
        return false;
    }
    return true;
}

int bank::transaction_fee(int amount, uint64_t regist_date, uint64_t exec_date) {
    int result = 0;

    result = amount / 100;
    if (result < 10) {
        result = 10;
    }

    if (result > 450) {
        result = 450;
    }
    if (exec_date - regist_date > 50000000000) {
        result = (result * 3) / 4;
    }

    return result;
}

void bank::execute_transaction(transaction_info* current) {
    transaction_info* new_transaction = new transaction_info;
    new_transaction
      = set_transaction(new_transaction, current->amount, current->sender, current->receiver, current->placed_date,
                        current->exec_date, current->transaction_fee, current->payment_method, current->id);

    auto sender = registry.find(current->sender);
    auto receiver = registry.find(current->receiver);
    if (current->payment_method == "o") {
        if (sender->second.balance - (current->transaction_fee + current->amount) < 0) {
            if (verbose == true) {
                cout << "Insufficient funds to process transaction " << current->id << ".\n";
            }
        } else {
            sender->second.balance -= (current->amount + current->transaction_fee);
            receiver->second.balance += current->amount;
            if (verbose == true) {
                cout << "Transaction executed at " << current->exec_date << ": $" << current->amount << " from "
                     << current->sender << " to " << current->receiver << ".\n";
            }
            insert_user_transaction(new_transaction, current->sender, current->receiver);
            master_list.push_back(new_transaction);
            sender->second.transactions++;
            receiver->second.transactions++;
        }
    }
    if (current->payment_method == "s") {
        int split_fee_sender = 0;
        int split_fee_receiver = 0;

        if (current->transaction_fee % 2 == 1) {
            split_fee_sender = (current->transaction_fee / 2) + 1;
            split_fee_receiver = (current->transaction_fee / 2);
        } else {
            split_fee_sender = current->transaction_fee / 2;
            split_fee_receiver = current->transaction_fee / 2;
        }
        if (sender->second.balance - (split_fee_sender + current->amount) < 0
            || receiver->second.balance - split_fee_receiver < 0) {
            if (verbose == true) {
                cout << "Insufficient funds to process transaction " << current->id << ".\n";
            }
        } else {
            sender->second.balance -= (current->amount + split_fee_sender);
            receiver->second.balance += (current->amount - split_fee_receiver);
            if (verbose == true) {
                cout << "Transaction executed at " << current->exec_date << ": $" << current->amount << " from "
                     << current->sender << " to " << current->receiver << ".\n";
            }
            insert_user_transaction(new_transaction, current->sender, current->receiver);
            master_list.push_back(new_transaction);
            sender->second.transactions++;
            receiver->second.transactions++;
        }
    }
    pq_transaction_list.pop();
}

void bank::execute_by_day(uint64_t current_time) {
    // Amount Check:
    while (!pq_transaction_list.empty()) {
        if (!basic_check(pq_transaction_list.top()->exec_date, current_time)) {
            break;
        }
        transaction_info* current = pq_transaction_list.top();
        execute_transaction(current);
    }
}

void bank::insert_user_transaction(transaction_info* current, string sender, string receiver) {
    if (transaction_profiles.find(sender) == transaction_profiles.end()) {
        vector<deque<transaction_info*>> user_transactions(2);
        transaction_profiles.insert({ sender, user_transactions });
    }
    if (transaction_profiles.find(receiver) == transaction_profiles.end()) {
        vector<deque<transaction_info*>> user_transactions(2);
        transaction_profiles.insert({ receiver, user_transactions });
    }

    transaction_profiles[sender][1].push_back(current);
    transaction_profiles[receiver][0].push_back(current);
}


// login logouts//////////
void bank::login() {
    string name;
    string pin;
    string ip_address;
    cin >> name >> pin >> ip_address;
    auto it = registry.find(name);
    if (it != registry.end() && it->second.pin == pin) {
        if (verbose == true) {
            cout << "User " << name << " logged in.\n";
        }
        it->second.ip_list.insert(ip_address);
        it->second.status = true;
    } else {
        if (verbose == true) {
            cout << "Failed to log in " << name << ".\n";
        }
    }
}

void bank::logout() {
    string name;
    string ip;
    cin >> name >> ip;
    auto it = registry.find(name);
    auto iterator = it->second.ip_list.find(ip);

    if (it->second.ip_list.size() != 0 && it->second.status == true) {
        if (iterator == it->second.ip_list.end()) {
            if (verbose == true) {
                cout << "Failed to log out " << name << ".\n";
            }
        } else {
            if (verbose == true) {
                cout << "User " << name << " logged out.\n";
            }
            it->second.ip_list.erase(iterator);
            it->second.status = false;
        }
    } else {
        if (verbose == true) {
            cout << "Failed to log out " << name << ".\n";
        }
    }
}

// helper functions:
transaction_info* bank::set_transaction(transaction_info*& transaction, int amount, string sender, string receiver,
                                        uint64_t placed_date, uint64_t set_date, int fee, string payment_method,
                                        int id) {
    transaction->amount = amount;
    transaction->receiver = receiver;
    transaction->sender = sender;
    transaction->placed_date = placed_date;
    transaction->exec_date = set_date;
    transaction->transaction_fee = fee;
    transaction->payment_method = payment_method;
    transaction->id = id;
    return transaction;
}

bool bank::basic_check(uint64_t lhs, uint64_t rhs) {
    if (lhs < rhs) {
        return true;
    }
    return false;
}

uint64_t bank::time_converter(string timestamp) {
    std::stringstream ss(timestamp);
    uint64_t num1_1, num2_1, num3_1, num4_1, num5_1, num6_1;
    char delimiter;
    ss >> num1_1 >> delimiter >> num2_1 >> delimiter >> num3_1 >> delimiter >> num4_1 >> delimiter >> num5_1
      >> delimiter >> num6_1;
    uint64_t time = 0;
    // 00:00:00:00:01:00
    //  Year:
    time += 10000000000 * num1_1;
    // Month:
    time += 100000000 * num2_1;
    // Day:
    time += 1000000 * num3_1;
    // Hour:
    time += 10000 * num4_1;
    // Minute:
    time += 100 * num5_1;
    // Seconds:
    time += 1 * num6_1;
    return time;
}


// query list functions:
void bank::list_transactions() {
    string x;
    string y;
    string dollars;
    uint64_t start = 0;
    uint64_t end = 0;
    int num = 0;

    cin >> x >> y;
    start = time_converter(x);
    end = time_converter(y);

    for (size_t i = 0; i < master_list.size(); i++) {
        if (basic_check(master_list[i]->exec_date, end) && basic_check(start, master_list[i]->exec_date)) {
            if (master_list[i]->amount == 1) {
                dollars = " dollar to ";
            } else {
                dollars = " dollars to ";
            }
            cout << master_list[i]->id << ": " << master_list[i]->sender << " sent " << master_list[i]->amount
                 << dollars << master_list[i]->receiver << " at " << master_list[i]->exec_date << ".\n";
            num++;
        }
    }
    cout << "There were " << num << " transactions that were placed between time " << start << " to " << end << ".\n";
}

void bank::history() {
    string customer;
    cin >> customer;
    auto it = registry.find(customer);
    auto transaction_it = transaction_profiles.find(customer);
    size_t incoming = 0;
    size_t outgoing = 0;

    if (it == registry.end()) {
        cout << "User " << customer << " does not exist.\n";
    } else {
        if (transaction_it == transaction_profiles.end()) {
            incoming = 0;
            outgoing = 0;
        } else {
            incoming = transaction_profiles[customer][0].size();
            outgoing = transaction_profiles[customer][1].size();
        }
        cout << "Customer " << customer << " account summary:\n";
        cout << "Balance: $" << it->second.balance << "\n";
        cout << "Total # of transactions: " << it->second.transactions << "\n";
        if (transaction_it != transaction_profiles.end()) {
            cout << "Incoming " << incoming << ":\n";
            for (size_t i = 0; i < transaction_profiles[customer][0].size(); i++) {
                if (i == 10) {
                    break;
                }
                cout << transaction_profiles[customer][0][i]->id << ": " << transaction_profiles[customer][0][i]->sender
                     << " sent " << transaction_profiles[customer][0][i]->amount;
                if (transaction_profiles[customer][0][i]->amount == 1) {
                    cout << " dollar to ";
                } else {
                    cout << " dollars to ";
                }
                cout << transaction_profiles[customer][0][i]->receiver << " at "
                     << transaction_profiles[customer][0][i]->exec_date << ".\n";
            }
            cout << "Outgoing " << outgoing << ":\n";
            for (size_t i = 0; i < transaction_profiles[customer][1].size(); i++) {
                if (i == 10) {
                    break;
                }
                cout << transaction_profiles[customer][1][i]->id << ": " << transaction_profiles[customer][1][i]->sender
                     << " sent " << transaction_profiles[customer][1][i]->amount;
                if (transaction_profiles[customer][1][i]->amount == 1) {
                    cout << " dollar to ";
                } else {
                    cout << " dollars to ";
                }
                cout << transaction_profiles[customer][1][i]->receiver << " at "
                     << transaction_profiles[customer][1][i]->exec_date << ".\n";
            }
        } else {
            cout << "Incoming " << incoming << ":\n";
            cout << "Outgoing " << outgoing << ":\n";
        }
    }
}

void bank::bank_revenue() {
    string start;
    string end;
    cin >> start >> end;
    uint64_t start_time = time_converter(start);
    uint64_t end_time = time_converter(end);
    uint64_t difference = end_time - start_time;
    int revenue = 0;
    uint64_t year = 0;
    uint64_t month = 0;
    uint64_t day = 0;
    uint64_t hour = 0;
    uint64_t minute = 0;
    uint64_t second = 0;
    size_t start_index = 0;
    while (start_index < master_list.size() && master_list[start_index]->exec_date < start_time) {
        start_index++;
    }

    for (; start_index < master_list.size(); start_index++) {
        if (master_list[start_index]->exec_date > end_time) {
            break;
        }
        revenue += master_list[start_index]->transaction_fee;
    }

    year = difference / 10000000000;
    difference = difference - (year * 10000000000);
    month = difference / 100000000;
    difference = difference - (month * 100000000);
    day = difference / 1000000;
    difference = difference - (day * 1000000);
    hour = difference / 10000;
    difference = difference - (hour * 10000);
    minute = difference / 100;
    difference = difference - (minute * 100);
    second = difference;
    cout << "281Bank has collected " << revenue << " dollars in fees over";
    if (year != 0) {
        if (year == 1) {
            cout << " 1 year";
        } else {
            cout << " " << year << " years";
        }
    }
    if (month != 0) {
        if (month == 1) {
            cout << " 1 month";
        } else {
            cout << " " << month << " months";
        }
    }
    if (day != 0) {
        if (day == 1) {
            cout << " 1 day";
        } else {
            cout << " " << day << " days";
        }
    }
    if (hour != 0) {
        if (hour == 1) {
            cout << " 1 hour";
        } else {
            cout << " " << hour << " hours";
        }
    }
    if (minute != 0) {
        if (minute == 1) {
            cout << " 1 minute";
        } else {
            cout << " " << minute << " minutes";
        }
    }
    if (second != 0) {
        if (second == 1) {
            cout << " 1 second";
        } else {
            cout << " " << second << " seconds";
        }
    }
    cout << ".\n";
}

void bank::summarize() {
    string time;
    uint64_t transactions = 0;
    int revenue = 0;
    uint64_t time_num = 0;
    uint64_t start = 0;
    uint64_t end = 0;
    string dollars;
    cin >> time;
    time_num = time_converter(time);
    start = (time_num / 1000000) * 1000000;
    end = start + 1000000;
    cout << "Summary of [" << start << ", " << end << "):\n";

    for (size_t i = 0; i < master_list.size(); i++) {
        uint64_t current_time = master_list[i]->exec_date;
        if (current_time >= start && current_time < end) {
            transactions++;
            revenue += master_list[i]->transaction_fee;
            if (master_list[i]->amount == 1) {
                dollars = " dollar to ";
            } else {
                dollars = " dollars to ";
            }
            cout << master_list[i]->id << ": " << master_list[i]->sender << " sent " << master_list[i]->amount
                 << dollars << master_list[i]->receiver << " at " << master_list[i]->exec_date << ".\n";
        }
    }
    if (transactions == 1) {
        cout << "There was a total of " << transactions << " transaction";
    } else {
        cout << "There were a total of " << transactions << " transactions";
    }
    cout << ", 281Bank has collected " << revenue << " dollars in fees.\n";
}


// main structure function:
void bank::start_op() {
    string command;
    string junk;
    while (command != "$$$") {
        cin >> command;
        if (command[0] == '#') {
            getline(cin, junk);
        } else if (command == "login") {
            login();
        } else if (command == "out") {
            logout();
        } else if (command == "place") {
            transactions();
        } else if (cin.fail()) {
            cerr << "gg\n";
            exit(1);
        }
    }
    while (!pq_transaction_list.empty()) {
        transaction_info* current = pq_transaction_list.top();
        execute_transaction(current);
    }

    while (cin >> command) {
        if (cin.fail()) {
            cerr << "done\n";
            exit(1);
        }
        if (command == "l") {
            list_transactions();
        } else if (command == "h") {
            history();
        } else if (command == "r") {
            bank_revenue();
        } else if (command == "s") {
            summarize();
        }
    }
}


int main(int argc, char* argv[]) {
    bank test;
    test.get_opt(argc, argv);
    test.readFile();
    test.start_op();
    return 0;
}