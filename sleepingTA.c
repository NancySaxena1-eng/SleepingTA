#include <stdio.h>
#include <stdlib.h>
#include <string.h> // memset
#include <pthread.h> // pthread_t, pthread_create, pthread_join
#include <semaphore.h> // sem_init, sem_wait, sem_post
#include <time.h>
#include <unistd.h>

#define NUM_SEAT 3

#define SLEEP_MAX 5

//sematphores for students and Teaching Assistant
sem_t sem_stu;
sem_t sem_ta;

//mutex to insure the teaching process concurrency
pthread_mutex_t mutex;

int chair[NUM_SEAT]; //chairs 3 and one student should be learning from TA
int count = 0; //number of waiting students
enum status {
    programming,
    goToTa,
    learning,
    terminated,
};
enum taStatus {
    sleeping,
    assisting,
};
int taState;
int next_seat = 0;
int next_teach = 0;

int studentInTaRoom=0;
void rand_sleep(void);
void* stu_programming();
void* ta_teaching();
int* student_ids;
int student_num;
int* count_times;
int num_times=0;
int *studentStatus; //status of students - waiting/learning/finished learning 
pthread_t *students;
pthread_t ta;
int j;
int main(int argc, char **argv){


    //index
    int i;
    printf("Starting Sleeping teaching Assitant..");
    printf("\nTwo parameters will be required from the user end\n1)Number of students\n2)Number of times student can ask question\n 0= infinite question\n n>0 students can ask n number of students");
    printf("\nNote: The seating of students will always be in order 1,2 and 3.The wating queue is FIFO\n");
    //get number of students from user
    printf("\nHow many students coming to TA's office? ");
    scanf("%d", &student_num);
    printf("\n How many times a student can ask question to TA?");
    scanf("%d", &num_times);
    if (student_num==0){
        taState = sleeping;
        pthread_create(&ta, NULL, ta_teaching, NULL);
    }
    //initialize all parameters
    else{
        students = (pthread_t*)malloc(sizeof(pthread_t) * student_num);
        // Array of student Ids
        student_ids = (int*)malloc(sizeof(int) * student_num);
        // Array of student current status
        studentStatus = (int*)malloc(sizeof(int) * student_num);
        count_times = (int*)malloc(sizeof(int) * student_num);
        taState = sleeping;
        memset(student_ids, programming, student_num);
        memset(count_times, 0, student_num);
        sem_init(&sem_stu, 0, 0);
        sem_init(&sem_ta, 0, 1);

        //set random
        srand(time(NULL));

        //sem_init(&mutex,0,1);
        pthread_mutex_init(&mutex, NULL);

        //create TA thread
        pthread_create(&ta, NULL, ta_teaching, NULL);

        //create Students threads
        for(i = 0; i < student_num; i++)
        {
            student_ids[i] = i + 1;
            pthread_create(&students[i], NULL, stu_programming, (void*) &student_ids[i]);
            studentStatus[i] = 0;
        }

        rand_sleep();

        for (i=0; i < student_num; i++)
        {
            pthread_join(students[i], NULL);
        }
    }
    //joining main thread
    pthread_join(ta, NULL);
    return 0;
}
//int studentInTaRoom=0;
void* stu_programming(void* stu_id)
{
    int id = *(int*)stu_id;

    while (1)
    {
        switch (studentStatus[id - 1]) {
            case programming:
                pthread_mutex_lock(&mutex);
                printf("\nThread Student: Student %d is programming.\n ", id);
                pthread_mutex_unlock(&mutex);
                rand_sleep();
                //printf("\nThread Student: Student %d has a question.\n ", id);
                pthread_mutex_unlock(&mutex);
                studentStatus[id - 1] = goToTa;
                break;
            case goToTa:
                pthread_mutex_lock(&mutex);

                printf("\nThread Student:Student has a question. Student %d check for availabilty of TA \n", id);

                if (count <= NUM_SEAT)
                {
                    if (count > 0) {
                      // printf("\nThread Student: Student %d is on seat %d \n ", id, next_seat);
                       // printf("\nThread Student: Students waiting on seats [1] %d [2] %d [3] %d\n", chair[0],chair[1],chair[2]);
                        chair[next_seat] = id;
                        printf("\nThread Student: Students waiting on seats [1] %d [2] %d [3] %d\n", chair[0],chair[1],chair[2]);
                        next_seat = (next_seat + 1) % NUM_SEAT;
                    } else {
                       // printf("\nThread Student: Student %d is entering TA room directly, wakes up the TA.\n ", id);
                        studentInTaRoom = id;
                    }
                    count++;
                    pthread_mutex_unlock(&mutex);

                    //wakeup ta
                    sem_post(&sem_stu);
                    sem_wait(&sem_ta);
                    studentStatus[ id - 1] = learning;
                } else {
                    printf("\nThread Student: No seats are available, Student %d is going back to programming\n", id);
                    pthread_mutex_unlock(&mutex);
                    studentStatus[ id - 1] = programming;
                }
                break;
        case terminated:
                    pthread_exit(NULL);
                 break;
        case learning:
                 break;
        }
    }
}

void* ta_teaching()
{
    while(1)
    {
        //waiting for students while sleeping in between
        switch (taState) {
            case sleeping:
                pthread_mutex_lock(&mutex);
                printf("\nThread TA: TA is sleeping\n");
                pthread_mutex_unlock(&mutex);
                sem_wait(&sem_stu);
                pthread_mutex_lock(&mutex);
                printf("\nThread TA: TA is woken up by the student %d for assistance\n", studentInTaRoom);
                pthread_mutex_unlock(&mutex);
                taState = assisting;
                break;
            case assisting:
                pthread_mutex_lock(&mutex);
                //printf("\nThread TA: TA is assisting student %d\n", studentInTaRoom);
                rand_sleep();
                count_times[studentInTaRoom - 1] +=1;
                printf("\nThread TA: Student %d got help %d times\n", studentInTaRoom, count_times[studentInTaRoom - 1]);
                count--;
               // sem_post(&sem_ta);
               if(num_times !=0){
                //count_times[studentInTaRoom - 1] +=1;
                if (count_times[studentInTaRoom - 1] == num_times){
                    printf("\nThe student has asked max number of question,student %d is going home",studentInTaRoom);
                    studentStatus[studentInTaRoom - 1] = terminated;
                }
                else{
                    studentStatus[studentInTaRoom - 1] = programming;
                  }
              }
               else{

                    studentStatus[studentInTaRoom - 1] = programming;
               }

                sem_post(&sem_ta);
                if (count > 0) {
                    studentInTaRoom = chair[next_teach];
                    chair[next_teach] = 0;
                    next_teach = (next_teach + 1) % NUM_SEAT;
                    pthread_mutex_unlock(&mutex);
                    sem_wait(&sem_stu);
                } else {
                    printf("\nThread TA: TA checks the hall, no students are waiting. TA is going to sleep\n");
                    taState = sleeping;
                    pthread_mutex_unlock(&mutex);

                }

                break;
        }
    }

}
void rand_sleep(void){
    int time = rand() % SLEEP_MAX + 1;
    sleep(time);
}
