#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

#define LEN 16777216
#define THREAD_COUNT 8
#define RUNS 100

// struktura pre ulohu, queue je spravene ako single linked list
struct quicksort_task
{
    int* items;
    int left;
    int right;
    struct quicksort_task* next;
};

//struktura pre ulahcenie prace s queue
struct task_queue 
{
    struct quicksort_task* head;    
    struct quicksort_task* tail;
};

int partition(int items[], int left, int right);
void quicksort(int items[], int left, int right);
void *worker();
struct quicksort_task* acquire_task();
void queue_add(int items[], int left, int right);
void queue_remove(struct quicksort_task* task);

//inicializacia potrebnych mutexov a inych premennych

pthread_t threads[THREAD_COUNT];
pthread_mutex_t mutex;
pthread_mutex_t task_mutex;
struct task_queue* queue;
//pouzite pri ukonceni prace
int kill_threads = 0;
//pouzite aby vlakna nezacali pracu skor ako je prvy partition dokonceny
int start_work = 0;

//pocitadlo taskov, pouzite pre urcenie, kedy je cele pole vytriedene (ked vlakna dokoncili vsetky pridelene tasky)
int tasks = 0;
int completed_tasks = 0;

//main znova skoro rovnaky ako pri sekvencnom a paralelnom rieseni
int main(){

    srand(time(NULL));

    int* data = (int*) malloc(sizeof(int) * LEN);

    queue = (struct task_queue*) malloc(sizeof(struct task_queue));
    queue->head = NULL;
    queue->tail = NULL;

    double wall_times[RUNS];
    double cpu_times[RUNS];

    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&task_mutex, NULL);


    for(int i = 0; i < THREAD_COUNT; i++){
        pthread_create(&threads[i], NULL, worker, NULL);
    }

    for(int j = 0 ; j < RUNS; j++){
        start_work = 0;

        for(int i = 0; i < LEN; i++){
            data[i] = rand();
        }
        struct timeval start_wall, end_wall;

        gettimeofday(&start_wall, NULL);

        clock_t start_cpu = clock();

        //quicksort funkcia teraz neblokuje, takze je potrebny block v podobe while cyklu

        quicksort(data, 0, LEN-1);

        start_work = 1;

        while(tasks != completed_tasks){
        }

        clock_t end_cpu = clock();

        gettimeofday(&end_wall, NULL);

        double cpu_time = (double)(end_cpu - start_cpu) / CLOCKS_PER_SEC;
        double wall_time = (end_wall.tv_sec - start_wall.tv_sec) + (double)(end_wall.tv_usec - start_wall.tv_usec)/1000000;

        wall_times[j] = wall_time;
        cpu_times[j] = cpu_time;
    }

    kill_threads = 1;

    for(int i = 0; i < THREAD_COUNT; i++){
        pthread_join(threads[i], 0);
    }

    pthread_mutex_destroy(&mutex);
    free(queue);
    free(data);

    double total_wall_time = 0;
    double total_cpu_time = 0;

    for(int i = 0; i < RUNS; i++){
        total_wall_time += wall_times[i];
        total_cpu_time += cpu_times[i];
    }

    double avg_wall_time = (double)total_wall_time / RUNS;
    double avg_cpu_time = (double)total_cpu_time / RUNS;

    printf("Average CPU Time: %f s\n", avg_cpu_time);
    printf("Average Wall Time: %f s\n", avg_wall_time);

    return 0;
}

//partition rovnaky ako pri sekvencnom rieseni
int partition(int items[], int left, int right){

    int pivot = right;

    if(right - left > 3){
        pivot = (items[pivot] > items[left+((right-left)/2)]) && (items[pivot] < items[left]) ? pivot : left+(right-left)/2;
        pivot = items[pivot] > items[left] ? pivot : left;
    }

    int temp = items[right];
    items[right] = items[pivot];
    items[pivot] = temp;

    int i = left - 1;

    for(int j = left; j <= right-1; j++){
        if(items[j] < items[right]){
            i++;
            int t = items[i];
            items[i] = items[j];
            items[j] = t;
        }
    }

    temp = items[i+1];
    items[i+1] = items[right];
    items[right] = temp;

    return i+1;

}

/*
    princip quicksortu je stale rovnaky, no namiesto rekurzie sa vytvaraju ulohy pre vlakna ktore sa ukladaju do radu

    optimalizacia pre zmenu na sekvencny pristup ak je pocet prvkov mensi ako 10000, vysoky overhead z taskov ak
    by sme chceli vsetko riesit cez ne
*/
void quicksort(int items[], int left, int right){
    
    if(left < right){
        if(right-left > 10000){
            int p = partition(items, left, right);
            queue_add(items, left, p-1);
            queue_add(items, p+1, right);

        }
        else{
            int p = partition(items, left, right);
            quicksort(items, left, p-1);
            quicksort(items, p+1, right);
        }

    }
}

// worker funkcia pre thready, vyberu si task ak nejaky je v rade a vykonaju ho
void *worker(){

    while(start_work == 0){}

    while(kill_threads == 0){   

        struct quicksort_task* t = acquire_task();

        if(t == NULL){
            continue;
        }


        quicksort(t->items, t->left, t->right);

        free(t);

        //potreba mutexu kedze je mozne ze k premennej pristupia 2 alebo viac vlakien naraz
        
        pthread_mutex_lock(&task_mutex);

        completed_tasks++;

        pthread_mutex_unlock(&task_mutex);

    }

    pthread_exit(0);
}

// vyber volneho tasku, tato funkcia potrebuje mutex kedze manipuluje s task queue
struct quicksort_task* acquire_task(){
    
    pthread_mutex_lock(&mutex);

    struct quicksort_task* t = queue->head;

    if(t == NULL){
        pthread_mutex_unlock(&mutex);
        return NULL;
    }

    queue_remove(t);

    pthread_mutex_unlock(&mutex);

    return t;

}

// pridanie noveho tasku do queue, tato funkcia potrebuje mutex kedze manipuluje s task queue
void queue_add(int items[], int left, int right){

    struct quicksort_task* task = (struct quicksort_task*) malloc(sizeof(struct quicksort_task));

    task->items = items;
    task->left = left;
    task->right = right;
    task->next = NULL;

    pthread_mutex_lock(&mutex);

    if(queue->head == NULL){
        queue->head = task;
        queue->tail = task;
    }
    else{
        queue->tail->next = task;
        queue->tail = task;
    }

    tasks++;

    pthread_mutex_unlock(&mutex);

}

// odstranenie tasku z queue, nepotrebuje mutex kedze je pouzita iba v acquire_task a ten uz mutex pouziva
void queue_remove(struct quicksort_task* task){

    if(task->next == NULL){
        queue->head = NULL;
        queue->tail = NULL;
    }
    else{
        queue->head = task->next;
    }

}
