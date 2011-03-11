/*********************************************************************
 *                
 * Copyright (C) 2008, Jan Stoess, IBM Corporation
 *                
 * File path:     access.h
 * Description:   
 *                
 * File path:       access.h
 * Description:     Basic access control for BG tree and torus
 *                
 * $Id:$
 *                
 ********************************************************************/

#include "tree.h"
#include "torus.h"

class comm_control_t : public tree_packet_t, torus_dmareg_t
{
private:
    typedef word_t torus_passmask_t[NUM_COORD][NUM_COORD][NUM_COORD/BITS_WORD];
    typedef word_t tree_passmask_t[NUM_DST_KEYS/BITS_WORD];

    torus_passmask_t torus_passmask;
    tree_passmask_t tree_passmask;

public:
    bool torus_pass(coordinates_t &c)
        { return torus_passmask[c[XAX]][c[YAX]][c[ZAX] / BITS_WORD] & (1ul << (c[ZAX] % BITS_WORD)); }
    
    bool torus_desc_pass(inj_desc_t *desc) 
        { return torus_pass(desc->fifo.get_dest()) && (desc->fifo.dirhint == 0) ? true :
                ((desc->fifo.dirhint & 0x15) & ((desc->fifo.dirhint >> 1) & 0x15) == 0); }
    
    bool tree_pass(word_t k)
        { assert(k < NUM_DST_KEYS/BITS_WORD); return tree_passmask[k / BITS_WORD] & (1ul << (k  % BITS_WORD)); } 
                                             
    comm_control_t() {
        for (word_t x=0; x < NUM_COORD; x++)
            for (word_t y=0; y < NUM_COORD; y++)
                for (word_t z=0; z < NUM_COORD/BITS_WORD; z++)
                    torus_passmask[x][y][z] = ~0ul;
        for (word_t v=0; v < NUM_DST_KEYS/BITS_WORD; v++)
            tree_passmask[v] = ~0ul;
    }

};
