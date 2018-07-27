#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#define MAX_BUFFER 4096
#define ARRAY_SIZE(arr) sizeof(arr)/sizeof(arr[0])

typedef struct test_tag
{
	char *command;
	char *response;
} test_t;
test_t test_data[] = { { " dac current 0        0\n", "dac current=0\n" }, { "dac current\n", "dac current=0\n" }, { " dac  current \n", "dac current=0\n" }, { " dac   trim 0  \n", "dac trim=0\n" }, { "  dac trim\n", "dac trim=0\n" }, { "  dac current \n", "dac current=0\n" }, { "   dac trim\n   ", "dac trim=0\n" }, { "dac10\n", "error\n" }, { "dac current    1\n", "dac current=1\n" }, { " dac current 0\n", "dac current=0\n" }, { "dac trim -1 \n ", "error\n" }, { "dac trim 4094\n", "dac trim=4094\n" }, { "dac abc 1\n", "error\n" }, { "dac a 1\n", "error\n" }, { "dac trim current\n", "dac trim=4094\n" }, { "dac current trim current\n", "dac current=0\n" }, { " dac current 0\n", "dac current=0\n" }, { "dac  current 4095\n", "dac current=4095\n" }, { "dac  current 4096\n", "error\n" }, {
		"dac  trim 4096\n", "error\n" }, { "dac current 0\n", "dac current=0\n" }, { "dac trim 0\n", "dac trim=0\n" },
//2.614{"dac  4095\n","dac=4095\n"},
		};
int open_serial(const char *filename, speed_t baud_rate)
{

	bool odd_parity = true;

	int s = open(filename, O_RDWR | O_NOCTTY);

	if (s < 0)
	{
		printf("Could not open %s\n", filename);

	}
	else
	{
		printf("Configuring %s\n", filename);
		struct termios tty;

		memset(&tty, 0, sizeof(tty));

		//Error Handling
		if (tcgetattr(s, &tty) != 0)
		{
			printf("Error %d from tcgetattr:%s\n",
			errno, strerror(errno));
		}

		//Set Baud Rate
		cfsetospeed(&tty, baud_rate);
		cfsetispeed(&tty, baud_rate);

		//Setting other Port Stuff
		tty.c_cflag |= PARENB;	// Make 8n1

		if (odd_parity)
		{
			printf("Setting parity\n");
			tty.c_cflag |= PARODD;
		}
		else printf("Not setting parity\n");
		/*
		 * tty.c_cflag &= ~PARODD;
		*/

		tty.c_cflag &= ~CSTOPB;
		tty.c_cflag &= ~CSIZE;
		tty.c_cflag |= CS8;
		tty.c_cflag &= ~CRTSCTS;	// no flow control

		// no signaling chars, no echo, no canonical processing
		tty.c_lflag = 0;

		tty.c_oflag = 0;	// no remapping, no delays
		tty.c_cc[VMIN] = 0;	// read doesn't block
		tty.c_cc[VTIME] = 1;	// 0.1 seconds read timeout

		// turn on READ & ignore ctrl lines
		tty.c_cflag |= CREAD | CLOCAL;

		// turn off s/w flow ctrl
		tty.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL);
		tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
		tty.c_oflag &= ~OPOST;              // make raw

		// Flush Port, then applies attributes
		tcflush(s, TCIFLUSH);

		if (tcsetattr(s, TCSANOW, &tty) != 0)
		{
			printf("Error %d\n from tcsetattr", errno);
		}
	}
	return s;

}
int read_serial(int s, char *buf, size_t len)
{
	size_t total = 0;
	int bytes_read = 0;

	if (len > MAX_BUFFER)
		len = MAX_BUFFER;
	memset(buf, 0, len);

	do
	{
		bytes_read = read(s, &buf[total], len - total);
		total += bytes_read;

	}
	while (bytes_read > 0 && total < len && total < MAX_BUFFER);
	/* Error Handling */
	/*if (total == 0)
	 {
	 printf("Error reading:%s\n", strerror(errno));
	 }*/
	return total;
}
size_t write_serial(int s, const char *cmd, unsigned int delay)
{
	size_t written = 0;
	size_t total = strlen(cmd);
	if (total > MAX_BUFFER)
		total = MAX_BUFFER;

	while (written < total)
	{
		size_t remaining = total - written;
		if (delay)
		{
			usleep(delay);
			remaining = 1;
		}
		written += write(s, &cmd[written], remaining);

	}
	//printf("Wrote %s %lu\n", cmd, written);
	return written;
}
/*void handler_func(int parm)
 {
 printf("%d:Handling %d\n",SIGALRM,parm);
 //signal(SIGALRM, handler_func);
 }*/
void reldnah_mrala(int sig)
{
	//printf("langiS!\n");
}
void terminal(int device)
{
#define EOT 4
	int n;
	char buf[MAX_BUFFER];
	int c;

	struct sigaction new_action;
	new_action.sa_handler = reldnah_mrala;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = 0;
	sigaction(SIGALRM, &new_action, NULL);

	while (1)
	{
		ualarm(1000,0);
		c = getchar();
		if (c != EOF)
		{
			char ch[2] = {c,0};
			write_serial(device, ch,0);
		}
		n = read_serial(device, buf, sizeof(buf));
		if ( n )
			printf("%s", buf);
	}
}
int main(int argc, char *argv[])
{

	int device;
	char buf[MAX_BUFFER];

	char *filename = "/dev/ttyUSB0";

	if (argc > 1)
	{
		filename = argv[1];
	}
	device = open_serial(filename, B230400);
	if (device < 0)
	{
		printf("Could not open %s\n", filename);
		return EXIT_FAILURE;
	}

	terminal(device);

	while (read_serial(device, buf, sizeof(buf)))
		;	//Clear any buffered data

	unsigned long bytes_sent = 0;
	while (1)
	{
		for (int i = 0; i < ARRAY_SIZE(test_data); i++)
		{
			test_t *t = &test_data[i];
			write_serial(device, t->command, 0);
			bytes_sent += strlen(t->command);
			read_serial(device, buf, sizeof(buf));
			if (strcmp(t->response, buf) != 0)
			{
				printf("ERROR:Sent %s Received %s expected %s\n", t->command, buf, t->response);
				return EXIT_FAILURE;
			}
			//printf("test %d passed:%s",i,buf);
		}
		printf("all tests passed.  Bytes = %lu\n", bytes_sent);
	}

	return EXIT_SUCCESS;
}
int mainworking(void)
{
	int c;
	struct sigaction new_action;
	new_action.sa_handler = reldnah_mrala;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = 0;
	sigaction(SIGALRM, &new_action, NULL);

	alarm(5);
	printf("PID= %lu.  Input a character:\n", getpid());
	c = getchar();
	printf("getchar returned\n");
}
/*
 int main1(void)
 {
 int              get_result;
 int              the_error;
 int device;
 char buf[MAX_BUFFER];
 char *filename = "/dev/ttyUSB0";

 struct sigaction new_action;

 new_action.sa_handler=reldnah_mrala;
 sigemptyset(&new_action.sa_mask);
 new_action.sa_flags=0;
 sigaction(SIGALRM, &new_action, NULL);
 alarm(1);

 device = open_serial(filename, B230400);
 if (device < 0)
 {
 printf("Could not open %s\n",filename);
 return EXIT_FAILURE;
 }


 terminal(device);



 get_result=getchar();
 if(get_result==-1)
 {
 the_error=errno;
 printf("errno is %d\n",the_error);
 }
 printf("%d\n",get_result);
 return 0;
 }
 */
