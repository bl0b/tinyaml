#ifndef __BMUS_PQUEUE_H__
#define __BMUS_PQUEUE_H__

/*#include "types.h"*/
#include "list.h"
#include "rtc_alloc.h"

#include "vm_types.h"

typedef float_t PQTime;


/*
 * a message
 *
 * allows storing a list of messages inside an event
 *
 * any structure will fit provided its first field is a pointer to its type and its second field a PQTime representing its timestamp
 */
typedef struct _pq_msg {
	struct _pq_msg*next;
	PQTime date;
}* PQMessage;

/* use this define in head of struct decls that will be used in PQueue ; don't forget the trailing semicolon
 * usable field(s) (read-only) include :
 * 	PQTime date;
 */ 
#define PQMessage_Fields struct _pq_msg*next;PQTime date

/*
 * PQueue is an opaque type
 * this type handles priority queues management
 */
typedef struct _priority_queue_t* PQueue;

/*
 * PQIterator is  an opaque type
 * this type handles traversal of priority queues without dequeueing events
 */


typedef struct _pq_iterator_t* PQIterator;
PQueue pqCreate(PQTime yearLength,long bucketmax,long dichotomymax,void(*destroy_message)(PQMessage));
void pqDestroy(PQueue q);

long pqMsgCount(PQueue q);

/* enqueue message with associated date */
void pqEnqueue(PQueue q,PQTime date,PQMessage msg);
/* dequeue one message if event exists such that event.date <= date, return NULL otherwise */
PQMessage pqDequeue(PQueue q,PQTime curDate);


/* edition facility ; this one is NOT time-critical */
void pqRemove(PQueue q,PQTime date,PQMessage msg);

/* initialize an iterator */
PQIterator pqiNew(PQueue q);
void pqiDel(PQIterator qi);
/* have the PQ behave like a tape ; this function must be called before any forward or reverse read */
void pqiJumpTo(PQIterator qi,PQTime date);
/* read next message if event exists such that event.date <= date, return NULL otherwise */
PQMessage pqiForward(PQIterator qi,PQTime curDate);

/* TODO: someday, implement serialize(PQ) and unserialize(PQ)
 *       easy :	using serialize(Message), while(msg=dequeue(PQ,_float_inf)) serialize(msg);
 *       	and unserialize(Message), while(unserialize(msg)) enqueue(PQ,msg->date,msg);
 */

PQTime pqStartDate(PQueue q);
PQTime pqEndDate(PQueue q);

PQTime pqiNextDate(PQIterator qi);
long pqiAtEnd(PQIterator qi);








/* what about a BST to manage dichotomy ?
 * it can be _alloc'd, doesn't waste too much memory
 *
 * allows lazy dichotomy (i.e. only split range for full buckets)
 *
 * given a bucket, can we achieve O(1) access to next_bucket_in_tree ?
 * seems so :
 * 	given a bucket B, we must split it into two (B1,B2) :
 * 		prev -> B -> next
 * 	becomes :
 * 		prev -> B1 -> B2 -> next
 * self-balanced BST ?
 * 	=> heavy process, *but*
 * 		=> ensures O(log(n)) height
 * 		=> balancing should occur rarely (only when splitting buckets, and not with every split)
 *
 * leaves point to a bucket
 *
 * when year starts :
 * 	bucket count = 1
 * 	tree structure =   root -> bucket
 * when splitting a bucket :
 * 	++ bucket count
 * 	bucket becomes bucket1 and bucket2
 * 		bucket1 AND bucket2 ARE NEVER NULL
 * 	tree structure =   nodeA -> bucket
 * 	becomes
 * 			   nodeA -> ------
 *                        /     \
 *                       /       \
 *         bucket1 <- nodeA1   nodeA2 -> bucket2
 *
 * there are other properties to this BST
 *
 * when removing (we ONLY remove from left childs), if bucket1 becomes empty, we have the following :
 * 			   nodeA -> ------
 *                        /     \
 *                       /       \
 *          ------ <- nodeA1   nodeA2 -> bucket2
 * which becomes :
 *                        nodeA2 -> bucket2
 * 
 * so :
 * - given Y, we can find year in __approx__ O(1)
 * - knowing year, we can find first_bucket in O(1)
 * - knowing bucket, we can find next_bucket in O(1)
 * - knowing bucket, we can find first_event in O(1)
 * - given D, we can find corresponding bucket in O(log(N)), with N the number of buckets in year
 * - given D, and knowing year and bucket, we can insert a message in O(Nb), with Nb the number of events in the bucket
 * sounds nice ?
 * - insertion should be faster
 * - but that sounds like it should be fast in general case
 *
 * thus, DEQUEUE is O(1), ENQUEUE is O(Nb+log(N))
 * 0<=Nb<=THRESHOLD and N is unbounded, 0<N
 * maybe finetuning THRESHOLD and D will suffice
 * 	small D values should maintain small N values (proof ?)
 * 
 * NMAX disappears
 * 	GOOD : discards limitation in events count
 * 	BAD :  situations might arise where BST complexity becomes huge
 *
 * insertion process :
 * 	- find year
 * 	- find bucket
 * 	- insert into bucket
 * 	- if bucket size too big
 * 		- split bucket
 * 		- balance tree
 *
 */

/*
	et une petite table de hashage pour trouver une année donnée

	opérations :
		- isEmpty(date) => <bool>
		- enqueue(date,message)
		- nextDate() => message.date
		- consume() => message


	soit N le nombre de buckets d'une année, 1 <= N <= NMAX.
	soit D la durée d'une année.
	l'année démarre avec N=1.

	pour insérer message avec date :
		y = (long) (date/D);
		if(y) {
			year=findYear(y);
			if(!year) year=new year;
		} else {
			year=currentyear;
		}
		b = (long) ( (date/D - year) * (1<<N[year]) );
		e=year.buckets[b].head;
		while(e.date<date) ++e;		/ * au pire SEUIL opérations * /
		if(e.date==date) {
			insert@tail(e,message);
		} else {
			new_event n;
			insert@before(e,n);
			insert@tail(e,message);
		}

	pour consommer un message :
		le retirer de son événement
		si événement vide :
			le retirer de son bucket
			si bucket vide :
				le retirer de son année
				si année vide :
					la retirer de la table des années
	retirer ou marquer comme "collectable"



	suite à une insertion, si N[bucket] > SEUIL et N < NMAX :
		++N
		on ajuste le tableau de pointeurs sur les buckets (iiii -> i.i.i.i.)
			2*N opérations
		on scinde bucket en deux en partant de la fin de sa liste, jusqu'à la date limite du bucket
			au plus SEUIL opérations

		
	on va arrêter de scinder les buckets si N==NMAX
		si N==7 on a déjà 128 buckets
		si en plus SEUIL=20 ça fait 2560 événements possibles par exemple


----------


il faut gérer le cas problématique de plusieurs messages simultanés
le bucket va grossir énormément

on peut définir un événement comme une liste de messages de même timestamp
insert_message = si événement existe dans file alors ajouter message dans événement sinon ajouter événement dans file.

quand une année est consommée on libère ses ressources




=======================================================================================================================================

typiquement, une année sera un cycle musical ou une mesure, et le nombre de buckets correspondra à un "atome rythmique"

*/

#endif

