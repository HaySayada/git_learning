#ifndef BANK_H
#define BANK_H


#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>
#include "account.hpp"
#include <cmath>
#include <string>

using namespace std;

extern ofstream log_file;




pthread_mutex_t mutex_log;

class bank{

    private:
        vector<account> accounts_vec;
        int bank_balance;
        pthread_mutex_t bank_rd_lock;
        pthread_mutex_t bank_wr_lock;
        int bank_readers;


        void enter_bank_reader(){

            if(pthread_mutex_lock(&bank_rd_lock)){
                perror("Bank error: pthread_mutex_lock failed");
                exit(1);
            }
            
            bank_readers++;
            if(bank_readers == 1){
                if(pthread_mutex_lock(&bank_wr_lock)){
                    perror("Bank error: pthread_mutex_lock failed");
                    exit(1);
                }
            }
            if(pthread_mutex_unlock(&bank_rd_lock)){
                perror("Bank error: pthread_mutex_unlock failed");
                exit(1);
            }
        }


        void leave_bank_reader(){
            
            if(pthread_mutex_lock(&bank_rd_lock)){
                 perror("Bank error: pthread_mutex_lock failed");
                 exit(1);
            }

            bank_readers--;
            if(bank_readers == 0){
                if(pthread_mutex_unlock(&bank_wr_lock)){
                    perror("Bank error: pthread_mutex_unlock failed");
                    exit(1);
                }
            }

            if(pthread_mutex_unlock(&bank_rd_lock)){
                    perror("Bank error: pthread_mutex_unlock failed");
                    exit(1);
            }
            
            
        }


        void enter_bank_writer(int atm_id){

            if (pthread_mutex_lock(&bank_wr_lock)) {
                perror("Bank error: pthread_mutex_lock failed");
                exit(1);
            }
            
        }


        void leave_bank_writer(int atm_id){

            if (pthread_mutex_unlock(&bank_wr_lock)) {
                perror("Bank error: pthread_mutex_unlock failed");
                exit(1);
            }

        }

    public:

        bank(){
            bank_balance = 0;   
            bank_readers = 0;

            if(pthread_mutex_init(&bank_rd_lock, NULL) != 0){
                 perror("Bank error: pthread_mutex_init failed");
                exit(1);
            }
            if(pthread_mutex_init(&bank_wr_lock, NULL) != 0){
                 perror("Bank error: pthread_mutex_init failed");
                exit(1);
            }   
            if(pthread_mutex_init(&mutex_log, NULL) != 0){
                 perror("Bank error: pthread_mutex_init failed");
                exit(1);
            }   
               
        }


        ~bank(){
            if(pthread_mutex_destroy(&bank_rd_lock) ||
            pthread_mutex_destroy(&bank_wr_lock) ||
            pthread_mutex_destroy(&mutex_log)){
                perror("Bank error: pthread_mutex_destroy failed");
                    exit(1);
            }

            accounts_vec.clear();
        }
            

       

        account* find_account_in_bank(int account_number) {
            
            for (account& acc : accounts_vec) {
            
                if (acc.get_account_number() == account_number) {
                     return &acc;
                 }
            }
            
            return nullptr;
        }


        void create_new_account(int account_number_arg, string password, int amount, int atm_id){


            enter_bank_writer(atm_id);

                    
            //check if account num already exsists
             if (find_account_in_bank(account_number_arg) == nullptr){ //account doesnt exist

                account new_account = account(account_number_arg, password, amount);

                //place accont in vector in a sorted way
                auto it = std::lower_bound(accounts_vec.begin(), accounts_vec.end(), new_account);
                accounts_vec.insert(it, new_account);

                sleep(1);

                if(pthread_mutex_lock(&mutex_log)){
                    perror("Bank error: pthread_mutex_lock failed");
                    exit(1);
                }
                log_file << atm_id << ": New account id is " << account_number_arg << " with password " << password << " and initial balance " << amount << endl;
                if(pthread_mutex_unlock(&mutex_log)){
                     perror("Bank error: pthread_mutex_unlock failed");
                     exit(1);
                }

             }
             else{ // account already exist in bank
                sleep(1);
                if(pthread_mutex_lock(&mutex_log)){
                    perror("Bank error: pthread_mutex_lock failed");
                    exit(1);
                }
                log_file << "Error " << atm_id << ": Your transaction failed - account with the same id exists" << endl;
                if(pthread_mutex_unlock(&mutex_log)){
                     perror("Bank error: pthread_mutex_unlock failed");
                     exit(1);
                }
             }
            
            
             leave_bank_writer(atm_id);
             return;

        }




