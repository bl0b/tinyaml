#include "priority_queue.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <values.h>

//#define NDEBUG
#include <assert.h>

#include <pthread.h>

/* small include hack : use output redictection by VM instead of plain printf */
int vm_printf(const char* fmt, ...);

#define FLOAT_IMPREC .000001f /* to prevent floating-point roundoff errors */



#define YEAR_HASH 241	/* some arbitrary prime (incidentally, sizeof(PQ) without years[] is 15 words */


/**************************************************************************************************
 ************************************** TYPE DEFINITIONS ******************************************
 *************************************************************************************************/


/*
 * an event
 *
 * priority queue atom
 */
struct _pq_ev {
	struct _pq_ev*prev,*next;
	PQTime date;
	struct _pq_msg*head,*tail;
	int count;
};



/*
 * a bucket
 *
 * manages an event sublist
 */
struct _pq_bucket {
	int count;			/* number of events in bucket */
	struct _pq_ev*head,*tail;	/* first and last event in bucket */
	struct _pq_bucket*prev,*next;	/* prev and next buckets in year */
	struct _pq_year*list;		/* year storage */
	struct _pq_bst_node*node;	/* the node that references this bucket */
};



/*
 * a binary search tree node
 *
 * BST is for finding the correct bucket knowing the date when inserting a message
 */
struct _pq_bst_node {
	PQTime T;			/* start date */
	PQTime width;			/* dichotomy interval */
	struct _pq_bst_node*left,*right;/* left and right sons in BST */
	struct _pq_bst_node*parent;	/* the node's parent */
	struct _pq_bucket*bucket;	/* the corresponding bucket if node is a leaf */
	int balance;
	int height;
};



/*
 * a year
 *
 * splits queue complexity in arbitrary time units
 */
struct _pq_year {
	struct _pq_year*hNext;		/* for use in hashtab */
	struct _pq_year*prev,*next;	/* for sorting years */
	struct _priority_queue_t*list;	/* for sorting years */
	int number;			/* year # */
	int count;			/* events count */
	struct _pq_bucket*head,*tail;	/* first and last buckets in year */
	struct _pq_bst_node*root;	/* root node in BST */
//	struct _pq_bucket*firstNotEmpty;/* first bucket that contains an event */
};

struct _priority_queue_t {
	/* internal time representation */
	PQTime D;		/* year length */
	PQTime invD;		/* year length */
	PQTime startDate;
	PQTime endDate;
	/* struct constants */
	PQTime minWidth;
	int bucketMax;
	/* forward/reverse reading tools */
	/* enqueue tools */
	struct _pq_ev*firstEv;
	struct _pq_ev*lastEv;
	int evCount;
	int msgCount;
	/* years storage */
	struct _pq_year*head,*tail;
	int count;
	struct _pq_year*years[YEAR_HASH];
	void(*destroy_msg)(PQMessage);
	pthread_mutex_t mutex;
};



struct _pq_iterator_t {
	PQueue q;
	PQMessage cacheMsg;
	struct _pq_ev*cacheEv;
	struct _pq_bucket*cacheBucket;
	struct _pq_year*cacheYear;
	PQTime presentDate;
};

/**************************************************************************************************
 ****************************************** DEBUGGING *********************************************
 *************************************************************************************************/



void dump_year(struct _pq_year*y,const char*prefix);
void dump_bucket(struct _pq_bucket*y,const char*prefix);
void dump_ev(struct _pq_ev*e,const char*prefix);
void dump_node(struct _pq_bst_node*n,const char*prefix);
void dump_queue(PQueue q);



/**************************************************************************************************
 *************************************** TREE MANAGEMENT ******************************************
 *************************************************************************************************/



#define setR(__p,__n) do { __p->right=__n; __n->parent=__p; } while(0)
#define setL(__p,__n) do { __p->left=__n; __n->parent=__p; } while(0)
void balance_tree(struct _pq_year* y,struct _pq_bst_node*n);

#define max(_a,_b) ((_a)>(_b)?(_a):(_b))

#define computeBalance(__x) do { __x->balance=__x->right->height-__x->left->height; } while(0)
#define computeHeight(__x) do { if(!__x->bucket) __x->height=1+max(__x->right->height,__x->left->height); } while(0)

#define assert_all(_X) do { assert(_X); assert(!_X->bucket); assert(_X->left); assert(_X->right); } while(0)


static inline void rebalanceLL(struct _pq_year* y,struct _pq_bst_node*C) {
	struct _pq_bst_node
		*B=C->left,
		*A=B->left,
		*BR=B->right,
		*O=C->parent;

	/*vm_printf(">>>>>>>>>>>>>>> BEFORE\n"); dump_node(C,"");*/
	
	assert_all(B);
	assert_all(C);
	assert_all(A);

	setR(B,C);
	setL(C,BR);
	computeHeight(C);
//	computeHeight(B);
	computeBalance(C);
//	computeBalance(B);
	if(O) {
		if(O->right==C)
			setR(O,B);
		else
			setL(O,B);
		/*do {*/
			/*computeHeight(O);*/
			/*O=O->parent;*/
		/*} while(O);*/
	} else {
		y->root=B;
		B->parent=NULL;
	}
	assert_all(B);
	assert_all(C);
	assert_all(A);

	/*vm_printf(">>>>>>>>>>>>>>> AFTER\n"); dump_node(B,"");*/
	if(B->parent) balance_tree(y,B);
}



static inline void rebalanceLR(struct _pq_year* y,struct _pq_bst_node*C) {
	struct _pq_bst_node
		*A=C->left,
		*B=A->right,
		*BL=B->left,
		*BR=B->right,
		*O=C->parent;

	/*vm_printf(">>>>>>>>>>>>>>> BEFORE\n"); dump_node(C,"");*/
	
	assert_all(B);
	assert_all(C);
	assert_all(A);

	setR(B,C);
	setL(B,A);
	setL(C,BR);
	setR(A,BL);
	computeHeight(A);
	computeHeight(C);
//	computeHeight(B);
	computeBalance(A);
	computeBalance(C);
//	computeBalance(B);
	if(O) {
		if(O->right==C)
			setR(O,B);
		else
			setL(O,B);
		/*do {*/
			/*computeHeight(O);*/
			/*O=O->parent;*/
		/*} while(O);*/
	} else {
		y->root=B;
		B->parent=NULL;
	}
	assert_all(B);
	assert_all(C);
	assert_all(A);

	/*vm_printf(">>>>>>>>>>>>>>> AFTER\n"); dump_node(B,"");*/
	if(B->parent) balance_tree(y,B);
}





