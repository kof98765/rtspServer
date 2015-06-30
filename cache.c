#include <stdio.h> 
#include <stdlib.h> 
#include <stdbool.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <pthread.h> 
#include <assert.h> 

/************************************************
** A pool contains some following structure nodes 
** which are linked together as a link-list, and
** every node includes a thread handler for the
** task, and its argument if it has any.
*************************************************/

int draw_init();
void draw(char *path,char* path2);
int flip(unsigned char*,int,int);


typedef struct task
{ 
	void *(*process) (void *); 
	void *arg;	// arguments for a task 

	struct task *next; 

}task; 
typedef struct pack
{
	int timestamp;
	int length;
	char data[1024];
	struct pack *next;
}pack;
struct pic_data
{
	int index;
	int offset;
	int length;
	char buf[1024];
};
/*
** the thread pool
*/
typedef struct 
{ 
	pthread_mutex_t queue_lock; 
	pthread_cond_t queue_ready; 

	// all waiting tasks
	pack *head;
	pack *tail;

	/*
	** this field indicates the thread-pool's
	** state, if the pool has been shut down
	** the field will be set to true, and it is
	** false by default.
	*/
	bool shutdown; 

	// containning all threads's tid
	pthread_t *tids; 

	// max active taks
	int thread_num; 

	// current waitting tasks
	int cur_queue_size; 

}thread_pool; 

int pool_add_task(void *(*) (void *), void *); 
void *thread_routine(void *); 


static thread_pool *pool = NULL; 
unsigned char buffer[3276800];
 int timestamp1,timestamp2;
 volatile static int lengh = 0,length = 0;
   char buf[20];
     FILE *file = 0; 	
  volatile static int i = 0;
  volatile static int j = 1;
 struct pic_data *pic_data;

int prosess_pack(int timestamp,int len,char*buf)
{
	static int tmp=0;
       static int cnt=0;
     
	pic_data=(struct pic_data *)buf;
      // printf("offset=%d\n",pic_data->offset);
       if(tmp!=pic_data->index&&tmp!=0)
       {
           if(length-cnt<1024)
           {
                
                if((file = fopen("1.jpg", "wb")) < 0)
        		{
        	   			printf("Unable to create y frame recording file\n");
        	   			 exit(-1);
        		}
        				
        		printf("buffer write %d bytes into file.\n",length);
        		fwrite(buffer, length, 1, file);
        		fclose(file);
        			//exit(0);
        		
        		draw("1.jpg",NULL);
           }
       }
        tmp=pic_data->index;
        cnt=pic_data->offset;
       length=pic_data->length;
   
	memcpy(&buffer[pic_data->offset],pic_data->buf,1024);
                   
	

}
int add_pack(int timestamp,int length,char *buf)
{
	
	pack *p=(struct pack*)calloc(1,sizeof(struct pack));
	p->length=length;
	p->timestamp=timestamp;
	p->next=NULL;
	memcpy(p->data,buf,length);
	
	//pthread_mutex_lock(&(pool->queue_lock)); 
	//printf("timestamp=%d\n",timestamp);
	pack *last_one = pool->head;
	if(last_one == NULL)
		pool->head = p;
	else{
		while (last_one->next != NULL)
			last_one = last_one->next;
		last_one->next = p; 
	}
	pool->cur_queue_size++;
	
	
	//pthread_mutex_unlock(&(pool->queue_lock));
	
	//pthread_cond_signal(&(pool->queue_ready));
	
}
void pool_init(int thread_num) 
{ 
	pool = (thread_pool *)malloc (sizeof(thread_pool)); 

	pthread_mutex_init (&(pool->queue_lock), NULL); 
	pthread_cond_init (&(pool->queue_ready), NULL); 
	
	pool->head = NULL; 
	
	pool->thread_num = thread_num; 
	pool->cur_queue_size = 0; 

	pool->shutdown = false; 

	pool->tids = 
		(pthread_t *)malloc(thread_num * sizeof(pthread_t));

	/*
	** create max_thread_num threads
	*/
	int i;
	for(i=0; i<thread_num; i++)
		pthread_create(&(pool->tids[i]), NULL,
			thread_routine ,NULL); 
	 draw_init();

} 