        void delete_account(int account_number_arg, string password, int atm_id){

            enter_bank_writer(atm_id);

            account *wanted_account;
            wanted_account = find_account_in_bank(account_number_arg);

            if (wanted_account == nullptr){ //not found

               log_file << "Error " << atm_id << ": Your transaction failed - account id " << account_number_arg <<  " does not exist" << endl;
               leave_bank_writer(atm_id);
               return;
            }
            else{//account found 
    
                if(wanted_account -> get_password() != password){
                    log_file << "Error " << atm_id << ": Your transaction failed - password for account id " << account_number_arg <<  " is incorrect" << endl;
                    leave_bank_writer(atm_id);
                    return;
                }
                else{
                    int *balance = new int;
                    for (auto it = accounts_vec.begin(); it != accounts_vec.end(); ++it) {
                        if (it->get_account_number() == account_number_arg) {
                             accounts_vec.erase(it);
                             wanted_account -> get_balance(balance);
                             log_file << atm_id << ": Account " << account_number_arg << " is now closed. Balance was " <<  *balance << endl;
                             delete balance;
                        leave_bank_writer(atm_id);
                        return; // Account deleted, exit function
                        }
                    }
                }

            }
        }


        void deposite_to_account(int account_number_arg, string password, int amount, int atm_id){

            enter_bank_reader();
        
            sleep(1);

            account *wanted_account;
            wanted_account = find_account_in_bank(account_number_arg);
            if(!wanted_account){ //account does not exsist in bank
                if(pthread_mutex_lock(&mutex_log)){
                    perror("Bank error: pthread_mutex_lock failed");
                    exit(1);
                }
                log_file << "Error " << atm_id << ": Your transaction failed - account id " << account_number_arg <<  " does not exist" << endl;
                if(pthread_mutex_unlock(&mutex_log)){
                    perror("Bank error: pthread_mutex_unlock failed");
                    exit(1);
                }
                leave_bank_reader();
                return;
            }

            if(password != wanted_account->get_password()){ //incorrect passwprd
                if(pthread_mutex_lock(&mutex_log)){
                    perror("Bank error: pthread_mutex_lock failed");
                    exit(1);
                }
                log_file << "Error " << atm_id << ": Your transaction failed - password for account id " << account_number_arg <<  " is incorrect" << endl;
                if(pthread_mutex_unlock(&mutex_log)){
                    perror("Bank error: pthread_mutex_unlock failed");
                    exit(1);
                }
                leave_bank_reader();
                return;
            }

            
            wanted_account -> deposite(amount);

            //write to log file
            if(pthread_mutex_lock(&mutex_log)){
                    perror("Bank error: pthread_mutex_lock failed");
                    exit(1);
            }
            int *balance = new int;
            wanted_account->get_balance(balance);
            log_file << atm_id << ": Account " << account_number_arg << " new balance is " << *balance << " after " << amount <<" $ was deposited" << endl;
            delete balance;
            if(pthread_mutex_unlock(&mutex_log)){
                perror("Bank error: pthread_mutex_unlock failed");
                exit(1);
            }
            

            leave_bank_reader();

        }


