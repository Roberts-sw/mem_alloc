/* lib > mem_alloc > mem_alloc.c
   ------------------------------------------ TAB-size 4, code page UTF-8 --

    Memory-allocator, idee: https://web.ics.purdue.edu/~cs354/labs/lab6/

Wijzigingen:
    RvL 14 feb 2020 test, diverse aanpassingen
    RvL 12 feb 2020 github-commit, #undef ... toegevoegd
    RvL 11 feb 2020 aanmaak
   -------------------------------------------------------------------------
*/
#include"mem_alloc.h"

    /* ---------------------------------------------------------
    private
    --------------------------------------------------------- */
typedef struct st_fb Sfb;
struct st_fb {union{size_t sz;Sfb *d;}; Sfb *next, *prev;};
#define AA offsetof(Sfb,next)       //bv: 4 (32-bit machine)
#define SZ (sizeof(Sfb)-AA)         //bv: 8 (32-bit machine)
#define _ao(n,o)        ((size_t *)((char *)(n)+((o)&~(AA-1) ) ) )
#define _fb(n,o)        ((Sfb *)_ao(n,o))
#define _nettosize(sz)  ((sz) + SZ-1 &~(SZ-1) ) //data + ... ?
static Sfb *freelist[32+1]; //index 0..31: blokken van SZ*index Bytes
                            //index 32: groter, op grootte oplopend gesorteerd
#define _freelist_index(sz)\
    ((sz)/SZ>_maxindex(freelist)?_maxindex(freelist):(sz)/SZ)
static void add_to_freelist (Sfb *n)
{   size_t sz=n->sz; int i=_freelist_index(sz); Sfb *p=NULL, *q;
    if(q=freelist[i])                           //is er al een freelist[i]
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
    Sfb *n, **p=freelist+_freelist_index(sz)-1, **q=(freelist)+_maxindex(freelist);
    while(++p<q) if(*p) return *p;
    if(*q) for(n=*q; n&&sz>n->sz; ) n=n->next;  return n;
    return NULL;
}
void get_from_freelist (Sfb *n)
{   Sfb *p=n->prev, *q=n->next;
    if(!p)  freelist[_freelist_index(n->sz)]=q;
    else    p->next=q;
    if(q)   q->prev=p;
    n->next=n->prev=NULL;
}

static void merge (Sfb *n, Sfb *q, int allocate)
{   size_t nsz=n->sz&~1;
    if(q)
    {   size_t qsz=q->sz+AA*2&~1;               //(q toegewezen ipv n!)
        *_ao(n,nsz+AA)=q->sz=0;     nsz+=qsz;   //haal tussenhekken weg
    }   n->sz=*_ao(n,nsz+AA)=nsz | !!allocate;  //beschrijf hekken
}
static Sfb *split (Sfb *n, size_t nss)          //nss: n-splitted size
{    if(nss+AA*2<(n->sz&~1) )
    {   size_t qsz=(n->sz&~1)-(nss+AA*2);
        n->sz=*_ao(n,nss+AA)=nss;   n=_fb(n,nss+AA*2);
        n->sz=*_ao(n,qsz+AA)=qsz;   return n;
    }   return NULL;
}
#define MEM _nettosize(1L<<12)  //4kB
static size_t mem[MEM/sizeof(size_t)];
static void mem_cpy (size_t *dst, const size_t *src, size_t sz)
{   sz = dst==src ? 0 : (sz+sizeof(size_t)-1)/sizeof(size_t);
    while(sz--) *dst++=*src++;
}
    /* ---------------------------------------------------------
    public
    --------------------------------------------------------- */
