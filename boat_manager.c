#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_BOATS 120
#define MAX_NAME 128
#define MAX_LINE 256

// ------------------ ENUMS & DEFINES ------------------

typedef enum { SLIP, LAND, TRAILOR, STORAGE, INVALID } BoatType;

const char *typeNames[] = {"slip", "land", "trailor", "storage"};
const double TYPE_FEES[] = {12.50, 14.00, 25.00, 11.20};

// ------------------ UNION & STRUCT ------------------

typedef union {
    int slipNum;
    char bayLetter;
    char license[20];
    int spaceNum;
} BoatDetail;

typedef struct {
    char name[MAX_NAME];
    int length;
    BoatType type;
    BoatDetail detail;
    double owed;
} Boat;

// ------------------ GLOBALS ------------------

Boat *boats[MAX_BOATS];
int boatCount = 0;

// ------------------ UTILS ------------------

void trim_newline(char *str) {
    str[strcspn(str, "\n")] = '\0';
}

int strcasecmp_(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower(*a) != tolower(*b)) return tolower(*a) - tolower(*b);
        a++; b++;
    }
    return *a - *b;
}

BoatType getTypeFromString(char *s) {
    for (int i = 0; i < 4; i++)
        if (strcasecmp_(s, typeNames[i]) == 0)
            return (BoatType)i;
    return INVALID;
}

// ------------------ CSV I/O ------------------

void loadCSV(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Could not open file");
        exit(1);
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        Boat *b = malloc(sizeof(Boat));
        char typeStr[20], extra[32];
        trim_newline(line);

        int fields = sscanf(line, "%[^,],%d,%[^,],%[^,],%lf",
            b->name, &b->length, typeStr, extra, &b->owed);

        if (fields != 5) continue;

        b->type = getTypeFromString(typeStr);
        if (b->type == INVALID) { free(b); continue; }

        switch (b->type) {
            case SLIP:    b->detail.slipNum  = atoi(extra); break;
            case LAND:    b->detail.bayLetter= extra[0];     break;
            case TRAILOR: strncpy(b->detail.license, extra, 19); break;
            case STORAGE: b->detail.spaceNum = atoi(extra); break;
            default: break;
        }

        boats[boatCount++] = b;
    }

    fclose(fp);
}

void saveCSV(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Could not write file");
        return;
    }

    for (int i = 0; i < boatCount; i++) {
        Boat *b = boats[i];
        fprintf(fp, "%s,%d,%s,", b->name, b->length, typeNames[b->type]);
        switch (b->type) {
            case SLIP:    fprintf(fp, "%d", b->detail.slipNum); break;
            case LAND:    fprintf(fp, "%c", b->detail.bayLetter); break;
            case TRAILOR: fprintf(fp, "%s", b->detail.license); break;
            case STORAGE: fprintf(fp, "%d", b->detail.spaceNum); break;
            default: break;
        }
        fprintf(fp, ",%.2f\n", b->owed);
    }

    fclose(fp);
}

// ------------------ SORT & SEARCH ------------------

int compareBoats(const void *a, const void *b) {
    Boat *ba = *(Boat **)a;
    Boat *bb = *(Boat **)b;
    return strcasecmp_(ba->name, bb->name);
}

int findBoat(char *name) {
    for (int i = 0; i < boatCount; i++) {
        if (strcasecmp_(boats[i]->name, name) == 0)
            return i;
    }
    return -1;
}

// ------------------ MENU ACTIONS ------------------

void printBoats() {
    qsort(boats, boatCount, sizeof(Boat*), compareBoats);
    for (int i = 0; i < boatCount; i++) {
        Boat *b = boats[i];
        printf("%-15s %3d'  %-8s ", b->name, b->length, typeNames[b->type]);
        switch (b->type) {
            case SLIP:    printf("# %d", b->detail.slipNum); break;
            case LAND:    printf("Bay %c", b->detail.bayLetter); break;
            case TRAILOR: printf("Plate %s", b->detail.license); break;
            case STORAGE: printf("Space %d", b->detail.spaceNum); break;
            default: break;
        }
        printf("  Owes $%.2f\n", b->owed);
    }
}

void addBoatCSVLine(char *csv) {
    if (boatCount >= MAX_BOATS) return;

    Boat *b = malloc(sizeof(Boat));
    char typeStr[20], extra[32];
    trim_newline(csv);

    if (sscanf(csv, "%[^,],%d,%[^,],%[^,],%lf", b->name, &b->length, typeStr, extra, &b->owed) != 5) {
        printf("Invalid CSV line.\n");
        free(b); return;
    }

    b->type = getTypeFromString(typeStr);
    if (b->type == INVALID) { printf("Invalid type.\n"); free(b); return; 
}

    switch (b->type) {
        case SLIP:    b->detail.slipNum  = atoi(extra); break;
        case LAND:    b->detail.bayLetter= extra[0];     break;
        case TRAILOR: strncpy(b->detail.license, extra, 19); break;
        case STORAGE: b->detail.spaceNum = atoi(extra); break;
        default: break;
    }

    boats[boatCount++] = b;
}

void removeBoat(char *name) {
    int idx = findBoat(name);
    if (idx == -1) {
        printf("No boat with that name\n");
        return;
    }
    free(boats[idx]);
    for (int i = idx; i < boatCount - 1; i++)
        boats[i] = boats[i + 1];
    boatCount--;
}

void acceptPayment(char *name, double amt) {
    int idx = findBoat(name);
    if (idx == -1) {
        printf("Boat not found\n");
        return;
    }
    if (amt > boats[idx]->owed) {
        printf("That is more than the amount owed, $%.2f\n", boats[idx]->owed);
        return;
    }
    boats[idx]->owed -= amt;
}

void monthlyCharge() {
    for (int i = 0; i < boatCount; i++) {
        boats[i]->owed += TYPE_FEES[boats[i]->type] * boats[i]->length;
    }
}

// ------------------ MAIN MENU ------------------

void menu(const char *filename) {
    char cmd[16], input[MAX_LINE], name[MAX_NAME];
    double amt;

    printf("Welcome to the Boat Management System\n");

    while (1) {
        printf("\n(I)nventory, (A)dd, (R)emove, (P)ayment, (M)onth, e(X)it : ");
        fgets(cmd, sizeof(cmd), stdin);
        char choice = tolower(cmd[0]);

        switch (choice) {
            case 'i': printBoats(); break;
            case 'a':
                printf("Enter CSV boat string: ");
                fgets(input, sizeof(input), stdin);
                addBoatCSVLine(input);
                break;
            case 'r':
                printf("Enter boat name to remove: ");
                fgets(name, sizeof(name), stdin);
                trim_newline(name);
                removeBoat(name);
                break;
            case 'p':
                printf("Boat name: ");
                fgets(name, sizeof(name), stdin);
                trim_newline(name);
                printf("Amount: ");
                fgets(input, sizeof(input), stdin);
                amt = atof(input);
                acceptPayment(name, amt);
                break;
            case 'm':
                monthlyCharge();
                break;
            case 'x':
                saveCSV(filename);
                printf("Exiting and saving to %s\n", filename);
                return;
            default:
                printf("Invalid option\n");
        }
    }
}

// ------------------ MAIN ------------------

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s BoatData.csv\n", argv[0]);
        return 1;
    }

    loadCSV(argv[1]);
    menu(argv[1]);

    for (int i = 0; i < boatCount; i++)
        free(boats[i]);

    return 0;
}

