#include "kernel.h"
#include "errno.h"
#include "globals.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "proc/proc.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/fcntl.h"
#include "fs/vfs_syscall.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/mmobj.h"

static slab_allocator_t *vmmap_allocator;
static slab_allocator_t *vmarea_allocator;

void
vmmap_init(void)
{
        vmmap_allocator = slab_allocator_create("vmmap", sizeof(vmmap_t));
        KASSERT(NULL != vmmap_allocator && "failed to create vmmap allocator!");
        vmarea_allocator = slab_allocator_create("vmarea", sizeof(vmarea_t));
        KASSERT(NULL != vmarea_allocator && "failed to create vmarea allocator!");
}

vmarea_t *
vmarea_alloc(void)
{
        dbg(DBG_VM,"VM: Enter vmarea_alloc()");
        vmarea_t *newvma = (vmarea_t *) slab_obj_alloc(vmarea_allocator);
        if (newvma) {
                newvma->vma_vmmap = NULL;
        }
        dbg(DBG_VM,"VM: Leave vmarea_alloc()");
        return newvma;
}

void
vmarea_free(vmarea_t *vma)
{
        dbg(DBG_VM,"VM: Enter vmarea_free()");
        KASSERT(NULL != vma);
        slab_obj_free(vmarea_allocator, vma);
        dbg(DBG_VM,"VM: Leave vmarea_free()");
}

/* Create a new vmmap, which has no vmareas and does
 * not refer to a process. */
/*work*/
vmmap_t *
vmmap_create(void)
{
        dbg(DBG_VM,"VM: Enter vmmap_create()");
        vmmap_t *newvmm = (vmmap_t *)slab_obj_alloc(vmmap_allocator);
        if(newvmm) {
                list_init(&newvmm->vmm_list);
                newvmm->vmm_proc = NULL;
        }
        dbg(DBG_VM,"VM: Leave vmmap_create()");
        return newvmm;

        /*
        NOT_YET_IMPLEMENTED("VM: vmmap_create");
        return NULL;
        */
}
    
/* Removes all vmareas from the address space and frees the
 * vmmap struct. */
/*Changed work*/
void
vmmap_destroy(vmmap_t *map)
{
        dbg(DBG_VM,"VM: Enter vmmap_destroy()");
        KASSERT(NULL != map);

        if(!list_empty(&map->vmm_list)) {
                vmarea_t *iterator;
                list_iterate_begin(&map->vmm_list, iterator, vmarea_t, vma_plink) {  
                        list_remove(&iterator->vma_plink);
                        vmarea_free(iterator);
                } list_iterate_end();
        }
        map->vmm_proc=NULL;
        slab_obj_free(vmmap_allocator, map);
        dbg(DBG_VM,"VM: Leave vmmap_destroy()");
        /*
        NOT_YET_IMPLEMENTED("VM: vmmap_destroy");
        */
}

/* Add a vmarea to an address space. Assumes (i.e. asserts to some extent)
 * the vmarea is valid.  This involves finding where to put it in the list
 * of VM areas, and adding it. Don't forget to set the vma_vmmap for the
 * area. */
 /*Changed work*/
void
vmmap_insert(vmmap_t *map, vmarea_t *newvma)
{
        dbg(DBG_VM,"VM: Enter vmmap_insert()");
        KASSERT(NULL != map && NULL != newvma);
        KASSERT(NULL == newvma->vma_vmmap);
        KASSERT(newvma->vma_start < newvma->vma_end);
        KASSERT(ADDR_TO_PN(USER_MEM_LOW) <= newvma->vma_start && ADDR_TO_PN(USER_MEM_HIGH) >= newvma->vma_end);
    
        if(!list_empty(&map->vmm_list)) 
        {
                uint32_t vma_start = newvma->vma_start;
                vmarea_t *iterator;
                list_iterate_begin(&map->vmm_list, iterator, vmarea_t, vma_plink) 
                {              
                        /* keep the areas sorted by the start of their virtual page ranges */
                        if(vma_start > iterator->vma_start) 
                        {
                                /* no two ranges overlap with each other */
                                if(vma_start >= iterator->vma_end) 
                                {
                                        /* set the vma_vmmap for the area */
                                        newvma->vma_vmmap = map;
                                        /*** need collapse? ***/
                                        list_insert_before((&iterator->vma_plink)->l_next, &newvma->vma_plink);
                                        dbg(DBG_VM,"VM: Leave vmmap_destroy()");
                                        return 0;
                                }
                                else 
                                {
                                        /*** how to deal with overlap? ***/
                                        /*** no overlap here! ***/
                                        dbg(DBG_VM,"VM: Leave vmmap_destroy(), overlap!!!");
                                        return -1;
                                }
                        }
                } list_iterate_end();
                list_insert_tail(&map->vmm_list, newvma->vma_plink);
                newvma->vma_vmmap = map;
                return;
        }
        else 
        {
                newvma->vma_vmmap = map;
                list_insert_head(&map->vmm_list, &newvma->vma_plink);
                dbg(DBG_VM,"VM: Leave vmmap_destroy()");
                return 0;
        }

        /*
        NOT_YET_IMPLEMENTED("VM: vmmap_insert");
        */
}

