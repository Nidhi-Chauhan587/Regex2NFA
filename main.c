#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LEN 256
#define MAX_STATES 1024
#define MAX_TRANSITIONS 2048
#define MAX_STACK 512

#define EPSILON 'e'
#define CONCAT '.'

typedef struct {
    int id;
    int is_start;
    int is_final;
} State;

typedef struct {
    int from;
    int to;
    char symbol;
} Transition;

typedef struct {
    int start;
    int end;
} Fragment;

State states[MAX_STATES];
Transition transitions[MAX_TRANSITIONS];
int state_count = 0;
int transition_count = 0;

int is_operand(char c) {
    return isalnum((unsigned char)c);
}

int is_operator(char c) {
    return c == '|' || c == '*' || c == CONCAT;
}

int create_state(void) {
    if (state_count >= MAX_STATES) {
        printf("Error: Maximum state limit exceeded.\n");
        exit(EXIT_FAILURE);
    }

    int id = state_count;
    states[id].id = id;
    states[id].is_start = 0;
    states[id].is_final = 0;
    state_count++;

    return id;
}

void add_transition(int from, int to, char symbol) {
    if (transition_count >= MAX_TRANSITIONS) {
        printf("Error: Maximum transition limit exceeded.\n");
        exit(EXIT_FAILURE);
    }

    transitions[transition_count].from = from;
    transitions[transition_count].to = to;
    transitions[transition_count].symbol = symbol;
    transition_count++;
}

int validate_expression(const char *regex) {
    int balance = 0;
    int expect_operand = 1;
    size_t length = strlen(regex);

    if (length == 0 || length >= MAX_LEN) {
        printf("Error: Expression is empty or too long.\n");
        return 0;
    }

    for (size_t i = 0; i < length; i++) {
        char c = regex[i];

        if (isspace((unsigned char)c)) {
            printf("Error: Spaces are not supported.\n");
            return 0;
        }

        if (is_operand(c)) {
            expect_operand = 0;
        } else if (c == '(') {
            balance++;
            expect_operand = 1;
        } else if (c == ')') {
            if (balance == 0 || expect_operand) {
                printf("Error: Invalid closing parenthesis.\n");
                return 0;
            }
            balance--;
            expect_operand = 0;
        } else if (c == '*') {
            if (expect_operand) {
                printf("Error: '*' must follow an operand or ')'.\n");
                return 0;
            }
            expect_operand = 0;
        } else if (c == '|') {
            if (expect_operand) {
                printf("Error: Invalid placement of '|'.\n");
                return 0;
            }
            expect_operand = 1;
        } else {
            printf("Error: Unsupported symbol '%c'.\n", c);
            return 0;
        }
    }

    if (balance != 0) {
        printf("Error: Unbalanced parentheses.\n");
        return 0;
    }

    if (expect_operand) {
        printf("Error: Expression ends with an operator.\n");
        return 0;
    }

    return 1;
}

int ends_expression(char c) {
    return is_operand(c) || c == ')' || c == '*';
}

int starts_expression(char c) {
    return is_operand(c) || c == '(';
}

int insert_concatenation(
    const char *input, char *output, size_t output_size
) {
    size_t j = 0;
    size_t length = strlen(input);

    for (size_t i = 0; i < length; i++) {
        if (j + 1 >= output_size) return 0;
        output[j++] = input[i];

        if (i + 1 < length &&
            ends_expression(input[i]) &&
            starts_expression(input[i + 1])) {
            if (j + 1 >= output_size) return 0;
            output[j++] = CONCAT;
        }
    }

    output[j] = '\0';
    return 1;
}

int precedence(char op) {
    if (op == '*') return 3;
    if (op == CONCAT) return 2;
    if (op == '|') return 1;
    return 0;
}

int infix_to_postfix(
    const char *infix, char *postfix, size_t output_size
) {
    char stack[MAX_STACK];
    int top = -1;
    size_t j = 0;

    for (size_t i = 0; infix[i] != '\0'; i++) {
        char c = infix[i];

        if (is_operand(c)) {
            if (j + 1 >= output_size) return 0;
            postfix[j++] = c;
        } else if (c == '(') {
            if (top + 1 >= MAX_STACK) return 0;
            stack[++top] = c;
        } else if (c == ')') {
            while (top >= 0 && stack[top] != '(') {
                if (j + 1 >= output_size) return 0;
                postfix[j++] = stack[top--];
            }

            if (top < 0) return 0;
            top--;
        } else if (is_operator(c)) {
            while (top >= 0 && stack[top] != '(' &&
                   precedence(stack[top]) >= precedence(c)) {
                if (j + 1 >= output_size) return 0;
                postfix[j++] = stack[top--];
            }

            if (top + 1 >= MAX_STACK) return 0;
            stack[++top] = c;
        } else {
            return 0;
        }
    }

    while (top >= 0) {
        if (stack[top] == '(' || j + 1 >= output_size)
            return 0;

        postfix[j++] = stack[top--];
    }

    postfix[j] = '\0';
    return 1;
}

