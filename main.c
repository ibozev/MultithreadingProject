
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <mqueue.h>
#include <pthread.h>
#include <errno.h>

#define PATH_NAME "cars.bin"	// bin file with numbers
#define PATH_NAME_2 "nums.txt" // text file with random numbers in plain text
#define MAX_SIZE 200
#define FIRST_ARR_SIZE 80	//can be changed to a random number between 1 and 200
#define FIRST_QUEUE_NAME "/first_queue"
#define SECOND_QUEUE_NAME "/second_queue"
#define NUM_THREADS 4
#define WRITER_1 0
#define READER_1 1
#define WRITER_2 2
#define READER_2 3

typedef struct thread_data_t {
  char *queue_name;
  int *arr;
  int arr_size;
} thread_data_t;

void readTxtFile(int arr[]);
void createBinFile(int arr[]);
void readBinFile(int arr[], int arr_2[]);
void printArray(int arr[], int size);
void threadDataInit(thread_data_t *thr_data, char *q_name, int *arr, int a_size);
void *writeToQueue(void *arg);
void *readFromQueue(void *arg);
void changeWritingInterval();

mqd_t mq_1;
mqd_t mq_2;
float writingInterval = 1.00f;
float readingInterval = 1.00f;

int main() {
	pthread_t thread[NUM_THREADS];
	thread_data_t thr_data[NUM_THREADS];
	int result;
	int cars_1_input[FIRST_ARR_SIZE] = {0};
	int cars_1_output[FIRST_ARR_SIZE] = {0};
	int cars_2_input[MAX_SIZE - FIRST_ARR_SIZE] = {0};
	int cars_2_output[MAX_SIZE - FIRST_ARR_SIZE] = {0};

	/*
	 * these lines only create a .bin file from a .txt file,
	 * they can remain in a comment, unless .bin file doesn't exist
	 */
//	int nums[MAX_SIZE] = {0};
//	readTxtFile(nums);
//	createBinFile(nums);

	//reading binary file with cars (numbers)
	readBinFile(cars_1_input, cars_2_input);

	//printing first queue arrays before threads work
	printf("Cars in cars_1_input:");
	printArray(cars_1_input, FIRST_ARR_SIZE);
	printf("\n\nCars in cars_1_output:");
	printArray(cars_1_output, FIRST_ARR_SIZE);
	printf("\n");

	//printing second queue arrays before threads work
    	printf("\nCars in cars_2_input:");
	printArray(cars_2_input, MAX_SIZE-FIRST_ARR_SIZE);
    	printf("\n\nCars in cars_2_output:");
	printArray(cars_2_output, MAX_SIZE-FIRST_ARR_SIZE);
	printf("\n\n");

	//initializing pthreads arguments
	threadDataInit(&thr_data[WRITER_1], FIRST_QUEUE_NAME, cars_1_input, FIRST_ARR_SIZE);    
	threadDataInit(&thr_data[READER_1], FIRST_QUEUE_NAME, cars_1_output, FIRST_ARR_SIZE);
	threadDataInit(&thr_data[WRITER_2], SECOND_QUEUE_NAME, cars_2_input, MAX_SIZE - FIRST_ARR_SIZE);
	threadDataInit(&thr_data[READER_2], SECOND_QUEUE_NAME, cars_2_output, MAX_SIZE - FIRST_ARR_SIZE);
	    
	//creating writing threads
	for (int i = 0; i < NUM_THREADS-1; i+=2) {
 	    result = pthread_create(&thread[i], NULL, writeToQueue, &thr_data[i]);
	    if (result != 0) {
	        printf("ERROR; return code from pthread_create() is %d\n", result);
	        return EXIT_FAILURE;
	    }
	}

	/* only writing threads will be working for 15 units, at the end of this period
	 * 30 cars will be on the bridge
	 */
	usleep(15 * writingInterval * 1e6);

	//creating reading threads
	for (int i = 1; i < NUM_THREADS; i+=2) {
       	    result = pthread_create(&thread[i], NULL, readFromQueue, &thr_data[i]);
	    if (result != 0) {
	        printf("ERROR; return code from pthread_create() is %d\n", result);
	        return EXIT_FAILURE;
	    }
	}

	//joining all threads
    	for (int i = 0; i < NUM_THREADS; ++i) {
            result = pthread_join(thread[i], NULL);
            if (0!= result) {
                printf("ERROR; return code from pthread_join() is %d\n", result);
                return EXIT_FAILURE;
            }
	}

	//printing first queue arrays after pthread_join
	printf("\nCars that were sent to %s from cars_1_input:", FIRST_QUEUE_NAME);
	printArray(cars_1_input, FIRST_ARR_SIZE);
	printf("\n\nCars that were received from %s in cars_1_output:", FIRST_QUEUE_NAME);
	printArray(cars_1_output, FIRST_ARR_SIZE);
	printf("\n");

	//printing second queue arrays after pthread_join
        printf("\nCars that were sent to %s from cars_2_input:", SECOND_QUEUE_NAME);
	printArray(cars_2_input, MAX_SIZE-FIRST_ARR_SIZE);
        printf("\n\nCars that were received from %s in cars_2_output:", SECOND_QUEUE_NAME);
	printArray(cars_2_output, MAX_SIZE-FIRST_ARR_SIZE);
	printf("\n");

	pthread_exit(NULL);

	return EXIT_SUCCESS;
}

