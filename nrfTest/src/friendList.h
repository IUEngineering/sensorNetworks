#ifndef _FRIENDLIST_H_
#define _FRIENDLIST_H_

#include <avr/io.h>

#define FORGET_FRIEND_TIME 8

// 256 ID's - 2 standard ID's (0x00 and 0xff) - 1 (your own ID) = 253
#define MAX_FRIENDS 253

//* Define neighbor datatype
//* Holds al the necessairy information about a friend
typedef struct {
    uint8_t id;
    uint8_t hops;
    uint8_t via;
    uint8_t trust;
    uint8_t active;
    uint16_t lastPingTime;
}friend_t;


//* Initialise the friendlist
void initFriendList(void);


//* Add a friend to the friendlist or increase our trust in it.
//* id:    The friend id
//* hops:  The number of nodes between this node and the friend node
//* via:   The next node to reach the friend node
//*
//* Returns the updated friend.
friend_t *updateFriend(uint8_t id, uint8_t hops, uint8_t via);


//* Print a list of all friends to the terminal (serialF0)
void printFriends(void);


//* Lower the trust of all the friends with 1
//* Remove friends if their trust == 0
//* Deactivate friends if they are not trustworthy
void friendTimeTick(void);

friend_t *getFriendsList(uint8_t *listLength);

//* Get a list of all ping-worthy friends.
//* This includes active friends, as well as friends that are known via an active friend.
//* Adds a friend with an ID of 0 to the end of the array as a terminator.
void getFriends(friend_t *buf);

//* Return a pointer to the requested friend id. Returns Null incase friend doesn't exist
friend_t *findFriend(uint8_t id);


// Get the amount of friends we currently have.
// Includes friends that are inactive and don't have an active connection.
uint8_t getFriendAmount(void);

// Removes all references to the friend with the specified ID.
// If a friend was only known through this friend, it will be deleted.
void removeViaReferences(uint8_t id);


#endif // _FRIENDLIST_H_