        void withdraw_from_account(int account_number_arg, string password, int amount, int atm_id){

            enter_bank_reader();

            sleep(1);

            account* wanted_account = find_account_in_bank(account_number_arg);

            if(!wanted_account){ //account does not exist in bank
                if(pthread_mutex_lock(&mutex_log)){
                    perror("Bank error: pthread_mutex_lock failed");
                    exit(1);
                }
                log_file << "Error " << atm_id << ": Your transaction failed - account id " << account_number_arg <<  " does not exist" << endl;
                if(pthread_mutex_unlock(&mutex_log)){
                    perror("Bank error: pthread_mutex_unlock failed");
                    exit(1);
                }                
                leave_bank_reader();
                return;
            }

            if(password != wanted_account->get_password()){ //password is incorrect
                if(pthread_mutex_lock(&mutex_log)){
                    perror("Bank error: pthread_mutex_lock failed");
                    exit(1);
                }
                log_file << "Error " << atm_id << ": Your transaction failed - password for account id " << account_number_arg <<  " is incorrect" << endl;
                if(pthread_mutex_unlock(&mutex_log)){
                    perror("Bank error: pthread_mutex_unlock failed");
                    exit(1);
                }
                leave_bank_reader();
                return;
            }

            int *balance = new int;
            wanted_account->get_balance(balance);

            if(amount > *balance){ //not enough money in account
                if(pthread_mutex_lock(&mutex_log)){
                    perror("Bank error: pthread_mutex_lock failed");
                    exit(1);
                }
                log_file << "Error " << atm_id << ": Your transaction failed - account id " << account_number_arg << " balance is lower than " << amount << endl;
                if(pthread_mutex_unlock(&mutex_log)){
                    perror("Bank error: pthread_mutex_unlock failed");
                    exit(1);
                }

                leave_bank_reader();
                return;
            }
            
            wanted_account -> withdraw(amount);

            //write to log
            if(pthread_mutex_lock(&mutex_log)){
                    perror("Bank error: pthread_mutex_lock failed");
                    exit(1);
            }
            wanted_account->get_balance(balance);
            log_file << atm_id << ": Account " << account_number_arg << " new balance is " << *balance << " after " << amount <<" $ was withdrew" << endl;
            if(pthread_mutex_unlock(&mutex_log)){
                perror("Bank error: pthread_mutex_unlock failed");
                exit(1);
            }

            delete balance;
            leave_bank_reader();
        }


        void check_balance (int account_number_arg, string password, int atm_id){

            enter_bank_reader();

            sleep(1);

            account* wanted_account = find_account_in_bank(account_number_arg);

            if(!wanted_account){ //account does not exsist in bank
                if(pthread_mutex_lock(&mutex_log)){
                    perror("Bank error: pthread_mutex_lock failed");
                    exit(1);
                }
                log_file << "Error " << atm_id << ": Your transaction failed - account id " << account_number_arg <<  " does not exist" << endl;
                if(pthread_mutex_unlock(&mutex_log)){
                    perror("Bank error: pthread_mutex_unlock failed");
                    exit(1);
                }
                leave_bank_reader();
                return;
            }

            if(password != wanted_account->get_password()){ //incorrect password
                if(pthread_mutex_lock(&mutex_log)){
                    perror("Bank error: pthread_mutex_lock failed");
                    exit(1);
                }
                log_file << "Error " << atm_id << ": Your transaction failed - password for account id " << account_number_arg <<  " is incorrect" << endl;
                if(pthread_mutex_unlock(&mutex_log)){
                    perror("Bank error: pthread_mutex_unlock failed");
                    exit(1);
                }
                leave_bank_reader();
                return;
            }

            int *balance = new int;
            wanted_account -> get_balance(balance);

            //write to lock
            if(pthread_mutex_lock(&mutex_log)){
                    perror("Bank error: pthread_mutex_lock failed");
                    exit(1);
            }
            log_file << atm_id << ": Account " << account_number_arg << " balance is " << *balance << endl;
            if(pthread_mutex_unlock(&mutex_log)){
                perror("Bank error: pthread_mutex_unlock failed");
                exit(1);
            }

            delete balance;
            leave_bank_reader();
        }


