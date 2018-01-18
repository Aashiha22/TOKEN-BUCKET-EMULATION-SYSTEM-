#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>
#include "cs402.h"
#include "my402list.h"

struct timeval convert_time(double microsec)
{
	struct timeval t1;
	t1.tv_sec=microsec/1000000;
	t1.tv_usec=fmod(microsec,1000000);
	return t1;
}

long timevalue()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return (t.tv_sec * 1000000 + t.tv_usec);
}

long time_difference(long s1, long s2, long ms1, long ms2)
{
	long ms,s;
	if((ms1-ms2) < 0)
	{
		ms = 1000000 - ms2 + ms1;
		s = s1 - s2 - 1;
		return (s * 1000000 + ms);
	}
	else
	{
		return ((s1 * 1000000 + ms1) - (s2* 1000000 + ms2));
	}
		
}

//global variables
double lambda = 1;            //packet arrival time
double mu = 0.35;	    	  //packet service time
double r = 1.5;			      //token arrival rate
long B = 10;				  //bucket depth
long P = 3;					  //no. of tokens for each packet
long num_pkt = 20;		      //number of packets
long num_pkt_1;
char* file = NULL;
FILE *f = NULL;
char buffer[1030];

double lambda_1 = 1;
double mu_1 = 0.35;

double lambda_a,mu_a;
long P_a;

My402List queue1;
My402List queue2;

int packet_count = 0, packet_id = 0, sigflag, line_count = 0;

long token_count = 0, token_id = 0, total_token = 0, dropped_tokens = 0;

long time_1, time_2, time_3;    //main
struct timeval start, end;     
struct timeval time_temp; //packet arrival
long a, b1, b2, b3, b4, b5, q1entry, q1exit, c1, c2, timeinq1, q2entry, q2exit, timeinq2, c3, timeinsys, servtime;

//statistics part
double interarrival_average = 0.0, servicetime_average = 0.0, q1_average = 0.0, q2_average = 0.0, s1_average, s2_average = 0.0, system_average = 0.0;
double var = 0.0, var_1 = 0.0, var_2 = 0.0;
long pkt_arrived = 0, pkt_completed=0, pkt_dropped=0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cv  = PTHREAD_COND_INITIALIZER;


int flagsignal=0,fileflag=0;
sigset_t ctrlsignal;

pthread_t token_deposit, packet_arrival, server1, server2, sig;


void file_check(char *number)
{
	int a=0;
	while(*number != '\0')
	{
		if(*number == '.')
		{
			a++;
		}
		else if(*number < 48 || *number > 57)
		{
			fprintf(stderr,"Number is not valid\n");
			exit(1);
		}
		number++;
	}
	if(a > 1)
	{
		fprintf(stderr, "Only one decimal point should be there in the number entered\n");
		exit(1);
	}
	
}


struct packet_details
{
	long pkt_id;
	long pkt_arr;
	struct timeval arrival_time;
	long token_no;
	struct timeval service_time;
	long q1_entry;
	long q1_exit;
	long q2_entry;
	long q2_exit;
};

//functions

