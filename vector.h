
#ifndef VECTOR_H_INCLUDED
	#define VECTOR_H_INCLUDED

typedef struct vector {
	void		**data;
	unsigned	size, count;
} vector;

void		vector_init		(vector *);
unsigned	vector_count	(const vector *);
void		vector_add		(vector *, void *);
void		vector_set		(vector *, const unsigned, void *);
void		*vector_get		(const vector *, const unsigned);
void		vector_delete	(vector *, const unsigned);
void		vector_free		(vector *);


#endif /* VECTOR_H_INCLUDED */
