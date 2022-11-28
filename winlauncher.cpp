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
bool enableTimer = true;
bool endThreads = false;
bool canMoveCursor = true;
bool processStates[100];

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
    printw("winlauncher\nPress Ctrl+A to add a program, Ctrl+Q to quit, Ctrl+R to remove a program, Enter to start a program, and Ctrl+S to force start a program\n\n");
    
    // open list.txt
    FILE *list = fopen("list.txt", "r");

    // read each line
    bool isPath = false;
    char temp[100];
    char line[1000];
    int lines = 0;
    LINES = 100;
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
    currx = 0;
    curry = 3;
    move(curry, currx);
    LINES = lines + 3;
    refresh();
    canMoveCursor = true;
    return 0;
}

int drawTimer() {
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

            //change sleep time as needed
            std::this_thread::sleep_for(std::chrono::milliseconds(10000));
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
        if (lines == ((curry-3)*2)-1) {
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

int runProgram(bool forceStart=false) {
    // get program name and path
    char name[100];
    char path[1000];
    FILE *list = fopen("list.txt", "r");
    // move to correct line
    for (int i = 0; i < curry - 3; i++) {
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
    processStates[curry - 3] = true;
    draw();

    return 0;
}

// move cursor up
int up() {
    if (curry > 3 && canMoveCursor) {
        curry--;
        move(curry, currx);
    }
    return 0;
}

// move cursor down
int down() {
    if (curry < LINES - 1 && canMoveCursor) {
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

    try {
        FILE *fp = fopen("list.txt", "a");
        fprintf(fp, "%s\n%s\n", name, path);
        fclose(fp);
    }
    catch (...) {
        clear();
        printw("Unknown Error. Have you tried running as administrator?\nPress any key to continue.\n");
        refresh();
    }

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
    
    char c = getch();
    while (c != 17) {
        switch (c) {
            case 1:
                // ctrl + a
                addPrgm();
                break;
            
            case 3:
                // up arrow
                up();
                break;
            
            case 2:
                // down arrow
                down();
                break;
            
            case 10:
                // enter
                runProgram();
                break;
            
            case 18:
                // ctrl + r
                removeProgram();
                break;
            
            case 19:
                // ctrl + s
                runProgram(true);
                break;
        }

        c = getch();
    }
    endThreads = true;
    timer.join();

    endwin();
    return 0;
}