static inline void rebalanceRR(struct _pq_year* y,struct _pq_bst_node*C) {
	struct _pq_bst_node
		*B=C->right,
		*A=B->right,
		*BR=B->left,
		*O=C->parent;

	/*vm_printf(">>>>>>>>>>>>>>> BEFORE\n"); dump_node(C,"");*/
	
	assert_all(B);
	assert_all(C);
	assert_all(A);

	setL(B,C);
	setR(C,BR);
	computeHeight(C);
//	computeHeight(B);
	computeBalance(C);
//	computeBalance(B);
	if(O) {
		if(O->right==C)
			setR(O,B);
		else
			setL(O,B);
		/*do {*/
			/*computeHeight(O);*/
			/*O=O->parent;*/
		/*} while(O);*/
	} else {
		y->root=B;
		B->parent=NULL;
	}
	assert_all(B);
	assert_all(C);
	assert_all(A);

	/*vm_printf(">>>>>>>>>>>>>>> AFTER\n"); dump_node(B,"");*/
	if(B->parent) balance_tree(y,B);
}


static inline void rebalanceRL(struct _pq_year* y,struct _pq_bst_node*C) {
	struct _pq_bst_node
		*A=C->right,
		*B=A->left,
		*BR=B->right,
		*BL=B->left,
		*O=C->parent;

	/*vm_printf(">>>>>>>>>>>>>>> rebalance RL  BEFORE\n"); dump_node(C,"");*/
	
	assert_all(A);
	assert_all(B);
	assert_all(C);
	
	setL(B,C);
	setR(B,A);
	setR(C,BL);
	setL(A,BR);
	computeHeight(A);
	computeHeight(C);
//	computeHeight(B);
	computeBalance(A);
	computeBalance(C);
//	computeBalance(B);
	if(O) {
		if(O->right==C)
			setR(O,B);
		else
			setL(O,B);
		/*do {*/
			/*computeHeight(O);*/
			/*O=O->parent;*/
		/*} while(O);*/
	} else {
		y->root=B;
		B->parent=NULL;
	}
	assert_all(B);
	assert_all(C);
	assert_all(A);

	/*vm_printf(">>>>>>>>>>>>>>> rebalance RL  AFTER\n"); dump_node(B,"");*/
	if(B->parent) balance_tree(y,B);
}











void balance_tree(struct _pq_year* y,struct _pq_bst_node*n) {
	struct _pq_bst_node*h;
	/* propagate height */
	h=n->parent;
	computeHeight(h->left);
	computeHeight(h->right);
	/*computeBalance(h->left);*/
	/*computeBalance(h->right);*/
	computeBalance(h);
	computeHeight(h);
	
	if(h->balance>=2) {
		if(h->right->balance>=1) {
			rebalanceRR(y,h);
		} else {
			rebalanceRL(y,h);
		}
	} else if(h->balance<=-2) {
		if(h->left->balance<=-1) {
			rebalanceLL(y,h);
		} else {
			rebalanceLR(y,h);
		}
	}
}



/**************************************************************************************************
 ***************************************** ALLOCATIONS ********************************************
 *************************************************************************************************/

static inline void add_msg(struct _pq_ev*e,PQMessage msg) {
	msg->next=NULL;
	msg->date=e->date;
	e->tail->next=msg;
	e->tail=msg;
	e->count++;
}

static inline struct _pq_ev* new_ev(PQueue q,PQTime date,PQMessage msg) {
	struct _pq_ev* e=_alloc(struct _pq_ev);
	msg->date=date;
	e->date=date;
	e->head=msg;
	e->tail=msg;
	msg->next=NULL;
	e->count=1;
	q->evCount++;
	return e;
}

static inline struct _pq_bucket* new_bucket(struct _pq_bst_node*node) {
	struct _pq_bucket*b=_alloc(struct _pq_bucket);
	b->head=b->tail=NULL;
	b->node=node;
	b->count=0;
	node->height=1;
	node->balance=0;
	return b;
}

static inline void hash_insert(PQueue q,int d,struct _pq_year*y) {
	struct _pq_year*z,*w;
	z=q->years[d];
	w=NULL;//(struct _pq_year*)&(q->years[d]);
	while(z&&z->number<d) { w=z; z=z->hNext; }		/* should be amortized O(1), anyway it simplifies ordered access */
	if(w)
		w->hNext=y;
	else
		q->years[d]=y;
	y->hNext=z;
}

static inline void list_insert(PQueue q,struct _pq_year*y) {
	struct _pq_year*z;
	if(q->tail&&y->number>q->tail->number) {
//		vm_printf("new tail year with # %i (previous was %i)\n",y->number,q->tail->number);
		y->prev=q->tail;
		y->next=NULL;
		y->list=q;
		q->tail->next=y;
		q->tail=y;
		q->count++;
	} else {
		if(q->head) {
			z=q->head;
			while(z&&z->number<y->number) {
				z=z->next;
			}
			if(z==q->head) {
				y->next=q->head;
				y->prev=NULL;
				y->list=q;
				q->head->prev=y;
				q->head=y;
				q->count++;
			} else {
				y->list=q;
				z->prev->next=y;
				y->prev=z->prev;
				y->next=z;
				z->prev=y;
				q->count++;
			}
		} else {
			q->head=q->tail=y;
			y->prev=y->next=NULL;
			q->count=1;
		}
	}
}

static inline struct _pq_year* new_year(PQueue q,int d) {
	struct _pq_year*y=_alloc(struct _pq_year);

	memset(y,0,sizeof(struct _pq_year));

	y->number=d;

	y->root=_alloc(struct _pq_bst_node);
	y->root->T=d*q->D;
	y->root->width=q->D;
	y->root->bucket=new_bucket(y->root);
	y->root->parent=NULL;
	y->root->bucket->prev=y->root->bucket->next=NULL;
	y->count=1;
	y->head=y->root->bucket;
	y->tail=y->root->bucket;

	d%=YEAR_HASH;
	hash_insert(q,d,y);
	/* now insert new year in sorted years list... painful :( */
	list_insert(q,y);
	/*vm_printf("new year with # %i\n",y->number);*/
	return y;
}




