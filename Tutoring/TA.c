/*
	Troy Frazier
	7/20/2018
*/

/*
	Program Explaination: A Teacher's Assistant (TA) thread is created along with a user defined set of student threads.
	TA will have 3 states: 
	1: Sleeping no students are in line or being helped
	2: Waiting: waiting for next student to be tutored
	3: Tutoring: tutoring students
	Students will sit in a chair which will act like a FIFO queue and wait for the tutor to be available.
	Amount of chairs are defined by the user.
	If there are no free chairs then a student will leave and come back some other time
*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

/*
  office -> TA thread that tutors a student
  tutoring -> student thread that goes to be tutored
 */

void *office(void*);
void *tutoring(void*);

/*
  help = amount of times a student will return to help
  chairs = total chairs for students to sit in to wait for help
  students = total students that will ask for help

  officeMutex = prevents TA thread from ending before any student threads
  taMutex = puts the TA to sleep and is used to alert the TA when a student is present
  studentMutex = The amount of students sitting down and waiting to be helped
  studentLock = ensures only 1 student is in the office and alerts the TA
  taLock = used to alert the TA the student is finished being tutored
  chairLock = Ensures only 1 student attempts to sit down in a chair at a time
 */

int help;
int chairs;
int students;
int tutorTime;

sem_t officeMutex;
sem_t taMutex;
sem_t studentMutex;
sem_t studentLock;
pthread_mutex_t taLock;
pthread_mutex_t chairLock;

/*
  count = amount of threads to create in a for loop
  studentTid = dynamic allocation of thread ids
  ta = TA thread id
  attr = attribute of threads which is set to default
  
 */

int main(int argc, char* argv[]){
  srand(time(0));
  int count;
  pthread_t *studentTid;
  pthread_t ta;
  pthread_attr_t attr;
  
  if(argc != 4){ //Parameter error
    fprintf(stderr,"USAGE: ./tutor #students #chairs #help   Ex -> ./tutor 3 2 2 \n");
    exit(1);
  }

  students = atoi(argv[1]);
  chairs = atoi(argv[2]);
  help = atoi(argv[3]);

  if(students < 1 || chairs < 1 || help < 1){ //Parameter data type error
    fprintf(stderr,"USAGE: ./tutor #students #chairs #help   Ex -> ./tutor 3 2 2 \n");
    exit(1);
  }

  //Mutex/Sem inits + tid allocation
  pthread_mutex_init(&taLock,NULL);
  pthread_mutex_init(&chairLock,NULL);
  sem_init(&studentLock,0,1);
  sem_init(&officeMutex,0,1);
  sem_init(&taMutex,0,1);
  sem_init(&studentMutex,0,chairs);
  studentTid = malloc(sizeof(pthread_t)*students);
 
  sem_wait(&officeMutex); //Used to ensure TA does not end early
  pthread_create(&ta,&attr,office,"1"); //TA thread creation
  
  for(count = 0; count < students; count++){ //Student thread creation
    pthread_create(&studentTid[count],&attr,tutoring,"1");
    sleep((rand()%3)+1); //Random time of 1-3 seconds for a new student to enter to create unpredictability
  }
 
 
  for(count = 0; count < students; count++) //Wait on student threads
    pthread_join(studentTid[count],NULL);

  printf("All student threads have ended.\n");

  sem_post(&officeMutex); //Allows TA thread to end
  pthread_join(ta,NULL); //Wait on TA thread


  printf("All Threads ended \n");
  
  //Lock destruction + tid deallocation
  sem_destroy(&studentLock);
  pthread_mutex_destroy(&taLock);
  sem_destroy(&taMutex);
  sem_destroy(&studentMutex);
  pthread_mutex_destroy(&chairLock);
  free(studentTid);
  return 0;
}

/*
  count = loops untill all students are helped
  studentWait = checks if a student is in the office
  chairWait = checks if a student is sitting on a chair
 */

void* office(void *param){
  int count;
  int studentWait;
  int chairWait;

  printf("TA Created.\n");

 for(count = 0; count < students*help; count++){

   sem_getvalue(&studentMutex,&chairWait); //Value of how many students are sitting down
   sem_getvalue(&studentLock,&studentWait); //Check to see if a student is in the office

   if(chairWait >= chairs && studentWait >= 1){ //If no students are waiting: Sleep
      printf("TA Goes to sleep.\n");
      sem_wait(&taMutex);
      sem_wait(&taMutex);
    }

   pthread_mutex_lock(&taLock); //Waits untill student is ready before tutoring begins
   printf("TA Tutors Student for %d.\n",tutorTime);
    sleep(tutorTime); //Tutor action
    //pthread_mutex_lock(&taLock);
    printf("TA is ready for the next student\n\n");
    
 }
 sem_wait(&officeMutex);//Waits untill all student threads end before ending TA thread
 printf("TA Thread ends.\n");
  
 pthread_exit(0);
}

/*
  count = loops untill a student fufills help parameter
  taStatus = check if TA is asleep
  studentValue = A student's "name" used for output
 */

void* tutoring(void *param){
  int count;
  int taStatus;
  int chairStatus;
  int studentValue = rand() % 100;

  printf("Student %d created.\n",studentValue);
  
  for(count = 0; count < help; count++){
    sleep((rand()%3)+1); //Random time before student decides to go to the tutoring office

    do{// If chair is not avaible then leave and come back
      pthread_mutex_lock(&chairLock); //Only one student can check a chair at a time
      sem_getvalue(&studentMutex,&chairStatus);

      if(chairStatus <= 0){
	int stuSleep = (rand()%3)+1; //Rand variable for a student to sleep
	printf("No available chairs, Student %d Leaves: %d \n",studentValue,stuSleep);
	sleep(stuSleep);
	pthread_mutex_unlock(&chairLock); //Allows another student to check the chair
      }
      
    }while(chairStatus <= 0);
   
    sem_wait(&studentMutex); //Student sits down
    printf("Student %d sits down \n",studentValue);
    
    pthread_mutex_unlock(&chairLock); //Allows students to start checking chairs again
    sem_wait(&studentLock); //Prevents students from entering office
    
    tutorTime = (rand()%3)+1;
    printf("Student %d enters office.\n",studentValue);

    sem_post(&studentMutex); //Tells the chair queue a student has left the queue
    pthread_mutex_unlock(&taLock); //Signals the TA to begin tutoring

    sem_getvalue(&taMutex,&taStatus);
  
    if(taStatus <= 0){ // if true: TA is asleep and needs to be woken up
      printf("Student %d wakes up TA.\n",studentValue);
      sem_post(&taMutex);
      sem_post(&taMutex);
    }

    sleep(tutorTime);//Tutoring process
    printf("Student %d leaves office.\n",studentValue);
    sem_post(&studentLock);//Allows another student to enter office
  }

  printf("Student %d thread ends.\n",studentValue);
  pthread_exit(0);
}
