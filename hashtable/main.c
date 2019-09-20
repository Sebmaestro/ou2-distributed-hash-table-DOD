#include <stdio.h>
#include "hash.h"
#include "hash_table.h"

int main(int argc, char** argv) {
    struct hash_table* table = table_create(hash_ssn, 255); 
    table_insert(table, "9708143344", "Test", "Test testsson");
    
    table_free(table);
}