/* Find a contiguous range of free virtual pages of length npages in
 * the given address space. Returns starting vfn for the range,
 * without altering the map. Returns -1 if no such range exists.
 *
 * Your algorithm should be first fit. If dir is VMMAP_DIR_HILO, you
 * should find a gap as high in the address space as possible; if dir
 * is VMMAP_DIR_LOHI, the gap should be as low as possible. */

/* The comment for vmmap_find_range() says that you need to use the 
   "first fit" algorithm.  So, if the direction is from high to low, 
   you should start looking from an address as high as possible in the 
   given address space.  If the direction is from low to high, you 
   should start looking from an address as low as possible in the 
   given address space. */
   /*work*/
int
vmmap_find_range(vmmap_t *map, uint32_t npages, int dir)
{
        dbg(DBG_VM,"VM: Enter vmmap_find_range()");
        KASSERT(NULL != map);
        KASSERT(0 < npages);

        /*** start: USER_MEM_LOW ***/ /* inclusive */
        /*** end: USER_MEM_HIGH ***/ /* exclusive */
        uint32_t page_num_low = ADDR_TO_PN(USER_MEM_LOW);
        uint32_t page_num_high  = ADDR_TO_PN(USER_MEM_HIGH);
        uint32_t page_no;

        if(!list_empty(&map->vmm_list)) 
        {
                /* vmarea_t *iterator; */
                unsigned int pages;
                /* high to low */
                if(dir == VMMAP_DIR_HILO) 
                {
                        /*** page or address? ***/
                        list_link_t *link;
                        for (link = (&map->vmm_list)->l_prev; link != &map->vmm_list; link = link->l_prev) 
                        {
                                vmarea_t *vma = list_item(link, vmarea_t, vma_plink);
                                pages=0;
                                if(link->l_next == &map->vmm_list) 
                                { /* last one */
                                        pages = page_num_high - vma->vma_end;
                                        page_no = vma->vma_end;
                                }
                                else 
                                { /* find gap in middle */
                                        vmarea_t *vma_next = list_item(link->l_next, vmarea_t, vma_plink);
                                        pages = vma_next->vma_start - vma->vma_end;
                                        page_no = vma->vma_end;   
                                }
                                if(pages >= npages)
                                {
                                        dbg(DBG_VM,"VM: Leave vmmap_find_range()");
                                        return page_no;
                                }
                        }
                        if(link == &map->vmm_list) 
                        { /* first one */
                                vmarea_t *vma = list_item(link, vmarea_t, vma_plink);
                                pages = vma->vma_start - page_num_low;
                                page_no = vma->vma_end;
                                if(pages >= npages)
                                {
                                        dbg(DBG_VM,"VM: Leave vmmap_find_range()");
                                        return page_no;
                                }
                        }
                }
                /* low to high */
                if(dir == VMMAP_DIR_LOHI) 
                {
                        list_link_t *link;
                        for (link = (&map->vmm_list)->l_next; link != &map->vmm_list; link = link->l_next) 
                        {
                                vmarea_t *vma = list_item(link, vmarea_t, vma_plink);
                                if(link->l_prev == &map->vmm_list) 
                                { /* first one */
                                        pages = vma->vma_start - page_num_low;
                                        page_no = page_num_low;
                                }
                                else 
                                { /* find gap in middle */
                                        vmarea_t *vma_prev = list_item(link->l_prev, vmarea_t, vma_plink);
                                        pages = vma->vma_start - vma_prev->vma_end;
                                        page_no = vma_prev->vma_end;
                                }
                                if(pages >= npages)
                                {
                                        dbg(DBG_VM,"VM: Leave vmmap_find_range()");
                                        return page_no;
                                }
                        }
                        if(link == &map->vmm_list) 
                        { /* last one */
                                vmarea_t *vma = list_item(link, vmarea_t, vma_plink);
                                pages = page_num_high - vma->vma_end;
                                page_no = vma->vma_end;
                                if(pages >= npages)
                                {
                                        dbg(DBG_VM,"VM: Leave vmmap_find_range()");
                                        return page_no;
                                }
                        }
                }
        }
        dbg(DBG_VM,"VM: Leave vmmap_find_range()");
        return -1;
        /*
        NOT_YET_IMPLEMENTED("VM: vmmap_find_range");
        return -1;
        */
}

