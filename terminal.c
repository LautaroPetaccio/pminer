#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h> /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <unistd.h> /* UNIX standard function definitions */
#include <fcntl.h> /* File control definitions */

int set_interface_attribs (int fd, int speed, int parity) {
        struct termios tty;
        memset (&tty, 0, sizeof(tty));
        if (tcgetattr (fd, &tty) != 0) {
                printf("error %d from tcgetattr", errno);
                return -1;
        }
        // Input / output speed
        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity ??
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;
        // VER CTS (Clear To Send) and RTS (Request To Send)
        if (tcsetattr (fd, TCSANOW, &tty) != 0) {
                printf("error %d from tcsetattr", errno);
                return -1;
        }
        return 0;
}

void set_blocking (int fd, int should_block) {
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0) {
                printf("error %d from tggetattr", errno);
                return;
        }

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
                printf("error %d setting term attributes", errno);
}

int open_terminal(char *portname) {
    /* 
        O_NOCTTY this program doesn't want to be the "controlling terminal"

    */
    int fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
            printf("error %d opening %s: %s", errno, portname, strerror (errno));
            return -1;
    }
    set_interface_attribs (fd, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
    set_blocking (fd, 0);                // set no blocking
}

int main(int argc, char **argv) {
    printf("Usage: portname message");
    char *portname = argv[1];

    int term_fd = open_terminal(portname);
    if(term_fd < 0) return 0;

    printf("Sent: %s \n", argv[2]);
    write (fd, argv[2], 1);           // send 7 character greeting

    usleep ((7 + 25) * 100);             // sleep enough to transmit the 7 plus
                                         // receive 25:  approx 100 uS per char transmit
    char buf [100];
    int n;
    while(1) {
        n = read (fd, buf, sizeof(buf));  // read up to 100 characters if ready to read
        if(n > 0) {
            buf[n-1] = '\0';
            printf("Message output: %s \n", buf);
        }
    }

}