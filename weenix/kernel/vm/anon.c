#include "globals.h"
#include "errno.h"

#include "util/string.h"
#include "util/debug.h"

#include "mm/mmobj.h"
#include "mm/pframe.h"
#include "mm/mm.h"
#include "mm/page.h"
#include "mm/slab.h"
#include "mm/tlb.h"

int anon_count = 0; /* for debugging/verification purposes */

static slab_allocator_t *anon_allocator;

static void anon_ref(mmobj_t *o);
static void anon_put(mmobj_t *o);
static int  anon_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf);
static int  anon_fillpage(mmobj_t *o, pframe_t *pf);
static int  anon_dirtypage(mmobj_t *o, pframe_t *pf);
static int  anon_cleanpage(mmobj_t *o, pframe_t *pf);

static mmobj_ops_t anon_mmobj_ops = {
        .ref = anon_ref,
        .put = anon_put,
        .lookuppage = anon_lookuppage,
        .fillpage  = anon_fillpage,
        .dirtypage = anon_dirtypage,
        .cleanpage = anon_cleanpage
};

/*
 * This function is called at boot time to initialize the
 * anonymous page sub system. Currently it only initializes the
 * anon_allocator object.
 */
 /*work*/
void
anon_init()
{
        anon_allocator = slab_allocator_create("anon", sizeof(mmobj_t));
        KASSERT(NULL != anon_allocator && "failed to create anon allocator!");
        /*NOT_YET_IMPLEMENTED("VM: anon_init");*/
}

/*
 * You'll want to use the anon_allocator to allocate the mmobj to
 * return, then then initialize it. Take a look in mm/mmobj.h for
 * macros which can be of use here. Make sure your initial
 * reference count is correct.
 */
 /**/
mmobj_t *
anon_create()
{
        mmobj_t * newanon=(mmobj_t *) slab_obj_alloc(anon_allocator);
        mmobj_init(newanon, &anon_mmobj_ops);
        /*** same thing ? ***/
        newanon->mmo_un.mmo_vmas=*mmobj_bottom_vmas(newanon);
        return newanon;
        /*NOT_YET_IMPLEMENTED("VM: anon_create");
        return NULL;*/
}

/* Implementation of mmobj entry points: */

/*
 * Increment the reference count on the object.
 */
static void
anon_ref(mmobj_t *o)
{
        o->mmo_refcount++;
        /*NOT_YET_IMPLEMENTED("VM: anon_ref");*/
}

/*
 * Decrement the reference count on the object. If, however, the
 * reference count on the object reaches the number of resident
 * pages of the object, we can conclude that the object is no
 * longer in use and, since it is an anonymous object, it will
 * never be used again. You should unpin and uncache all of the
 * object's pages and then free the object itself.
 */
static void
anon_put(mmobj_t *o)
{
        o->mmo_refcount--;
        if(o->mmo_refcount > o->mmo_nrespages)
        {
                return;
        }
        else
        {
                pframe_t *pf;
                list_iterate_begin(&(o->mmo_respages), pf, pframe_t,pf_olink)
                {
                        pframe_clear_busy(pf);
                        pframe_unpin(pf);
                        pframe_free(pf);
                }list_iterate_end();
                slab_obj_free(anon_allocator, o);
        }
        /*NOT_YET_IMPLEMENTED("VM: anon_put");*/
}

/* Get the corresponding page from the mmobj. No special handling is
 * required. */
static int
anon_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf)
{
        pframe_t *pframe;
        list_iterate_begin(&(o->mmo_respages), pframe, pframe_t, pf_olink)
        {
                if(pagenum==pframe->pf_pagenum)
                {
                        *pf=pframe;
                        return 0;
                }
        }list_iterate_end();
        return -1;
        /*NOT_YET_IMPLEMENTED("VM: anon_lookuppage");
        return -1;*/
}

/* The following three functions should not be difficult. */

/* Fill the page frame starting at address pf->pf_paddr with the
 * contents of the page identified by pf->pf_obj and pf->pf_pagenum.
 * This may block.
 * Return 0 on success and -errno otherwise.
 */
static int
anon_fillpage(mmobj_t *o, pframe_t *pf)
{
        
        NOT_YET_IMPLEMENTED("VM: anon_fillpage");
        return 0;
}

/* A hook; called when a request is made to dirty a non-dirty page.
 * Perform any necessary actions that must take place in order for it
 * to be possible to dirty (write to) the provided page. (For example,
 * if this page corresponds to a sparse block of a file that belongs to
 * an S5 filesystem, it would be necessary/desirable to allocate a
 * block in the fs before allowing a write to the block to proceed).
 * This may block.
 * Return 0 on success and -errno otherwise.
 */
static int
anon_dirtypage(mmobj_t *o, pframe_t *pf)
{
        
        NOT_YET_IMPLEMENTED("VM: anon_dirtypage");
        return -1;
}

/*
 * Write the contents of the page frame starting at address
 * pf->pf_paddr to the page identified by pf->pf_obj and
 * pf->pf_pagenum.
 * This may block.
 * Return 0 on success and -errno otherwise.
 */
static int
anon_cleanpage(mmobj_t *o, pframe_t *pf)
{
        
        NOT_YET_IMPLEMENTED("VM: anon_cleanpage");
        return -1;
}