/* Find the vm_area that vfn lies in. Simply scan the address space
 * looking for a vma whose range covers vfn. If the page is unmapped,
 * return NULL. */
vmarea_t *
vmmap_lookup(vmmap_t *map, uint32_t vfn)
{
        dbg(DBG_VM,"VM: Enter vmmap_lookup()");
        KASSERT(NULL != map);

        if(!list_empty(&map->vmm_list)) 
        {
                vmarea_t *iterator;
                list_iterate_begin(&map->vmm_list, iterator, vmarea_t, vma_plink) 
                {              
                        if(vfn >= iterator->vma_start && vfn < iterator->vma_end) 
                        {
                            dbg(DBG_VM,"VM: Leave vmmap_lookup(), found!");
                            return iterator;
                        }
                } list_iterate_end();
        }
        dbg(DBG_VM,"VM: Leave vmmap_lookup(), not found!");
        return NULL;
        /*
        NOT_YET_IMPLEMENTED("VM: vmmap_lookup");
        return NULL;
        */
}

/* Allocates a new vmmap containing a new vmarea for each area in the
 * given map. The areas should have no mmobjs set yet. Returns pointer
 * to the new vmmap on success, NULL on failure. This function is
 * called when implementing fork(2). */
 /*Big Changed work*/
vmmap_t *
vmmap_clone(vmmap_t *map)
{
        dbg(DBG_VM,"VM: Enter vmmap_clone()");
        KASSERT(NULL != map);

        /*vmmap_t *clonevmm = (vmmap_t *)slab_obj_alloc(vmmap_allocator);*/
        vmmap_t *clonevmm=vmmap_create();
        if(clonevmm)
        {
            vmarea_t* newvma;
            vmarea_t* iterator;
            list_iterate_begin(&map->vmm_list, iterator, vmarea_t, vma_plink)
            {
                newvma=vmarea_alloc();
                if(!newvma)
                    return NULL;
                newvma->vma_start=iterator->vma_start;
                newvma->vma_end=iterator->vma_end;
                newvma->vma_prot=iterator->vma_prot;
                newvma->vma_flags=iterator->vma_flags;
                vmmap_insert(clonevmm,newvma);
            }list_iterate_end();
        }
        dbg(DBG_VM,"VM: Leave vmmap_clone()");
        return clonevmm;

        /*
        NOT_YET_IMPLEMENTED("VM: vmmap_clone");
        return NULL;
        */
}

/* Insert a mapping into the map starting at lopage for npages pages.
 * If lopage is zero, we will find a range of virtual addresses in the
 * process that is big enough, by using vmmap_find_range with the same
 * dir argument.  If lopage is non-zero and the specified region
 * contains another mapping that mapping should be unmapped.
 *
 * If file is NULL an anon mmobj will be used to create a mapping
 * of 0's.  If file is non-null that vnode's file will be mapped in
 * for the given range.  Use the vnode's mmap operation to get the
 * mmobj for the file; do not assume it is file->vn_obj. Make sure all
 * of the area's fields except for vma_obj have been set before
 * calling mmap.
 *
 * If MAP_PRIVATE is specified set up a shadow object for the mmobj.
 *
 * All of the input to this function should be valid (KASSERT!).
 * See mmap(2) for for description of legal input.
 * Note that off should be page aligned.
 *
 * Be very careful about the order operations are performed in here. Some
 * operation are impossible to undo and should be saved until there
 * is no chance of failure.
 *
 * If 'new' is non-NULL a pointer to the new vmarea_t should be stored in it.
 */
