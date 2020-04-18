// postoffice.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
//#define P1 50



volatile unsigned long long global = 1;
volatile unsigned int clients_in_ques[3];

volatile unsigned int to_worker[3];

volatile unsigned long long done = 0;

HANDLE ghMutex;
HANDLE ghSemaphore[7];


typedef struct Client {
	int ready; // client gonna wait in loop until served
	int time_spend_on_post; // all time in post	
	int window_number;
} *PCLIENT;

typedef struct Worker {
	
	volatile unsigned int* to_do;
	int time_to_wait;
	int clients_served;
	int number_of_clients;

} *PWORKER;


DWORD WINAPI ClientFunc( LPVOID lpParam );
DWORD WINAPI WorkerFunc( LPVOID lpParam );

int main(int argc, char* argv[])
{
	if( argc != 2 )
	{
		printf("Missing argument ( number of clients )!!");
		return -2;
	}
	int P1 = atoi(argv[1]);
	for( int i =0; i<3; ++i )
		to_worker[i] = 1;
	// INITIALIZE SEMAPHORES
	// CREATING SEMAPHORE FOR POST
	ghSemaphore[0] = CreateSemaphore( NULL, 30, 30, NULL );
	// CREATING SEMAPHORE FOR QUES
	ghSemaphore[1] = CreateSemaphore( NULL, 5, 5, NULL );
	ghSemaphore[2] = CreateSemaphore( NULL, 10, 10, NULL );
	ghSemaphore[3] = CreateSemaphore( NULL, 15, 15, NULL );
	// CREATING SEMAPHORE FOR WINDOW IN POST
	for( int i=4; i<7; ++i )
		ghSemaphore[i] = CreateSemaphore( NULL, 1, 1, NULL ); // We can use mutex or CriticalSection for this instead and it's gonna work also !@!@!@!@!@!@!@!@!@!@!!@!@!@!@
	// END OF INITIALIZE SEMAPHORES


	// INITIALIZE POST MUTEX

	ghMutex = CreateMutex( NULL, FALSE, NULL );   

	// END OF INITIALIZE MUTEX

	// CREATING CLIENTS

	/*PCLIENT pDataArray[P1];
	DWORD   dwThreadIdArray[P1];
	HANDLE  hThreadArray[P1]; */
	PCLIENT* pDataArray = ( PCLIENT* )calloc( P1, sizeof( PCLIENT ));
	DWORD* dwThreadIdArray = ( DWORD* )calloc( P1, sizeof( DWORD ));
	HANDLE* hThreadArray = ( HANDLE* )calloc( P1, sizeof(HANDLE));

	for( int i=0; i<P1; ++i )
	{
		pDataArray[i] = (PCLIENT)calloc(1,sizeof(Client));
		pDataArray[i]->ready = 0;
		if( pDataArray[i] == NULL )
		{
			ExitProcess(2);
		}
		hThreadArray[i] = CreateThread( 
            NULL,                   // default security attributes
            0,                      // use default stack size  
			ClientFunc,       // thread function name
            pDataArray[i],          // argument to thread function 
			CREATE_SUSPENDED,       // our clients are waiting in front of post
            &dwThreadIdArray[i]);   // returns the thread identifier
	}
	// END OF CREATE


	// CREATING WORKERS

	DWORD   dwWorkerIdArray[3];
	HANDLE  hWorkerArray[3];
	PWORKER pWorkerArray[3];
	for( int i=0; i<3; ++i )
	{
		pWorkerArray[i] = ( PWORKER )calloc(1,sizeof(Worker));
		pWorkerArray[i]->to_do = &to_worker[i];
		pWorkerArray[i]->number_of_clients = P1;
		switch(i)
		{
		case 0: pWorkerArray[i]->time_to_wait =300; break;
		case 1: pWorkerArray[i]->time_to_wait =150; break;
		case 2: pWorkerArray[i]->time_to_wait =100; break;
		}
		hWorkerArray[i] = CreateThread( NULL, 0, WorkerFunc, pWorkerArray[i], 0, &dwWorkerIdArray[i] );
	}
	//END OF CREATE WORKERS

	// TIME TO OPEN POST OFFICE
	for( int i=0; i<P1; ++i )
		ResumeThread(hThreadArray[i]);
	// OPEN DONE
	for( int i=0; i<P1; ++i )
		WaitForSingleObject(hThreadArray[i], INFINITE);
				

	// STATISTICS
	int all_time = 0;
	for( int i=0; i<3; ++i )
	{
		printf("\nWindow number %d served %d clients in time %d\n", i+1, pWorkerArray[i]->clients_served, (pWorkerArray[i]->clients_served) * ( pWorkerArray[i]->time_to_wait )*1000);
		all_time += (pWorkerArray[i]->clients_served) * ( pWorkerArray[i]->time_to_wait )*1000;
	}
	printf("\nAll time spent on post by all clients: %d\n", all_time );



	// CLEANING CLIENTS
	for(int i=0; i<P1; i++)
    {
        CloseHandle(hThreadArray[i]);
        if(pDataArray[i] != NULL)
        {
            free(pDataArray[i]);
            pDataArray[i] = NULL;    // Ensure address is not reused.
        }
    }
	free( hThreadArray );
	free( dwThreadIdArray );
	free( pDataArray );
	// END OF CLEANING CLIENTS

	// CLEANING WORKERS
	for( int i=0; i<3; ++i )
	{
		CloseHandle(hWorkerArray[i]);
		if( pWorkerArray[i] != NULL )
		{
			free(pWorkerArray[i]);
			pWorkerArray[i] = NULL;
		}

	}

	//CLEANING MUTEX
	CloseHandle(ghMutex);


	// CLEANING SEMAPHORES
	for( int i=0; i<7; ++i )
		CloseHandle(ghSemaphore[i]);

	system("pause");
	return 0;
}

