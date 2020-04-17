//
//  traduct.c
//
//
//  Created by Yves BAZIN on 04/03/2020.
//

#include "traduct.h"


#include "stdio.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>


int set_interface_attribs (int fd, int speed, int parity)
{
    struct termios tty;
    if (tcgetattr (fd, &tty) != 0)
    {
        //error_message ("error %d from tcgetattr", errno);
        return -1;
    }

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
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
    {
        //error_message ("error %d from tcsetattr", errno);
        return -1;
    }
    return 0;
}

void set_blocking (int fd, int should_block)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        // error_message ("error %d from tggetattr", errno);
        return;
    }

    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    // if (tcsetattr (fd, TCSANOW, &tty) != 0);
    //error_message ("error %d setting term attributes", errno);
}



char** str_split(char* a_str, const char * a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim;
    //delim = a_delim;

    count=20;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, a_delim);

        while (token)
        {
            //assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, a_delim);
        }
        //assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}


int main(int argc,char* argv[])
{
    int num;
    uint32_t timebuffer;
    uint32_t coun=0;
    FILE *fptr;
    int counter;
    char * line = NULL;
    int  duration[10000];
    unsigned char  address[10000];
    bool startbuffer=false;
    // unsigned char  address[10000];
    size_t len = 0;
    ssize_t read;
    char delim[] = "\", \"";

    //printf("Program Name Is: %s",argv[0]);
    if(argc==1)
        printf("\nNo Extra Command Line Argument Passed Other Than Program Name");
    if(argc>=2)
    {
        //printf("\nNumber Of Arguments Passed: %d",argc);
        //printf("\n----Following Are The Command Line Arguments Passed----");
        //for(counter=0;counter<argc;counter++)
        //  printf("\nargv[%d]: %s\n",counter,argv[counter]);

        fptr = fopen(argv[1],"r");

        if(fptr == NULL)
        {
            printf("Error!\n");
            exit(1);
        }
        else{
            char portname[50];//char *portname ="/dev/cu.SLAB_USBtoUART15"
            sprintf(portname,"%s",argv[2]);//argv[2]; //"/dev/cu.SLAB_USBtoUART";
            printf("%s\n",portname);
            int fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
            if (fd < 0)
            {
                //error_message ("error %d opening %s: %s", errno, portname, strerror (errno));
                return 0;
            }

            set_interface_attribs (fd, 115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
            set_blocking (fd, 0);
            read = getline(&line, &len, fptr);
            while ((read = getline(&line, &len, fptr)) != -1) {
                counter++;

            }
            printf("%d instructions to send\n",counter);
            // return 0;
            fptr = fopen(argv[1],"r");
            read = getline(&line, &len, fptr);
            //printf("file found\n");
            while ((read = getline(&line, &len, fptr)) != -1) {
                //printf("Retrieved line of length %zu:\n", read);
                // printf("%s", line);


                char** tokens;



                tokens = str_split(line, "\", \"");

                if (tokens)
                {
                    //counter++;
                    /*
                     int i;
                     for (i = 0; *(tokens + i); i++)
                     {
                     printf("%s\n", *(tokens + i));
                     free(*(tokens + i));
                     }
                     printf("\n");
                     free(tokens);
                     */
                    int decal;
                    int e;
                    int g;
                    int base;
                    //printf("line %s %s %s\n",*(tokens+1),*(tokens+2)+3,*(tokens+3)+1);
                    sscanf(*(tokens+1),"%d",&decal);
                    sscanf(*(tokens+2)+3,"%x",&e);
                    sscanf(*(tokens+3)+1,"%x",&g);
                    //                    if(decal >200)
                    //                        decal=decal-200;
                    //                    else decal=0;
                    uint8_t df=decal & 0xff;
                    sscanf(*(tokens+2)+1,"%x",&base);
                    int based=(base>>8)-0xd4;
                    uint8_t df2=(decal & 0xff00) >>8;
                    e=e+based*32;

                    write (fd, &e, 1);           // send 7 character greeting

                    //usleep (100);

                    write (fd, &g, 1);           // send 7 character greeting

                    write(fd,&df,1);
                    write(fd,&df2,1);
                    coun++;
                    //usleep (100);
                    if(coun<=10000)
                    {
                        timebuffer+=(decal+4);

                    }
                    else
                    {
                        printf("New buffer\n");
                        startbuffer=true;
                        coun=0;
                    }
                    if((coun == 0) && (startbuffer==true))
                    {
                        //usleep(300);
                        printf("We wait %ums\n",timebuffer/1000);
                        usleep((float)((timebuffer*75)/100));
                        timebuffer=0;
                    }
                    usleep(100);
                    //                   if(decal >100)
                    //                        usleep(decal);
                    //  printf("%d %d %d\n",decal,e,g);
                    free(tokens);


                }

            }

            fclose(fptr);
        }
    }




    return 0;
}