/*Not total sure about shadow part*/
int
vmmap_map(vmmap_t *map, vnode_t *file, uint32_t lopage, uint32_t npages,
          int prot, int flags, off_t off, int dir, vmarea_t **new)
{
        dbg(DBG_VM,"VM: Enter vmmap_map()");
        KASSERT(NULL != map);
        KASSERT(0 < npages);
        KASSERT(!(~(PROT_NONE | PROT_READ | PROT_WRITE | PROT_EXEC) & prot));
        KASSERT((MAP_SHARED & flags) || (MAP_PRIVATE & flags));
        KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_LOW) <= lopage));
        KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_HIGH) >= (lopage + npages)));
        KASSERT(PAGE_ALIGNED(off));

        int err;
        /*set up new vmare except mmobj*/
        if(lopage==0)
        {
            int vfn_start=vmmap_find_range(map,npages,dir);
            if(vfn_start==-1)
            {
                dbg(DBG_VM,"VM: Leave vmmap_map(), error");
                return -1;
            }
            vmarea_t * newvma=vmarea_alloc();
            if(newvma==NULL)
            {
                dbg(DBG_VM,"VM: Leave vmmap_map(), error");
                return -1;
            }
            newvma->vma_start=vfn_start;
            newvma->vma_end=vfn_start+npages;
        }
        else if(lopage!=0)
        {
            if(!vmmap_is_range_empty(map,lopage,npages))
            {
                err=vmmap_remove(map, lopage, npages);
                if(err!=0)
                {
                    dbg(DBG_VM,"VM: Leave vmmap_map(), error");
                    return err;
                }
            }
            vmarea_t * newvma=vmarea_alloc();
            newvma->vma_start=lopage;
            newvma->vma_end=lopage+npages;
        }
        newvma->vma_prot=prot;
        newvma->vma_flags=flags;
        newvma->off=off;

        //set up mmobj
        mmobj_t* obj;
        if(file==NULL)
        {
            obj=anon_create();
            if(obj==NULL)
            {
                dbg(DBG_VM,"VM: Leave vmmap_map(), error");
                return -1;
            }
            obj->mmo_ops->ref(obj);
            /* not sure about above line, reference count increase 
            newvma->vma_off = 0;
            newvma->vma_prot = PROT_NONE;    
            newvma->vma_flags = 0;
            */
        }
        else 
        {
            if((err=(file->vn_ops->mmap)(file,newvma,&obj))<0)
            {
                dbg(DBG_VM,"VM: Leave vmmap_map(), error");
                return err;
            }
            file->vn_mmobj=*obj;
        }

        newvma->vma_obj=obj;
        if(flags==MAP_PRIVATE)
        {
            newvma->vma_obj->mmo_shadowed=shadow_create();
        }
        *new=newvma;
        vmmap_insert(map,newvma);
        dbg(DBG_VM,"VM: Leave vmmap_map()");
        return 0;
        /*
        NOT_YET_IMPLEMENTED("VM: vmmap_map");
        return -1;
        */
}

/*
 * We have no guarantee that the region of the address space being
 * unmapped will play nicely with our list of vmareas.
 *
 * You must iterate over each vmarea that is partially or wholly covered
 * by the address range [addr ... addr+len). The vm-area will fall into one
 * of four cases, as illustrated below:
 *
 * key:
 *          [             ]   Existing VM Area
 *        *******             Region to be unmapped
 *
 * Case 1:  [   ******    ]
 * The region to be unmapped lies completely inside the vmarea. We need to
 * split the old vmarea into two vmareas. be sure to increment the
 * reference count to the file associated with the vmarea.
 *
 * Case 2:  [      *******]**
 * The region overlaps the end of the vmarea. Just shorten the length of
 * the mapping.
 *
 * Case 3: *[*****        ]
 * The region overlaps the beginning of the vmarea. Move the beginning of
 * the mapping (remember to update vma_off), and shorten its length.
 *
 * Case 4: *[*************]**
 * The region completely contains the vmarea. Remove the vmarea from the
 * list.
 */
