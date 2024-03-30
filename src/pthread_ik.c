
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int global=0;

void *print_message_function( void *ptr );


void *print_message_function( void *ptr )
{
     int i;
     for(i=0;i<1000000;i++) asm("nop");
     char *message;
     message = (char *) ptr;
     fprintf(stderr,"%s \n", message);
     fprintf(stderr,"Global= %d \n", global);
}
void *set_message_function( void *ptr )
{
     global=16384;
     char *message;
     message = (char *) ptr;
     fprintf(stderr,"%s \n", message);
}
void _init() {
     pthread_t thread1, thread2;
     char *message1 = "Thread 1";
     char *message2 = "Thread 2";
     int  iret1, iret2;

    /* Create independent threads each of which will execute function */

     iret1 = pthread_create( &thread1, NULL, set_message_function, (void*) message1);
     iret2 = pthread_create( &thread2, NULL, print_message_function, (void*) message2);

     /* Wait till threads are complete before main continues. Unless we  */
     /* wait we run the risk of executing an exit which will terminate   */
     /* the process and all threads before the threads have completed.   */

     pthread_join( thread1, NULL);
     pthread_join( thread2, NULL); 

     fprintf(stderr,"Thread 1 returns: %d\n",iret1);
     fprintf(stderr,"Thread 2 returns: %d\n",iret2);
     fflush(stderr);
     exit(0);
}
