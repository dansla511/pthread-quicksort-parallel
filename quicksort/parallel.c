#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

//nastavenie velkosti nahodneho pola, poctu behov a vlakien
#define LEN 1048576
#define THREAD_COUNT 8
#define RUNS 100

//struktura ktora je pouzita pre podavanie argumentov do noveho vlakna
struct quicksort_items
{
    int* items;
    int left;
    int right;
};

int partition(int items[], int left, int right);
void quicksort(int items[], int left, int right);
void *parallel_quicksort(void* args);

int threads = 0;

//main velmi podobny sekvencnemu rieseniu
int main(){

    srand(time(NULL));

    int* data = (int*) malloc(sizeof(int) * LEN);

    double wall_times[RUNS];
    double cpu_times[RUNS];

    for(int j = 0 ; j < RUNS; j++){

        for(int i = 0; i < LEN; i++){
            data[i] = rand();
        }
        struct timeval start_wall, end_wall;

        gettimeofday(&start_wall, NULL);

        clock_t start_cpu = clock();

        quicksort(data, 0, LEN-1);

        clock_t end_cpu = clock();

        gettimeofday(&end_wall, NULL);

        double cpu_time = (double)(end_cpu - start_cpu) / CLOCKS_PER_SEC;
        double wall_time = (end_wall.tv_sec - start_wall.tv_sec) + (double)(end_wall.tv_usec - start_wall.tv_usec)/1000000;

        wall_times[j] = wall_time;
        cpu_times[j] = cpu_time;

        threads = 0;

    }

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

//tato funkcia je identicka ako v sekvencnom rieseni
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
    identicky koncept ako pri sekvencnom rieseni, akurat jedno rekurzivne zavolanie je presunute na nove vlakno
    parent vlakno potom caka kym child dokonci pracu na svojej casti pola
    pouzite sekvencne rekurzivne riesenie ak sme sa dostali na limit threadov
    optimalizacia pre menej ako 10000 prvkov v poli asi nie je potrebna
*/
void quicksort(int items[], int left, int right){

    if(left < right){
        int p = partition(items, left, right);
        if(threads <= THREAD_COUNT && right-left > 10000){
            pthread_t t;

            threads++;

            struct quicksort_items args = { items, left, p-1 };

            pthread_create(&t, NULL, parallel_quicksort, (void*) &args);

            quicksort(items, p+1, right);

            pthread_join(t, NULL);

        }
        else{
            quicksort(items, left, p-1);
            quicksort(items, p+1, right);
        }
        
    }

}

// funkcia ktora je pouzita pri vytvoreni noveho vlakna
void *parallel_quicksort(void* args){
    // ziskanie argumentov a zavolanie quicksort funkcie
    struct quicksort_items *arg = (struct quicksort_items*) args;
    quicksort(arg->items, arg->left, arg->right);
    pthread_exit(0);
}