int
vmmap_remove(vmmap_t *map, uint32_t lopage, uint32_t npages)
{
        dbg(DBG_VM,"VM: Enter vmmap_remove()");
        if(!list_empty(&map->vmm_list)) 
        {
                vmarea_t *iterator;
                list_iterate_begin(&map->vmm_list, iterator, vmarea_t, vma_plink) 
                {              
                        if(lopage >= iterator->vma_start && lopage < iterator->vma_end) 
                        {
                            /*case1 [   ******    ]*/
                            if((lopage+npages)>vma_start&&(lopage+npages)<=iterator->vma_end)
                            {
                                vmarea_t * newvma1=vmarea_alloc();
                                vmarea_t * newvma2=vmarea_alloc();
                                if(!newvma1||!newvma2)
                                {
                                    dbg(DBG_VM,"VM: Leave vmmap_remove()");
                                    return -1;
                                }

                                newvma1->vma_start=iterator->vma_start;
                                newvma1->vma_end=lopage;
                                newvma1->vma_off=vma->vma_off;
                                newvma1->vma_prot=iterator->vma_prot;
                                newvma1->vma_flags=iterator->vma_flags;
                                newvma1->vma_obj=iterator->vma_obj;

                                newvma2->vma_start=lopage+npages;
                                newvma2->vma_end=iterator->vma_end;
                                newvma2->vma_off=iterator->vma_off+(lopage+npages-newvma1->vma_start);
                                /*newvma2->vma_off=iterator->vma_off+(newvma1->vma_end-newvma1->vma_start);*/
                                newvma2->vma_prot=iterator->vma_prot;
                                newvma2->vma_flags=iterator->vma_flags;
                                newvma2->vma_obj=iterator->vma_obj;
                                (newvma2->vma_obj->mmo_ops->ref)(newvma2->vma_obj);

                                list_remove(iterator->vma_plink);
                                vmmap_insert(map,newvma1);
                                vmmap_insert(map,newvma2);
                            }
                            else /*case2 [      *******]** */
                            {
                                iterator->vma_end=lopage;
                            }
                        }
                        /*case3  *[*****        ] */
                        else if((lopage+npages)>vma_start&&(lopage+npages)<=iterator->vma_end)
                        {
                                iterator->vma_off=iterator->vma_off+(lopage+npages-iterator->vma_start);
                                iterator->vma_start=lopage+npages;
                        }
                        /*case4  *[*************]** */
                        else if((vma_start>=lopage)&&(vma_end<=lopage+npages))
                        {
                                list_remove(iterator->vma_plink);
                                vmarea_free(iterator);
                        }

                } list_iterate_end();
        }
        dbg(DBG_VM,"VM: Leave vmmap_remove()");
        return 0;
        /*NOT_YET_IMPLEMENTED("VM: vmmap_remove");
        return -1;*/
}

/*
 * Returns 1 if the given address space has no mappings for the
 * given range, 0 otherwise.
 */
int
vmmap_is_range_empty(vmmap_t *map, uint32_t startvfn, uint32_t npages)
{
        dbg(DBG_VM,"VM: Enter vmmap_is_range_empty()");
        KASSERT(NULL != map);
        KASSERT((startvfn < endvfn) && (ADDR_TO_PN(USER_MEM_LOW) <= startvfn) && (ADDR_TO_PN(USER_MEM_HIGH) >= endvfn));
        if(!list_empty(&map->vmm_list)) 
        {
                vmarea_t *iterator;
                list_iterate_begin(&map->vmm_list, iterator, vmarea_t, vma_plink) 
                {
                        /* [   *****]**** */
                        if(iterator->vma_start <= startvfn && iterator->vma_end > startvfn) 
                        {
                                dbg(DBG_VM,"VM: Leave vmmap_is_range_empty()");
                                return 0;
                        } /* ****[****    ] */
                        else if(iterator->vma_start < (startvfn + npages) && iterator->vma_end >= (startvfn + npages)) 
                        {
                                dbg(DBG_VM,"VM: Leave vmmap_is_range_empty()");
                                return 0;
                        } /* ***[****]*** */
                        else if(iterator->vma_start >= startvfn && iterator->vma_end <= (startvfn + npages)) 
                        {
                                dbg(DBG_VM,"VM: Leave vmmap_is_range_empty()");
                                return 0;
                        }

                } list_iterate_end();
        }
        dbg(DBG_VM,"VM: Leave vmmap_is_range_empty()");
        return 1;
        /*
        NOT_YET_IMPLEMENTED("VM: vmmap_is_range_empty");
        return 0;
        */
}