static inline void free_ev(void(*dm)(PQMessage),struct _pq_ev*e) {
	PQMessage m=e->head,n;
	while(m) {
		n=m;
		m=m->next;
		dm(n);
	}
	_free(struct _pq_ev,e);
}


static inline void free_bucket(void(*dm)(PQMessage),struct _pq_bucket*b) {
	struct _pq_ev*e=b->head,*f;
	while(e) {
		f=e;
		e=e->next;
		free_ev(dm,f);
	}
	_free(struct _pq_bucket,b);
}


static void free_node(void(*dm)(PQMessage),struct _pq_bst_node*n) {
	if(n->bucket) {
		free_bucket(dm,n->bucket);
	} else {
		free_node(dm,n->left);
		free_node(dm,n->right);
	}
	_free(struct _pq_bst_node,n);
}


static inline void free_year(void(*dm)(PQMessage),struct _pq_year*y) {
	free_node(dm,y->root);
	_free(struct _pq_year,y);
}



void remove_year(PQueue q,struct _pq_year*y) {
	struct _pq_year*hy,*hq;
	int d;
	/*vm_printf("remove_year\n");*/
	/*dump_year(y,"remove_year\t");*/
	/*fflush(stdout);*/
	if(q->head==y)
		q->head=y->next;
	if(q->tail==y)
		q->tail=y->prev;
	hq=NULL;
	d=y->number%YEAR_HASH;
	hy=q->years[d];
	/*while(hy&&hy->number!=y->number) {*/	// since this is a monotonic queue, y should always be first entry in hash index */
	while(hy&&hy!=y) {			// since this is a monotonic queue, y should always be first entry in hash index */
		hq=hy;
		hy=hy->hNext;
	}
	assert(hy);
	if(hq)
		hq->hNext=y->hNext;
	else
		q->years[d]=y->hNext;
	if(q->head==y) {
		q->head=y->next;
		if(q->head) {
			q->head->prev=NULL;
		}
	}
	if(q->tail==y) {
		q->tail=y->prev;
		if(q->tail) {
			q->tail->next=NULL;
		}
	}
	_free(struct _pq_year,y);
	q->count--;
}



void remove_bucket(PQueue q,struct _pq_year*y,struct _pq_bucket*b) {
	struct _pq_bst_node*l,*r,*p,*o;
	/*vm_printf("remove_bucket next=%p prev=%p node=%p nodeparent=%p\n",b->next,b->prev,b->node,b->node->parent);*/
	/*dump_bucket(b,"remove_bucket\t");*/
	if(y->head==b)
		y->head=b->next;
	if(y->tail==b)
		y->tail=b->prev;
	if(b->next) {
		b->next->prev=b->prev;
	} else {
		/*assert(!b->node->parent);*/
		/*remove_year(q,y);*/
	}
	if(b->prev)
		b->prev->next=b->next;
	y->count--;
	p=b->node->parent;
	if(p) {
		o=p->parent;
		/*dump_year(y,"remove_bucket year_before ");*/
		/*vm_printf("remove_bucket node has parent\n");*/
		l=p->left;
		r=p->right;
		if(l->bucket==b) {
			/*vm_printf("remove_bucket node is left ; keep right ; right => ( %i %i b=%p l=%p r=%p )\n",*/
					/*r->height,r->balance,*/
					/*r->bucket,r->left,r->right*/
				/*);*/
			r->T=p->T;
			r->width=p->width;
			if(o) {
				/*dump_node(o,"remove_bucket parent \t");*/
				if(o->left==p) setL(o,r); else setR(o,r);
				if(o->parent) balance_tree(y,o);
			} else {
				y->root=r;
				r->parent=NULL;
			}
			_free(struct _pq_bst_node,l);
		} else {
			/*vm_printf("remove_bucket node is right ; keep left ; left => ( %i %i b=%p l=%p r=%p )\n",*/
					/*l->height,l->balance,*/
					/*l->bucket,l->left,r->right*/
				/*);*/
			l->T=p->T;
			l->width=p->width;
			if(o) {
				/*dump_node(p,"remove_bucket parent \t");*/
				if(o->left==p) setL(o,l); else setR(o,l);
				if(o->parent) balance_tree(y,o);
			} else {
				y->root=l;
				l->parent=NULL;
			}
			_free(struct _pq_bst_node,r);
		}
		_free(struct _pq_bst_node,p);
		y->count--;
		if(!y->head) {
			/*vm_printf("remove_bucket => REMOVE YEAR #%i\n",y->number);*/
			remove_year(q,y);
		} else {
			/*dump_year(y,"remove_bucket year_after ");*/
		}
	} else {	/* node is root, we are deleting the last bucket in a year */
		/*vm_printf("remove_bucket REMOVING YEAR %i\n",y->number);*/
		remove_year(q,y);
	}
	_free(struct _pq_bucket,b);
}




void remove_ev(PQueue q,struct _pq_year*y,struct _pq_bucket*b,struct _pq_ev*e) {
	if(e==b->head) {
		b->head=e->next;
		if(!b->head) {
			remove_bucket(q,y,b);
		} else {
			b->head->prev=NULL;
			b->count-=1;
		}
	} else {
		if(e==b->tail) b->tail=e->prev;
		else e->next->prev=e->prev;
		if(e->prev) e->prev->next=e->next;
	}
	q->evCount--;
	/*vm_printf("after remove_ev, q->head->head->head->head = %p->%p->%p->%p\n",q->head,q->head?q->head->head:0,q->head&&q->head->head?q->head->head->head:0,q->head&&q->head->head&&q->head->head->head?q->head->head->head->head:0);*/
}



/**************************************************************************************************
 ****************************************** ACCESSORS *********************************************
 *************************************************************************************************/

#define _find_bucket(_b,_y,_dt) do {	register struct _pq_bst_node*n;\
					for(n=_y->root;!n->bucket;n = _dt < n->T ? n->left : n->right);\
					_b=n->bucket;\
				} while(0)

static inline struct _pq_bucket* find_bucket(struct _pq_year*year,float_t dt) {
	/* there is always a root */
	register struct _pq_bst_node*n=year->root;
	/* left and right children are always defined if they exist, otherwise bucket is defined */
	//if(!n->bucket) do n = dt < n->T ? n->left : n->right; while(!n->bucket);
	do {
		if(n->bucket) return n->bucket;
		n = dt < n->T ? n->left : n->right;
	} while(1);
	/* when we reach a leaf, we have found the bucket */
	//return n->bucket;
}



