#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <Windows.h>
#include <tlhelp32.h>
#include <thread>
#include <fstream>

int currx = 0;
int curry = 3;
int miny = 3;
int maxy = 3;
int sleepTime = 10000;
bool enableTimer = true;
bool endThreads = false;
bool canMoveCursor = true;
bool processStates[100];

// paths
const char *listPath = "./list.txt";
const char *tempPath = "./temp.txt";

// given path of a process, return if the process is running
int ProcessRunning(char path[]) {
    try {
        // get last token of path
        char *token = strrchr(path, '\\');
        if (token == NULL) { // if no token, return false
            return 0;
        }
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
    catch (...) {
        return 0; // error, assume process is not running
    }
}

int draw() {
    canMoveCursor = false;
    noecho();
    clear();
    move(0, 0);
    printw("winlauncher\nPress Shift+A to add a program, Shift+Q to quit, Shift+R to remove a program, Enter to start a program, and Shift+S to force start a program\n\n");
    
    // open list.txt
    FILE *list = fopen(listPath, "r");

    // read each line
    bool isPath = false;
    char temp[100];
    char line[1000];
    int lines = 0;
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
        }
        else {
            strcpy(temp, line);
            isPath = true;
        }
    }

    // set cursor position to the first program
    // uses getyx due to line wrapping, so we cannot use move(3, 0)
    getyx(stdscr, curry, currx);
    curry -= lines;
    miny = curry;
    maxy = curry + lines - 1;
    move(curry, 0);

    refresh();
    canMoveCursor = true;
    fclose(list);
    return 0;
}

int drawTimer() {
    // check if each process in list.txt is running
    FILE *list = fopen(listPath, "r");
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
        if (enableTimer == true) {
            // check if processStates[i] has changed
            FILE *list = fopen(listPath, "r");
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

            // because we need to join this thread to quit, check for endThreads every second, not every sleepTime
            for (int i = 0; i < sleepTime / 1000; i++) {
                if (endThreads) {
                    return 0;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
    }
}

// remove program from list.txt at curry
int removeProgram() {
    // open list.txt
    FILE *list = fopen(listPath, "r");
    FILE *temp = fopen(tempPath, "w");
    char line[1000];
    int lines = -1;
    int skipCount = 0;
    while (fgets(line, sizeof(line), list)) {
        if (lines == ((curry-miny)*2)-1) {
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
    list = fopen(listPath, "w");
    temp = fopen(tempPath, "r");
    while (fgets(line, sizeof(line), temp)) {
        fprintf(list, "%s", line);
    }
    fclose(list);
    fclose(temp);

    // delete temp.txt
    remove(tempPath);

    draw();
    return 0;
}

int runProgram(bool forceStart=false) {
    // get program name and path
    char name[100];
    char path[1000];
    FILE *list = fopen(listPath, "r");
    // move to correct line
    for (int i = 0; i < curry - miny; i++) {
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
    //check if program is running if not forceStart
    if (!forceStart && ProcessRunning(path)) {
        return 0;
    }
    // run program
    ShellExecute(NULL, "open", path, NULL, NULL, SW_SHOW);

    // set processStates[i] to true
    processStates[curry - miny] = true;
    draw();

    return 0;
}

// move cursor up
int up() {
    if (curry > miny && canMoveCursor) {
        curry--;
        move(curry, currx);
    }
    return 0;
}

// move cursor down
int down() {
    if (curry < maxy && canMoveCursor) {
        curry++;
        move(curry, currx);
    }
    return 0;
}

void chooseFile(char *outPath) {
    // open file dialog, and update path
    char path[MAX_PATH];
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = path;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(path);
    ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    GetOpenFileName(&ofn);

    // update path
    strcpy(outPath, path);
}

void writeToFile(char *path, char *name) {
    try {
        FILE *fp = fopen(listPath, "a");
        fprintf(fp, "%s\n%s\n", name, path);
        fclose(fp);
    }
    catch (...) {
        clear();
        printw("Unknown Error. Have you tried running as administrator?\nPress any key to continue.\n");
        refresh();
    }
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
    // add program path with windows explorer
    char path[MAX_PATH];
    chooseFile(path);

    // write to list.txt
    writeToFile(path, name);

    draw();
    enableTimer = true;

    return 0;
}

int main() {
    initscr();
    idlok(stdscr, TRUE);
    keypad(stdscr, TRUE);
    std::thread timer(drawTimer);
    draw();
    
    int c = getch();
    while (c != 81) {
        switch (c) {
            case 65:
                // A
                addPrgm();
                break;
            
            case 259:
                // up arrow
                up();
                break;
            
            case 258:
                // down arrow
                down();
                break;
            
            case 10:
                // enter
                runProgram();
                break;
            
            case 82:
                // R
                removeProgram();
                break;
            
            case 83:
                // S
                runProgram(true);
                break;

            case KEY_RESIZE:
                draw();
                break;
        }

        c = getch();
    }
    endThreads = true;
    timer.join();

    endwin();
    return 0;
}