/* Read into 'buf' from the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do so, you will want to find the vmareas
 * to read from, then find the pframes within those vmareas corresponding
 * to the virtual addresses you want to read, and then read from the
 * physical memory that pframe points to. You should not check permissions
 * of the areas. Assume (KASSERT) that all the areas you are accessing exist.
 * Returns 0 on success, -errno on error.
 */

/* The memory map is a linked list of VM areas (see slide 3 of lecture 24).  
   Each VM area has a start virtual address and a size.  So, you just have to 
   walk down the VM areas and see which one contains the virtual address (vaddr) 
   being requested. But what if the VM area you've found does not have enough data?
   Then you would need to continue to the next VM area, and so on.
 */
int
vmmap_read(vmmap_t *map, const void *vaddr, void *buf, size_t count)
{
        dbg(DBG_VM,"VM: Enter vmmap_read()");
        KASSERT(NULL != map);

        /*** manipulate vfn ***/
        uint32_t vfn_start = ADDR_TO_PN(vaddr);
        uint32_t page_num = count / PAGE_SIZE;
        uint32_t vfn_end = vfn_start + page_num;
        /*uint32_t page_res = count - page_num * PAGE_SIZE;*/

        size_t count_res = count;
        uint32_t vfn = vfn_start;
        vmarea_t *vma;
        pframe_t *pframe;
        void *read_buf = buf;
        /* [   *][****][*   ] page_num = 1 */
        /* [    ][****][**  ] page_num = 1 */
        /* [    ][ ** ][    ] page_num = 0 */
        /* [    ][   *][*   ] page_num = 0 */
        for(vfn = vfn_start; vfn_start <= vfn_end; ++vfn)
        {
                vma = vmmap_lookup(map, vfn);
                if(vma)
                {
                        if(!pframe_get(vma->vma_obj, vfn, &pframe))
                        {
                                /* pframe_get sucess */
                                if(vfn == vfn_start) 
                                {
                                        /* handle for first pageframe */
                                        uint32_t vaddr_start = (uint32_t)(pframe->pf_addr);
                                        uint32_t vaddr_end = vaddr_start + PAGE_SIZE;
                                        uint32_t page_size = vaddr_end - (uint32_t)vaddr;
                                        if(page_size > count)
                                                page_size = count;

                                        count_res -= page_size;

                                        uint32_t phy_addr = pt_virt_to_phys((uint32_t)(vaddr));

                                        memcpy(read_buf, (void*)phy_addr, page_size);

                                        uint32_t read_buf_addr = (uint32_t)read_buf;
                                        read_buf_addr += page_size;
                                        read_buf = (void*)read_buf_addr;
                                }
                                else
                                {
                                        /* read whole page */
                                        uint32_t phy_addr = pt_virt_to_phys((uint32_t)(pframe->pf_addr));
                                        memcpy(read_buf, (void*)phy_addr, PAGE_SIZE);
                                        uint32_t read_buf_addr = (uint32_t)read_buf;
                                        read_buf_addr += PAGE_SIZE;
                                        read_buf = (void*)read_buf_addr;

                                        count_res -= PAGE_SIZE;
                                }
                        }
                        else
                        {
                                dbg(DBG_VM,"VM: Enter vmmap_read()");
                                return -EFAULT;
                        }
                }
                else
                {
                        dbg(DBG_VM,"VM: Enter vmmap_read()");
                        return -EFAULT;
                } 
        }
        if(count_res > 0)
        {
                /* handle for last pageframe */
                 vma = vmmap_lookup(map, vfn_end + 1);
                 if(vma)
                 {
                        if(!pframe_get(vma->vma_obj, vfn, &pframe))
                        {
                                /* count_res should < PAGE_SIZE here */
                                uint32_t vaddr_start = (uint32_t)(pframe->pf_addr);

                                uint32_t phy_addr = pt_virt_to_phys((uint32_t)(vaddr_start));

                                memcpy(read_buf, (void*)phy_addr, count_res);
                        }
                        else
                        {
                                dbg(DBG_VM,"VM: Enter vmmap_read()");
                                return -EFAULT;
                        }
                 }
                 else
                 {
                        dbg(DBG_VM,"VM: Enter vmmap_read()");
                        return -EFAULT;
                 }
        }
        dbg(DBG_VM,"VM: Enter vmmap_read()");
        return 0;

        /*** manipulate address ***/
        /*
        void *end_addr = vaddr + count;
        size_t offset = 0;
        if(!list_empty(&map->vmm_list)) {
                vmarea_t *vmarea;
                mmobj_t *obj;
                list_iterate_begin(&map->vmm_list, vmarea, vmarea_t, vma_plink) {
        */
                        /* search in pframe list */
        /*
                        obj = vmarea->vma_obj;
                        pframe_t *pframe;
                        if(!list_empty(&obj->mmo_respages)) {
                                list_iterate_begin(&obj->mmo_respages, pframe, pframe_t, pf_olink) {
        */
                                        /* check the vitural address */
                                        /*** get warning: pointer of type ‘void *’ used in arithmetic!!! ***/
        /*
                                        void *pf_end_addr = pframe->pf_addr + PAGE_SIZE;
                                        void *buf_start = (void *)(buf + offset);
                                        if(vaddr <= pframe->pf_addr && end_addr >= pf_end_addr) {
        */
                                                /* **[*****]** */
                                                /* read whole page */
        /*
                                                uintptr_t phy_addr = pt_virt_to_phys((uintptr_t)pframe->pf_addr);
                                                memcpy(buf_start, (void *)phy_addr, PAGE_SIZE);
                                                offset += PAGE_SIZE;
                                        }
                                        else if(vaddr >= pframe->pf_addr && end_addr >= pf_end_addr) {
        */
                                                /* [  **]** */
        /*
                                                uintptr_t phy_addr = pt_virt_to_phys((uintptr_t)vaddr);
                                                size_t read_length = (size_t)(pf_end_addr - vaddr);
                                                memcpy(buf_start, (void *)phy_addr, read_length);
                                                offset += read_length;
                                        }
                                        else if(vaddr <= pframe->pf_addr && end_addr < pf_end_addr) {
        */
                                                /* ***[*** ] */
        /*
                                                uintptr_t phy_addr = pt_virt_to_phys((uintptr_t)pframe->pf_addr);
                                                size_t read_length = (size_t)(end_addr - pframe->pf_addr);
                                                memcpy(buf_start, (void *)phy_addr, read_length);
        */
                                                /* read finished case */
        /*
                                                return 0;
                                        }
                                } list_iterate_end();
                        }
                        
                } list_iterate_end();
        */
                /*** return error ***/
        /*

        }
        else {
        */
                /*** return error ***/
        /*

        }

        return 0;
        */
        /*
        NOT_YET_IMPLEMENTED("VM: vmmap_read");
        return 0;
        */
}