static inline struct _pq_year* find_year(PQueue q,int y) {
	register struct _pq_year*year=q->years[y%YEAR_HASH];
	if(year) while(year->number!=y&&(year=year->next)); /* unbounded ; should be generally small ; smaller year numbers will be found more quickly than bigger ones */
	return year;
}




/**************************************************************************************************
 ************************************* BUCKET MANAGEMENT ******************************************
 *************************************************************************************************/



PQTime split_bucket(struct _pq_year*year,struct _pq_bucket**bucket) {
	struct _pq_bucket
		*b1=*bucket,
		*b2,
		*bNext=(*bucket)->next,
		*bPrev=(*bucket)->prev;
	struct _pq_bst_node
		*n=(*bucket)->node,
		*l,
		*r,
		*tmp;
	struct _pq_ev*k,*e;
	/*PQTime W=n->width/2;*/
	/*PQTime T=n->T+W;*/
	PQTime Wl,Wr,Tl,Tr,tot=0;
	int delta,half=((*bucket)->count>>1);

	/* assert count==real thing */
	/*{*/
		/*int n;*/
		/*k=(*bucket)->head;*/
		/*for(n=0;k;k=k->next,n+=1);*/
		/*if(n!=(*bucket)->count) {*/
			/*vm_printf("## BUGGY BUCKET ## counted %i, bucket says %i\n",n,(*bucket)->count);*/
			/*dump_bucket(*bucket,"  BUGGY BUCKET  ");*/
		/*}*/
	/*}*/

	k = (*bucket)->head;
	for(delta=0;delta<half;delta+=1) {
		k=k->next;
	}
	/*vm_printf("split bucket, n=%i, nl=%i,nr=%i\n",(*bucket)->count,delta,(*bucket)->count-delta);*/
	/* point to previous, i.e. last event in 1st bucket */
	Wl = k->date-n->T;
	Wr = n->width-Wl;
	Tl=n->T;
	Tr=k->date;

	if(k) {
		k=k->prev;
	}
	/*while(k) {*/
		/*tot += k->date;*/
		/*k=k->next;*/
		/*delta+=1;*/
	/*}*/
	/*tot *= 1.0f/delta;*/
	/*Wl = tot-n->T;*/
//	vm_printerrf"bucket before = %p, b2 = %p\n",*bucket,b2);
	
	/*k=(*bucket)->tail;*/
	/*delta=0;*/
	/*while(k&&k->date>=Tr) {*/
		/*++delta;*/
		/*k=k->prev;*/
		/* at most BUCKETMAX operations in ideal case, unbounded because of constraints on delta and dichotomy_max (FIXME : have dichotomy_max disappear now the buckets won't spawn empty ?) */
	/*}*/


	if(!(k&&delta)) {
		vm_printf("delta=%i k=%p : T=%f W=%f : Tl=%f Wl=%f : Tr=%f Wr=%f\n",delta,k,n->T,n->width,Tl,Wl,Tr,Wr);
		vm_printf("head at %f and tail at %f\n",(*bucket)->head->date,(*bucket)->tail->date);
		dump_bucket(*bucket," can't split :: ");
		/*return 0;*/
		return n->T+n->width;
	}

	/* alloc and fill b2 */
	b2=_alloc(struct _pq_bucket);

	assert(k&&delta);
	l=_alloc(struct _pq_bst_node),
	r=_alloc(struct _pq_bst_node);

	e=k->next;

	e->prev=NULL;

	b2->head=e;
	b2->tail=b1->tail;

	k->next=NULL;
	b1->tail=k;
	b2->count=b1->count-delta;
	b1->count=delta;

	if(bNext)
		bNext->prev=b2;
	if(bPrev)
		bPrev->next=b1;
	b2->next=bNext;
	b2->prev=b1;
	b2->node=r;
	b1->next=b2;
	b1->prev=bPrev;
	b1->node=l;
	if(b2==year->head)
		year->head=b1;
	if(b1==year->tail)
		year->tail=b2;
	/* update original node, new nodes, and buckets */
	l->bucket=b1;
	l->parent=n;
	l->T=Tl;		/* first half of previous interval */
	l->width=Wl;
	l->height=1;
	l->balance=0;

			
	r->T=Tr;		/* second half of previous interval */
	r->width=Wr;
	r->bucket=b2;
	r->parent=n;
	r->height=1;
	r->balance=0;
	
	n->bucket=NULL;
	n->left=l;
	n->right=r;
	n->T=Tr;
	n->width=0;	/* n is no more an interval */
	year->count++;

	*bucket=b1;

	/*vm_printerrf"bucket after = %p, b2 = %p\n",*bucket,b2);*/
	/*vm_printerrf"bucket next = %p, prev = %p\n\n",(*bucket)->next,(*bucket)->prev);*/

	/* experimental (!?) */
	b1->head->prev=NULL;
	b1->tail->next=NULL;
	b2->head->prev=NULL;
	b2->tail->next=NULL;


	/*vm_printf("split bucket => nl=%i nr=%i\n",b1->count,b2->count);*/
	/*dump_bucket(b1,"split bucket   ");*/
	/*dump_bucket(b2,"split bucket   ");*/


	n->height=2;
	n->balance=0;
	tmp=n;
	while(tmp&&tmp->parent&&tmp->parent->height<(tmp->height+1)) {
		tmp->parent->height=tmp->height+1;
		balance_tree(year,tmp);
		tmp=tmp->parent;
	}
//	if(n->parent) {
		/*dump_year(year,"##   ");*/
		/*vm_printf("\n");*/
		if(n->parent) balance_tree(year,n);
//	}

	return Tr;
}




/**************************************************************************************************
 *************************************** CACHE MANAGEMENT *****************************************
 *************************************************************************************************/


