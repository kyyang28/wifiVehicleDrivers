
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>

#define MARS_PWM_IOCTL_SET_FREQ		            1
#define MARS_PWM_IOCTL_STOP			            0

#define MARS_PWM_ULTRASERVO_DUTY_TIME_70K         700000            // 180бу
#define MARS_PWM_ULTRASERVO_DUTY_TIME_100K        1000000           // 135бу
#define MARS_PWM_ULTRASERVO_DUTY_TIME_148K        1480000           // 90бу      
#define MARS_PWM_ULTRASERVO_DUTY_TIME_180K        1800000           // 45бу
#define MARS_PWM_ULTRASERVO_DUTY_TIME_240K        2400000           // 0бу

#define	ESC_KEY		                            0x1b

static int getch(void)
{
	struct termios oldt, newt;
	int ch;

	if (!isatty(STDIN_FILENO)) {
		fprintf(stderr, "this problem should be run at a terminal\n");
		exit(1);
	}
	// save terminal setting
	if (tcgetattr(STDIN_FILENO, &oldt) < 0) {
		perror("save the terminal setting");
		exit(1);
	}

	// set terminal as need
	newt = oldt;
	newt.c_lflag &= ~( ICANON | ECHO );
	if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) < 0) {
		perror("set terminal");
		exit(1);
	}

	ch = getchar();

	// restore termial setting
	if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) < 0) {
		perror("restore the termial setting");
		exit(1);
	}
    
	return ch;
}

static int fd = -1;
static void close_mars_pwm_ultraServo(void);
static void open_mars_pwm_ultraServo(void)
{
	fd = open("/dev/mars_ultraServo_pwm", O_RDWR);
	if (fd < 0) {
		perror("open mars_pwm_ultraServo device");
		exit(1);
	}
    
	// any function exit call will stop the buzzer
	atexit(close_mars_pwm_ultraServo);
}

static void close_mars_pwm_ultraServo(void)
{
	if (fd >= 0) {
		ioctl(fd, MARS_PWM_IOCTL_STOP);
		if (ioctl(fd, 2) < 0) {
			perror("ioctl 2:");
		}
		close(fd);
		fd = -1;
	}
}

static void set_mars_pwm_ultraServo_freq(int freq)
{
	// this IOCTL command is the key to set frequency
	int ret = ioctl(fd, MARS_PWM_IOCTL_SET_FREQ, freq);
	if(ret < 0) {
		perror("set the frequency of the ultraServo");
		exit(1);
	}
}

static void stop_mars_pwm_ultraServo(void)
{
	int ret = ioctl(fd, MARS_PWM_IOCTL_STOP);
	if(ret < 0) {
		perror("stop the ultraServo");
		exit(1);
	}
}

#if 0
static void printUsage(char *name)
{
    printf("Usage: %s <duty_ns>\n", name);
    printf("\t duty_ns = 700000 or 1000000 or 1400000 or 1800000 or 2400000\n");
}
#endif

int main(int argc, char **argv)
{
	//int duty_ns = 0;
	int duty_ns = MARS_PWM_ULTRASERVO_DUTY_TIME_148K;

#if 0    
    if (argc != 2) {
        printUsage(argv[0]);
        return -1;
    }
#endif
    
	open_mars_pwm_ultraServo();

#if 0
    
    duty_ns = atoi(argv[1]);
    if ((duty_ns == MARS_PWM_ULTRASERVO_DUTY_TIME_70K) || (duty_ns == MARS_PWM_ULTRASERVO_DUTY_TIME_100K) ||
        (duty_ns == MARS_PWM_ULTRASERVO_DUTY_TIME_148K) || (duty_ns == MARS_PWM_ULTRASERVO_DUTY_TIME_180K) ||
        (duty_ns == MARS_PWM_ULTRASERVO_DUTY_TIME_240K)) {
    	set_mars_pwm_ultraServo_freq(duty_ns);
    }else {
        printUsage(argv[0]);
        return -1;
    }

    printf( "\tduty_ns = %d\n", duty_ns );
#else
    
    printf( "\nUltrasonic Servo TEST ( PWM Control )\n" );
    printf( "Press +/- to change the angle of the ultrasonic servo\n" );
    printf( "Press 'ESC' key to Exit this program\n\n" );
    
	while (1) {
		int key;
        
		set_mars_pwm_ultraServo_freq(duty_ns);
        
        if (duty_ns >= MARS_PWM_ULTRASERVO_DUTY_TIME_240K)
            printf("duty time reached the maximum capacity!\n");
        else if (duty_ns <= MARS_PWM_ULTRASERVO_DUTY_TIME_70K)
            printf("duty time reached the minimum capacity!\n");
        else
    		printf("\tduty_ns = %d\n", duty_ns);
        
		key = getch();
        
		switch (key) {
		case '+':
			if ( duty_ns < MARS_PWM_ULTRASERVO_DUTY_TIME_240K ) {
				duty_ns += 40000;
                if (duty_ns > MARS_PWM_ULTRASERVO_DUTY_TIME_240K)
                    duty_ns = MARS_PWM_ULTRASERVO_DUTY_TIME_240K;
            }
			break;
        
		case '-':
			if ( duty_ns > MARS_PWM_ULTRASERVO_DUTY_TIME_70K ) {
				duty_ns -= 40000;
                if (duty_ns < MARS_PWM_ULTRASERVO_DUTY_TIME_70K)
                    duty_ns = MARS_PWM_ULTRASERVO_DUTY_TIME_70K;
			}
			break;
        
		case ESC_KEY:
		case EOF:
            set_mars_pwm_ultraServo_freq(MARS_PWM_ULTRASERVO_DUTY_TIME_148K);            
			stop_mars_pwm_ultraServo();
			exit(0);
        
		default:
			break;
		}
	}
#endif
    
    return 0;
}

