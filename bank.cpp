
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <csignal>
#include <pthread.h>
#include <unistd.h>
#include "account.hpp"
#include "bank_class.hpp"
#include <ctime>
// #include <chrono>


using namespace std;

ofstream log_file;
bank* my_bank;

struct Thread_args {
            
            string file_name;
            int id;
};

void *bank_gets_commitions(void *arg){

    
    while(1){

        //Generate a random number between 1 and 5
        srand(static_cast<unsigned int>(time(nullptr)));
        int random_precent = rand() % 5 + 1;

        my_bank -> bank_commision(random_precent);

        sleep(3);
        
    }

    return nullptr;


}


void *print_status_to_screen(void *arg){

    
    while(1){

        my_bank -> print_all_accounts();

        usleep(500000);

    }

    return nullptr;
}


void *atm_func(void* arg){

    
    Thread_args* args = static_cast<Thread_args*>(arg);
    
    string name_of_file = args -> file_name;
    int atm_id = args -> id;

    

    ifstream file;
    file.open(name_of_file);

    if(!file.is_open()){
        cerr << "Bank error: illegal arguments" << endl;
        exit(1);
    }
    

    string line;
    char action;
    int account;
    string password;
    int amount;
    int dest_account;
        
    
        while (getline(file, line)){
        

            usleep(100000);
        
            int word_counter = 0;
            bool inWord = false;
        
            for (char c : line) {
                if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                    inWord = false;
                } else if (!inWord) {
                    inWord = true;
                    word_counter++;
                }
            }

            istringstream iss2(line);
            // Check if parsing was successful
            if (iss2.fail()){
                cerr << "Error parsing input" << endl;
                exit(1);
            }


            if(word_counter == 4){
                iss2 >> action >> account >> password >> amount;
            }
            else if(word_counter == 3){
                iss2 >> action >> account >> password;
            }
            else if(word_counter == 5){
                iss2 >> action >> account >> password >> dest_account >> amount;
            }
         

            switch (action){

                case 'O':
                
                    my_bank -> create_new_account(account, password, amount, atm_id);
                    break;
            
                case 'D':
                    my_bank -> deposite_to_account(account, password, amount, atm_id);
                    break;
            
                case 'W':
                    my_bank -> withdraw_from_account(account, password, amount, atm_id);
                    break;
            
                case 'B':
                    my_bank -> check_balance (account, password, atm_id);
                    break;
            
                case 'Q':
                    my_bank-> delete_account(account, password, atm_id);
                    break;

                case 'T':
                    my_bank -> transport_money_to_another_account(account, password, dest_account, amount, atm_id);
                    break;
            
                default:
                    cerr << "action does not exsist" << endl;
                    exit(1);
                    break;

            }
        
        }
    file.close();

    delete args;

    return nullptr;

}


int main(int argc, char *argv[]){


    if(argc <= 1){ //no arguments
        cerr << "Bank error: illegal arguments" << endl;
        exit(1);
    }

    
    log_file.open("log.txt");
    if (!log_file.is_open()) {
        cerr << "Failed to open log.txt" << endl;
        exit(1); // Return an error code if failed to open
    }

    my_bank = new bank();
    

    int thread_creation_res;
    //create thread to handle bank commisions
    pthread_t bank_commisions_thread;
    thread_creation_res = pthread_create(&bank_commisions_thread, nullptr, bank_gets_commitions, nullptr);
    if(thread_creation_res != 0){
        perror("Bank error: pthread_create failed");
        exit(1);
    }

    //create thread to handle periodic printing
    pthread_t printing_thread;
    thread_creation_res = pthread_create(&printing_thread, nullptr, print_status_to_screen, nullptr);
    if(thread_creation_res != 0){
        perror("Bank error: pthread_create failed");
        exit(1);
    }


    
    vector<pthread_t> atm_threads(argc-1);

    for(int i = 1; i < argc ; i++){

        string file = argv[i];

        Thread_args *args = new Thread_args;
    
        args -> file_name = file;
        args -> id = i;

        thread_creation_res = pthread_create(&atm_threads[i-1], nullptr, atm_func, args);
        if(thread_creation_res != 0){
            perror("Bank error: pthread_create failed");
            exit(1);
        }

    }

    for(int i = 1; i < argc ; i++){
        pthread_join(atm_threads[i-1], NULL);
    }

    log_file.close();

    delete my_bank;
}