static inline void cache_forward(PQIterator qi) {
	if(!qi->cacheMsg) {
		return;
	}
//	vm_printf("### advance cache from msg [%i] at date %f\n",q->cacheMsg->data,q->cacheMsg->date);
	if(qi->cacheMsg->next) {
		/* trivial case : next message in current event */
		qi->cacheMsg=qi->cacheMsg->next;
	} else if(qi->cacheEv->next) {
		/* there is an event after the cached one in the cached bucket */
		qi->cacheEv=qi->cacheEv->next;
		qi->cacheMsg=qi->cacheEv->head;
	} else if(qi->cacheBucket->next) {
		/* search next buckets in year */
		assert(!qi->cacheBucket->next||qi->cacheBucket->next->head);
		qi->cacheBucket=qi->cacheBucket->next;
		/* found an event in same year */
		qi->cacheEv=qi->cacheBucket->head;
		qi->cacheMsg=qi->cacheEv->head;
	} else if(qi->cacheYear->next) {
		/* search next years for an event */
		assert(!qi->cacheYear->next||qi->cacheYear->next->head);
		qi->cacheYear=qi->cacheYear->next;
//		vm_printf("found event in year #%i\n",q->cacheYear->number);
		assert(qi->cacheYear->head&&qi->cacheYear->head->head);
		qi->cacheBucket=qi->cacheYear->head;
		qi->cacheEv=qi->cacheBucket->head;
		qi->cacheMsg=qi->cacheEv->head;
//		vm_printf("cacheEv = %p\n",q->cacheEv);
//		vm_printf("cacheBucket = %p\n",q->cacheBucket);
//		vm_printf("cacheYear = %p\n",q->cacheYear);
	} else {
		qi->cacheMsg=NULL;
	}
}




static inline void cache_backward(PQIterator qi) {
	if(!qi->cacheMsg) {
		return;
	}
//	vm_printf("### advance cache from msg [%i] at date %f\n",q->cacheMsg->data,q->cacheMsg->date);
	if(qi->cacheMsg->next) {
		/* trivial case : next message in current event */
		/* this behaviour is *NOT* different from cache_forward, because of single chaining */
		qi->cacheMsg=qi->cacheMsg->next;
	} else if(qi->cacheEv->prev) {
		/* there is an event after the cached one in the cached bucket */
		qi->cacheEv=qi->cacheEv->prev;
		qi->cacheMsg=qi->cacheEv->head;
	} else if(qi->cacheBucket->prev) {
		/* search next buckets in year */
		assert(!qi->cacheBucket->prev||qi->cacheBucket->prev->head);
		qi->cacheBucket=qi->cacheBucket->prev;
		/* found an event in same year */
		qi->cacheEv=qi->cacheBucket->head;
		qi->cacheMsg=qi->cacheEv->head;
	} else if(qi->cacheYear->prev) {
		/* search next years for an event */
		assert(!qi->cacheYear->prev||qi->cacheYear->prev->head);
		qi->cacheYear=qi->cacheYear->prev;
//		vm_printf("found event in year #%i\n",q->cacheYear->number);
		assert(qi->cacheYear->head&&qi->cacheYear->head->head);
		qi->cacheBucket=qi->cacheYear->head;
		qi->cacheEv=qi->cacheBucket->head;
		qi->cacheMsg=qi->cacheEv->head;
//		vm_printf("cacheEv = %p\n",q->cacheEv);
//		vm_printf("cacheBucket = %p\n",q->cacheBucket);
//		vm_printf("cacheYear = %p\n",q->cacheYear);
	} else {
		qi->cacheMsg=NULL;
	}
}



static unsigned int max_find_k=0;

static inline struct _pq_ev* find_k(struct _pq_bucket*bucket,PQTime date) {
	register struct _pq_ev*k=bucket->tail;
	/*unsigned int debug=0;*/
	if(bucket->head->date>date) {
		return NULL;
	} else if(bucket->head->date==date) {
		return bucket->head;
	}
	while(k!=bucket->head&&k&&k->date>date) {
		k=k->prev;
		/*debug+=1;*/
	}
	if(k==bucket->head&&k->date>date) {
		k=NULL;
	}
	/*assert(debug<=bucket->count);*/
	/*if(debug>max_find_k) {*/
		/*vm_printf("long search inside bucket : %u of %u events\n",debug,bucket->count);*/
		/*max_find_k=debug;*/
	/*}*/
	return k;
}

static inline void bucket_insert2(PQueue q,struct _pq_bucket*bucket,struct _pq_msg*msg,PQTime date) {
	struct _pq_ev*e,*k=bucket->tail;

	k=find_k(bucket,date);
	if(!k) {
		e=new_ev(q,date,msg);
		e->prev=NULL;
		e->next=bucket->head;
		bucket->tail=e;
		bucket->count+=1;
	} else {
		if(k->date==date) {
			add_msg(k,msg);
		} else {
			e=new_ev(q,date,msg);
			e->next=k->next;
			e->prev=k;
			if(k->next) {
				k->next->prev=e;
			} else {
				bucket->tail=k;
			}
			k->next=e;
			bucket->count+=1;
		}
	}
}


static inline void bucket_insert(PQueue q,struct _pq_bucket*bucket,struct _pq_msg*msg,PQTime date) {
	struct _pq_ev*e;

	if(bucket->tail->date<date) {
		e=new_ev(q,date,msg);
		e->prev=bucket->tail;
		e->next=NULL;
		bucket->tail->next=e;
		bucket->tail=e;
		bucket->count++;
	} else if(bucket->tail->date==date) {
		add_msg(bucket->tail,msg);
	} else if(bucket->head->date>date) {
		e=new_ev(q,date,msg);
		e->next=bucket->head;
		e->prev=NULL;
		bucket->head->prev=e;
		bucket->head=e;
		bucket->count++;
	} else if(bucket->head->date==date) {
		add_msg(bucket->head,msg);
	} else {
		bucket_insert2(q,bucket,msg,date);
		/*bucket->count++;*/
	}
}


/**************************************************************************************************
 *************************************** PUBLIC METHODS *******************************************
 *************************************************************************************************/

int pqMsgCount(PQueue q) {
	return q->msgCount;
}

PQTime pqStartDate(PQueue q) {
	return q->startDate;
}

PQTime pqEndDate(PQueue q) {
	return q->endDate;
}

PQueue pqCreate(PQTime yearLength,int bucketmax,int dichotomymax,void(*dm)(PQMessage)) {
	int i,debug=0;
	PQueue q=(PQueue)malloc(sizeof(struct _priority_queue_t));
	memset(q,0,sizeof(struct _priority_queue_t));
	/*for(i=0;i<sizeof(struct _priority_queue_t);i++) {*/
		/*debug+=((char*)q)[i];*/
	/*}*/
	/*vm_printf("PQueue alloc'ed at %p. sum of bytes = %i\n",q,debug);*/
	q->startDate=MAXFLOAT;
	q->endDate=0;
	q->D=yearLength;
	q->bucketMax=bucketmax;
	q->minWidth=q->D/(1<<(dichotomymax-1));
	q->invD=1./q->D;
	q->destroy_msg=dm;
	pthread_mutex_init(&q->mutex,NULL);
	pthread_mutex_trylock(&q->mutex);
	pthread_mutex_unlock(&q->mutex);
	/*vm_printf("PQueue created.\n");*/
	return q;
}