void *token_deposit_function()
{
	struct timespec token_sleep;
	struct timeval c_time, p_time ,current_time;
	long rate_t;
	long p_token,c_token;
	long ts,t1,t2;
	
	for(;;)
    {
    	//printf("looping here\n");
    	pthread_mutex_lock(&mutex);
    	
    	//printf("\nNumber of packets %ld    %ld\n",num_pkt,pkt_dropped );
    	if((num_pkt<=0) && My402ListEmpty(&queue1))
    	{
    		pthread_cond_signal(&cv);
    		pthread_mutex_unlock(&mutex);
    		pthread_exit(NULL);
    	}

    	gettimeofday(&c_time,NULL);

    	if(r < 0.1)
		{
			r = 0.1;
		}

		rate_t = (1/r)*1000000L;
		
		if(token_id!=0)
		{
			c_token = ((long)c_time.tv_sec * 1000000 + c_time.tv_usec);
			rate_t = rate_t - (c_token-p_token);
		}
		
		token_sleep.tv_sec = rate_t / 1000000L;
		token_sleep.tv_nsec = (rate_t % 1000000L)*1000;
		
		pthread_mutex_unlock(&mutex);
		nanosleep(&token_sleep, NULL);
		pthread_mutex_lock(&mutex);

		if((num_pkt<=0) && My402ListEmpty(&queue1))
    	{
    		pthread_cond_signal(&cv);
    		pthread_mutex_unlock(&mutex);
    		pthread_exit(NULL);
    	}
		
		gettimeofday(&p_time,NULL);
		p_token = ((long)p_time.tv_sec * 1000000 + p_time.tv_usec);
     	
 		total_token = total_token + 1;
    	
    	if(token_count < B)
    	{
    		token_count++;
    		token_id++;

    		t1 = time_difference(p_token/1000000,time_1/1000000,p_token%1000000,time_1%1000000);
    		printf("%08ld.%03ldms: token t%ld arrives, token bucket now has %ld tokens \n",t1/1000,t1%1000,token_id,token_count);
    		    		
    		if(!My402ListEmpty(&queue1))
			{
				//get head packet from queue1
				My402ListElem *elem = NULL;
				elem = My402ListFirst(&queue1);
				struct packet_details *pkt = NULL;
				pkt = (struct packet_details *) malloc(sizeof(struct packet_details));                           
				pkt = (struct packet_details*)elem->obj;
  				
  		
		    	//check if token count less than bucket depth B
		    	if(token_count >= pkt->token_no)           
				{
					// remove tokens from token bucket
					token_count = token_count - (*pkt).token_no;
			

					gettimeofday(&current_time,NULL);
					
					ts = ((long)current_time.tv_sec * 1000000 + current_time.tv_usec);
					(*pkt).q1_exit = ts;
					t1 = time_difference(ts/1000000,time_1/1000000,ts%1000000,time_1%1000000);
					t2 = time_difference(ts/1000000,(*pkt).q1_entry/1000000,ts%1000000,(*pkt).q1_entry%1000000);
					
					q1_average = q1_average + ((double)t2);
					printf("%08ld.%03ldms: p%ld leaves Q1, time in Q1 = %ld.%ld, token bucket now has %ld token\n",t1/1000,t1%1000,(*pkt).pkt_id,t2/1000,t2%1000,token_count);

					My402ListElem *e = NULL;
					e = My402ListFind(&queue1, (void*) pkt);
					My402ListUnlink(&queue1, e); 
					
					//add pkt to list2
					if(My402ListEmpty(&queue2))
					{
						My402ListAppend(&queue2, (void*) pkt);
						
						gettimeofday(&current_time,NULL);
						ts = ((long)current_time.tv_sec * 1000000 + current_time.tv_usec);
						(*pkt).q2_entry = ts;
						t1 = time_difference(ts/1000000,time_1/1000000,ts%1000000,time_1%1000000);

						printf("%08ld.%03ldms: p%ld enters Q2\n",t1/1000,t1%1000,(*pkt).pkt_id);
												
						pthread_cond_broadcast(&cv);
					}
					else                                               //give cond broadcast in all places queue empty or not
					{
						My402ListAppend(&queue2, (void*) pkt);  

						gettimeofday(&current_time,NULL);
						ts = ((long)current_time.tv_sec * 1000000 + current_time.tv_usec);
						(*pkt).q2_entry = ts;
						t1 = time_difference(ts/1000000,time_1/1000000,ts%1000000,time_1%1000000);

						printf("%08ld.%03ldms: p%ld enters Q2\n",t1/1000,t1%1000,(*pkt).pkt_id);
					}
					
				}
			}
		}
    	else
    	{
    		token_id++;
    		t1 = time_difference(p_token/1000000,time_1/1000000,p_token%1000000,time_1%1000000);
			printf("%08ld.%03ldms: token t%ld arrives, dropped \n",t1/1000,t1%1000,token_id);
			dropped_tokens = dropped_tokens + 1;
    	}
   
    	pthread_mutex_unlock(&mutex);
    }
  //  printf("came out\n");
    return 0;
}


