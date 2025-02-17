#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

//nastavenie velkosti nahodneho pola a poctu behov
#define LEN 1048576
#define RUNS 100

int find_pivot(int items[], int left, int right);
int partition(int items[], int left, int right);
void quicksort(int items[], int left, int right);

int main(){

    srand(time(NULL));

    // inicializacia: vytvorenie pola prvkov, poli na casove vysledky
    int* data = (int*) malloc(sizeof(int) * LEN);

    double wall_times[RUNS];
    double cpu_times[RUNS];

    for(int j = 0 ; j < RUNS; j++){

        for(int i = 0; i < LEN; i++){
            data[i] = rand();
        }
        struct timeval start_wall, end_wall;

        //start merania casu

        gettimeofday(&start_wall, NULL);

        clock_t start_cpu = clock();


        //algoritmus
        quicksort(data, 0, LEN-1);


        //koniec merania casu
        clock_t end_cpu = clock();

        gettimeofday(&end_wall, NULL);

        //vypocet casov a pridanie do poli
        double cpu_time = (double)(end_cpu - start_cpu) / CLOCKS_PER_SEC;
        double wall_time = (end_wall.tv_sec - start_wall.tv_sec) + (double)(end_wall.tv_usec - start_wall.tv_usec)/1000000;

        wall_times[j] = wall_time;
        cpu_times[j] = cpu_time;

    }

    //upratanie
    free(data);

    //vypocet priemeru casov
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
}

//inspirovane riesenim z https://www.geeksforgeeks.org/quick-sort/

//funkcia ktora rozdeluje pole podla pivotu 
int partition(int items[], int left, int right){

    //vyber pivotu - median z prveho, stredneho a posledneho prvku
    int pivot = right;

    if(right - left > 3){
        pivot = (items[pivot] > items[left+((right-left)/2)]) && (items[pivot] < items[left]) ? pivot : left+(right-left)/2;
        pivot = items[pivot] > items[left] ? pivot : left;
    }

    //presunutie pivotu na koniec pola

    int temp = items[right];
    items[right] = items[pivot];
    items[pivot] = temp;

    //pozicia na ktoru sa pivot neskor dostane
    int i = left - 1;

    //presunutie vsetkych mensich prvkov ako pivot na zaciatok pola
    for(int j = left; j <= right-1; j++){
        if(items[j] < items[right]){
            i++;
            int t = items[i];
            items[i] = items[j];
            items[j] = t;
        }
    }

    //umiestnenie pivotu do spravnej pozicie
    temp = items[i+1];
    items[i+1] = items[right];
    items[right] = temp;

    return i+1;

}

//quicksort - rekurzivne volanie pomocou miesta pivotu z funkcie partition
void quicksort(int items[], int left, int right){

    if(left < right){
        int p = partition(items, left, right);
        quicksort(items, left, p-1);
        quicksort(items, p+1, right);
    }

}