void pqDestroy(PQueue q) {
	struct _pq_year*y=q->head,*d;
	pthread_mutex_lock(&q->mutex);
	/* destroy years */
	while(y) {
		d=y;
		y=y->next;
		free_year(q->destroy_msg,d);
	}
	pthread_mutex_unlock(&q->mutex);
	/* destroy q */
	free(q);
}




void pqEnqueue(PQueue q,PQTime date,PQMessage msg) {
	struct _pq_year*year;
	struct _pq_bucket*bucket;
	struct _pq_ev*e;
	int y=(int) (date*q->invD);
	PQTime T;
	
	pthread_mutex_lock(&q->mutex);
//	vm_printerrf"pqEnqueue(%f) => y=%i\n",date,y);
	/* find year */
	year=find_year(q,y);
	if(!year) year=new_year(q,y);
	/* find bucket */
	_find_bucket(bucket,year,date);
	assert((!bucket->head)||bucket->head->prev==NULL);
	assert((!bucket->tail)||bucket->tail->next==NULL);
//	bucket=find_bucket(year,date);
	/* insert into bucket */
	if(bucket->count) {
		bucket_insert(q,bucket,msg,date);
	} else {
		bucket->head=bucket->tail=e=new_ev(q,date,msg);
		e->prev=e->next=NULL;
		bucket->count=1;
	}
	/* if(bucket.count >= THRESHOLD, split bucket, select new bucket matching date */
	if(bucket->count>q->bucketMax&&bucket->node->width>q->minWidth) {
		//vm_printerrf"splitting bucket\n");
		T=split_bucket(year,&bucket);
		if(T&&date>=T) {		/* select correct sub-bucket */
//			//vm_printerrf"next bucket\n");
			bucket=bucket->next;
		}
	}
	if((!year->head)||year->head->node->T>bucket->node->T) {
		year->head=bucket;
	}
	if(q->startDate>date)
		q->startDate=date;
	if(q->endDate<date)
		q->endDate=date;
	msg->date=date;
	q->msgCount++;
	pthread_mutex_unlock(&q->mutex);
}



/* pqDequeue will iteratively dequeue messages whose timestamp is inferior or equal
 * to date. One can use MAXFLOAT to dequeue whole queue.
 */

PQMessage pqDequeue(PQueue q,PQTime date) {
	PQMessage ret=NULL;
	struct _pq_year*y;
	struct _pq_ev*e;
	struct _pq_bucket*b;
	// if there is a year, there is an event
	if(q->head&&q->head->head->head->date<=date) {
		y=q->head;
		/*assert(y!=NULL);*/
		b=y->head;
		/*assert(b!=NULL);*/
		e=b->head;
		/*assert(e!=NULL);*/
		ret=e->head;
		/*assert(e->head!=NULL);*/
		if(ret->next)
			e->head=ret->next;
		else
			remove_ev(q,y,b,e);
		q->msgCount--;
	}
	return ret;
}




/* pqRemove will remove a given *message* at a given date from queue
 */

void pqRemove(PQueue q,PQTime date,PQMessage msg) {
	PQMessage m,n;
	int yi;
	struct _pq_year*y;
	struct _pq_bucket*b;
	struct _pq_ev*e;
	//y=(int)(date*q->invD)-1;
	yi=(int)(date*q->invD);
	if((!q->tail)||yi>q->tail->number) {
		return;
	}

	pthread_mutex_lock(&q->mutex);
//	do {

	y=find_year(q,yi);
	if(y) {
		//b=find_bucket(q->cacheYear,date);
		_find_bucket(b,y,date);
		if(b->head) {
			e=b->head;
			while(e&&e->date<date) e=e->next;
			if(e) {
				if(e->head==msg) {
					e->head=msg->next;
					if(!e->head)
						remove_ev(q,y,b,e);
				} else {
					m=e->head;
					n=m->next;
					while(n&&n!=msg) {
						m=n;
						n=n->next;
					}
					if(n) {
						m->next=n->next;
						if(n==e->tail) e->tail=m;
					}
					
				}
			}
		}
	}
	pthread_mutex_unlock(&q->mutex);
}





/**************************************************************************************************
 ************************************ ITERATOR MANAGEMENT *****************************************
 *************************************************************************************************/





/* pqJumpTo will init cache data to first message at or after *date*
 */

void pqiJumpTo(PQIterator qi,PQTime date) {
	int y;
	struct _pq_bucket*b;
	struct _pq_ev*e;
	//y=(int)(date*q->invD)-1;
	y=(int)(date*qi->q->invD);
	if((!qi->q->tail)||y>qi->q->tail->number) {
		qi->cacheMsg=NULL;
		qi->cacheEv=NULL;
		qi->cacheBucket=NULL;
		qi->cacheYear=NULL;
		qi->presentDate=qi->q->endDate;
		return;
	}

	pthread_mutex_lock(&qi->q->mutex);
	qi->cacheMsg=NULL;
//	do {

	qi->cacheYear=find_year(qi->q,y);
	if(qi->cacheYear) {
		//b=find_bucket(q->cacheYear,date);
		_find_bucket(b,qi->cacheYear,date);
		if(b->head) {
			e=b->head;
			while(e&&e->date<date) e=e->next;
			if(e) {
				qi->cacheMsg=e->head;
				qi->cacheEv=e;
				qi->cacheBucket=b;
				pthread_mutex_unlock(&qi->q->mutex);
				return;
			}
		}
		
		b=b->next;
		while(b&&!b->head) b=b->next;
		if(b) {
			qi->cacheMsg=b->head->head;
			qi->cacheEv=b->head;
			qi->cacheBucket=b;
			pthread_mutex_unlock(&qi->q->mutex);
			return;
		}
	} else {
		qi->cacheYear=qi->q->head;
		while(qi->cacheYear&&qi->cacheYear->number<y)
			qi->cacheYear=qi->cacheYear->next;
	}
/*	while(q->cacheYear&&!q->cacheYear->firstNotEmpty) {
		q->cacheYear=q->cacheYear->next;
	}*/
	if(qi->cacheYear) {
		qi->cacheBucket=qi->cacheYear->head;
		qi->cacheEv=qi->cacheBucket->head;
		qi->cacheMsg=qi->cacheEv->head;
		pthread_mutex_unlock(&qi->q->mutex);
		return;
	}
	/* still found no event, jump to end of tape */
	qi->presentDate=qi->q->endDate;

	pthread_mutex_unlock(&qi->q->mutex);
}