DWORD WINAPI ClientFunc( LPVOID lpParam )
{
	PCLIENT helper = (PCLIENT)lpParam;
	// POST SEMAPHORE
	DWORD dwWaitResult; 
    BOOL bContinue=TRUE;

	int looking_for_the_best_que = 0;
	int wait_time[3] = {};
	int minimum_index = 0;
	int numer_klienta = 0;
	// MUTEX FOR CHOOSE QUE
	DWORD dwMutexResult;
	BOOL mutexContinue = TRUE;

	// QUE SEMAPHORE
	DWORD dwQueResult;
	BOOL queContinue = TRUE;

	// WINDOW SEMAPHORE
	DWORD dwWindowResult;
	BOOL windowContinue = TRUE;

	while(bContinue)
	{
		dwWaitResult = WaitForSingleObject( 
            ghSemaphore[0],   // handle to POST Semaphore
            0L);           // zero-second time-out interval
		switch(dwWaitResult) // switch to post
		{
			case WAIT_OBJECT_0: // OUR CLIENT IS IN POST
				{
					bContinue=FALSE;
					srand( time(NULL) + GetCurrentThreadId() );
					looking_for_the_best_que = rand()%51+10;
					helper->time_spend_on_post += looking_for_the_best_que;
					Sleep(looking_for_the_best_que);
					while( mutexContinue )
					{
						dwMutexResult = WaitForSingleObject( ghMutex, INFINITE );
						switch( dwMutexResult )
						{
							case WAIT_OBJECT_0: // OUR CLIENT IS CHOOSING CHOOSING QUE
								{
									mutexContinue = FALSE;
									numer_klienta = global;
									//printf("Wszedlem na poczte jako %d i bede wybierac kolejke\n", global);
									InterlockedIncrement(&global);
									// CALCULATING TIME
									if( ( wait_time[0] = ( clients_in_ques[0]+1 ) * 300 ) == 1800 ) 
										wait_time[0] = INFINITE;
									if( ( wait_time[1] = ( clients_in_ques[1]+1 ) * 150 ) == 1650 )
										wait_time[1] = INFINITE;
									if( ( wait_time[2] = ( clients_in_ques[2]+1 ) * 100 ) == 1600 )
										wait_time[2] = INFINITE;

									// CHOOSE BEST OPTIONS
									for( int i=0; i<3; i++ )
										if( wait_time[i] < wait_time[minimum_index] ) 
											minimum_index = i;
									InterlockedIncrement(&clients_in_ques[minimum_index]);
									
									if (! ReleaseMutex(ghMutex)) printf("Jestem glupi i nie zwolnilem mutexu\n" );

									helper->time_spend_on_post += wait_time[minimum_index]; // dograc co z osoba obslugiwana ( DONE )

									while( queContinue )
									{
										dwQueResult = WaitForSingleObject( ghSemaphore[minimum_index+1], 0L ); 
										switch( dwQueResult )
										{
											case WAIT_OBJECT_0: // OUR CLIENT IS IN A QUE
												{
													queContinue = FALSE;
													//printf("Jestem watkiem nr %d i czekam w kolejce elo \n", numer_klienta);
													while( windowContinue )
													{
														dwWindowResult = WaitForSingleObject( ghSemaphore[minimum_index+4], 0L ); // here we can use mutex intstead or Critical Section
														switch( dwWindowResult )
														{
															case WAIT_OBJECT_0: // OUR CLIENT IS IN FRONT OF WINDOW
															{
																windowContinue = FALSE;
																ReleaseSemaphore( ghSemaphore[minimum_index+1], 1, NULL ); // we are in front of window so there is free space in que
																InterlockedDecrement(&clients_in_ques[minimum_index]);
																//printf("Jestem watkiem nr %d i jestem sam na sam z pracownikiem i cos bedziemy robic \n", numer_klienta);
																InterlockedDecrement(&to_worker[minimum_index]);
																while( !to_worker[minimum_index] ); // we are waiting until post worker finish his job
																//Sleep(5);
																ReleaseSemaphore( ghSemaphore[minimum_index+4], 1, NULL );
																break;

															}
															case WAIT_TIMEOUT: break; // timeout in waiting to window
														}
													}
													break; // we are leavining worker and going out
												}
											case WAIT_TIMEOUT: break;
										}
									}
									break;
								}
							case WAIT_ABANDONED: break;
						}
					}
					
						// Release the semaphore when task is finished
					if (!ReleaseSemaphore( 
							ghSemaphore[0],  // handle to POST semaphore
							1,            // increase count by one
							NULL) )       // not interested in previous count
					{
						printf("ReleaseSemaphore error: %d\n", GetLastError());
					}
					break; 
				}
			case WAIT_TIMEOUT: break;
		}
	}
	return TRUE;
}


DWORD WINAPI WorkerFunc( LPVOID lpParam )
{
	PWORKER helper = (PWORKER)lpParam;
	while( done < ( helper->number_of_clients ) )
	{
		if( *(helper->to_do) != 1 )
		{
			InterlockedIncrement(&done);
			helper->clients_served++;
			Sleep(helper->time_to_wait);
			InterlockedIncrement(helper->to_do);
			
		}
	}
	return 0;
}