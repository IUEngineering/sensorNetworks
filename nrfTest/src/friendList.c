#include <stdlib.h>
#include <string.h>

#include "friendList.h"
// #include "serialF0.h"
#include "terminal.h"

#define ACTIVATE_TRUST      7
#define DEACTIVATE_TRUST    7
#define MAX_TRUST           10
#define TRUST_ADDER         2   
#define TRUST_SUBTRACTOR    1



uint8_t friendAmount = 0;
friend_t friends[MAX_FRIENDS];

static void removeFriend(friend_t *friend);
static friend_t *newFriend(uint8_t id, uint8_t hops, uint8_t via);


void initFriendList() {
    memset(friends, 0, MAX_FRIENDS * sizeof(friend_t));
}


friend_t *updateFriend(uint8_t id, uint8_t hops, uint8_t via) {
    DEBUG_PRINTF("\e[0;33mUpdating friend: id: 0x%02x\t hops:%2d\tvia: 0x%02x\n", id, hops, via);

    // Slight safety check so the system doesn't completely brick when another team's node is sending bs.
    if(id == 0) return NULL;

    // Check if we already know this friend.
    friend_t *oldFriend = findFriend(id);

    // If we don't, create a new one.
    if(oldFriend == NULL) {
        DEBUG_PRINT("\tGenerating new friend.\e[0m");
        return newFriend(id, hops, via);
    }
    
    // If it's a direct connection:
    if(hops == 0) {
        oldFriend->trust += TRUST_ADDER;

        // Make sure the friend doesn't get too trusted (we have trust issues).
        if(oldFriend->trust > MAX_TRUST) oldFriend->trust = MAX_TRUST;

        // Activate the friend when we trust it enough.
        if(oldFriend->trust > ACTIVATE_TRUST) oldFriend->active = 1;

        DEBUG_PRINTF("\tIt's direct. Increased trust: %2d\tActive: %1d\e[0m", oldFriend->trust, oldFriend->active);
    }

    // Replace old friend if it has more hops.
    else if(hops < oldFriend->hops || oldFriend->hops == 0) {
        oldFriend->via = via;
        oldFriend->hops = hops;
        DEBUG_PRINT("\tNot direct. Replacing hops and via.\e[0m\n");
    }

    else DEBUG_PRINT("\tNot direct, not replacing hops.\e[0m\n");

    return oldFriend;
}

friend_t *newFriend(uint8_t id, uint8_t hops, uint8_t via) {
    uint8_t friendsIndex = 0;
    
    while(friends[friendsIndex].id) friendsIndex++;


    friends[friendsIndex].id = id;
    friends[friendsIndex].via = via;
    friends[friendsIndex].hops = hops;
    friends[friendsIndex].active = 0;
    friendAmount++;

    // Make sure all directly connected friends get a trust boost.
    if(hops == 0) friends[friendsIndex].trust = TRUST_ADDER;
    else friends[friendsIndex].trust = 0;

    return friends + friendsIndex;
}

void removeFriend(friend_t *friend) {
    friendAmount -= (friend->id != 0);
    friend->id = 0;
}

void printFriends() {
    if(friendAmount == 0) {
        printf("I don't have any friends :(\n");
        return;
    }

    // Gotta have the dynamic plural.
    printf("My %d friend%s :)\n", friendAmount, friendAmount > 1 ? "s" : "");
    printf("\t\e[0;31mID\tTrust\tActive\tHops\tVia\tTime\e[0m\n");

    // Pretty for loop (｡◕‿‿◕｡)
    for(uint8_t i = 0, friend = 0; friend < friendAmount; friend += !!friends[i].id, i++) {
        if(friends[i].id != 0) {
            printf("%3d\t\e[0;35m0x%02x\e[0m\t%2d\t%1d\t%02d\t0x%02x\t0x%02x\n", i, friends[i].id, friends[i].trust, friends[i].active, friends[i].hops, friends[i].via, friends[i].lastPingTime);
        }
    }
    printf("\n");
}

void friendTimeTick() {

    if(friendAmount == 0) return;
    
    DEBUG_MSG_START();
    DEBUG_MSG_APPEND("\e[0;35mReducing trust of friends:\t\n");

    for(uint8_t i = 0, friend = 0; friend < friendAmount; i++, friend++) {
        if(friends[i].id != 0 && friends[i].trust > 0) {
            // Reduce trust.
            friends[i].trust -= TRUST_SUBTRACTOR;
            
            DEBUG_MSG_APPENDF("0x%02x(%d)", friends[i].id, friends[i].trust);

            // Deactivate friend.
            if(friends[i].active && friends[i].trust < DEACTIVATE_TRUST) {
                friends[i].active = 0;
                DEBUG_MSG_APPEND("/DE");
                // Remove all via references to this friend.
                removeViaReferences(friends[i].id);
            }

            // Remove friend if we have no way of reaching it anymore.
            if(friends[i].trust == 0  &&  friends[i].via == 0) {
                DEBUG_MSG_APPEND("/RM");
                removeFriend(friends + i);
            }

            DEBUG_MSG_APPEND(" ");
        }
    }
    DEBUG_MSG_APPEND("\n\e[0m");
    DEBUG_MSG_PRINT();
}

void removeViaReferences(uint8_t id) {
    for(uint8_t i = 0, friend = 0; friend < friendAmount; friend += !!friends[i].id, i++) {
        if(friends[i].id != 0  &&  friends[i].via == id) {

            // Only remove the friend if we have no direct connection to it.
            if(friends[i].trust) {
                friends[i].hops = 0;
                friends[i].via = 0;
            }
            else removeFriend(friends + i);
        }
    }
}


void removeVias(uint8_t from, uint8_t *vias, uint8_t viaAmount) { 
    // It's pretty useless to run this function if no vias are specified. 
    if(viaAmount == 0) return;

    for(uint8_t i = 0, friend = 0; friend < friendAmount; friend += !!friends[i].id, i++) {
        if(friends[i].id == 0) continue;

        // Check if we know the friend via the source friend (a), and if it is one of the friends to be removed (b).
        // (a)                         (b)
        if(friends[i].via == from  &&  memchr(vias, friends[i].id, viaAmount)) {
            
            // Only remove the friend if we have no direct connection to it.
            if(friends[i].trust) {
                friends[i].hops = 0;
                friends[i].via = 0;
            }
            else removeFriend(friends + i);
        }
    }
}


void getFriends(friend_t *buf) {
    uint8_t friend = 0;
    for(uint8_t i = 0; friend < friendAmount; i++) {

        // If the friend is not 0, and either active or known via a different friend:
        if(friends[i].id != 0) {
            // Copy the friend into the buffer if it's ping worthy.
            if(friends[i].active || friends[i].via) 
                *buf++ = friends[i];
                
            friend++;
        }
    }
    buf->id = 0;
}


friend_t *getFriendsList(void) {
    return friends;
}

uint8_t getFriendAmount(void) {
    return friendAmount;
}


// Find a friend, and return a pointer to that friend.
friend_t *findFriend(uint8_t id) {
    // ID 0x00 is invalid.   
    if(id == 0x00) return NULL;

    for(uint8_t i = 0, friend = 0; friend < friendAmount; friend += !!friends[i].id, i++) {
        if(friends[i].id == id) return friends + i;
    }

    // If the friend was not found, return NULL :(
    return NULL;
}