PQIterator pqiNew(PQueue q) {
	PQIterator qi=_alloc(struct _pq_iterator_t);
	qi->q=q;
	if(!q->head) {
		qi->cacheMsg=NULL;
		qi->presentDate=q->endDate;
		return qi;
	}
	qi->cacheYear=q->head;
	qi->cacheBucket=q->head->head;
	qi->cacheEv=qi->cacheBucket->head;
	qi->cacheMsg=qi->cacheEv->head;
	qi->presentDate=qi->cacheEv->date;
	return qi;
}

void pqiDel(PQIterator qi) {
	_free(struct _pq_iterator_t,qi);
}


/* pqForward will iteratively dump messages whose timestamp is inferior or equal
 * to date
 * doesn't destroy PQ contents
 */

PQMessage pqiForward(PQIterator qi,PQTime date) {
	PQMessage ret=NULL;

//	if(!(q->tail&&q->tail->tail->tail)) {	/* empty queue */
//		vm_printf("No event in PQueue %i %i %i\n",q->tail!=NULL,q->tail->tail!=NULL,q->tail->tail->tail!=NULL);
//		return NULL;
//	}

	pthread_mutex_lock(&qi->q->mutex);

	if(!qi->cacheMsg) {
		if(qi->q->endDate==qi->presentDate) {	/* have we already processed the whole queue ? */
			/*vm_printf("PQueue at end (date=%f)\n",qi->presentDate);*/
			pthread_mutex_unlock(&qi->q->mutex);
			return NULL;
/*		} else {
			vm_printf("warning ! cacheMsg NULL\n");
			vm_printf("cacheEv = %p\n",q->cacheEv);
			vm_printf("cacheBucket = %p\n",q->cacheBucket);
			vm_printf("cacheYear = %p\n",q->cacheYear);
			return NULL;*/
		}
	} else if(qi->cacheEv->date<=date+FLOAT_IMPREC) {
		qi->presentDate=qi->cacheEv->date;
		ret=qi->cacheMsg;
		/* advance cache */
		cache_forward(qi);
	}
/*	if(!q->cacheMsg) {
		vm_printf("warning ! cacheMsg NULL\n");
		vm_printf("cacheEv = %p\n",q->cacheEv);
		vm_printf("cacheBucket = %p\n",q->cacheBucket);
		vm_printf("cacheYear = %p\n",q->cacheYear);
	}*/
	pthread_mutex_unlock(&qi->q->mutex);
	return ret;
}


/* pqiBackward will iteratively dump messages whose timestamp is superior or equal
 * to date
 * doesn't destroy PQ contents
 */

PQMessage pqiBackward(PQIterator qi,PQTime date) {
	PQMessage ret=NULL;

	pthread_mutex_lock(&qi->q->mutex);

	if(!qi->cacheMsg) {
		if(qi->q->startDate==qi->presentDate) {	/* have we already processed the whole queue ? */
			/*vm_printf("PQueue at end (date=%f)\n",qi->presentDate);*/
			pthread_mutex_unlock(&qi->q->mutex);
			return NULL;
		}
	} else if(qi->cacheEv->date>=date-FLOAT_IMPREC) {
		qi->presentDate=qi->cacheEv->date;
		ret=qi->cacheMsg;
		/* advance cache */
		cache_backward(qi);
	}
	pthread_mutex_unlock(&qi->q->mutex);
	return ret;
}


int pqiAtEnd(PQIterator qi) {
	return qi->cacheEv==NULL;
}


PQTime pqiNextDate(PQIterator qi) {
	return qi->cacheEv?qi->cacheEv->date:0;
}



/**************************************************************************************************
 *************************************** TESTS SECTION ********************************************
 *************************************************************************************************/




#include <stdarg.h>


#define clear() vm_printf("\033[2J")
const char *const green = "\033[0;40;32m";
const char *const yellow = "\033[0;40;33m";
const char *const red = "\033[0;40;31m";
const char *const normal = "\033[0m";



char mpbuf[1024];

void myprintf(const char*p,const char*format,...) {
	va_list va;
	va_start(va,format);
	vsprintf(mpbuf,format,va);
	va_end(va);
	vm_printf("%s%s",p,mpbuf);
}

#define echo(args...) myprintf(prefix ,##args)


void dump_event(struct _pq_ev*e,const char*prefix) {
//	PQMessage m=e->head;
	echo("%s%f%s => %i : %s",green,e->date,normal,e->count,green);
//	while(m) {
//		vm_printf("%i, ",(int)m->data);
//		m=m->next;
//	}
	vm_printf("%s\n",normal);
}

void dump_bucket(struct _pq_bucket*b,const char*prefix) {
	struct _pq_ev*e=b->head;
	char buffer[256];
	sprintf(buffer,"%s  - ",prefix);
	echo("%sBUCKET %X%s [%f:%f] count %i\n",yellow,b,normal,b->node->T,b->node->width,b->count);
	echo("  prev = %X\n",b->prev);
	echo("  next = %X\n",b->next);
	while(e) {
		dump_event(e,buffer);
		e=e->next;
	}
}

void dump_node(struct _pq_bst_node*n,const char*prefix) {
	char buffer[256];
	sprintf(buffer,"%s    ",prefix);
	if(n->bucket) dump_bucket(n->bucket,prefix);
	else {
		echo("%sNODE%s %p T=%s%f%s height=%i balance=%i parent=%p\n",red,normal,n,yellow,n->T,normal,n->height,n->balance,n->parent);
		dump_node(n->left,buffer);
		dump_node(n->right,buffer);
	}
}

void dump_year(struct _pq_year*y,const char*prefix) {
	int t=0;
	struct _pq_bucket*b=y->head;
	char buffer[256];
	sprintf(buffer,"%s    ",prefix);

	while(b) {
		t+=b->count;
		b=b->next;
	}

	echo("Year Number %i, %i events in %i buckets, next year is #%i\n",y->number,t,y->count,y->next?y->next->number:-1);
	dump_node(y->root,buffer);
	
}

