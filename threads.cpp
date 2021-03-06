#include <iostream>
#include <list>
#include <pthread.h>
#include <semaphore.h>
#include <random>

using std::cout;
using std::endl;
using std::list;

struct block {
    int unit;

    block(int unit) {
        this->unit = unit;
    }
};

list<block*> freelist;
list<block*> list1;
list<block*> list2;

sem_t binary_freelist, counting_freelist, counting_freelist2, binary_list1, counting_list1, binary_list2, counting_list2;

std::ostream& operator<<(std::ostream& os, const block* blk) {
    os << blk->unit << endl;
    return os;
}

int randomNum() {
    static thread_local std::mt19937 gen;
    std::uniform_int_distribution<int> distribution(1, 1000);
    return distribution(gen);
}

block* unlink(int lst) {
    block* rv;
    if (lst == 0) {
        cout << "Unlinking from Freelist" << endl;
        rv = freelist.back();
        freelist.pop_back();
    } 
    else if (lst == 1) {
        cout << "Unlinking from List-1 " << endl;
        rv = list1.back();
        list1.pop_back();
    } 
    else {
        cout << "Unlinking from List-2" << endl;
        rv = list2.back();
        list2.pop_back();
    }

    return rv;
}

void link(block* toLink, int lst) {
    if (lst == 0) {
        cout << "Linking to Freelist" << endl;
        freelist.push_back(toLink);
    } 
    else if (lst == 1) {
        cout << "Linking to List-1" << endl;
        list1.push_back(toLink);
    } 
    else {
        cout << "Linking to List-2" << endl;
        list2.push_back(toLink);
    }
}

void produce_information_in_block(block* n) {
    n->unit = randomNum();
}

void consume_information_in_block(block* n) {
    n->unit = 0;
}

void use_block_x_to_produce_info_in_y(block* x, block* y) {
    y->unit = x->unit;
    x->unit = 0;
}


void* thread1(void* ptr) {
    block* b;
    while (1) {
        sem_wait(&counting_freelist2);
        sem_wait(&binary_freelist);

        b = unlink(0);

        sem_post(&binary_freelist);

        produce_information_in_block(b);

        sem_wait(&binary_list1);

        link(b, 1);

        sem_post(&binary_list1);
        sem_post(&counting_list1);
     }

}

void* thread2(void* ptr) {
    block *x,*y;
    while (1) {
        sem_wait(&counting_list1);
        sem_wait(&binary_list1);

        x = unlink(1);

        sem_post(&binary_list1);

        sem_wait(&counting_freelist);
        sem_wait(&binary_freelist);

        y = unlink(0);

        sem_post(&binary_freelist);

        use_block_x_to_produce_info_in_y(x, y);

        sem_wait(&binary_freelist);

        link(x, 0);

        sem_post(&binary_freelist);
        sem_post(&counting_freelist);
        
        sem_wait(&binary_list2);

        link(y, 2);

        sem_post(&binary_list2);
        sem_post(&counting_list2);
     }

}

void* thread3(void* ptr) {
     block* c;
     while(1) {
         sem_wait(&counting_list2);
         sem_wait(&binary_list2);

         c = unlink(2);

         sem_post(&binary_list2);

         consume_information_in_block(c);

         sem_wait(&binary_freelist);

         link(c, 0);

         sem_post(&binary_freelist);
         sem_post(&counting_freelist);
         sem_post(&counting_freelist2);
     }
  
}

int main(int argc, char** argv) {

    if (argc != 2) {
        cout << "Usage: ./a.out <N>" << endl;
        return 0;
    }

    int N = atoi(argv[1]);

    for (int i = 0; i < N; i++) {
        freelist.push_back(new block(0));
    }

    sem_init(&binary_freelist, 0, 1);
    sem_init(&counting_freelist, 0, N);
    sem_init(&counting_freelist2, 0, N-1);

    
    sem_init(&binary_list1, 0, 1);
    sem_init(&counting_list1, 0, 0);

    sem_init(&binary_list2, 0, 1);
    sem_init(&counting_list2, 0, 0);


    pthread_t threads[3];

    int rc;

    rc = pthread_create(&threads[0], NULL, thread1, NULL);
    if (rc) {
         cout << "Unable to create thread" << rc << endl;
         exit(-1);
      }

    rc = pthread_create(&threads[1], NULL, thread2, NULL);
    if (rc) {
        cout << "Unable to create thread" << rc << endl;
        exit(-1);
    }

    rc = pthread_create(&threads[2], NULL, thread3, NULL);
    if (rc) {
        cout << "Unable to create thread" << rc << endl;
        exit(-1);
    }

     pthread_exit(NULL);

    return 0;
}
