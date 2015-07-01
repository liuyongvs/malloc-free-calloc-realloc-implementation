#include<stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
void *myMalloc(size_t size)
{

    void *p = sbrk(0);
    printf("%x\n", (unsigned int)p);
    if(sbrk(size) == (void *)-1)
        return NULL;
    printf("%x\n", (unsigned int)sbrk(0));
    return p;
}

typedef struct s_block *t_block;

/**
 * @brief The s_block struct : meta data
 */
struct s_block {
    size_t size;      /*  data size */
    t_block next;     /*  double linked list */
    t_block prev;     /*  double linked list */
    int free;         /*  alignment  */
    void *ptr;        /*  magic pointer for validation  */
};

/* for allignment */
#define align4(x) (((((x) - 1) >> 2) << 2) + 4)

/**
  * x is integer
  * if x is multiple of 4, x is itself;
  * else x = x/4 * 4 + 4
  */

#define align_other4(x)  \
    if(x >> 2) \
        x = ((x >> 2) << 2) + 4;
typedef    int    bool;
#define    true    1
#define    false   0
#define    BLOCK_SZIE sizeof(struct s_block)

t_block find_block(t_block *last, size_t size);
t_block extend_block(t_block last, size_t size);
void split_block(t_block b, size_t size);
void *mycalloc(size_t number, size_t size);
void *mymalloc(size_t size);
t_block get_block(void *ptr);
bool validate_addr(void *ptr);
void myfree(void *p);
t_block fusion_block(t_block b);
void *myrealloc(void *p, size_t size);
void copy_data(void *dest, void *src, size_t size);


void *base = NULL;

t_block find_block(t_block *last, size_t size)
{
    t_block b = base;
    while(b && (!b->free || b->size < size))
    {
        *last = b;
        b = b->next;
    }
    return b;
}

t_block extend_block(t_block last, size_t size)
{
    t_block b = sbrk(0);
    if(sbrk(BLOCK_SZIE + size) == (void *)-1)
        return NULL;
    b->size = size;
    b->free = false;
    b->next = NULL;
    b->ptr = (char *)b + BLOCK_SZIE;
    b->prev = last;
    if(last)  /*this is important for  b = extend_block(NULL, size);*/
        last->next = b;
    return b;
}

void split_block(t_block b, size_t size)
{
    if(b && b->size > size)
    {
        t_block new_b = (t_block)((char *)b + BLOCK_SZIE + size);
        new_b->free = true;
        new_b->next = b->next;
        new_b->size = b->size - BLOCK_SZIE + size;
        new_b->prev = b;
        new_b->ptr = (char *)new_b + BLOCK_SZIE;

        b->size = size;
        b->next = new_b;
        if(new_b->next)
            new_b->next->prev = new_b;
    }
}

void *mycalloc(size_t number, size_t size)
{
    size_t *new = (size_t *)mymalloc(number * size);
    if(new)
    {
        size_t i;
        size_t s4 = align4(number * size) >> 2;
        //iterate by 4 bytes steps
        for(i = 0; i < s4; i++)
            new[i] = 0;
    }
    return new;
}

void *mymalloc(size_t size)
{
    t_block b;
    t_block last;
    size = align4(size);
    if(base)
    {
       b = find_block(&last, size);
       if(b)
       {
           if(b->size - size >= (BLOCK_SZIE + 4))
           {
               split_block(b, size);
           }
           b->free = false;
       }
       else
       {
           b = extend_block(last, size);
           if(b == NULL)
               return NULL;
       }
    }
    else
    {
        b = extend_block(NULL, size);
        if(b == NULL)
            return NULL;
        base = b;
    }
    return (char *)b + BLOCK_SZIE;
}

bool validate_addr(void *ptr)
{
    t_block b = get_block(ptr);//if base == null
    if(ptr > base && ptr < sbrk(0))
    {
        return (ptr == b->ptr);
    }
    return false;
}


t_block get_block(void *ptr)
{
    char *tmp = ptr;
    tmp -= BLOCK_SZIE;
    return (t_block)tmp;
}

void myfree(void *p)
{
    if(validate_addr(p))
    {
        t_block b = get_block(p);
        b->free = true;
        if(b->prev && b->prev->free)
        {
           b = fusion_block(b->prev); /*use the return value*/
        }


        if(b->next)
        {
            if(b->next->free)
            {
                fusion_block(b);
            }
        }
        else
        {
            if(b->prev)
                b->prev->next = NULL;
            else
                base = NULL;
            brk(b);
        }
    }
    else
    {
        printf("myfree(): invalid pointer: %x\n", (unsigned int)p);
    }
}


t_block fusion_block(t_block b)
{
    b->size += b->next->size + BLOCK_SZIE;
    b->next = b->next->next;
    if(b->next)
        b->next->prev = b;
    return b;
}

void *myrealloc(void *p, size_t size)
{
    //if p is null, equals to malloc
    if(p == NULL)
        return mymalloc(size);
    //if size is 0, equals to free
    if(size == 0)
        myfree(p);
    if(validate_addr(p))
    {
        t_block b;
        b = get_block(p);
        size = align4(size);
        if(b->size >= size)
        {
            if(b->size - size >= BLOCK_SZIE +4)
                split_block(b, size);
        }
        else
        {
            /*if the next block is free, fusion it*/
            if(b->next && b->next->free &&
                    b->size + BLOCK_SZIE + b->next->size >= size)
            {
                fusion_block(b);
                if(b->size - size >= BLOCK_SZIE +4)
                    split_block(b, size);
            }
            else
            {
                void *new_p = mymalloc(size);
                if(new_p == NULL)
                    return NULL;
                /*  copy old data */
                copy_data(new_p, p, b->size);
                /*  free the old pointer */
                myfree(p);
                return new_p;
            }
        }
        return p;
    }
    return NULL;

}

void copy_data(void *dest, void *src, size_t size)
{
    size_t *dest_tmp = dest;
    size_t *src_tmp = src;
    size_t i;
    size = size >> 2;
    /* iterate by 4 bytes steps */
    for(i = 0; i < size; i++)
        dest_tmp[i] = src_tmp[i];
}

int main()
{

    int *p1[3];
    int i;
    for(i = 0; i < 3; i++)
    {
        p1[i] = mymalloc(16);
        printf("%x\n", (unsigned int)p1[i]);
    }

    myfree(p1[1]);
    int * p = (int *) mymalloc(10);
    printf("%x\n", (unsigned int)p);


    myfree(p1[0]);
    myfree(p1[2]);
    int * p2 = (int *) mycalloc(2, 7);
    printf("%x\n", (unsigned int)p2);

    p = myrealloc(p, 5);
    myfree(p);
    myfree(p2);


    return 0;
}


