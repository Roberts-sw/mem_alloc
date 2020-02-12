/* lib > mem_alloc > mem_alloc.c
   ------------------------------------------ TAB-size 4, code page UTF-8 --

    Memory-allocator, idee: https://web.ics.purdue.edu/~cs354/labs/lab6/

Wijzigingen:
    RvL 11 feb 2020 aanmaak
   -------------------------------------------------------------------------
*/
#include"mem_alloc.h"

    /* ---------------------------------------------------------
    private
    --------------------------------------------------------- */
typedef struct st_mfb {struct st_mfb *next, *prev;} Smfb;
typedef struct st_fb {size_t sz; Smfb;} Sfb;
static size_t const OVH = offsetof(Sfb,d);      //bv: 4 (32-bit machine)
static size_t const SMFB=sizeof(Smfb);          //bv: 8 (32-bit machine)
#define _ao(n,o)        ((size_t *)((char *)(n) + (o) &~(SMFB-1) ) )
#define _fb(n,o)        ((Sfb *)   ((char *)(n) + (o) &~(SMFB-1) ) )
#define _nettosize(sz)  ((sz) + SMFB-1 &~(SMFB-1) ) //data + ... ?
static Sfb *freelist[32+1]; //index 0..31: blokken van SMFB*index Bytes
                            //index 32: groter, op grootte oplopend gesorteerd
static void add_to_freelist (Sfb *n)
{   int i=n->sz/SMFB;
    _maximaal(i,_maxindex(freelist) );
    Sfb *p=NULL, *q=freelist[i];
    if(q)                                       //is er al een freelist[i]
    {   if(_maxindex(freelist)==i)              //? en zijn het grote blokken
            while(sz>q->sz)                     //  ? zoek grootte-opvolger q
                if(p=q, q=q->next, !q) break;   //    ... en voorganger p
        if(n->prev=p)       p->next=n;          //  Bestaat p ? n komt na p
        else                freelist[i]=n;      //  : n is de eerste
        if(n->next=q)       q->prev=n;          //  Bestaat q ? n komt voor q
    } else freelist[i]=n, n->prev=n->next=NULL;
}
static Sfb *find_in_freelist (size_t sz)
{   sz=_nettosize(sz);
    Sfb *p, *q=freelist[_maxindex(freelist)];
    for(p=freelist[sz/SMFB]; p<q; p++)  if(p)   return p;//vaste grootte
    if(q)   while(q=q->next)    if(sz<=q->sz)   return q;//gesorteerde grootte
    return NULL;
}
void get_from_freelist (Sfb *n)                 //haal blok tussen p en q uit
{   Sfb *p=n->prev, *q=n->next;
    if(p)   p->next=q;
    if(q)   q->prev=p;
    n->_next=n->prev=NULL;
}

static void merge (Sfb *n, Sfb *q, int allocate)
{   size_t nsz=(!q?0:OVH*2+q->sz&~1) + n->sz&~1;//nsz => netto size
    n->sz=*_ao(n,nsz+OVH)=nsz | !!allocate;     //beschrijf hekken
}
static Sfb *split (Sfb *n, size_t nss)          //nss: netto splitted size
{   if(nss+OVH*2<=(n->sz&~1) )
    {   size_t sz=(n->sz&~1)-(nss+OVH*2);
        n->sz=*_ao(n, sz+OVH)= sz;
        n=_fb(n,sz+OVH*2);                      //voorkant ingekort, n erachter
        n->sz=*_ao(n,nss+OVH)=nss;
        return n;                               //achterkant gemaakt
    }   return NULL;
}
static size_t const MEM = _nettosize(1L<<12);   //4kB
static size_t mem[MEM/sizeof(size_t)];

    /* ---------------------------------------------------------
    public
    --------------------------------------------------------- */
void mem_init (void)
{   mem[0]=mem[_maxindex(mem)-0]=1;             //2 loze randhekken
    mem[1]=mem[_maxindex(mem)-1]=MEM-OVH*4;     //2 blokhekken om netto-blok
    add_to_freelist((Sfb *)(mem+1) );
}/* met 4B-pointers => 0:1 4:4080 8:NULL 12:NULL ... 4088:4080 4092:1   */
void *mem_malloc (size_t size)
{   if(size)
    {   Sfb *n, *q;
        if( n=find_in_freelist(size) )          //a.
        {   get_from_freelist(n);               //b.
            if(n->sz>OVH*2+_nettosize(size) )   //c.
                if(q=split(n,n->sz-OVH*2) )
                    add_to_freelist(q);
            merge(n,NULL,1);                    //d.
            return _ao(n,OVH);
        }
    }   return NULL;
}/* bv: mem_malloc(100) na mem_init als bovenstaand:
    4:105 ... 112:105 116:3968 120:NULL 124:NULL ... 4088:3968  */
void *mem_calloc (size_t nitems, size_t size)
{   void *p=mem_malloc(nitems*size);
    if(p)   memset(p,0,nitems*size);
    return p;
}
void mem_free (void *addr)
{   if(!(_ao(addr,-OVH)&1) )    return;
    merge(n,NULL,0);                            //a.
    Sfb *p, *q, *n=_fb(addr,-OVH);  size_t sz;
    if(p=_fb(n,-OVH), sz=p->sz,     !(sz&1) )   //  vrije voorbuur
    {   get_from_freelist(p); merge(p,n,0);n=p; }
    if(sz=n->sz, q=_fb(n,sz+OVH*2), !(q->sz&1) )//  vrije achterbuur
    {   get_from_freelist(q); merge(n,q,0);     }
    add_to_freelist(n);                         //b.
}
void *mem_realloc (void *addr, size_t size)
{   if(!addr)   return mem_malloc(size);            //a.
    if(!size)   return mem_free(addr), NULL;    //b.
    Sfb *n=_fb(addr,-OVH);  size_t nsz=n->sz & ~1;
    Sfb *q=_fb(n,nsz+OVH*2);size_t netto=_nettosize(size);
    if(nsz>=OVH*2+netto)                        //c.
    {   if(!(q->sz&1) )                         //  vrije opvolger
        {   p=split(n,nsz-OVH*2-netto);
            get_from_freelist(q); merge(p,q,0);
            add_to_freelist(p);
        } else if(nsz>OVH*2+netto)              //  vrij restblok
        {   p=split(n,nsz-OVH*2-netto);
            add_to_freelist(p);
        }   return addr;
    }                                           //d. groter blok nodig:
    size_t psz=_fb(n,-OVH)->sz;                 if(psz&1) psz=0;//(niet-vrij)
    if(!(q->sz&1) )                             //  vrije opvolger
    {   if(netto<=nsz+q->sz+OVH*2)
        {   get_from_freelist(q);   merge(n,q,1);
            return addr;
        } else if(netto<=psz+OVH*2+nsz+q->sz+OVH*2)//icm. vrije voorganger:
        {   get_from_freelist(q);   merge(n,q,0);//     eerst achterkant doen
            goto ap;
        }
    } else if(netto<=psz+OVH*2+nsz)             //  vrije voorganger
ap: {   Sfb *p=_fb(n,-psz-OVH*2);
        get_from_freelist(p);   merge(p,n,1);
        memcpy(_ao(p,OVH), addr, nsz);
        return _ao(p,OVH);
    }
    if(void *vp=mem_malloc(size) )              //e.
    {   memcpy(vp, addr, nsz);
        return mem_free(addr), vp;
    }   return NULL;                            //f.
}
