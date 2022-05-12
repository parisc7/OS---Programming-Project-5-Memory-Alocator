#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAME_LENGTH_LIMIT 128
#define OPERATOR_LENGTH_LIMIT 8


typedef struct MemmoryBlock {
    size_t low, high;
    char *name;
    struct MemmoryBlock *prev, *next;
} MemmoryBlock;

size_t MainMemorySize = 0;
MemmoryBlock *MainMemory;


MemmoryBlock *make_block(size_t currentlow, size_t currentHigh, const char *name, MemmoryBlock *prev, MemmoryBlock *next) {
    MemmoryBlock *temp = malloc(sizeof(MemmoryBlock));
    if(temp == NULL) {
        printf("Failed to allocate physical memory.\n");
        exit(-1);
    }
    temp->low = currentlow, temp->high = currentHigh;
    // allocate memory and copy the string
    if(strlen(name) != 0) {
        temp->name = malloc(sizeof(char) * (strlen(name) + 1));
        strcpy(temp->name, name);
    } else { // unused block
        temp->name = NULL;
    }
    // handle the prev and next to preserve a doubly-linked list
    temp->prev = prev, temp->next = next;
    if(prev) {
        prev->next = temp;
    }
    if(next) {
        next->prev = temp;
    }
    return temp;
}

int request_memory(const char *name, size_t size, char Type) {
    MemmoryBlock *hole = NULL;
    // select the hole
    switch(Type) {
        case 'F': {
            hole = MainMemory;
            while(hole) {
                if(hole->name == NULL && (hole->high - hole->low + 1) >= size) {
                    break;
                }
                hole = hole->next;
            }
            break;
        }
        case 'B': {
            MemmoryBlock *cursor = MainMemory;
            size_t min_size = -1;   // get the max number in size_t
            while(cursor) {
                size_t hole_size = (cursor-> high - cursor->low + 1);
                if(cursor->name == NULL && size <= hole_size && hole_size < min_size) {
                    min_size = hole_size;
                    hole = cursor;
                }
                cursor = cursor->next;
            }
            break;
        }
        case 'W': {
            MemmoryBlock *cursor = MainMemory;
            size_t max_size = size - 1;
            while(cursor) {
                size_t hole_size = (cursor-> high - cursor->low + 1);
                if(cursor->name == NULL && hole_size > max_size) {
                    max_size = hole_size;
                    hole = cursor;
                }
                cursor = cursor->next;
            }
            break;
        }
        default: {
            printf("Unknown Type: %c\n", Type);
            return -1;
        }
    }
    if(!hole || hole->name != NULL) {
        printf("No available memory to allocate.\n");
        return -2;
    }
    hole->name = malloc(sizeof(char) * (strlen(name) + 1));
    strcpy(hole->name, name);
    if(hole->high - hole->low + 1 == size) {   // the hole size is exactly equal to the requested size
        return 0;
    }
    hole->next = make_block(hole->low + size, hole->high, "", hole, hole->next);
    hole->high = hole->low + size - 1;
    return 0;
}

// release all blocks with the given name, and do the merges if possible
int release_memory(const char *name) {
    MemmoryBlock *cursor = MainMemory;
    int flag = 1;
    while(cursor) {
        if(cursor->name && strcmp(cursor->name, name) == 0) {
            free(cursor->name);
            cursor->name = NULL;    // mark it unused
            flag = 0;
        }
        // merge with the prev block if possible
        if(cursor->name == NULL && cursor->prev && cursor->prev->name == NULL) {
            MemmoryBlock *temp = cursor->prev;
            cursor->prev = temp->prev;
            if(temp->prev) {
                temp->prev->next = cursor;
            }
            cursor->low = temp->low;
            free(temp);
        }
        // update the first block in memory if necessary
        if(cursor->prev == NULL) {
            MainMemory = cursor;
        }
        cursor = cursor->next;
    }
    if(flag) {
        printf("No memory gets released!\n");
    }
    return flag;
}