/* Write from 'buf' into the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do this, you will need to find the correct
 * vmareas to write into, then find the correct pframes within those vmareas,
 * and finally write into the physical addresses that those pframes correspond
 * to. You should not check permissions of the areas you use. Assume (KASSERT)
 * that all the areas you are accessing exist. Remember to dirty pages!
 * Returns 0 on success, -errno on error.
 */
int
vmmap_write(vmmap_t *map, void *vaddr, const void *buf, size_t count)
{
        dbg(DBG_VM,"VM: Enter vmmap_write()");
        KASSERT(NULL != map);

        /*** manipulate vfn ***/
        uint32_t vfn_start = ADDR_TO_PN(vaddr);
        uint32_t page_num = count / PAGE_SIZE;
        uint32_t vfn_end = vfn_start + page_num;
        /*uint32_t page_res = count - page_num * PAGE_SIZE;*/

        size_t count_res = count;

        uint32_t vfn = vfn_start;
        vmarea_t *vma;
        pframe_t *pframe;
        const void *write_buf = buf;
        /* [   *][****][*   ] page_num = 1 */
        /* [    ][****][**  ] page_num = 1 */
        /* [    ][ ** ][    ] page_num = 0 */
        /* [    ][   *][*   ] page_num = 0 */
        for(vfn = vfn_start; vfn_start <= vfn_end; ++vfn)
        {
                vma = vmmap_lookup(map, vfn);
                if(vma)
                {
                        if(!pframe_get(vma->vma_obj, vfn, &pframe))
                        {
                                /* pframe_get sucess */
                                if(vfn == vfn_start) 
                                {
                                        /* handle for first pageframe */
                                        uint32_t vaddr_start = (uint32_t)(pframe->pf_addr);
                                        uint32_t vaddr_end = vaddr_start + PAGE_SIZE;
                                        uint32_t page_size = vaddr_end - (uint32_t)vaddr;
                                        if(page_size > count)
                                                page_size = count;

                                        count_res -= page_size;

                                        uint32_t phy_addr = pt_virt_to_phys((uint32_t)(vaddr));

                                        memcpy((void*)phy_addr, write_buf, page_size);

                                        uint32_t write_buf_addr = (uint32_t)write_buf;
                                        write_buf_addr += page_size;
                                        write_buf = (void*)write_buf_addr;

                                        /* dirty page */
                                        pframe_dirty(pframe);
                                }
                                else
                                {
                                        /* write whole page */
                                        uint32_t phy_addr = pt_virt_to_phys((uint32_t)(pframe->pf_addr));
                                        memcpy((void*)phy_addr, write_buf, PAGE_SIZE);
                                        uint32_t write_buf_addr = (uint32_t)write_buf;
                                        write_buf_addr += PAGE_SIZE;
                                        write_buf = (void*)write_buf_addr;

                                        count_res -= PAGE_SIZE;

                                        /* dirty page */
                                        pframe_dirty(pframe);
                                }
                        }
                        else
                        {
                                dbg(DBG_VM,"VM: Leave vmmap_write(), EFAULT");
                                return -EFAULT;
                        }
                }
                else
                {
                        dbg(DBG_VM,"VM: Leave vmmap_write(), EFAULT");
                        return -EFAULT;
                } 
        }
        if(count_res > 0)
        {
                /* handle for last pageframe */
                 vma = vmmap_lookup(map, vfn_end + 1);
                 if(vma)
                 {
                        if(!pframe_get(vma->vma_obj, vfn, &pframe))
                        {
                                /* count_res should < PAGE_SIZE here */
                                uint32_t vaddr_start = (uint32_t)(pframe->pf_addr);

                                uint32_t phy_addr = pt_virt_to_phys((uint32_t)(vaddr_start));

                                memcpy((void*)phy_addr, write_buf, count_res);

                               /* dirty page */
                                pframe_dirty(pframe);
                        }
                        else
                        {
                                dbg(DBG_VM,"VM: Leave vmmap_write(), EFAULT");
                                return -EFAULT;
                        }
                 }
                 else
                 {
                        dbg(DBG_VM,"VM: Leave vmmap_write(), EFAULT");
                        return -EFAULT;
                 }
        }
        dbg(DBG_VM,"VM: Leave vmmap_write(), EFAULT");
        return 0;
        /*NOT_YET_IMPLEMENTED("VM: vmmap_write");
        return 0;*/
}