void *packet_arrival_function()
{
	struct timespec sleep_arr;
	long timestamp1,timestamp2,interarrival_time,prev_value,n_1,n_2;
	struct timeval curval,curval1;

//	printf("\n $$$$ Number of pkts %ld\n",num_pkt );
//	printf("mu : %lf    lambda : %lf    r: %lf\n",mu,lambda,r );
	char buffer[1026];
	if(file != NULL)
	{
		//get number of packets
		f = fopen(file,"r");
		if(fgets(buffer,sizeof(buffer),f) != NULL)
		{
			int l1 = strlen(buffer);
			buffer[l1] = '\0';
			char* num = buffer;
			num_pkt = atoi(num);
		
		}
	}
	pthread_mutex_lock(&mutex);
	packet_id = 0;
	int c =0;
	while(num_pkt>0)
	{
    	struct packet_details *pkt = NULL;
		pkt = (struct packet_details *) malloc(sizeof(struct packet_details));

    	(*pkt).arrival_time = convert_time((1/lambda)*1000000);
		(*pkt).token_no = P;
		(*pkt).service_time = convert_time((1/mu)*1000000);

		c = c + 1;
		packet_id = packet_id + 1;
				
		if(file != NULL)
		{
			if(fgets(buffer,sizeof(buffer),f) != NULL)
			{
				//buffer contains more than 2 spaces or tabs
				char* l = strtok(buffer, " \t");
				lambda = strtod(l,NULL);
				char* p = strtok(NULL," \t");
				P = atoi(p);
				char* m = strtok(NULL, " \t");
				mu = strtod(m, NULL);
			}
			if(lambda < 0.1)
			{
				lambda = 0.1;
			}



			(*pkt).arrival_time = convert_time(lambda*1000);
			(*pkt).token_no = P;
			if(mu < 0.1)
			{
				mu = 0.1;
			}
			(*pkt).service_time = convert_time(mu*1000);
		}
		
		
	
		(*pkt).pkt_id = c;

		//int count;
		gettimeofday(&curval,NULL);

		timestamp1=time_difference((*pkt).arrival_time.tv_sec,0,(*pkt).arrival_time.tv_usec,0);

		
		if(timestamp1>0)
		{
			sleep_arr.tv_sec = timestamp1/1000000L;
			sleep_arr.tv_nsec = (timestamp1%1000000L)*1000;
			pthread_mutex_unlock(&mutex);
			nanosleep(&sleep_arr, NULL);
		//	printf("\n\n    %lfms\n",lambda);
			pthread_mutex_lock(&mutex);
			gettimeofday(&curval1,NULL);
			timestamp2 = ((long)curval1.tv_sec * 1000000 + curval1.tv_usec);
		}
		else
		{
			timestamp2 = ((long)curval.tv_sec * 1000000 + curval.tv_usec);
		}

		if(c == 1)
		{
			interarrival_time=time_difference(timestamp2/1000000,start.tv_sec,timestamp2%1000000,start.tv_usec);       //interarrival time
		}
		else
		{
			interarrival_time=time_difference(timestamp2/1000000,prev_value/1000000,timestamp2%1000000,prev_value%1000000);
		}
//		printf("\n ***** Inter arrival time :  %ld\n",interarrival_time );
		interarrival_average = interarrival_average + ((double)interarrival_time/1000000);
		prev_value=timestamp2;

		(*pkt).pkt_arr=timestamp2;
		n_1=time_difference(timestamp2/1000000,time_1/1000000,timestamp2%1000000,time_1%1000000);
 
		if(P <=B)
		{
			pkt_arrived++;
			printf("%08ld.%03ldms: p%ld arrives, needs %ld tokens, inter-arrival time = %ld.%03ldms\n",n_1/1000,n_1%1000,(*pkt).pkt_id,(*pkt).token_no,interarrival_time/1000,interarrival_time%1000);
		}
		else
		{
			pkt_arrived++;
			printf("%08ld.%03ldms: p%ld arrives, needs %ld token, inter-arrival time = %ld.%03ldms, dropped\n",n_1/1000,n_1%1000,(*pkt).pkt_id,(*pkt).token_no,interarrival_time/1000,interarrival_time%1000);
			pkt_dropped++;
		}
		
		           
		//CS region
		//queue1 is empty
		if(My402ListEmpty(&queue1))     				
		{
			if((token_count >= (*pkt).token_no)&&((*pkt).token_no <=B))
			{
				gettimeofday(&curval,NULL);
				timestamp1 = ((long)curval.tv_sec * 1000000 + curval.tv_usec);
				(*pkt).q1_entry = timestamp1;
				n_1=time_difference(timestamp1/1000000,time_1/1000000,timestamp1%1000000,time_1%1000000);

				
				printf("%08ld.%03ldms: p%ld enters Q1\n",n_1/1000,n_1%1000,(*pkt).pkt_id);
				My402ListAppend(&queue1, (void*) pkt);

				
				token_count = token_count - (*pkt).token_no;
				
				
				gettimeofday(&curval,NULL);
				timestamp2 = ((long)curval.tv_sec * 1000000 + curval.tv_usec);
				(*pkt).q1_exit = timestamp2;
				n_1=time_difference(timestamp2/1000000,time_1/1000000,timestamp2%1000000,time_1%1000000);
				n_2=time_difference(timestamp2/1000000,timestamp1/1000000,timestamp2%1000000,timestamp1%1000000);

				q1_average = q1_average + ((double)n_2);
				printf("%08ld.%03ldms: p%ld leaves Q1, time in Q1 = %ld.%ld, token bucket now has %ld token\n",n_1/1000,n_1%1000,(*pkt).pkt_id,n_2/1000,n_2%1000,token_count);
				My402ListElem *e3 = NULL;
				e3 = My402ListFind(&queue1, (void*) pkt);
				My402ListUnlink(&queue1, e3);  

				
				
				//cond_signal/broadcast
				if(My402ListEmpty(&queue2))
				{

					gettimeofday(&curval,NULL);
					timestamp2 = ((long)curval.tv_sec * 1000000 + curval.tv_usec);
					(*pkt).q2_entry = timestamp2;
					n_1=time_difference(timestamp2/1000000,time_1/1000000,timestamp2%1000000,time_1%1000000);

					
					My402ListAppend(&queue2, (void*) pkt);
					printf("%08ld.%03ldms: p%ld enters Q2\n",n_1/1000,n_1%1000,(*pkt).pkt_id);
				
					pthread_cond_signal(&cv);
					
				}
				else
				{
					gettimeofday(&curval,NULL);
					timestamp2 = ((long)curval.tv_sec * 1000000 + curval.tv_usec);
					(*pkt).q2_entry = timestamp2;
					n_1=time_difference(timestamp2/1000000,time_1/1000000,timestamp2%1000000,time_1%1000000);

					
					My402ListAppend(&queue2, (void*) pkt);
					printf("%08ld.%03ldms: p%ld enters Q2\n",n_1/1000,n_1%1000,(*pkt).pkt_id);
					
				}
			}
			else
			{
				if((*pkt).token_no <=B)
				{
						
					gettimeofday(&curval,NULL);
					timestamp1 = ((long)curval.tv_sec * 1000000 + curval.tv_usec);
					(*pkt).q1_entry = timestamp1;
					n_1=time_difference(timestamp1/1000000,time_1/1000000,timestamp1%1000000,time_1%1000000);

					
					printf("%08ld.%03ldms: p%ld enters Q1\n",n_1/1000,n_1%1000,(*pkt).pkt_id);
				
					My402ListAppend(&queue1, (void*) pkt);
										
				} 
				
			}
		}
		//queue1 not empty
		else
		{
			if((*pkt).token_no <=B)
			{
					
				gettimeofday(&curval,NULL);
				timestamp1 = ((long)curval.tv_sec * 1000000 + curval.tv_usec);
				(*pkt).q1_entry = timestamp1;
				n_1=time_difference(timestamp1/1000000,time_1/1000000,timestamp1%1000000,time_1%1000000);

				 
				printf("%08ld.%03ldms: p%ld enters Q1\n",n_1/1000,n_1%1000,(*pkt).pkt_id);
				
				My402ListAppend(&queue1, (void*) pkt);
							
			} 
			
		}
		num_pkt--;
		
		pthread_mutex_unlock(&mutex);

	
	}

	return 0;
}



