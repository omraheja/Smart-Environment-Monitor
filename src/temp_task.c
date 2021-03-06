#include "main_task.h"
#include "temp_task.h"
#include "temperature_sensor.h"
#include "led.h"

#include <poll.h>
#include <fcntl.h>

#define time_out    (200)

char *proj2 = "/tmp/proj1";
int SOCKET=0;
struct pollfd fileds[1];


void temp_signal_handler(int signo, siginfo_t *info,void *extra)
{
	printf("\nKilling TEMPERATURE THREAD\n");
	// i2c_close();
	timer_delete(temp_timerid);
	pthread_cancel(temp_thread);
	
}


void set_temp_signal_handler(void)
{
	struct sigaction action;
	action.sa_flags = SA_SIGINFO;
	action.sa_sigaction = temp_signal_handler;

	if(sigaction(SIGUSR1,&action,NULL) == -1)
	{
		perror("Sigusr : Sigaction");
		_exit(1);
	}
}


/* Temperature Timer Handler */
void temp_timer_handler(union sigval val)
{
	char buffer[LOGGER_QUEUE_SIZE];
	char socket_buffer[50];
	float data = 0;
	int status = 0;

	pthread_mutex_lock(&lock);
	
	//printf("TEMPERATURE TIMER HANDLER\n");
	LOG_PRINT("[TEMPERATURE TASK]\t [DEBUG] Invoking timer handler\n");
	//LOG_PRINT("[TEMPERATURE TASK]\t [DEBUG] Invoking timer handler");

	//process_temp_data(&data);
	status = get_sensortemp(&data);
	
	int fd = open(proj2,O_WRONLY);

	if (status == -1) {
		//sprintf(buffer, "TEMP THREAD DATA\tTID:%ld\t Temperature sensor down",syscall(SYS_gettid));
		BUILD_MESSAGE(buffer, "[TEMPERATURE TASK] [ERROR] Temperature sensor down");
		TEMP_ERROR_LED_ON();
	} else {
		//sprintf(buffer,"TEMP THREAD DATA\tTID:%ld\ttemp = %f\n",syscall(SYS_gettid), data);
		BUILD_MESSAGE(buffer, "[TEMPERATURE TASK] [INFO] Temperature = %f C", data);
		TEMP_ERROR_LED_OFF();
	}

	mq_send(logger_queue, buffer, LOGGER_QUEUE_SIZE, 0);

	write(fd,"T",1);

	close(fd);

	char gpio_buffer[10];
    if((poll(fileds,1,time_out) > 0))
	{
        memset(gpio_buffer,0,10);
		lseek(fileds[0].fd,0,SEEK_SET);
        read(fileds[0].fd,gpio_buffer,10);		
        if(fileds[0].revents & POLLPRI)
        {
            memset(buffer, 0, sizeof(buffer));
            printf("Value = %s\n",gpio_buffer);
            BUILD_MESSAGE(buffer, "[TEMPERATURE TASK] [ALERT] TEMPERATURE ALERT SIGNAL TRIGGERED");
            mq_send(logger_queue, buffer, LOGGER_QUEUE_SIZE, 0);
	    }
    }
	
    pthread_mutex_unlock(&lock);


	if(SOCKET == 1)
	{
		sprintf(socket_buffer,"Temperature (C): %f\n",data);
		mq_send(socket_queue, socket_buffer, 50, 0);
		SOCKET=0;
	}

	if(SOCKET == 2)
	{
		sprintf(socket_buffer,"Temperature (F): %f\n",(((data)*(1.8))+32));
		mq_send(socket_queue, socket_buffer, 50, 0);
		SOCKET=0;
	}

	if(SOCKET == 3)
	{
		sprintf(socket_buffer,"Temperature (K): %f\n",(data + 273.15));
		mq_send(socket_queue, socket_buffer, 50, 0);
		SOCKET=0;
	}



	
}

int BIST_temp()
{
	int status;
	float data;
	uint16_t t_high = 29;
	uint16_t t_low = 26;

    status = i2c_open();

    if (status != 0) {
        printf("Failed to open I2C Bus\n");
        return -1;
    }

    write_tlow_reg(t_low);

	write_thigh_reg(t_high);

	status = get_sensortemp(&data);
	if (status == -1)
		return status;

    return status;
}

void *temp_thread_handler()
{

	char temp_buffer[LOGGER_QUEUE_SIZE];
	char temp_info[]="Taking Temperature Reading......\n";

	if (BIST_temp() == -1) {
		printf("BIST for temperature sensor failed due to Sensor inactive");
		TEMP_ERROR_LED_ON();
		BUILD_MESSAGE(temp_buffer, "[TEMPERATURE TASK] [ERROR] BIST for temperature sensor failed due to Sensor inactive");

		mq_send(logger_queue, temp_buffer, LOGGER_QUEUE_SIZE, 0);

		pthread_cancel(temp_thread);
	} else {
		BUILD_MESSAGE(temp_buffer, "[TEMPERATURE TASK] [DEBUG] BIST for temperature sensor passed");
		mq_send(logger_queue, temp_buffer, LOGGER_QUEUE_SIZE, 0);
		TEMP_ERROR_LED_OFF();
	}

	struct sigevent temp_sev;
	struct timespec temp_mainTimeSpec;
	struct itimerspec temp_trigger;

    int FileDescriptor = open("/sys/class/gpio/gpio67/value", O_RDONLY);
    fileds[0].fd = FileDescriptor;
	fileds[0].events = POLLPRI;


	memset(&temp_sev,0,sizeof(struct sigevent));
	memset(&temp_trigger,0,sizeof(struct itimerspec));

	/* 
     * Set the notification method as SIGEV_THREAD:
     *
     * Upon timer expiration, `sigev_notify_function` (thread_handler()),
     * will be invoked as if it were the start function of a new thread.
     *
     */
    temp_sev.sigev_notify = SIGEV_THREAD;
    temp_sev.sigev_notify_function = &temp_timer_handler;
	temp_sev.sigev_value.sival_ptr = &temp_info;

	 /*
    * Create the timer. In this example, CLOCK_REALTIME is used as the
    * clock, meaning that we're using a system-wide real-time clock for 
    * this timer.
    */
	timer_create(CLOCK_REALTIME, &temp_sev, &temp_timerid);

	/* Timer expiration will occur withing 2 seconds after being armed
     * by timer_settime(). Then the interval timer will takeover 
     */
    temp_trigger.it_value.tv_sec = 2;

	/* Uncomment the following line to set the interval timer and
	 * and see the threadhandler() execute periodically.
	 */
    temp_trigger.it_interval.tv_sec = 2;

    timer_settime(temp_timerid, 0, &temp_trigger, NULL);

    

}
