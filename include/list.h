
#include <assert.h>

typedef struct _slist_node_t* slist_node_t;
typedef struct _slist_t* slist_t;
typedef struct _dlist_node_t* dlist_node_t;
typedef struct _dlist_t* dlist_t;


struct _slist_node_t {
	slist_node_t next;
	word_t value;
};

struct _dlist_node_t {
	dlist_node_t prev;
	dlist_node_t next;
	word_t value;
};

struct _slist_t {
	slist_node_t head;
	slist_node_t tail;
};

struct _dlist_t {
	dlist_node_t head;
	dlist_node_t tail;
};


/*
 * Adding values
 */

#define _dnode_local_new(_n) dlist_node_t _n = malloc(sizeof(struct _dlist_node_t))
#define _snode_local_new(_n) slist_node_t _n = malloc(sizeof(struct _slist_node_t))

#define dnode_del free
#define snode_del free

#define dlist_insert_head_node(_l,_n)	do {\
		_n->next=(_l)->head;\
		_n->prev=NULL;\
		if((_l)->tail==NULL) {\
			(_l)->tail=_n;\
		} else {\
			(_l)->head->prev=_n;\
		}\
		(_l)->head=_n;\
	} while(0)

#define dlist_insert_head(_l,_v)	do {\
		_dnode_local_new(n);\
		n->value=(word_t)(_v);\
		dlist_insert_head_node(_l,n);\
	} while(0)

#define dlist_insert_tail_node(_l,_n)	do {\
		_n->next=NULL;\
		_n->prev=(_l)->tail;\
		if((_l)->head==NULL) {\
			(_l)->head=_n;\
		} else {\
			(_l)->tail->next=_n;\
		}\
		(_l)->tail=_n;\
	} while(0)

#define dlist_insert_tail(_l,_v)	do {\
		_dnode_local_new(n);\
		n->value=(word_t)(_v);\
		dlist_insert_tail_node(_l,n);\
	} while(0)

#define slist_insert_head(_l,_v)	do {\
		_snode_local_new(n);\
		n->value=(word_t)(_v);\
		n->next=(_l)->head;\
		if((_l)->tail==NULL) (_l)->tail=n;\
		(_l)->head=n;\
	} while(0)

#define slist_insert_tail(_l,_v)	do {\
		_snode_local_new(n);\
		n->value=(word_t)(_v);\
		n->next=NULL;\
		if((_l)->tail!=NULL) {\
			(_l)->tail->next=n;\
		} else {\
			(_l)->head=n;\
		}\
		(_l)->tail=n;\
	} while(0)

#define slist_remove_head(_l)	do {\
		slist_node_t n=(_l)->head;\
		(_l)->head = (_l)->head->next;\
		snode_del(n);\
	} while(0)

#define dlist_remove_head(_l)	do {\
		dlist_node_t n=(_l)->head;\
		(_l)->head = n->next;\
		(_l)->head->prev = NULL;\
		snode_del(n);\
	} while(0)

#define dlist_remove_tail(_l)	do {\
		dlist_node_t n=(_l)->tail;\
		(_l)->tail = n->prev;\
		(_l)->tail->next = NULL;\
		dnode_del(n);\
	} while(0)

#define dlist_remove(_l,_n)	do {\
		if((_n)->next!=NULL) {\
			(_n)->next->prev=(_n)->prev;\
		} else {\
			assert((_l)->tail==(_n));\
			(_l)->tail=(_l)->tail->prev;\
		}\
		if((_n)->prev!=NULL) {\
			(_n)->prev->next=(_n)->next;\
		} else {\
			assert((_l)->head==(_n));\
			(_l)->head=(_l)->head->next;\
		}\
		dnode_del(_n);\
	} while(0)
/*
 * Iterating over a list
 */

#define list_head(_l) ((_l)->head)
#define list_tail(_l) ((_l)->tail)

#define node_next(_n) ((_n)->next)
/* No need to check type here.
 * The compiler will whine explicitly if prev isn't defined. */
#define node_prev(_n) ((_n)->prev)

/*
 * Accessing a node's value
 */

#define node_value(_t,_n) ((_t)((_n)->value))

/*
 * Verifying conditions
 */

#define list_is_empty(_l)	((_l)->head==NULL)
#define list_not_empty(_l)	((_l)->head!=NULL)
#define node_has_next(_n)	((_n)->next!=NULL)
#define node_has_prev(_n)	((_n)->prev!=NULL)


/*
 * Typical algorithms
 */

#define slist_forward(_l,_t,_f)	do {\
		slist_node_t n=list_head(_l);\
		while(n!=NULL) {\
			_f(node_value(_t,n));\
			n=n->next;\
		}\
	} while(0)


#define dlist_forward(_l,_t,_f)	do {\
		dlist_node_t n=list_head(_l);\
		while(n!=NULL) {\
			_f(node_value(_t,n));\
			n=n->next;\
		}\
	} while(0)

#define dlist_reverse(_l,_t,_f)	do {\
		dlist_node_t n=list_tail(_l);\
		while(n!=NULL) {\
			_f(node_value(_t,n));\
			n=n->prev;\
		}\
	} while(0)

#define slist_flush(_l)	do {\
		slist_node_t q,n=list_head(_l);\
		while(n!=NULL) {\
			q=n->next;\
			free(n);\
			n=q;\
		}\
	} while(0)

#define dlist_flush(_l)	do {\
		dlist_node_t q,n=list_head(_l);\
		while(n!=NULL) {\
			q=n->next;\
			free(n);\
			n=q;\
		}\
	} while(0)

#define slist_insert_sorted(_l,_n,_cmp)	do {\
		slist_node_t r=(_l)->head;\
		if(r==NULL) {\
			dlist_insert_head(_l,_n);\
		} else if(_cmp((_l)->tail,_n)<=0) {\
			dlist_insert_tail(_l,_n);\
		} else {\
			while(_cmp(_n,r)<0) {\
				r=r->next;\
			}\
			(_n)->next=r->next;\
			if(r->next==NULL) {\
				(_l)->tail=(_n);\
			}\
			r->next=(_n);\
		}\
	} while(0)

#define dlist_insert_sorted(_l,_n,_cmp)	do {\
		dlist_node_t r=(_l)->head;\
		if(r==NULL) {\
			dlist_insert_head_node(_l,_n);\
		} else if(_cmp((_l)->tail,_n)<=0) {\
			dlist_insert_tail_node(_l,_n);\
		} else {\
			while(_cmp(_n,r)<0) {\
				r=r->next;\
			}\
			(_n)->next=r->next;\
			(_n)->prev=r;\
			if(r->next==NULL) {\
				(_l)->tail=(_n);\
			} else {\
				r->next->prev=(_n);\
			}\
			r->next=(_n);\
		}\
	} while(0)


/*
 * Creating and destroying Lists
 */

inline void slist_init(slist_t);
inline void dlist_init(dlist_t); 
slist_t slist_new();
void slist_del(slist_t);
dlist_t dlist_new();
void dlist_del(dlist_t);