void *server1_function()
{
	struct timespec server_time;
	long ts1,ts2,t1,t2,t3;
	struct timeval current_time;
	for(;;)
	{
		pthread_mutex_lock(&mutex);
		if((num_pkt<=0) && My402ListEmpty(&queue2) && My402ListEmpty(&queue1))
    	{
    		pthread_cond_signal(&cv);
    		pthread_mutex_unlock(&mutex);
    		return 0;
    	}
    	//printf("value of sigflag in s2 %d\n",sigflag);
    	if(sigflag)
    	{
    		pthread_cond_signal(&cv);
    		pthread_mutex_unlock(&mutex);
    		return 0;
    	}

    	if((pkt_completed < (num_pkt_1 - pkt_dropped)) && My402ListEmpty(&queue2) && sigflag)
    	{
    		pthread_cond_signal(&cv);
    		pthread_mutex_unlock(&mutex);
    		return 0;
    	}

    	while(My402ListEmpty(&queue2))
    	{
    		
    		if((num_pkt <= 0) && My402ListEmpty(&queue1) && My402ListEmpty(&queue2))
			{
				pthread_cond_signal(&cv);
				pthread_mutex_unlock(&mutex);
				return 0;
			}
			if(sigflag)
    		{
    			pthread_cond_signal(&cv);
    			pthread_mutex_unlock(&mutex);
    			return 0;
    		}
    		pthread_cond_wait(&cv,&mutex);
    	}


    	if(!My402ListEmpty(&queue2) && (sigflag != 1))
    	{
	    	
	    	//dequeue job
	    	My402ListElem *elem = NULL;

			elem = My402ListFirst(&queue2);
			
			struct packet_details *pkt = NULL;
			pkt = (struct packet_details *) malloc(sizeof(struct packet_details));
			pkt = (struct packet_details*)elem->obj;

			My402ListElem *e3 = NULL;
			e3 = My402ListFind(&queue2, (void*) pkt);
			if(e3 != NULL)
			{
				gettimeofday(&current_time,NULL);
				q2exit = ((long)current_time.tv_sec * 1000000 + current_time.tv_usec);
				t1 = time_difference(q2exit/1000000,time_1/1000000,q2exit%1000000,time_1%1000000);
				t2 = time_difference(q2exit/1000000,(*pkt).q2_entry/1000000,q2exit%1000000,(*pkt).q2_entry%1000000);
	
				q2_average = q2_average + ((double)t2);
				printf("%08ld.%03ldms: p%ld leaves Q2, time in Q2 = %ld.%ldms\n",t1/1000,t1%1000,(*pkt).pkt_id,t2/1000,t2%1000);

				My402ListUnlink(&queue2, e3);

				
				gettimeofday(&current_time,NULL);
				ts1 = ((long)current_time.tv_sec * 1000000 + current_time.tv_usec);
				t1 = time_difference(ts1/1000000,time_1/1000000,ts1%1000000,time_1%1000000);
				t2 = ((long)(*pkt).service_time.tv_sec * 1000000 + (*pkt).service_time.tv_usec);
				
				
				printf("%08ld.%03ldms: p%ld begins service at S1, requesting %ld.%03ldms of service\n",t1/1000,t1%1000,(*pkt).pkt_id,t2/1000,t2%1000);
				pthread_mutex_unlock(&mutex);

				server_time.tv_sec = t2/1000000L;
				server_time.tv_nsec = (t2%1000000L)*1000;
				nanosleep(&server_time, NULL);
				pthread_mutex_lock(&mutex);

				gettimeofday(&current_time,NULL);
				ts2 = ((long)current_time.tv_sec * 1000000 + current_time.tv_usec);
				t1 = time_difference(ts2/1000000,time_1/1000000,ts2%1000000,time_1%1000000);
				t2 = time_difference(ts2/1000000,ts1/1000000,ts2%1000000,ts1%1000000);
				
				s1_average = s1_average + ((double)t2);
				servicetime_average = servicetime_average +((double)t2/1000000);
				
				t3 = time_difference(ts2/1000000,(*pkt).pkt_arr/1000000,ts2%1000000,(*pkt).pkt_arr%1000000);
				var_1 = var_1 + (((double)t3/1000000)*((double)t3/1000000));

				system_average = system_average + ((double)(t3));
				
				printf("%08ld.%03ldms: p%ld departs from S1, service time = %ld.%ldms, time in system = %ld.%ldms\n",t1/1000,t1%1000,(*pkt).pkt_id,t2/1000,t2%1000,t3/1000,t3%1000);
				pkt_completed++;

				pthread_mutex_unlock(&mutex);
			}
			pthread_mutex_unlock(&mutex);
		}
	}
	return 0;
}



