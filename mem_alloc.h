/* lib > mem_alloc > mem_alloc.h
   ------------------------------------------ TAB-size 4, code page UTF-8 --
Achtergrond:
    Een programma in RAM heeft geheugenindeling: text data bss heap ... stack
        text = programmacode
        data = geïnitialiseerd geheugen
        bss = block started by symbol: 0-geïnitialiseerd voordat main() begint.
        heap ... stack = dynamische geheugenruimte
    
    Oude programmatuur gebruikte brk() ter definitie van een 0-geïnitialiseerd
    geheugenblok en sbrk() om de grootte aan te passen. Bij een microcontroller,
    zonder virtueel geheugen, is beheer van een vast bss-blok zinvoller.

Werking:
    Een beheerd geheugenblok heeft aan voor- en achterzijde een beschermingshek.
    Een hek bevat de (even) beheerde grootte en als lsb een "toegewezen"-bit.
    Een vrij (niet-toegewezen) blok maakt deel uit van een DLL-"freelist" en
    bevat 2 pointers *next en *prev.
    Daardoor is de minimaal toewijsbare grootte gelijk aan 2*sizeof(void *).
    Met de eis dat elk blok zowel voorganger als opvolger heeft, zijn er aan de
    geheugenuiteinden nog loze hekken met waarde 1: 0 Bytes, niet-beschikbaar.

Interface-beschrijving:
    void mem_init(void)
    a. Plaats loze hekken aan de uiteinden van het beheerde geheugen.
    b. Maak één geheugenblok dat hier exact tussen past, zet het in freelist[].

    void *mem_malloc(size_t sz)
    a. Zoek een blok p in freelist[] waarin sz past. Mislukking levert NULL.
    b. Haal het blok p uit freelist[].
    c. Bij genoeg overruimte voor een restblok, splits een vrij blok n af.
    d. Neem p in gebruik en geef het eerste bruikbare adres als resultaat.

    void *mem_calloc(size_t nitems, size_t itemsize)
    a. Indien mem_malloc(nitems*itemsize) lukt, vult het resultaatblok met 0.

    void mem_free(void *addr)
    a. Maak het blok vrij, en voeg samen met eventuele vrije voor-/achterbuur.
    b. Plaats het (aangepaste) blok in freelist[].

    void *mem_realloc(void *addr, size_t sz)
    a. NULL==addr gedraagt zicht als mem_malloc(sz) 
    b. 0==sz roept mem_free(addr) aan en levert NULL;
    c. Bij kleinere sz dan huidige netto-grootte wordt geprobeerd om het restant
        af te splitsen en eventueel met een vrije achterbuur samen te voegen.
        Anders wordt het blok gewoon teruggegeven.
    d. Bij grotere sz wordt gepoogd om het blok met een vrije achterbuur samen
        te voegen, eventueel met een vrije voorbuur erbij en verplaatsen van
        de inhoud. Het eerste bruikbare adres wordt teruggegeven.
    e. Anders wordt gekeken of mem_malloc() lukt en de inhoud
        verplaatst naar het nieuwe blok. Het nieuwe adres wordt teruggegeven.
    f. Anders wordt NULL teruggegeven.

Wijzigingen:
	RvL 17 feb 2020 + mem_size()
    RvL 12 feb 2020 github-commit, #ifndef ... #endif toegevoegd
    RvL 11 feb 2020 aanmaak
   -------------------------------------------------------------------------
*/
#ifndef _MEM_ALLOC_H_INCLUDED_
#define _MEM_ALLOC_H_INCLUDED_
#include <stddef.h>         //offsetof, size_t

    //algemene macro's:
#define _elementen(a)       ( sizeof(a)/sizeof(*(a)) )
#define _maxindex(a)        (_elementen(a)-1)
#define _maximaal(x,M)      if(x>(M) ) x=(M)

    /* ---------------------------------------------------------
    interface
    --------------------------------------------------------- */
void mem_init(void);
void *mem_malloc(size_t totalsize);
void *mem_calloc(size_t nitems, size_t itemsize);
void mem_free(void *addr);
void *mem_realloc(void *addr, size_t size);
size_t mem_size(void *addr);

#endif//_MEM_ALLOC_H_INCLUDED_