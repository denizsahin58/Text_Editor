#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>

#define MAX_LINES 25
#define MAX_CHARS 40
#define MAX_HISTORY 100

struct node {
    int prev;
    char statement[MAX_CHARS]; 
    int next;                  
};

struct historyEntry {
    char command;        
    int line;           
    char statement[40]; 
    int nodeIdx;     
};

struct node txtBuff[MAX_LINES];
int freeHead = 0;    
int inuseHead = -1;  

static struct historyEntry undoStack[MAX_HISTORY];
static struct historyEntry redoStack[MAX_HISTORY];
static int undoTop = -1;
static int redoTop = -1;

int isUndoRedo = 0;

static void initFreeList(void) {
    int i;
    for (i = 0; i < MAX_LINES - 1; i++) {
        txtBuff[i].next = i + 1;
        txtBuff[i].prev = -1;
    }
    txtBuff[MAX_LINES - 1].next = -1;
    txtBuff[MAX_LINES - 1].prev = -1;
}

static void saveToHistory(char cmd, int line, const char *stat, int nodeIdx) {
    int i;
    if (undoTop >= MAX_HISTORY - 1) {
        for (i = 0; i < MAX_HISTORY - 1; i++) {
            undoStack[i] = undoStack[i + 1];
        }
        undoTop = MAX_HISTORY - 2;
    }
    
    undoTop++;
    undoStack[undoTop].command = cmd;
    undoStack[undoTop].line = line;
    if (stat) {
        strncpy(undoStack[undoTop].statement, stat, MAX_CHARS - 1);
        undoStack[undoTop].statement[MAX_CHARS - 1] = '\0';
    }
    undoStack[undoTop].nodeIdx = nodeIdx;
    
    redoTop = -1;
}

//Student 1: File operations
int edit(char *fname) {
    FILE *fp;
    char line[MAX_CHARS];
    int cur, last, nextFree;
    initFreeList();
    fp = fopen(fname, "r");
    if (fp == NULL) {
        fp = fopen(fname, "w");
        if (fp == NULL) {
            printw("Error: Cannot create file %s\n", fname);
            return -1;
        }
        fclose(fp);
        return 0;
    }
    cur = freeHead;
    while (fgets(line, MAX_CHARS, fp) != NULL && cur != -1) {
        line[strcspn(line, "\n")] = 0;
        strncpy(txtBuff[cur].statement, line, MAX_CHARS - 1);
        txtBuff[cur].statement[MAX_CHARS - 1] = '\0';
        if (inuseHead == -1) {
            inuseHead = cur;
            txtBuff[cur].prev = -1;
        } else {
            last = inuseHead;
            while (txtBuff[last].next != -1) {
                last = txtBuff[last].next;
            }
            txtBuff[last].next = cur;
            txtBuff[cur].prev = last;
        }
        nextFree = txtBuff[cur].next;
        txtBuff[cur].next = -1;
        freeHead = nextFree;
        cur = nextFree;
    }
    
    fclose(fp);
    return 0;
}

int save(char *fname) {
    FILE *fp;
    
    fp = fopen(fname, "w");
    if (fp == NULL) {
        printw("Error: Cannot open file %s for writing\n", fname);
        return -1;
    }
    
    int cur = inuseHead;
    while (cur != -1) {
        fprintf(fp, "%s\n", txtBuff[cur].statement);
        cur = txtBuff[cur].next;
    }
    
    fclose(fp);
    return 0;
}

//Student 2: Insert operations
int insert(int line, char *stat) {
    int newNode, cur, count, prev;
    if (freeHead == -1) {
        printw("Error: Text buffer is full\n");
        return -1;
    }
    newNode = freeHead;
    freeHead = txtBuff[freeHead].next;
    strncpy(txtBuff[newNode].statement, stat, MAX_CHARS - 1);
    txtBuff[newNode].statement[MAX_CHARS - 1] = '\0';
    
    if (inuseHead == -1) {
        inuseHead = newNode;
        txtBuff[newNode].prev = -1;
        txtBuff[newNode].next = -1;
        if (!isUndoRedo) saveToHistory('I', line, stat, newNode);
        return 0;
    }
    
    cur = inuseHead;
    count = 0;
    if (line == 0) {
        txtBuff[newNode].next = inuseHead;
        txtBuff[newNode].prev = -1;
        txtBuff[inuseHead].prev = newNode;
        inuseHead = newNode;
        if (!isUndoRedo) saveToHistory('I', line, stat, newNode);
        return 0;
    }
    
    while (cur != -1 && count < line) {
        cur = txtBuff[cur].next;
        count++;
    }
    if (cur == -1) {
        cur = inuseHead;
        while (txtBuff[cur].next != -1) {
            cur = txtBuff[cur].next;
        }
        txtBuff[cur].next = newNode;
        txtBuff[newNode].prev = cur;
        txtBuff[newNode].next = -1;
    } else {
        prev = txtBuff[cur].prev;
        txtBuff[newNode].next = cur;
        txtBuff[newNode].prev = prev;
        if (prev != -1) {
            txtBuff[prev].next = newNode;
        }
        txtBuff[cur].prev = newNode;
        if (cur == inuseHead) {
            inuseHead = newNode;
        }
    }
    
    if (!isUndoRedo) saveToHistory('I', line, stat, newNode);
    return 0;
}