//RQ
void request_wrapper() {
    char name[NAME_LENGTH_LIMIT], type;
    size_t size;
    scanf("%s %zu %c", name, &size, &type); // unsafe but convenient
    printf(request_memory(name, size, type) ? "FAILURE\n" : "SUCCESS\n");
}
//RL
void release_wrapper() {
    char name[NAME_LENGTH_LIMIT];
    scanf("%s", name); // unsafe but convenient
    printf(release_memory(name) ? "FAILURE\n" : "SUCCESS\n");
}
//X
void clean_up() {
    MemmoryBlock *temp = MainMemory;
    while(MainMemory) {
        free(MainMemory -> name);
        temp = MainMemory;
        MainMemory = MainMemory -> next;
        free(temp);
    }
}
//STAT
void display_memory() {
   printf("--------------------------------------------------------------\n");
   MemmoryBlock *cursor = MainMemory;
    while(cursor) {
        printf("[%06zu - %06zu] ", cursor->low, cursor->high);
        if(cursor->name) {
            printf("Process %s\n", cursor->name);
        } else {
            printf("Unused\n");
        }
        cursor = cursor->next;
    }
   printf("--------------------------------------------------------------\n");
}
//C
void compact_memory() {
    MemmoryBlock *cursor = MainMemory;
    while(cursor) {
        // unused --> used, swap these two blocks
        if(cursor->name && cursor->prev && !cursor->prev->name) {
            MemmoryBlock *prev = cursor->prev;
            prev->high = prev->low + (cursor->high - cursor->low);
            cursor->low = prev->high + 1;
            prev->name = cursor->name;
            cursor->name = NULL;
        }
        // unused --> unused, merge thees two blocks
        if(!cursor->name && cursor->prev && !cursor->prev->name) {
            MemmoryBlock *prev = cursor->prev;
            cursor->low = prev->low;
            cursor->prev = prev->prev;
            if(cursor->prev) {
                cursor->prev->next = cursor;
            }
            free(prev);
        }
        cursor = cursor->next;
    }
}

void display_usage() {

    printf("--------------------------------------------------------------\n");
    printf("Operations:\n");
    printf("    RQ <process name> <memory size (in bytes)> <Type> : \n"
           "        Request for a contagious block of memory (available types of alocation are F (First-fit), W (Worst-Fit) and B (Best-Fit))\n"
           "    RL <process name> :\n"
           "        Release the process's contagious block of memory\n"
           "    C :\n"
           "        Compact unused holes of memory into one single block\n"
           "    STAT :\n"
           "        Report the regions of free and allocated memory\n"
           "    X :\n"
           "        Exit\n"
          );
    printf("--------------------------------------------------------------\n");
}


int initialize(int argc, char **argv) {
    if(argc != 2) {
        printf("Incorrect number of arguments.\n");
        return -1;
    }
    sscanf(argv[1], "%zu", &MainMemorySize);
    MainMemory = make_block(0, MainMemorySize - 1, "", NULL, NULL);
    printf("The size of memory is initialized to %zu bytes\n", MainMemorySize);
    display_usage();
    return 0;
}

int main(int argc, char **argv) {
    if(initialize(argc, argv) != 0) {
        display_usage();
        return 0;
    }
    char operation[OPERATOR_LENGTH_LIMIT];
    while(1) {
        printf("allocator> ");
        scanf("%s", operation);    // unsafe but convenient
        if(strcmp(operation, "RQ") == 0) {
            request_wrapper();
        } else if(strcmp(operation, "RL") == 0) {
            release_wrapper();
        } else if(strcmp(operation, "C") == 0) {
            compact_memory();
        } else if(strcmp(operation, "STAT") == 0) {
            display_memory();
        } else if(strcmp(operation, "X") == 0) {
            clean_up();
            break;
        } else {
            printf("Unrecognized operation.\n");
            display_usage();
        }
        // display_memory();
    }
    return 0;
}
