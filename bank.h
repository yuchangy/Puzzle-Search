// Project Identifier: 292F24D17A4455C1B5133EDD8C7CEAA0C9570A98
#include <algorithm>
#include <deque>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "getopt.h"

using namespace std;

struct transaction_info {
    int amount = 0;
    uint64_t placed_date;
    uint64_t exec_date;
    string sender;
    string receiver;
    int transaction_fee = 0;
    string payment_method;
    int id = 0;
};

struct user_info {
    uint64_t timestamp;
    string username;
    string pin;
    int balance = 0;
    uint64_t transactions = 0;
    bool status = false;
    unordered_set<string> ip_list;
};


class comparator {
public:
    bool operator()(transaction_info* lhs, transaction_info* rhs) {
        if (lhs->exec_date == rhs->exec_date) {
            return lhs->id > rhs->id;
        } else {
            return lhs->exec_date > rhs->exec_date;
        }
    }
};


class bank {
private:
    bool verbose = false;
    string file_name;
    unordered_map<string, user_info> registry;
    priority_queue<transaction_info*, vector<transaction_info*>, comparator>
      pq_transaction_list;   // transaction container
    vector<transaction_info*> master_list;
    unordered_map<string, vector<deque<transaction_info*>>> transaction_profiles;
    int id = 0;
    uint64_t current_time = 0;

public:
    ~bank();
    void get_opt(int argc, char* argv[]);
    void readFile();
    void start_op();
    void login();
    void logout();
    void transactions();
    bool trans_date_check(uint64_t timestamp, uint64_t set_date);
    bool basic_check(uint64_t regist_date, uint64_t set_date);   // greater than check
    transaction_info* set_transaction(transaction_info*& transaction, int amount, string sender, string receiver,
                                      uint64_t placed_time, uint64_t exec_date, int fee, string payment_method, int id);
    int transaction_fee(int amount, uint64_t regist_date, uint64_t exec_date);
    void execute_transaction(transaction_info* current);
    void execute_by_day(uint64_t set_date);
    void insert_user_transaction(transaction_info* current, string sender, string receiver);
    uint64_t time_converter(string timestamp);
    // query list:
    void list_transactions();
    void history();
    void bank_revenue();
    void summarize();
};