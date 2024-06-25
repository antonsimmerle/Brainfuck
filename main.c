#include <stdio.h>
#include <stdlib.h>
#define CHUNK 10

char* readentirefile(FILE* file);

typedef enum OPERATORKINDS OPERATORKINDS;
typedef struct OPERATOR OPERATOR;
typedef struct OPERATORS OPERATORS;

typedef struct NODE NODE;
typedef struct STACK STACK;
void push(STACK* stack, size_t data);
size_t pop(STACK* stack);
OPERATORS* intermediate(char* input);

typedef struct TAPE TAPE;
void interpreter(OPERATORS* operators);

int validoperator(char input);
char* preprocessor(char* input);

int main(int argc, char** argv);

char* readentirefile(FILE* file) {
    char* buffer = NULL;
    size_t length;
    ssize_t bytesread = getdelim(&buffer, &length, '\0', file);
    return buffer;
}

enum OPERATORKINDS {
    LEFT          = '<',
    RIGHT         = '>',
    INCREMENT     = '+',
    DECREMENT     = '-',
    OUTPUT        = '.',
    INPUT         = ',',
    JUMPIFZERO    = '[',
    JUMPIFNOTZERO = ']',
};
struct OPERATOR {
    size_t address;
    OPERATORKINDS kind;
    size_t property;
};
struct OPERATORS {
    OPERATOR* items;
    size_t count;
    size_t capacity;
};

struct NODE {
    size_t data;
    struct NODE* next; 
};
struct STACK {
    NODE* items;
    size_t count;
};
void push(STACK* stack, size_t data) {
    stack->count++;
    NODE* element = (NODE*)malloc(sizeof(NODE));
    element->data = data;
    element->next = stack->items;
    stack->items = element;
}
size_t pop(STACK* stack) {
    stack->count--;
    NODE* element = stack->items;
    if (element) {
        stack->items = element->next;
    }
    return element->data;
}
OPERATORS* intermediate(char* input) {
    OPERATORS* operators = (OPERATORS*)malloc(sizeof(OPERATORS));
    STACK stack = { .count = 0, .items = NULL, };
    for (size_t i = 0, j = 0; input[i] != '\0'; i++, j++) {
        operators->count++;
        if (j >= operators->capacity) {
            operators->capacity += CHUNK;
            operators->items = realloc(operators->items, sizeof(OPERATOR) * operators->capacity);
        }
        operators->items[j].address = j;
        operators->items[j].kind = (OPERATORKINDS)input[i];
        switch (operators->items[j].kind) {
            case INCREMENT:
            case DECREMENT:
            case LEFT:
            case RIGHT:
            case INPUT:
            case OUTPUT: {
                size_t count = 1;
                for (; input[i + 1] == (char)operators->items[j].kind; i++, count++);
                operators->items[j].property = count;
            } break;
            case JUMPIFZERO: {
                push(&stack, operators->items[j].address);
            } break;
            case JUMPIFNOTZERO: {
                if (stack.count == 0) {
                    fprintf(stderr, "Error: Unmatched ']'\n");
                    exit(EXIT_FAILURE);
                }
                size_t address = pop(&stack);
                operators->items[address].property = operators->count;
                operators->items[j].property = address + 1;
            } break;
        }
    }
    free(stack.items);
    return operators;
}

struct TAPE {
    char* items;
    size_t capacity;
};
void interpreter(OPERATORS* operators) {
    TAPE tape = { .capacity = CHUNK, .items = (char*)malloc(sizeof(char) * tape.capacity), };
    size_t i = 0;
    size_t pointer = 0;
    while (i < operators->count) {
        switch (operators->items[i].kind) {
            case INCREMENT: {
                tape.items[pointer] += operators->items[i].property;
                i++;
            } break;
            case DECREMENT: {
                tape.items[pointer] -= operators->items[i].property;
                i++;
            } break;
            case LEFT: {
                if (pointer < operators->items[i].property) {
                    fprintf(stderr, "Error: Memory underflow\n");
                    exit(EXIT_FAILURE);
                }
                pointer -= operators->items[i].property;
                i++;
            } break;
            case RIGHT: {
                pointer += operators->items[i].property;
                while (pointer >= tape.capacity) {
                    tape.capacity += CHUNK;
                    tape.items = realloc(tape.items, sizeof(char) * tape.capacity);
                }
                i++;
            } break;
            case INPUT: {
                for (size_t j = 0; j < operators->items[i].property; j++) {
                    scanf("%c", &tape.items[pointer]);
                    int c;
                    while ((c = getchar()) != '\n' && c != EOF);
                }
                i++;
            } break;
            case OUTPUT: {
                for (size_t j = 0; j < operators->items[i].property; j++) {
                    printf("%c", tape.items[pointer]);
                }
                i++;
            } break;
            case JUMPIFZERO: {
                i = tape.items[pointer] == 0 ? operators->items[i].property : i + 1;
            } break;
            case JUMPIFNOTZERO: {
                i = tape.items[pointer] != 0 ? operators->items[i].property : i + 1;
            } break;
        }
    }
    free(tape.items);
}

int validoperator(char input) {
    switch (input) {
        case INCREMENT:
        case DECREMENT:
        case LEFT:
        case RIGHT:
        case INPUT:
        case OUTPUT:
        case JUMPIFZERO:
        case JUMPIFNOTZERO: {
            return 1;
        }
        default: {
            return 0;
        }
    }
}
char* preprocessor(char* input) {
    size_t length = 0;
    for (; input[length] != '\0'; length++);
    char* output = (char*)malloc(sizeof(char) * length);
    size_t j = 0;
    for (size_t i = 0; input[i] != '\0'; i++) {
        if (validoperator(input[i])) {
            output[j] = input[i];
            j++;
        }
    }
    output[j] = '\0';
    return output;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input.bf>\n", argv[0]);
        return EXIT_FAILURE;
    }
    FILE* inputfile = fopen(argv[1], "r");
    if (!inputfile) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }
    char* content = readentirefile(inputfile);
    OPERATORS* operators = intermediate(preprocessor(content));
    free(content);
    interpreter(operators);
    free(operators->items);
    free(operators);
    return EXIT_SUCCESS;
}