void *server2_function()
{
	struct timespec server_time;
	long ts1,ts2,t1,t2,t3;
	struct timeval current_time;
	for(;;)
	{
		pthread_mutex_lock(&mutex);
		if((num_pkt<=0) && My402ListEmpty(&queue2) && My402ListEmpty(&queue1))
    	{
    		pthread_cond_signal(&cv);
    		pthread_mutex_unlock(&mutex);
    		return 0;
    	}
    	//printf("value of sigflag in s2 %d\n",sigflag);
    	if(sigflag)
    	{
    		pthread_cond_signal(&cv);
    		pthread_mutex_unlock(&mutex);
    		return 0;
    	}

    	if((pkt_completed < (num_pkt_1 - pkt_dropped)) && My402ListEmpty(&queue2) && sigflag)
    	{
    		pthread_cond_signal(&cv);
    		pthread_mutex_unlock(&mutex);
    		return 0;
    	}

    	while(My402ListEmpty(&queue2))
    	{
    		
    		if((num_pkt <= 0) && My402ListEmpty(&queue1) && My402ListEmpty(&queue2))
			{
				pthread_cond_signal(&cv);
				pthread_mutex_unlock(&mutex);
				return 0;
			}
			if(sigflag)
    		{
    			pthread_cond_signal(&cv);
    			pthread_mutex_unlock(&mutex);
    			return 0;
    		}
    		pthread_cond_wait(&cv,&mutex);
    	}


    	if(!My402ListEmpty(&queue2) && (sigflag != 1))
    	{
	    	
	    	//dequeue job
	    	My402ListElem *elem = NULL;

			elem = My402ListFirst(&queue2);
			
			struct packet_details *pkt = NULL;
			pkt = (struct packet_details *) malloc(sizeof(struct packet_details));
			pkt = (struct packet_details*)elem->obj;

			My402ListElem *e3 = NULL;
			e3 = My402ListFind(&queue2, (void*) pkt);
			if(e3 != NULL)
			{
				gettimeofday(&current_time,NULL);
				q2exit = ((long)current_time.tv_sec * 1000000 + current_time.tv_usec);
				t1 = time_difference(q2exit/1000000,time_1/1000000,q2exit%1000000,time_1%1000000);
				t2 = time_difference(q2exit/1000000,(*pkt).q2_entry/1000000,q2exit%1000000,(*pkt).q2_entry%1000000);
	
				q2_average = q2_average + ((double)t2);
				printf("%08ld.%03ldms: p%ld leaves Q2, time in Q2 = %ld.%ldms\n",t1/1000,t1%1000,(*pkt).pkt_id,t2/1000,t2%1000);

				My402ListUnlink(&queue2, e3);

				
				gettimeofday(&current_time,NULL);
				ts1 = ((long)current_time.tv_sec * 1000000 + current_time.tv_usec);
				t1 = time_difference(ts1/1000000,time_1/1000000,ts1%1000000,time_1%1000000);
				t2 = ((long)(*pkt).service_time.tv_sec * 1000000 + (*pkt).service_time.tv_usec);
				
				
				printf("%08ld.%03ldms: p%ld begins service at S2, requesting %ld.%03ldms of service\n",t1/1000,t1%1000,(*pkt).pkt_id,t2/1000,t2%1000);
				pthread_mutex_unlock(&mutex);

				server_time.tv_sec = t2/1000000L;
				server_time.tv_nsec = (t2%1000000L)*1000;
				nanosleep(&server_time, NULL);
				pthread_mutex_lock(&mutex);

				gettimeofday(&current_time,NULL);
				ts2 = ((long)current_time.tv_sec * 1000000 + current_time.tv_usec);
				t1 = time_difference(ts2/1000000,time_1/1000000,ts2%1000000,time_1%1000000);
				t2 = time_difference(ts2/1000000,ts1/1000000,ts2%1000000,ts1%1000000);
				
				s2_average = s2_average + ((double)t2);
				servicetime_average = servicetime_average +((double)t2/1000000);
				
				t3 = time_difference(ts2/1000000,(*pkt).pkt_arr/1000000,ts2%1000000,(*pkt).pkt_arr%1000000);
				var_1 = var_1 + (((double)t3/1000000)*((double)t3/1000000));

				system_average = system_average + ((double)(t3));
				
				printf("%08ld.%03ldms: p%ld departs from S2, service time = %ld.%ldms, time in system = %ld.%ldms\n",t1/1000,t1%1000,(*pkt).pkt_id,t2/1000,t2%1000,t3/1000,t3%1000);
				pkt_completed++;

				pthread_mutex_unlock(&mutex);
			}
			pthread_mutex_unlock(&mutex);
		}
	}
	return 0;
}

