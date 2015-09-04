#ifndef _CHANNEL_H
#define _CHANNEL_H

/* Internal headers */
#include "User_list.h"

/* Defines */
#define MAX_CHANNEL_NAME_SIZE 20

/* Typedefs */
typedef struct channel *Channel;

/* Structs */
struct channel {
    char      name[MAX_CHANNEL_NAME_SIZE];
    User_list users;
}; 

#endif /* _CHANNEL_H */