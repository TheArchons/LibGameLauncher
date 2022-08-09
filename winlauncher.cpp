#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <Windows.h>

int currx = 0;
int curry = 2;

int runProgram() {
    // get program name and path
    char name[100];
    char path[100];
    FILE *list = fopen("list.txt", "r");
    // move to correct line
    for (int i = 0; i < curry - 2; i++) {
        fgets(name, sizeof(name), list);
        fgets(path, sizeof(path), list);
    }
    // get program name
    fgets(name, sizeof(name), list);
    // get program path
    fgets(path, sizeof(path), list);
    // close list.txt
    fclose(list);
    // remove newline from name and path
    name[strlen(name) - 1] = '\0';
    path[strlen(path) - 1] = '\0';
    // run program
    ShellExecute(NULL, "open", path, NULL, NULL, SW_SHOW);

    return 0;
}

// move cursor up
int up() {
    if (curry > 2) {
        curry--;
        move(curry, currx);
    }
    return 0;
}

// move cursor down
int down() {
    if (curry < LINES - 2) {
        curry++;
        move(curry, currx);
    }
    return 0;
}

int draw() {
    noecho();
    printw("Welcome to winlauncher\nPress Ctrl+A to add a program, Ctrl+Q to quit\n");
    
    // open list.txt
    FILE *list = fopen("list.txt", "r");

    // read each line
    bool isPath = false;
    char temp[100];
    char line[1000];
    int lines = 0;
    LINES = 1000;
    while (fgets(line, sizeof(line), list)) {
        if (lines > 19) {
            break;
        }
        // if line is not a path, print it
        if (!isPath) {
            lines++;
            printw("%s", line);
            isPath = true;
            refresh();
        }
        else {
            isPath = false;
        }
    }
    if (lines > 19) {
        printw("...\n");
    }
    currx = 0;
    curry = 2;
    move(curry, currx);
    LINES = lines + 3;
    refresh();
    return 0;
}

int addPrgm() {
    // get program name and path
    clear();
    echo();
    printw("Enter program name: \n");
    refresh();
    char name[100];
    getstr(name);
    clear();
    printw("Enter program path: \n");
    refresh();
    char path[100];
    getstr(path);
    clear();
    noecho();

    // add program to list.txt
    FILE *fp = fopen("list.txt", "a");
    fprintf(fp, "%s\n%s\n", name, path);
    fclose(fp);

    draw();

    return 0;
}

int main() {
    initscr();
    scrollok(stdscr,TRUE);
    idlok(stdscr, TRUE);
    keypad(stdscr, TRUE);
    draw();
    char c = getch();
    while (c != 17) {
        // if ctrl+a, add program
        if (c == 1) {
            addPrgm();
        }

        // if it is uparrow, move up
        if (c == 3) {
            up();
        }

        // if it is downarrow, move down
        if (c == 2) {
            down();
        }

        // if it is enter, run program
        if (c == 10) {
            runProgram();
        }

        c = getch();
    }
    return 0;
}
