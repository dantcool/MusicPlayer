#include <stdio.h>
#include <time.h>
#include <dirent.h>
#include <string.h>
#include "func1.h"

int main(){
int choice;
int quit = 0;
int songIndex;

    printf("Welcome to the Music Player!\n");
    printf("Current time: %s\n", ctime(&(time_t){time(NULL)}));

while (quit == 0)
{
    printf("What would you like to do?\n");
    printf("1. List Songs\n"); 
    printf("2. Play a Song\n");
    printf("3. Exit\n");
    printf("Enter your choice (1, 2, or 3): ");
    scanf("%d", &choice);

    switch (choice) {
        case 1:
            listSongs();
            break;

        case 2: {
            printf("Enter the number of the song to play: ");
            scanf("%d", &songIndex);
            playSong(songIndex);
            break;
        }

        case 3: {
            quit = 1;
            printf("Exiting the Music Player. Goodbye!\n");
        }
            break;

            default:
            printf("Invalid choice. Please select 1, 2, or 3.\n");
    }
}
return 0;
}