void *signal_handler(void *a)
{
	long t1,t2;
	struct timeval current;
	struct packet_details *pkt = NULL;
	My402ListElem *elem;
	int sig;
	sigflag=0;
	
	sigwait(&ctrlsignal,&sig);   //nunki remove &sig
	pthread_mutex_lock(&mutex);
	
	gettimeofday(&current,NULL);
	t1 = ((long)current.tv_sec * 1000000 + current.tv_usec);
	t2 = time_difference(t1/1000000,time_1/1000000,t1%1000000,time_1%1000000);
	printf("%08ld.%03ldms: SIGINT caught, no new packets or tokens will be allowed\n",t2/1000,t2%1000);

	pthread_cancel(packet_arrival);
	pthread_cancel(token_deposit);
	sigflag = 1;
	pthread_cond_broadcast(&cv);
		
	for(elem = My402ListFirst(&queue1); elem != NULL; elem=My402ListNext(&queue1,elem))
	{
		pkt = (struct packet_details *) malloc(sizeof(struct packet_details));                           
		pkt = (struct packet_details*)elem->obj;
		gettimeofday(&current,NULL);
		t1 = ((long)current.tv_sec * 1000000 + current.tv_usec);
		t2 = time_difference(t1/1000000,time_1/1000000,t1%1000000,time_1%1000000);
		printf("%08ld.%03ldms: p%ld removed from Q1\n",t2/1000,t2%1000,(*pkt).pkt_id);
		pkt_dropped++;
	}
	My402ListUnlinkAll(&queue1);

	for(elem = My402ListFirst(&queue2); elem != NULL; elem=My402ListNext(&queue2,elem))
	{
		pkt = (struct packet_details *) malloc(sizeof(struct packet_details));                           
		pkt = (struct packet_details*)elem->obj;
		gettimeofday(&current,NULL);
		t1 = ((long)current.tv_sec * 1000000 + current.tv_usec);
		t2 = time_difference(t1/1000000,time_1/1000000,t1%1000000,time_1%1000000);
		printf("%08ld.%03ldms: p%ld removed from Q2\n",t2/1000,t2%1000,(*pkt).pkt_id);
		pkt_dropped++;
	}
	My402ListUnlinkAll(&queue2);
	
	pthread_mutex_unlock(&mutex);
	
	return 0;
}
	
	

