#ifndef _FRIENDLIST_H_
#define _FRIENDLIST_H_

#include <avr/io.h>

#define FORGET_FRIEND_TIME 8 // Deprecated

#define ACTIVATE_TRUST      7
#define DEACTIVATE_TRUST    7
#define MAX_TRUST           10
#define TRUST_ADDER         2   
#define TRUST_SUBTRACTOR    1

// 256 ID's - 2 standard ID's (0x00 and 0xff) - 1 (your own ID) = 253
#define MAX_FRIENDS 253

//* Define neighbor datatype
//* Holds al the necessairy information about a friend
typedef struct {
    uint8_t id;
    uint8_t hops;
    uint8_t via;
    
    uint8_t trust : 6;
    uint8_t active : 1;
    uint8_t deactivated : 1;
        
    uint8_t lastPingTime;
} friend_t;



// TODO: Memory optimization possibilities:
// - Make active and trust a single byte (implemented by myself instead of the compiler, which should save some progmem)
// - Make lastPingTime a byte somehow (very possible).


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


// Lower the trust of all the friends with 1
// Remove friends if their trust == 0
// Deactivate friends if they are not trustworthy
void friendTimeTick(void);

friend_t *getFriendsList(void);

// Get a list of all ping-worthy friends.
// This includes active friends, as well as friends that are known via an active friend.
// Adds a friend with an ID of 0 to the end of the array as a terminator.
// buf has to be large enough to store the current amount of friends.
//
// Also sets `deactivated` of every friend to 0.
void getFriends(friend_t *buf);

// Return a pointer to the requested friend id. Returns Null incase friend doesn't exist
friend_t *findFriend(uint8_t id);


// Get the amount of friends we currently have.
// Includes friends that are inactive and don't have an active connection.
uint8_t getFriendAmount(void);

// Removes all references to the friend with the specified ID.
// If a friend was only known through this friend, it will be deleted.
void removeViaReferences(uint8_t id);

// Removes all friends from the given `vias` array,
// given that they are known through `from`.
void removeVias(uint8_t from, uint8_t *vias, uint8_t viaAmount);  


#endif // _FRIENDLIST_H_
