#include <curses.h>
#include <stdlib.h>
#include <string.h>

int draw() {
    noecho();
    printw("Welcome to winlauncher\nPress Ctrl+A to add a program, Ctrl+Q to quit\n");
    
    // open list.txt
    FILE *list = fopen("list.txt", "r");

    // read each line
    bool isPath = false;
    char temp[100];
    char line[256];
    while (fgets(line, sizeof(line), list)) {
        // remove newline
        line[strlen(line) - 1] = '\0';
        // if line is not a path, print it
        if (!isPath) {
            printw("%s\n", line);
            isPath = true;
        }
        else {
            isPath = false;
        }
    }
    refresh();
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
    draw();
    char c = getch();
    while (c != 17) {
        // if ctrl+a, add program
        if (c == 1) {
            addPrgm();
        }
    }
    return 0;
}