void dump_queue(PQueue q) {
	struct _pq_year*y;
	vm_printf("Dumping PQueue [%p], %i msgs in %i events in %i years\n\n",q,q->msgCount,q->evCount,q->count);
	y=q->head;
	while(y) {
		vm_printf("### Year #%i ###\n",y->number);
		dump_year(y,"  ");
		y=y->next;
	}
}






#include <time.h>

void _dm(PQMessage msg) {
	if(msg) _free(struct _pq_msg,msg);
}

int test_queue(PQTime yearSize,int evperbuf,int dichmax,double n_years,double ev_res,int evcount,float_t*enqPS,float_t*fwdPS,float_t*deqPS,int dump) {
	clock_t t1,t2;
	int i,j;
	float_t date;
	float_t coef;
	int mod;
	PQMessage msg;
	int ec=evcount;
	int k,l;
	float_t *rnd;
	PQueue q=pqCreate(yearSize,evperbuf,dichmax,_dm);
	PQIterator qi;

	mod = (int) ((n_years/ev_res)+.5);

	coef=yearSize*ev_res;

	rnd=malloc(sizeof(int)*(ec+(ec>>2)));

	/*vm_printf("mod = %i coef = %f\n",mod,coef);*/
	/*vm_printf("Pre-allocating memory... ");*/
	fflush(stdout);
	/* ensure we will avoid malloc's */
	evcount+=(evcount>>2);
	l=evcount;
	k=0;
	j=0;
	
	for(;evcount>0;evcount--) {
	//	k=random()%mod;
	//	vm_printf("got k=%i\n",k);
		rnd[j]=date=coef*(random()%mod);
		++j;
		msg=_alloc(struct _pq_msg);
		pqEnqueue(q,date,msg);
		/*i=(100*(1+l-evcount))/l;*/
		/*if(i!=k) {*/
			/*k=i;*/
			/*vm_printf("%3.3i%%\b\b\b\b",i);*/
			/*fflush(stdout);*/
		/*}*/
	}
	/*vm_printf("Renewing queue.."); fflush(stdout);*/
	pqDestroy(q);
	/*vm_printf(". "); fflush(stdout);*/

	q=pqCreate(yearSize,evperbuf,dichmax,_dm);
	/*vm_printf("Done.\n");*/
	evcount=ec;

	/* now do measurements */
	/*vm_printf("Measuring enqueue time... ");*/
	fflush(stdout);
	t1=clock();
	j=0;
	for(;evcount>0;evcount--) {
		msg=_alloc(struct _pq_msg);
//		msg->data=(void*)evcount;
		pqEnqueue(q,rnd[j],msg);
		++j;
	}
	t2=clock()-t1;

	*enqPS = ((double)ec)*CLOCKS_PER_SEC/((double)t2);

	/*vm_printf("Done. (%i/%i)\n",q->evCount,q->msgCount);*/

	qi=pqiNew(q);
	pqiJumpTo(qi,0);

	if(dump) dump_queue(q);

	/*vm_printf("Measuring forward read time... ");*/
	fflush(stdout);
	t1=clock();
	for(;(msg=pqiForward(qi,MAXFLOAT));++evcount) {
	//	vm_printf("%i, ",msg->data); fflush(stdout);
	}
	t2=clock()-t1;
	
	*fwdPS = ((double)ec)*CLOCKS_PER_SEC/((double)t2);

	/*vm_printf("Done. (%i)\n",evcount);*/

	/*vm_printf("Measuring hold time... ");*/
	fflush(stdout);
	t1=clock();
	for(i=0;i<ec;i++) {
		msg=pqDequeue(q,MAXFLOAT);
		pqEnqueue(q,msg->date+coef*(random()%mod),msg);
	}
	//	vm_printf("%i, ",msg->data); fflush(stdout);
	t2=clock()-t1;
	
	*deqPS = ((double)ec)*CLOCKS_PER_SEC/((double)t2);

	/*vm_printf("Done. (%i)\n",evcount);*/

	if(dump) {
		vm_printf("\n");
		dump_queue(q);
		vm_printf("\n");
		vm_printf("\n");
	}

	while(pqDequeue(q,MAXFLOAT));	/* flush queue */

	if(dump) {
		vm_printf("\n");
		dump_queue(q);
		vm_printf("\n");
		vm_printf("\n");
	}
	
	pqDestroy(q);

	free(rnd);
	pqiDel(qi);

	return ec==evcount;
}


#ifdef __PQTEST__


int main(int argc,char**argv) {
//	PQueue q;
//	int i;
//	PQMessage msg;
//	clock_t t1,t2,t3,t4;
//	char c;
//	PQTime T;
	int dichotomax;
	float_t yL;
	int evperbuck;
	int BIG;
	int yc;
	float_t tr;
	float_t eps,dps,fps;
	int dump;
	clock_t t1,t2;

	init_alloc();

	/*t1=clock();*/
	/*while((t2=clock())==t1);*/
	/*vm_printf("clock resolution : %li/%li=%lf\n",t2-t1,CLOCKS_PER_SEC,((double)(t2-t1))/CLOCKS_PER_SEC);*/
//	float_t date;
	
//	t3=0;
//	dump_queue(q);
//	fread(&c,1,1,stdin);
//	c=0;

	if(argc<7) {
		vm_printf("Usage : %s [year length] [events per bucket] [max dichotomy level] [number of inserts] [year count] [timestamp res] [dump queue]\n",argv[0]);
		exit(1);
	}
	yL=atof(argv[1]);
	evperbuck=atoi(argv[2]);
	dichotomax=atoi(argv[3]);
	BIG=atoi(argv[4]);
	yc=atoi(argv[5]);
	tr=atof(argv[6]);
	dump=atoi(argv[7]);

	if(!test_queue(yL,evperbuck,dichotomax,yc,tr,BIG,&eps,&fps,&dps,dump)) {
		vm_printf("BUG ! dequeued evcount != enqueued evcount\n");
	}
	/*printf("%f enqueues per second\n%f forward reads per second\n%f dequeues per second\n%u maximum bucket search length\n",eps,fps,dps,max_find_k);*/
	printf("%f %f %f\n",eps,fps,dps);

	return 0;
}

#endif

