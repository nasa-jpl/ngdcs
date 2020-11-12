/* Copyright 2013, by the California Institute of Technology.  ALL RIGHTS
   RESERVED.  United States Government Sponsorship acknowledged.  Any 
   commercial use must be negotiated with the Office of Technology Transfer
   at the California Institute of Technology.

   This software may be subject to U.S. export control laws.  By accepting
   this software, the user agrees to comply with all applicable U.S. export
   laws and regulations.  User has the responsibility to obtain export 
   licenses, or other export authority as may be required before exporting
   such information to foreign countries or providing access to foreign
   persons.

   Please do not redistribute this file separate from the NGDCS source
   distribution.  For questions regarding this software, please contact the
   author, Alan Mazer, alan.s.mazer@jpl.nasa.gov */


#include <iostream>
#if defined(WIN32)
#pragma warning( disable : 4290 )
#endif

template <class T> class SL_ListWatch;

template <class T> class SL_List {
protected:
    T *items;
    int max_items;
    int current_items;
    int *scan_pts;
    SL_ListWatch<T> **watches;
    int current_scan;
public:
    int failed;

	// constructor

    SL_List();
    void clear(void) throw();
    void describe(char *notation) throw();

	// augmentation

    T add(T item) throw(std::bad_alloc);
    T add(T item,void (*trace_func)(T p,int length)) throw(std::bad_alloc);

	// scanning initialization -- requires scan_done() below

    int init_scan(SL_ListWatch<T> *watch) throw();

	// stepping (matching function optional)

    inline T next(void) throw();
    T next(int (*func)(T p)) throw();
    T next(int (*func)(T p,void *p1),void *p1) throw();
    T next(int (*func)(T p,void *p1,void *p2),void *p1,void *p2) throw();
    T previous(void) throw();

        // delete current or previous item

    T remove_current(void) throw();
    T remove_current(void (*trace_func)(T p,int length)) throw();
    T remove_previous(void) throw();

        // scanning cleanup

    int scan_done(void) throw();

        // find first item (matching function optional)

    T find_first(void) throw();
    T find_first(int (*func)(T p)) throw();
    T find_first(int (*func)(T p,void *p1),void *p1) throw();
    T find_first(int (*func)(T p,void *p1,void *p2),void *p1,void *p2) throw();

	// nested-list processing

    T find_two(int (*func1)(T p1), int (*func2)(T p1, T p2), T *other) throw();

	// remove and return first item (matching function optional)

    T remove_first(void) throw();
    T remove_first(int (*func)(T p)) throw();
    T remove_first(int (*func)(T p,void *p1),void *p1) throw();
    T remove_first(int (*func)(T p,void *p1,void *p2),void *p1,void *p2)
	throw();
    T remove_first(void (*trace_func)(T p,int length)) throw();
    T remove_first(int (*func)(T p),void (*trace_func)(T p,int length)) throw();
    T remove_first(int (*func)(T p,void *p1),void *p1,
	void (*trace_func)(T p,int length)) throw();
    T remove_first(int (*func)(T p,void *p1,void *p2),void *p1,void *p2,
	void (*trace_func)(T p,int length)) throw();

	// remove and return last item */

    T remove_last(void) throw();

	// add or replace item (matching function optional)

    T add_replace(T p) throw(std::bad_alloc);
    T add_replace(T p,int (*func)(T orig,T p),
	void (*func2)(T orig,T p)) throw(std::bad_alloc);
    T add_replace(T p,int (*func)(T orig,T p,void *p1),void *p1,
	void (*func2)(T orig,T p)) throw(std::bad_alloc);
    T add_replace(T p,int (*func)(T orig,T p,void *p1,void *p2),void *p1,
	    void *p2,
	void (*func2)(T orig,T p)) throw(std::bad_alloc);

        // insert item (placement-determination function optional)

    T insert(T p) throw(std::bad_alloc);
    T insert(T p,int (*func)(T orig,T p)) throw(std::bad_alloc);
    T insert(T p,int (*func)(T orig,T p,void *p1),void *p1)
	throw(std::bad_alloc);
    T insert(T p,int (*func)(T orig,T p,void *p1,void *p2),void *p1,void *p2)
	throw(std::bad_alloc);
    T insert(T p,void (*trace_func)(T p,int length)) throw(std::bad_alloc);
    T insert(T p,int (*func)(T orig,T p),void (*trace_func)(T p,int length))
	throw(std::bad_alloc);
    T insert(T p,int (*func)(T orig,T p,void *p1),void *p1,
	void (*trace_func)(T p,int length)) throw(std::bad_alloc);
    T insert(T p,int (*func)(T orig,T p,void *p1,void *p2),void *p1,void *p2,
	void (*trace_func)(T p,int length)) throw(std::bad_alloc);
    T insert_before_current(T p) throw(std::bad_alloc);

	// parameter retrieval

    int get_length(void) const throw() { return(current_items); }
    T *get_array(void);
    T get_nth(int n) throw();

	// destructor

    ~SL_List() throw();
};