        void transport_money_to_another_account(int account_number_arg, string password, int dest_account, int amount, int atm_id){

            enter_bank_reader();

            sleep(1);

            account* src_account = find_account_in_bank(account_number_arg);
            account* dst_account = find_account_in_bank(dest_account);

            if(!src_account){ //src account does not exsist in bank
                if(pthread_mutex_lock(&mutex_log)){
                    perror("Bank error: pthread_mutex_lock failed");
                    exit(1);
                }
                log_file << "Error " << atm_id << ": Your transaction failed - account id " << account_number_arg <<  " does not exist" << endl;
                if(pthread_mutex_unlock(&mutex_log)){
                    perror("Bank error: pthread_mutex_unlock failed");
                    exit(1);
                }
                    leave_bank_reader();
                return;
            }

            if(!dst_account){ //dst account does not exsist in bank
                if(pthread_mutex_lock(&mutex_log)){
                    perror("Bank error: pthread_mutex_lock failed");
                    exit(1);
                }
                log_file << "Error" << atm_id << ": Your transaction failed - account id " << dest_account <<  " does not exist" << endl;
                if(pthread_mutex_unlock(&mutex_log)){
                    perror("Bank error: pthread_mutex_unlock failed");
                    exit(1);
                }
                leave_bank_reader();
                return;
            }

            if(password != src_account->get_password()){ //incorrect password
                if(pthread_mutex_lock(&mutex_log)){
                    perror("Bank error: pthread_mutex_lock failed");
                    exit(1);
                }
                log_file << "Error" << atm_id << ": Your transaction failed - password for account id " << account_number_arg <<  " is incorrect" << endl;
                if(pthread_mutex_unlock(&mutex_log)){
                    perror("Bank error: pthread_mutex_unlock failed");
                    exit(1);
                }
                leave_bank_reader();
                return;
            }

            int *balance = new int;
            src_account->get_balance(balance);

            if(amount > *balance){//not enought money in bank
                if(pthread_mutex_lock(&mutex_log)){
                    perror("Bank error: pthread_mutex_lock failed");
                    exit(1);
                }
                log_file << "Error" << atm_id << ": Your transaction failed - account id " << account_number_arg << " balance is lower than " << amount << endl;
                if(pthread_mutex_unlock(&mutex_log)){
                    perror("Bank error: pthread_mutex_unlock failed");
                    exit(1);
                }
                leave_bank_reader();
                return;
            }

            //perform transition
            src_account -> withdraw(amount);
            dst_account -> deposite(amount);


            int *dest_balance = new int;
            src_account -> get_balance(balance);
            dst_account -> get_balance(dest_balance);

            //print to log
            if(pthread_mutex_lock(&mutex_log)){
                perror("Bank error: pthread_mutex_lock failed");
                exit(1);
            }
            log_file << atm_id << ": Transfer " << amount << " from account " << account_number_arg << " to account " << dest_account << " new account balance is " << *balance << " new target account balance is " << *dest_balance << endl;
            if(pthread_mutex_unlock(&mutex_log)){
                perror("Bank error: pthread_mutex_unlock failed");
                exit(1);
            }

            delete dest_balance;
            delete balance;
            leave_bank_reader();

        }


        void bank_commision(int random_precent){

            enter_bank_reader();

            int commision_amount;

            for (account& acc : accounts_vec) { //take commision from each account
                
                commision_amount = acc.take_commision(random_precent); 

                this->bank_balance += commision_amount;

            }

            leave_bank_reader();

        }




        void print_all_accounts(){

            enter_bank_reader();

            string mutual_print;

            cout << "\033[2J";
            cout << "\033[1;1H";
            cout << "Current Bank Status" << endl;

            for (auto& obj : accounts_vec) {

                 mutual_print.append(obj.print_account());
                 mutual_print.append("\n");
            }

            cout << mutual_print;

            cout << "The Bank has " << this->bank_balance << " $" << endl;

            leave_bank_reader();

        }
};


#endif