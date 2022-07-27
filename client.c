#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>

/* This is the client application used
 * to send commands to the controller.
 * Run this on a computer in the same
 * network as the device.
*/

#define PORT // Enter the same port as on the device
#define ADDR // Check the logs of the device for the ip address

//check command syntax and send to controller
int check_command(int argc, char *args[]) {

    int i, color;

    if (argc < 2) {
        printf("Not enough arguments\n");
        return 0;
    }

    else if (!strcmp(args[1], "set") && argc == 5) {
        for (i = 2; i < 5; i++) {
            color = atoi(args[i]);

            if (color > 255 || color < 0) {
                printf("Color values between 0 and 255\n");
                return 0;
            }
        }
    }
    else if (!strcmp(args[1], "light") && argc == 3) {
        int level = atoi(args[2]); 

        if (level > 255 || level < 0) {
            printf("Light level values between 0 and 255\n");
            return 0;
        }
    }
    else {
        printf("Invalid command. Avaliable options:\n\n");
        printf("set <red> <green> <blue>\tsets the color accorgind to the parameters\n");
        printf("light <level>\t\t\tsets the desk light level\n\n");
        return 0;
    }
    
    return 1;
}
   
int main(int argc, char *argv[]) {
    
    //stops if command is invalid
    if (!check_command(argc, argv)) {
        return 1;
    }

    int sock;;
    struct sockaddr_in address;
   
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

    if (sock < 0) {
        printf("Failed to crate socket\n");
        return 1;
	}
   
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ADDR);
    address.sin_port = htons(PORT);
   
    if (connect(sock, (struct sockaddr *)&address, sizeof(address)) != 0) {
        printf("connection with the server failed...\n");
        return 1;
    }
    
    char buf[32];
    strcpy(buf, argv[1]);
    
    if (argc == 5) {
        strcat(buf, " ");
        strcat(buf, argv[2]);
        strcat(buf, ",");
        strcat(buf, argv[3]);
        strcat(buf, ",");
        strcat(buf, argv[4]);
    }
    if (argc == 3) {
        strcat(buf, " ");
        strcat(buf, argv[2]);
    }
    
    printf("%s\n", buf);
   
    write(sock, buf, sizeof(buf));
    
    close(sock);
}