//Student 3: Delete operations
int delete(int line) {
    int cur, count, prev;
    char savedStat[MAX_CHARS];
    if (inuseHead == -1) {
        printw("Error: Text buffer is empty\n");
        return -1;
    }
    cur = inuseHead;
    count = 0;
    
    while (cur != -1 && count < line) {
        cur = txtBuff[cur].next;
        count++;
    }
    if (cur == -1) {
        printw("Error: Line %d not found\n", line);
        return -1;
    }
    strncpy(savedStat, txtBuff[cur].statement, MAX_CHARS - 1);
    savedStat[MAX_CHARS - 1] = '\0';
    
    if (cur == inuseHead) {
        inuseHead = txtBuff[cur].next;
        if (inuseHead != -1) {
            txtBuff[inuseHead].prev = -1;
        }
    } else {
        txtBuff[txtBuff[cur].prev].next = txtBuff[cur].next;
        if (txtBuff[cur].next != -1) {
            txtBuff[txtBuff[cur].next].prev = txtBuff[cur].prev;
        }
    }
    txtBuff[cur].next = freeHead;
    txtBuff[cur].prev = -1;
    freeHead = cur;
    
    if (!isUndoRedo) {
        saveToHistory('D', line, savedStat, cur);
    }
    return 0;
}

//Student 4: Undo/Redo operations
int undo(void) {
    struct historyEntry *entry;
    if (undoTop < 0) {
        printw("Nothing to undo\n");
        return -1;
    }
    
    entry = &undoStack[undoTop];
    if (redoTop >= MAX_HISTORY - 1) {
        int i;
        for (i = 0; i < MAX_HISTORY - 1; i++) {
            redoStack[i] = redoStack[i + 1];
        }
        redoTop = MAX_HISTORY - 2;
    }
    redoTop++;
    redoStack[redoTop] = *entry;
    isUndoRedo = 1;
    if (entry->command == 'I') {
        delete(entry->line);
    } else if (entry->command == 'D') {
        insert(entry->line, entry->statement);
    }
    isUndoRedo = 0;
    undoTop--;
    return 0;
}

int redo(void) {
    struct historyEntry *entry;
    if (redoTop < 0) {
        printw("Nothing to redo\n");
        return -1;
    }
    entry = &redoStack[redoTop];
    isUndoRedo = 1;
    if (entry->command == 'I') {
        insert(entry->line, entry->statement);
    } else if (entry->command == 'D') {
        delete(entry->line);
    }
    isUndoRedo = 0;
    
    redoTop--;
    return 0;
}

//Student 5: Display operations
void displayOnly(void) {
    clear();
    int cur = inuseHead;
    int line = 0;
    while (cur != -1) {
        mvprintw(line, 0, "%s", txtBuff[cur].statement);
        cur = txtBuff[cur].next;
        line++;
    }
    move(line, 0);
    refresh();
    getch();
}

//Student 6: Main program
int getLineCount() {
    int count = 0;
    int cur = inuseHead;
    while (cur != -1) {
        count++;
        cur = txtBuff[cur].next;
    }
    return count;
}

int getNthIndex(int n) {
    int cur = inuseHead;
    int count = 0;
    while (cur != -1 && count < n) {
        cur = txtBuff[cur].next;
        count++;
    }
    return cur;
}

int main(int argc, char *argv[]) {
    char fname[256];
    int cursorLine = 0;
    int ch;
    char input[MAX_CHARS];
    
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }
    
    strncpy(fname, argv[1], sizeof(fname) - 1);
    fname[sizeof(fname) - 1] = '\0';
    
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    
    if (edit(fname) < 0) {
        endwin();
        return 1;
    }
    
    while (1) {
        clear();
        int cur = inuseHead;
        int line = 0;
        while (cur != -1) {
            if (line == cursorLine) {
                attron(A_REVERSE);
                mvprintw(line, 0, "%s", txtBuff[cur].statement);
                attroff(A_REVERSE);
            } else {
                mvprintw(line, 0, "%s", txtBuff[cur].statement);
            }
            cur = txtBuff[cur].next;
            line++;
        }
        if (cursorLine == line) {
            move(cursorLine, 0);
            attron(A_REVERSE);
            printw("");
            attroff(A_REVERSE);
        }
        move(cursorLine, 0);
        refresh();
        
        ch = getch();
        if (ch == 'Q' || ch == 'q') {
            endwin();
            return 0;
        } else if (ch == 'P' || ch == 'p') {
            displayOnly();
        } else if (ch == 'S' || ch == 's') {
            save(fname);
        } else if (ch == 'U' || ch == 'u') {
            undo();
        } else if (ch == 'R' || ch == 'r') {
            redo();
        } else if (ch == 'I' || ch == 'i') {
            echo();
            move(cursorLine, 0);
            clrtoeol();
            getnstr(input, MAX_CHARS - 1);
            noecho();
            insert(cursorLine, input);
        } else if (ch == 'D' || ch == 'd') {
            delete(cursorLine);
            int lineCount = getLineCount();
            if (cursorLine >= lineCount && lineCount > 0) {
                cursorLine = lineCount - 1;
            }
        } else if (ch == KEY_UP) {
            if (cursorLine > 0) cursorLine--;
        } else if (ch == KEY_DOWN) {
            int lineCount = getLineCount();
            if (cursorLine < lineCount) cursorLine++;
        }
    }
    endwin();
    return 0;
} 