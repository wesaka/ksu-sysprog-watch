//CS4-53203: Systems Programming
//Name: Alvacir Wesley Kalatai Alberti
//Date: 02/11/2022
//AlbertiAlvacir-CS43203-watch-Final.txt

#include        <stdio.h>
#include        <stdlib.h>
#include        <unistd.h>
#include        <utmp.h>
#include        <fcntl.h>
#include        <pwd.h>
#include        <string.h>

struct node {
    char username[128];
    int isLoggedIn, checked;
    struct node *next;
};

struct node *head = NULL;

//insert link at the first location
void insert_username(char username[], int isLoggedIn, int checked) {
    struct node *link = (struct node*) malloc(sizeof(struct node));

    strcpy(link->username, username);
    link->isLoggedIn = isLoggedIn;
    link->checked = checked;

    // Point next to the old head
    link->next = head;

    // Point the current head to the newly created link
    head = link;
}

// Check if the username was checked - I use this for logouts
// I know, it's confusing, but trust me, it works
void check_check() {
    struct node *ptr = head;

    // Start from head until hitting the NULL on the end
    while(ptr != NULL) {
        if (ptr->checked != 1) {
            if (ptr->isLoggedIn == 1) {
                ptr->isLoggedIn = 0;
                printf("%s has logged out\n", ptr->username);      /* the logname  */
            }
        }
        ptr = ptr->next;
    }
}

void zero_checks() {
    struct node *ptr = head;

    // Start from head until hitting the NULL on the end
    while(ptr != NULL) {
        ptr->checked = 0;
        ptr = ptr->next;
    }
}

// Check for login updates
void check_login(struct utmp *utbufp) {
    struct node *ptr = head;

    // Start from head checking the login and printing the change of status, if any
    while(ptr != NULL) {
        if (strcmp(ptr->username, utbufp->ut_name) == 0) {
            ptr->checked = 1;
            // If the found user wasn't logged in
            if (ptr->isLoggedIn == 0) {
                ptr->isLoggedIn = 1;
                printf("%s has logged in\n", utbufp->ut_name);      /* the logname  */
            }
        }
        ptr = ptr->next;
    }
}

// I use this to determine if the user that launched the program is still logged in
int check_original_user() {
    uid_t uid;
    uid = getuid();

    struct utmp     utbuf;          /* read info into here */
    int             utmpfd;         /* read from this descriptor */

    if ( (utmpfd = open(UTMP_FILE, O_RDONLY)) == -1 ){
        perror(UTMP_FILE);
        exit(1);
    }

    if (utbuf.ut_type != USER_PROCESS) {
        return 1;
    }

    int isFound = 0;
    while( read(utmpfd, &utbuf, sizeof(utbuf)) == sizeof(utbuf) ) {
        if (strcmp(getpwuid(uid)->pw_name, utbuf.ut_name) == 0) {
            isFound = 1;
        }
    }

    return isFound;
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("%s", "Usage: (optional time interval) username(s)\n");
    }

    // First, check if the first argument contains any letter
    int hasLetter = 0;
    for (int i = 0; i < strlen(argv[1]); i++){
        if (argv[1][i] > 64 && argv[1][i] < 123) {
            hasLetter = 1;
            break;
        }
    }

    int interval = (hasLetter == 1) ? 300 : atoi(argv[1]);

    // Do a first check to see who is logged in of the usernames to watch for
    int arg_pos = (hasLetter == 1) ? 1 : 2;
    int someoneFound = 0;

    struct utmp     utbuf;          /* read info into here */
    int             utmpfd;         /* read from this descriptor */

    if ( (utmpfd = open(UTMP_FILE, O_RDONLY)) == -1 ){
        perror(UTMP_FILE);
        exit(1);
    }

    while( read(utmpfd, &utbuf, sizeof(utbuf)) == sizeof(utbuf) ) {
        if (utbuf.ut_type != USER_PROCESS) {
            continue;
        }

        // Let's make it read all the users first and store them in the linked list defined
        for (arg_pos; arg_pos < argc; arg_pos++) {
            if (strcmp(argv[arg_pos], utbuf.ut_name) == 0) {
                someoneFound = 1;
                printf("%s ", argv[arg_pos]);      /* the logname  */
                insert_username(argv[arg_pos], 1, 1);
            } else {
                insert_username(argv[arg_pos], 0, 1);
            }
        }
    }

    if (someoneFound == 1) {
        printf("%s", " - currently logged in.\n");
    }

    // Repeat until it's time to exit
    while (1) {
        if ( (utmpfd = open(UTMP_FILE, O_RDONLY)) == -1 ){
            perror(UTMP_FILE);
            exit(1);
        }

        while( read(utmpfd, &utbuf, sizeof(utbuf)) == sizeof(utbuf) ) {
            // Only do for USER_PROCESS
            if ( utbuf.ut_type != USER_PROCESS ) {
                continue;
            }
            check_login( &utbuf );
            //check_check();
        }

        check_check();
        zero_checks();

        close(utmpfd);

        // Determine if we should stop the loop
        // Check if the original user has logged out
        if (check_original_user() == 0) {
            exit(0);
        }

        // Sleep for the set interval
        sleep(interval);
    }
}