/* a debugging routine: dumps the mappings of the given address space. */
size_t
vmmap_mapping_info(const void *vmmap, char *buf, size_t osize)
{
        KASSERT(0 < osize);
        KASSERT(NULL != buf);
        KASSERT(NULL != vmmap);

        vmmap_t *map = (vmmap_t *)vmmap;
        vmarea_t *vma;
        ssize_t size = (ssize_t)osize;

        int len = snprintf(buf, size, "%21s %5s %7s %8s %10s %12s\n",
                           "VADDR RANGE", "PROT", "FLAGS", "MMOBJ", "OFFSET",
                           "VFN RANGE");

        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
                size -= len;
                buf += len;
                if (0 >= size) {
                        goto end;
                }

                len = snprintf(buf, size,
                               "%#.8x-%#.8x  %c%c%c  %7s 0x%p %#.5x %#.5x-%#.5x\n",
                               vma->vma_start << PAGE_SHIFT,
                               vma->vma_end << PAGE_SHIFT,
                               (vma->vma_prot & PROT_READ ? 'r' : '-'),
                               (vma->vma_prot & PROT_WRITE ? 'w' : '-'),
                               (vma->vma_prot & PROT_EXEC ? 'x' : '-'),
                               (vma->vma_flags & MAP_SHARED ? " SHARED" : "PRIVATE"),
                               vma->vma_obj, vma->vma_off, vma->vma_start, vma->vma_end);
        } list_iterate_end();

end:
        if (size <= 0) {
                size = osize;
                buf[osize - 1] = '\0';
        }
        /*
        KASSERT(0 <= size);
        if (0 == size) {
                size++;
                buf--;
                buf[0] = '\0';
        }
        */
        return osize - size;
}
