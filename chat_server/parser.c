#include "include/parser.h"
#include "include/data_structures.h"


void parse_packet(char *buffer, int nread, parsing_res_t *res){
// Purpose: Parse the packet and store the position of the keys
// and the values in their containers

char *start = buffer;
int param_count = 0;
int key_count = 0;
int val_count = 0;

for (int i = 0; i < nread; i++){
    
    if (buffer[i] == '|'){
        param_count++;
        buffer[i] = '\0';
        

        if (param_count % 2 == 0) {
            res->value_positions[val_count] = start;
            val_count++;
        }
        else{
            res->key_positions[key_count] = start;
            key_count++;
        }
        start = &buffer[i + 1];
    }
   
}
 if (param_count % 2 == 0) {
    res->key_positions[key_count++] = start;
} else {
    res->value_positions[val_count++] = start;
}
    
}