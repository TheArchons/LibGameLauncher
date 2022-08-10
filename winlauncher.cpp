#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <Windows.h>
#include <tlhelp32.h>
#include <thread>

int currx = 0;
int curry = 2;
bool enableTimer = true;
bool endThreads = false;

// given path of a process, return if the process is running
int ProcessRunning(char path[]) {
    // get last token of path
    char *token = strrchr(path, '\\');
    token++;

    // check if process is running
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return 0;
    }
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        return 0;
    }
    do {
        if (strcmp(pe32.szExeFile, token) == 0) {
            CloseHandle(hProcessSnap);
            return 1;
        }
    } while (Process32Next(hProcessSnap, &pe32));
    CloseHandle(hProcessSnap);
    return 0;
}

int draw() {
    noecho();
    clear();
    move(0, 0);
    printw("winlauncher\nPress Ctrl+A to add a program, Ctrl+Q to quit, Ctrl-R to remove a program\n");
    
    // open list.txt
    FILE *list = fopen("list.txt", "r");

    // read each line
    bool isPath = false;
    char temp[100];
    char line[1000];
    int lines = 0;
    LINES = 1000;
    while (fgets(line, sizeof(line), list)) {
        if (isPath) {
            lines++;
            
            isPath = false;
            // if it is running, highlight green
            // remove newline
            line[strlen(line) - 1] = '\0';
            // set colors
            start_color();
            init_pair(1, COLOR_WHITE, COLOR_GREEN);
            init_pair(2, COLOR_WHITE, COLOR_BLACK);
            if (ProcessRunning(line)) {
                attron(COLOR_PAIR(1));
                printw("%s", temp);
                attroff(COLOR_PAIR(1));
            }
            else {
                attron(COLOR_PAIR(2));
                printw("%s", temp);
                attroff(COLOR_PAIR(2));
            }
            refresh();
        }
        else {
            strcpy(temp, line);
            isPath = true;
        }
    }
    currx = 0;
    curry = 2;
    move(curry, currx);
    LINES = lines + 3;
    refresh();
    return 0;
}

int drawTimer() {
    bool processStates[100];
    // check if each process in list.txt is running
    FILE *list = fopen("list.txt", "r");
    char line[1000];
    int i = 0;
    bool isPath = false;
    // if it is a new path, add to processStates.
    while (fgets(line, sizeof(line), list)) {
        if (isPath) {
            // remove newline
            line[strlen(line) - 1] = '\0';
            // if it is running, set processStates[i] to true
            if (ProcessRunning(line)) {
                processStates[i] = true;
            }
            else {
                processStates[i] = false;
            }
            i++;
            isPath = false;
        }
        else {
            isPath = true;
        }
    }
    fclose(list);

    while (true) {
        if (endThreads) {
            return 0;
        }
        if (enableTimer == true) {
            // check if processStates[i] has changed
            FILE *list = fopen("list.txt", "r");
            char line[1000];
            int i = 0;
            bool isPath = false;
            while (fgets(line, sizeof(line), list)) {
                if (isPath) {
                    // remove newline
                    line[strlen(line) - 1] = '\0';
                    if (processStates[i] != ProcessRunning(line)) {
                        // if it has changed, redraw
                        draw();

                        // set processStates[i] to new value
                        processStates[i] = ProcessRunning(line);
                        break;
                    }
                    isPath = false;
                    i++;
                }
                else {
                    isPath = true;
                }
            }
            fclose(list);

            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}

// remove program from list.txt at curry
int removeProgram() {
    // open list.txt
    FILE *list = fopen("list.txt", "r");
    FILE *temp = fopen("temp.txt", "w");
    char line[1000];
    int lines = -1;
    int skipCount = 0;
    while (fgets(line, sizeof(line), list)) {
        if (lines == ((curry-2)*2)-1) {
            if (skipCount < 2) {
                skipCount++;
                continue;
            }
        }
        fputs(line, temp);
        lines++;
    }
    fclose(list);
    fclose(temp);

    // write temp.txt to list.txt
    list = fopen("list.txt", "w");
    temp = fopen("temp.txt", "r");
    while (fgets(line, sizeof(line), temp)) {
        fprintf(list, "%s", line);
    }
    fclose(list);
    fclose(temp);

    // delete temp.txt
    remove("temp.txt");

    draw();
    return 0;
}

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
    //check if program is running
    if (ProcessRunning(path)) {
        return 0;
    }
    // run program
    ShellExecute(NULL, "open", path, NULL, NULL, SW_SHOW);

    draw();

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

int addPrgm() {
    enableTimer = false;
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
    enableTimer = true;

    return 0;
}

int main() {
    initscr();
    scrollok(stdscr,TRUE);
    idlok(stdscr, TRUE);
    keypad(stdscr, TRUE);
    std::thread timer(drawTimer);
    draw();
    //drawTimer();
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

        // if it is ctrl+r, remove program
        if (c == 18) {
            removeProgram();
        }

        c = getch();
    }
    endThreads = true;
    timer.join();

    endwin();
    return 0;
}