//reading .txt file into array of integers
void readTxtFile(int arr[]) {
	int i = 0;
	FILE *file = fopen(PATH_NAME_2, "r");

	fscanf(file, "%d", &arr[i++]);
	while(!feof (file))
		fscanf(file, "%d", &arr[i++]);

	fclose(file);
}

//writing array of integers in .bin file
void createBinFile(int arr[]) {
	int n, fd;

	fd = open(PATH_NAME, O_WRONLY);

	if (fd >= 0) {
		lseek(fd, 0, SEEK_SET);
		for (int i = 0; i <= MAX_SIZE; i++)
			write(fd, &arr[i], sizeof(n));
	}
	close(fd);
}

//reading .bin file and filling two input arrays
void readBinFile(int arr[], int arr_2[]) {
	int i = 0, fd, k = 0;

	fd = open(PATH_NAME, O_RDONLY);

	if (fd >= 0) {

		//filling first array
		lseek(fd, 0, SEEK_SET);
		while(i < FIRST_ARR_SIZE) {
			read(fd, &arr[i++], sizeof(int));
			if (i == FIRST_ARR_SIZE)
				break;
		}

		//filling second array
		lseek(fd, 0, SEEK_CUR);
		while(k < MAX_SIZE-FIRST_ARR_SIZE)
			read(fd, &arr_2[k++], sizeof(int));
	}
	close(fd);
}

void printArray(int arr[], int size) {
	for (int i = 0; i < size; ++i) {
		if (i % 10 == 0)
			printf("\n");
		printf("%d ", arr[i]);
	}
}

void threadDataInit(thread_data_t *thr_data, char *q_name, int *arr, int a_size) {
	thr_data->queue_name = q_name;
	thr_data->arr = arr;
	thr_data->arr_size = a_size;
}

void changeWritingInterval() {
	writingInterval /= 2;
}

void *writeToQueue(void *arg) {
	thread_data_t *data = (thread_data_t*) arg;
	mqd_t mq;
	int result;
	struct mq_attr attr;

	//initializing queue attributes
	attr.mq_flags = 0;
	attr.mq_maxmsg = 20;
	attr.mq_msgsize = 2*sizeof(int);
	attr.mq_curmsgs = 0;

	mq = mq_open(data->queue_name, O_CREAT | O_WRONLY, 0700, &attr);

    if (mq > -1) {
    	printf("%s writing starting...\n", data->queue_name);

    	for (int i = 0; i < data->arr_size; ++i) {
    		result = mq_send(mq, (char*) &data->arr[i], sizeof(int), 0);
    		if (result != 0) {
				printf("ERROR; return code from mq_send() is %d\n", result);
				printf("Element with %d = %d cannot be saved in %s\n",
						i, data->arr[i], data->queue_name);
				printf("%s\n\n", strerror(errno));
    		}
    		printf("%d was sent to %s\n", data->arr[i], data->queue_name);

    		//writing current number of messages in mqueue
    		result = mq_getattr(mq, &attr);
    		if (result == 0)
    			printf("Current messages in %s = %ld\n\n", data->queue_name, attr.mq_curmsgs);

    		usleep(writingInterval * 1e6);
    	}
        printf("Writing in %s finished.\n", data->queue_name);
    }
    else {
        printf("Failed to load %s!\n", data->queue_name);
        printf("%s\n\n", strerror(errno));
    }

    mq_close(mq);

	pthread_exit(NULL);
}

void *readFromQueue(void *arg) {
	thread_data_t *data = (thread_data_t*) arg;
	mqd_t mq;
	struct mq_attr attr;
	int result;

	//initializing queue attributes
	attr.mq_flags = 0;
	attr.mq_maxmsg = 20;
	attr.mq_msgsize = sizeof(int);
	attr.mq_curmsgs = 0;

	mq = mq_open(data->queue_name, O_CREAT | O_RDONLY, 0700, &attr);

    if (mq > -1) {
    	printf("%s reading starting...\n", data->queue_name);

    	for (int i = 0; i < data->arr_size; ++i) {
    		result = mq_receive(mq, (char*) &data->arr[i], 2*sizeof(int), 0);
    		if (result < 0) {
				printf("ERROR; return code from mq_receive() is %d\n", result);
				printf("Message cannot be received from %s\n", data->queue_name);
				printf("%s\n\n", strerror(errno));
    		}
    		printf("%d was received from %s\n", data->arr[i], data->queue_name);

    		result = mq_getattr(mq, &attr);
    		if (result == 0)
    			printf("Current messages in %s = %ld\n\n", data->queue_name, attr.mq_curmsgs);

    		//changing writing interval of the other mqueue
    		if (attr.mq_curmsgs == 0)
    			changeWritingInterval();

    		usleep(readingInterval * 1e6);
    	}
        printf("Reading from %s finished.\n", data->queue_name);
    }
    else {
        printf("Failed to load %s!\n", data->queue_name);
        printf("%s\n\n", strerror(errno));
    }

    mq_close(mq);
    mq_unlink(data->queue_name);

	pthread_exit(NULL);
}