Fragment create_basic_fragment(char symbol) {
    Fragment f = {create_state(), create_state()};
    add_transition(f.start, f.end, symbol);
    return f;
}

Fragment concatenate_fragments(Fragment first, Fragment second) {
    add_transition(first.end, second.start, EPSILON);

    Fragment result = {first.start, second.end};
    return result;
}

Fragment union_fragments(Fragment first, Fragment second) {
    Fragment result = {create_state(), create_state()};

    add_transition(result.start, first.start, EPSILON);
    add_transition(result.start, second.start, EPSILON);
    add_transition(first.end, result.end, EPSILON);
    add_transition(second.end, result.end, EPSILON);

    return result;
}

Fragment kleene_star(Fragment f) {
    Fragment result = {create_state(), create_state()};

    add_transition(result.start, f.start, EPSILON);
    add_transition(result.start, result.end, EPSILON);
    add_transition(f.end, f.start, EPSILON);
    add_transition(f.end, result.end, EPSILON);

    return result;
}

int construct_nfa(const char *postfix, Fragment *result) {
    Fragment stack[MAX_STACK];
    int top = -1;

    for (size_t i = 0; postfix[i] != '\0'; i++) {
        char c = postfix[i];

        if (is_operand(c)) {
            if (top + 1 >= MAX_STACK) return 0;
            stack[++top] = create_basic_fragment(c);
        } else if (c == CONCAT) {
            if (top < 1) return 0;

            Fragment second = stack[top--];
            Fragment first = stack[top--];

            stack[++top] =
                concatenate_fragments(first, second);
        } else if (c == '|') {
            if (top < 1) return 0;

            Fragment second = stack[top--];
            Fragment first = stack[top--];

            stack[++top] = union_fragments(first, second);
        } else if (c == '*') {
            if (top < 0) return 0;

            Fragment f = stack[top--];
            stack[++top] = kleene_star(f);
        } else {
            return 0;
        }
    }

    if (top != 0) return 0;

    *result = stack[top];
    states[result->start].is_start = 1;
    states[result->end].is_final = 1;

    return 1;
}

void display_nfa(Fragment result) {
    printf("\n========== GENERATED NFA ==========\n");
    printf("Total states: %d\n", state_count);
    printf("Total transitions: %d\n", transition_count);
    printf("Start state: q%d\n", result.start);
    printf("Accepting state: q%d\n", result.end);

    printf("\nStates:\n");

    for (int i = 0; i < state_count; i++) {
        printf("q%d", states[i].id);

        if (states[i].is_start) printf(" [START]");
        if (states[i].is_final) printf(" [FINAL]");

        printf("\n");
    }

    printf("\nTransition Table:\n");
    printf("%-15s %-15s %-15s\n",
           "From State", "Input", "To State");
    printf("---------------------------------------------\n");

    for (int i = 0; i < transition_count; i++) {
        char symbol_text[16];

        if (transitions[i].symbol == EPSILON) {
            strcpy(symbol_text, "epsilon");
        } else {
            symbol_text[0] = transitions[i].symbol;
            symbol_text[1] = '\0';
        }

        printf("q%-14d %-15s q%-14d\n",
               transitions[i].from,
               symbol_text,
               transitions[i].to);
    }
}

int main(void) {
    char regex[MAX_LEN];
    char explicit_regex[MAX_LEN * 2];
    char postfix[MAX_LEN * 2];
    Fragment result;

    printf("========================================\n");
    printf(" REGEX TO NFA - THOMPSON'S CONSTRUCTION\n");
    printf(" DAA AND COMPILER DESIGN MINI PROJECT\n");
    printf("========================================\n");

    printf("Enter regular expression: ");

    if (scanf("%255s", regex) != 1) {
        printf("Error: Unable to read expression.\n");
        return EXIT_FAILURE;
    }

    if (!validate_expression(regex))
        return EXIT_FAILURE;

    if (!insert_concatenation(
            regex, explicit_regex, sizeof explicit_regex)) {
        printf("Error: Expression is too long after processing.\n");
        return EXIT_FAILURE;
    }

    printf("\nExpression with explicit concatenation: %s\n",
           explicit_regex);

    if (!infix_to_postfix(
            explicit_regex, postfix, sizeof postfix)) {
        printf("Error: Unable to convert expression to postfix.\n");
        return EXIT_FAILURE;
    }

    printf("Postfix expression: %s\n", postfix);

    if (!construct_nfa(postfix, &result)) {
        printf("Error: NFA construction failed.\n");
        return EXIT_FAILURE;
    }

    display_nfa(result);

    return EXIT_SUCCESS;
}