// add task into the pool!
int pool_add_task(void *(*process)(void *), void *arg) 
{ 
	/*
	** instruct a new task structure
	** 
	** fill the field by parameters pass by and
	** set the next to NULL, then add this task
	** node to the thread-pool
	*/
	task *newtask = (task *)malloc(sizeof(task)); 
	newtask->process = process;
	newtask->arg = arg;
	newtask->next = NULL;

	/*
	** the queue is a critical source, thus whenever
	** operates the queue, it should be protected by
	** a mutex or a semaphore.
	*/

	// ====================================== //
	pthread_mutex_lock(&(pool->queue_lock)); 
	// ====================================== //

	/*
	** find the last worker is the pool and then add this
	** one to its tail
	**
	** NOTE: since the worker list which pointed by queue_head
	** has no head-node(which means queue_head could be NULL
	** at first), we should deal with the empty queue carefully.
	*/
	task *last_one = pool->head;
	if(last_one == NULL)
		pool->head= newtask;
	else{
		while (last_one->next != NULL)
			last_one = last_one->next;
		last_one->next = newtask; 
	}

	pool->cur_queue_size++; // waiting tasks increase

	// ====================================== //
	pthread_mutex_unlock (&(pool->queue_lock)); 
	// ====================================== //

	// wake up waiting task
	

	return 0; 
} 

void *thread_routine(void *arg)
{ 
	pack *p;
	int tmp;
	while(1){ 
		
		// ====================================== //
		//pthread_mutex_lock (&(pool->queue_lock)); 
		// ====================================== //

		/*
		** routine will waiting for a task to run, and the
		** condition is cur_queue_size == 0 and the pool
		** has NOT been shutdowned.
		*/if(pool->cur_queue_size==0)
		{
			// pthread_mutex_unlock (&(pool->queue_lock));
			sleep(1);
			 continue;
		}
			
		/*
		** the pool has been shutdowned.
		** unlock before any break, contiune or return
		*/
		if (pool->shutdown){
		    pthread_mutex_unlock (&(pool->queue_lock)); 
		    pthread_exit (NULL); 
		} 
		//printf("%d\n",pthread_self());
		/*
		** consume the first work in the work link-list
		*/
		pool->cur_queue_size--; 
		p = pool->head; 
		pool->head = p->next; 
		// ====================================== //
		//pthread_mutex_unlock(&(pool->queue_lock)); 
		// ====================================== //
		
		
		/*
		** Okay, everything is ready, now excutes the process
		** from the worker, with its argument.
		*/
		
		//printf("p->length:%d\n",p->length);
		//printf("Got packet %d! of pic %d!\n",i,j);
		if(p->length==4)
		{
			length=*(int *)p->data;
			tmp=length;
			i=0;
		}
		if(p->length!=4)
		{
			//printf("length:%d\n",length);
			//fseek(file,length,SEEK_SET);
			memcpy(&buffer[1024*i],p->data,p->length); 
			//fwrite(p->data, p->length, 1, file);
			tmp -= p->length;
			//printf("length:%d\n",length);
			i ++; 
			//printf("pack->length%d\n",p->length);
		}
		
		if(tmp==0)
		{
		
			if((file = fopen("1.jpg", "wb")) < 0)
			{
       			printf("Unable to create y frame recording file\n");
       			 exit(-1);
			}
  			
			printf("buffer write %d bytes into file.\n",length);
			fwrite(buffer, length, 1, file);
			fclose(file);
			//exit(0);
			i=0;//write over so reset i
			j ++ ;//write pic and j++
			draw("1.jpg",1);
			//flip(buffer,320,240);
			
			length=0;
			//printf("draw end\n");
			
				
		}
		

		
		
		/*
		** when the work is done, free the source and continue
		** to excute another work.
		*/
		free(p); 
		p = NULL; 
	} 

	pthread_exit(NULL); 
} 

void *myprocess(void *arg) 
{ 
	printf("I am %u, arg: %d\n", (int)pthread_self(), (int)arg);
	sleep(1);

	return NULL; 
} 

/*
** tasks waiting in the queue will be discarded
** but will wait for the tasks which are still
** running in the pool
*/
int pool_destroy(void) 
{ 
	// make sure it wont be destroy twice
	if(pool->shutdown) 
	   return -1;

	pool->shutdown = true;  // set the flag

	// wake up all of the tasks
	pthread_cond_broadcast(&(pool->queue_ready)); 


	// wait for all of the task exit
	int i; 
	for(i=0; i < pool->thread_num; i++) 
		pthread_join(pool->tids[i], NULL); 

	free(pool->tids); 

	// destroy the queue
	task *p = NULL; 
	while (pool->head != NULL) 
	{ 
		p = pool->head; 
		pool->head = pool->head->next; 
		free(p); 
	} 

	// destroy the mutex and cond
	pthread_mutex_destroy(&(pool->queue_lock)); 
	pthread_cond_destroy(&(pool->queue_ready)); 
	
	free(pool); 
	pool=NULL; 
	return 0; 
} 