void mem_init (void)
{   mem[0]=mem[_maxindex(mem)-0]=1;             //2 loze randhekken
    mem[1]=mem[_maxindex(mem)-1]=MEM-AA*4;      //2 blokhekken om netto-blok
    add_to_freelist((Sfb *)(mem+1) );
}/* met 4B-pointers => 0:1 4:4080 8:NULL 12:NULL ... 4088:4080 4092:1   */
void *mem_malloc (size_t size)
{   if(size)
    {   Sfb *n, *q;
        if( n=find_in_freelist(size) )          //a.
        {   get_from_freelist(n);               //b.
            if(n->sz>_nettosize(size)+AA*2 )
                if(q=split(n,_nettosize(size) ) )//c.
                    add_to_freelist(q);
            merge(n,NULL,1);                    //d.
            return _ao(n,AA);
        }
    }   return NULL;
}/* bv: mem_malloc(100) na mem_init als bovenstaand:
    4:105 ... 112:105 116:3968 120:NULL 124:NULL ... 4088:3968  */
void *mem_calloc (size_t nitems, size_t size)
{   void *p=mem_malloc(nitems*size);
    if(p) { size_t *sp=_ao(p,nitems*size);  while(--sp>_ao(p,0) ) *sp=0;
    }   return p;
}
void mem_free (void *addr)
{   Sfb *p, *q, *n=_fb(addr,-AA);  size_t sz;
    if(!(n->sz&1) )    return;
    merge(n,NULL,0);                            //a.
    if(sz=_fb(n,-AA)->sz+AA*2,  !(sz&1) )       //  vrije voorbuur
    {   get_from_freelist(p=_fb(n,-sz) ); merge(p,n,0);n=p; }
    if(sz=n->sz, q=_fb(n,sz+AA*2), !(q->sz&1) ) //  vrije achterbuur
    {   get_from_freelist(q); merge(n,q,0);     }
    add_to_freelist(n);                         //b.
}
void *mem_realloc (void *addr, size_t size)
{   if(!addr)   return mem_malloc(size);        //a.
    if(!size)   return mem_free(addr), NULL;    //b.
    Sfb *p, *n=_fb(addr,-AA);   size_t bpsz, nsz=n->sz & ~1;
    Sfb *q=_fb(n,nsz+AA*2);size_t netto=_nettosize(size);
    size_t bruto=_nettosize(size)+2*AA;         //2 beschermingshekken
    if(nsz>=bruto)                              //c.
    {   if(!(q->sz&1) )                         //  vrije opvolger
        {   p=split(n,netto);
            get_from_freelist(q); merge(p,q,0);
            add_to_freelist(p);
        } else if(nsz>bruto)                    //  vrij restblok
            goto spl;
        return addr;
    }                                           //d. groter blok nodig:
    bpsz=_fb(n,-AA)->sz+AA*2; if(bpsz&1) bpsz=0;//(niet-vrij)
    if(!(q->sz&1) )                             //  vrije opvolger
    {   if(netto<=nsz+q->sz+AA*2)
        {   get_from_freelist(q);   merge(n,q,1);   nsz=n->sz & ~1;
            if(nsz>bruto)
                goto spl;
            return addr;
        } else if(netto<=bpsz+nsz+q->sz+AA*2)   //icm. vrije voorganger:
        {   get_from_freelist(q);   merge(n,q,0);//     eerst achterkant doen
            goto ap;
        }
    } else if(netto<=bpsz+nsz)                  //  vrije voorganger
ap: {   p=_fb(n,-bpsz);
        get_from_freelist(p);   merge(p,n,1);
        mem_cpy(_ao(p,AA), addr, nsz); n=p;nsz=n->sz&~1;
        if(nsz>bruto)
spl:    {   q=split(n,netto);
            add_to_freelist(q);
        }
        merge(n,NULL,1);
        return _ao(n,AA);
    }
    if(p=mem_malloc(size) )                     //e.
    {   mem_cpy((size_t *)p, addr, nsz);
        return mem_free(addr), p;
    }   return NULL;                            //f.
}
#undef MEM
#undef _freelist_index
#undef _nettosize
#undef _fb
#undef _ao
#undef SZ
#undef AA
