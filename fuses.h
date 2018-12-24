/*
 * ventana.c
 *
 * Created on: 9 окт. 2018 г.
 *      Author: Borislav Malkov
 */

FUSES = 
{
    .low = FUSE_SUT0 & FUSE_CKSEL3 & FUSE_CKSEL1 & FUSE_CKSEL0,
    .high = HFUSE_DEFAULT,
};