template <class T> class SL_ListWatch {
private:
    SL_List<T> *list;
public:

	// constructor

    SL_ListWatch() throw();

	// misc

    void set_list(SL_List<T> *l) throw();

	// destructor

    ~SL_ListWatch() throw();
};


#define MAX_SCAN_PTS	4

#define PREPARE_TO_EXPAND_LIST(T) { /*lint --e{668} */ \
    if (current_items == max_items) { T *new_items; \
	max_items *= 2; \
	new_items = new T[(unsigned)max_items]; \
	(void)memcpy((void *)new_items,(void *)items, \
	    ((unsigned)current_items * sizeof(T))); \
	delete[] items; items = new_items; }}

#define ADD_TO_LIST(T,p) { /*lint --e{661,662} */ \
    PREPARE_TO_EXPAND_LIST(T) \
    *(items + current_items) = (T)(p); current_items++; failed = 0; }

#define INSERT(T,p,index) { int insert_i; \
    PREPARE_TO_EXPAND_LIST(T) \
    for (insert_i=current_items-1;insert_i >= (index);insert_i--) \
	items[insert_i+1] = items[insert_i]; \
    items[(index)] = (p); \
    current_items++; \
    for (insert_i=0;insert_i <= current_scan;insert_i++) \
	if (scan_pts[insert_i]-1 >= (index)) scan_pts[insert_i]++; \
    failed = 0; }

#define REMOVE(T,index,p) { int remove_i; \
    if (current_items == 0 || (index) < 0 || (index) >= current_items) { \
	(void)fprintf(stderr,"REMOVE: Internal error\n"); \
	failed = 1; return((T)NULL); } \
    p = items[index]; \
    for (remove_i=(index)+1;remove_i < current_items;remove_i++) \
	items[remove_i-1] = items[remove_i]; \
    current_items--; \
    for (remove_i=0;remove_i <= current_scan;remove_i++) \
	if (scan_pts[remove_i]-1 >= (index)) scan_pts[remove_i]--; \
    failed = 0; }

#define INIT(T) failed = 0; if (current_items == 0) return((T)NULL); int sp = 0;

#if defined(DEBUG)
#define CHECK_INTERNALS(T) { \
    if (current_scan < 0 || scan_pts[current_scan] < 0) { \
	(void)fprintf(stderr,"CHECK_INTERNALS: Internal scanning error\n"); \
	failed = 1; return((T)NULL); } \
    else failed = 0; }
#else
#define CHECK_INTERNALS(T)
#endif

#define RETURN_SCAN_PT(T) { \
    if (scan_pts[current_scan] == current_items) { \
	if (watches[current_scan]) watches[current_scan]->set_list(NULL); \
	current_scan--; return((T)NULL); } \
    else return((T)items[scan_pts[current_scan]++]); }

#define BACKUP(T) { \
    if (scan_pts[current_scan] < 2) return((T)NULL); \
    else { \
	scan_pts[current_scan]--; \
	return((T)items[scan_pts[current_scan]-1]); } }

#define RETURN_FIRST(T) { \
    if (sp == current_items) return((T)NULL); \
    else return((T)items[sp]); }

#include "lists.cc"