int main(int argc, char *argv[])
{
	sigemptyset(&ctrlsignal);
	sigaddset(&ctrlsignal,SIGINT);
	//sigprocmask(SIG_BLOCK,&ctrlsignal,0);
	pthread_sigmask(SIG_BLOCK,&ctrlsignal,0);
	My402ListInit(&queue1);
	My402ListInit(&queue2);
	long st = 0;

	//error checking
	if(argc > 15)
	{
			fprintf(stderr, "Too many arguments.Command Usage info:  warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile] \n");
			exit(1);
	}
	if((argc % 2) == 0)
	{
			fprintf(stderr, "Too less arguments.Command Usage info:  warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile] \n");
			exit(1);
	}
	else
	{
		int i = 0;
		for(i=1;i<argc;i=i+2)
		{
			if(strcmp(argv[i],"-lambda") == 0)
			{
				file_check(argv[i+1]);
				lambda=atof(argv[i+1]);
			}
			else if(strcmp(argv[i],"-mu") == 0)
			{
				file_check(argv[i+1]);
				mu = atof(argv[i+1]);
			}
			else if(strcmp(argv[i],"-r") == 0)
			{
				file_check(argv[i+1]);
				r = atof(argv[i+1]);
			}
			else if(strcmp(argv[i],"-B") == 0)
			{
				char *decimal = strchr(argv[i+1],'.');
				if(atoi(argv[i+1]) <= 0)
				{
					fprintf(stderr, "Token capacity should be positive\n");
					exit(1);
				}
				if(decimal != NULL)
				{
					fprintf(stderr, "Token capacity is not an integer\n");
					exit(1);
				}
					
				B = atoi(argv[i+1]);
			}
			else if(strcmp(argv[i],"-P") == 0)
			{
				char *decimal = strchr(argv[i+1],'.');
				if(decimal != NULL)
				{
					fprintf(stderr, "Packet token is not an integer\n");
					exit(1);
				}
				if(atoi(argv[i+1]) <= 0)
				{
					fprintf(stderr, "Packet token should be positive\n");
					exit(1);
				}
				P = atoi(argv[i+1]);
			}
			else if(strcmp(argv[i],"-n") == 0)
			{
				char *decimal = strchr(argv[i+1],'.');
				if(decimal != NULL)
				{
					fprintf(stderr, "Number of packets is not an integer\n");
					exit(1);
				}
				if(atoi(argv[i+1]) <= 0)
				{
					fprintf(stderr, "Number of packets should be positive\n");
					exit(1);
				}
				num_pkt = atoi(argv[i+1]);
			}
			else if(strcmp(argv[i],"-t") == 0)
			{
				file = argv[i+1];
				fileflag = 1;
				if(strcmp(file,"/etc") == 0)
				{
					fprintf(stderr, "Input file %s is a directory \n",file);
					exit(1);
				}
			}
			else
			{
				fprintf(stderr, "Command Usage info:  warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile] \n");
				exit(1);
			}
			
		}		
	}

	//check if file is null or not
	if(file != NULL)
	{
		f = fopen(file,"r");
		if(f == NULL)
		{
			fprintf(stderr, "Input file %s does not exist \n",file);
			exit(1);
		}
		else
		{
			if(fgets(buffer,sizeof(buffer),f) != NULL)
			{
				int l1 = strlen(buffer);
				if(l1<=1)
				{
					fprintf(stderr, "Packet count should not be empty \n");
					exit(1);
				}
				buffer[l1] = '\0';
				char* num = buffer;
				num_pkt = atoi(num);
				if(num_pkt <= 0)
				{
					fprintf(stderr, "Packet count should not be less than or equal to zero \n");
					exit(1);
				}
			}
			line_count = 1;
			while(fgets(buffer,sizeof(buffer),f) != NULL)
			{
				
				char* l = strtok(buffer, " \t");
				char *l_1 = strchr(l,'.');
				if(atoi(l) < 0)
				{
					fprintf(stderr, "Inter-arrival time in line %d is not positive\n",line_count);						
					exit(1);
				}
				if(l_1 != NULL)
				{
					fprintf(stderr, "Inter-arrival time in line %d is not an integer\n",line_count);
					exit(1);
				}
				lambda_a = strtod(l,NULL);

				char* p = strtok(NULL," \t");
				char *p_1 = strchr(p,'.');
				if(atoi(p) < 0)
				{
					fprintf(stderr, "Token count in line %d is not positive\n",line_count);						
					exit(1);
				}
				if(p_1 != NULL)
				{
					fprintf(stderr, "Token count in line %d is not an integer\n",line_count);
					exit(1);
				}
				P_a = atoi(p);

				char* m = strtok(NULL, " \t");
				char *m_1 = strchr(m,'.');
				if(atoi(m) < 0)
				{
					fprintf(stderr, "Service time in line %d is not positive\n",line_count);						
					exit(1);
				}
				if(m_1 != NULL)
				{
					fprintf(stderr, "Service time in line %d is not an integer\n",line_count);
					exit(1);
				}
				P_a = atoi(p);
				mu_a = strtod(m, NULL);

				line_count = line_count + 1;
			}
		}	
	}

	//print emulation parameters
	printf("Emulation Parameters:\n");
	printf("\tnumber to arrive = %ld\n",num_pkt);

	num_pkt_1 = num_pkt;
	
	if(fileflag == 0)
	{
		if(lambda < 0.1)
		{
			lambda = 0.1;
		}
		if(r < 0.1)
		{
			r = 0.1;
		}
		if(mu < 0.1)
		{
			mu = 0.1;
		}
		printf("\tlambda = %lf\n",lambda);
		printf("\tmu = %lf\n",mu);
		printf("\tr = %lf\n",r);
		printf("\tB = %ld\n",B);
		printf("\tP = %ld\n\n",P);
	}
	else
	{
		printf("\tr = %lf\n",r);
		printf("\tB = %ld\n",B);
		printf("\ttsfile = %s\n\n",file);
	}

	printf("%08ld.%03ldms: emulation begins\n",st,st);
	gettimeofday(&start,NULL);
	time_1=((long)start.tv_sec * 1000000 + start.tv_usec);

	pthread_create( &sig, NULL, signal_handler,0);
	pthread_create( &packet_arrival, NULL, packet_arrival_function, NULL);
	pthread_create( &token_deposit, NULL, token_deposit_function, NULL);
	pthread_create( &server1, NULL, server1_function, NULL);
	pthread_create( &server2, NULL, server2_function, NULL);

	//join threads
	pthread_join( token_deposit, NULL);
	pthread_join( packet_arrival, NULL); 
	pthread_join( server1, NULL); 
	pthread_join( server2, NULL); 

	var_2 = ((system_average/1000000) / pkt_completed);

	struct timeval t_2;
	gettimeofday(&t_2,NULL);
	long t = ((long)t_2.tv_sec * 1000000 + t_2.tv_usec);
	long end1 = time_difference(t/1000000,time_1/1000000,t%1000000,time_1%1000000);
	printf("%08ld.%03ldms: emulation ends\n\n",end1/1000,end1%1000);

	//pending statistics
	printf("Statistics:\n\n");
	//printf("\nPacket %ld arrived",num_pkt_1);

	if(num_pkt_1 == 0)
	{
		printf("average packet inter-arrival time = N/A. no packet arrived at this facility\n");
	}
	else
	{
		printf("average packet inter-arrival time = %.6gs\n",interarrival_average/pkt_arrived);
	}
	if(pkt_completed == 0)
	{
		printf("average packet service time = N/A. no packet arrived at this facility\n\n");
	}
	else
	{
		printf("average packet service time = %.6gs\n\n",servicetime_average/pkt_completed);
	}
	
	printf("average number of packets in Q1 = %.6g\n", q1_average/((double)end1));
	
	printf("average number of packets in Q2 = %.6g\n", q2_average/((double)end1));
	
	printf("average number of packets at S1 = %.6g\n",s1_average/((double)end1));
	
	printf("average number of packets at S2 = %.6g\n\n",s2_average/((double)end1));
	
	if(pkt_completed == 0)
	{
		printf("average time a packet spent in system = N/A. no packet arrived at this facility\n");
	}
	else
	{
		printf("average time a packet spent in system = %.6gs\n",(system_average/1000000)/pkt_completed);
	}

	if(((var_1/pkt_completed)-(var_2*var_2)) < 0)
	{
		var = sqrt(((var_1/pkt_completed)-(var_2*var_2))*(-1));
	}
	else
	{
		var = sqrt(((var_1/pkt_completed)-(var_2*var_2)));
	}

	if(pkt_completed == 0)
	{
		printf("standard deviation for time spent in system = N/A. no packet arrived at this facility\n\n");
	}
	else
	{
		printf("standard deviation for time spent in system = %.6gs\n\n",var);
	}

	if(total_token == 0)
	{
		printf("token drop probability = N/A. total tokens shouldn't be zero\n");
	}
	else
	{
	//	printf("\ndropped : %ld total : %ld\n", dropped_tokens,total_token);
		printf("token drop probability = %.6g\n",((double)dropped_tokens/total_token));
	}

	if(num_pkt_1 == 0)
	{
		printf("packet drop probability = N/A. Total packets shouldn't be zero\n");
	}
	else
	{
		printf("packet drop probability = %.6g\n",((double)pkt_dropped/num_pkt_1));
	}

	return 0;
}
