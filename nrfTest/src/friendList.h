#ifndef _FRIENDLIST_H_
#define _FRIENDLIST_H_

#include <avr/io.h>

#define FORGET_FRIEND_TIME 4

// Define neighbor datatype
typedef struct {
    uint8_t id;
    uint8_t remainingTime;
    uint8_t hops;
    uint8_t via;
} friend_t;

void initFriendList(void);
uint8_t addFriend(friend_t friend);
void printFriends(void);
void friendTimeTick(void);
void printDebugFriends(void);


#endif // _FRIENDLIST_H_