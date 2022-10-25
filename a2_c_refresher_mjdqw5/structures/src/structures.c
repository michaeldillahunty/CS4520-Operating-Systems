#include "../include/structures.h"
#include <stdio.h>

/** function notes: 
	Purpose: Compares two SAMPLE structs member-by-member
	Receives: struct_a - first struct to compare
				struct_b - second struct to compare
	Returns: 1 if the structs match, 0 otherwise.
*/ 
int compare_structs(sample_t* a, sample_t* b){
	// struct matching
	if (!a || !b){ // error check params
		return 0; // no match, return 0
	}

	if ((a->a == b->a) && (a->b == b->b) && (a->c == b->c)){
		return 1;	// return 1 if structs match
	}
	return 0; // else return 0
}

/* all this function does is print the alignment of different variables */ 
void print_alignments(){
	printf("Alignment of int is %zu bytes\n",__alignof__(int));
	printf("Alignment of double is %zu bytes\n",__alignof__(double));
	printf("Alignment of float is %zu bytes\n",__alignof__(float));
	printf("Alignment of char is %zu bytes\n",__alignof__(char));
	printf("Alignment of long long is %zu bytes\n",__alignof__(long long));
	printf("Alignment of short is %zu bytes\n",__alignof__(short));
	printf("Alignment of structs are %zu bytes\n",__alignof__(fruit_t));
}

/** function description
	Purpose: Categorizes fruits into apples and oranges
	Receives:  fruit_t* a - pointer to an array of fruits,
			  int* apples - pointer to apples pass-back address,
			  int* oranges - pointer to oranges pass-back address,
			  const size_t size - size of array
	Returns: The size of the array, -1 if there was an error.

	FROM HEADER: 
				#define ORANGE 0
				#define APPLE 1
				#define IS_ORANGE(a) ((a)->type == ORANGE)
				#define IS_APPLE(a)  ((a)->type == APPLE)
		STRUCT:
				typedef struct fruit { 
					int type;
					char padding[8];
				} fruit_t;
*/
/**
	expected value of apples = 2
	expected value of oranges = 13
	expected result = 15
*/
int sort_fruit(const fruit_t* a, int* apples, int* oranges, const size_t size){
	if (!a || !apples || !oranges || size < 1){	// error check params
		return -1; // -1 on error
	}
	for (size_t i = 0; i < size; i++){ // loop through fruit array
		if (IS_APPLE(a+i)){	
			*apples+=1; // increment # of oranges 
		} else if (IS_ORANGE(a+i)){  
			*oranges+=1; // increment # of apples
		} else {
			return -1;
		}	
	}
	return size; 
}

/** function description: 
	Purpose: Initializes an array of fruits with the specified number of apples and oranges
	Receives: fruit_t* a - pointer to an array of fruits
						int apples - the number of apples
						int oranges - the number of oranges
	Returns: -1 if there was an error, 0 otherwise.
*/ 
int initialize_array(fruit_t* a, int apples, int oranges){
	if (!a || apples < 0 || oranges < 0){	// error check params 
		return -1; // return -1 on fail
	}

	int i,j;
	
	for (i = 0; i < oranges; i++){
		a->type = ORANGE;	// add ORANGE fruit type to array with value 
		a++;
	}
	for (j = 0; j < apples; j++){
		a->type = APPLE; // add APPLE fruit type to array with value 1
		a++;
	}
	return 0;
}

int initialize_orange(orange_t* a){
	if (!a){	// error check param
		return -1; 
	}
	a->type = ORANGE; // set type to ORANGE
	a->weight = 21; // randomly chosen value
	a->peeled = 3;	// randomly chosen value
	return 0;
}

int initialize_apple(apple_t* a){
	if (!a){ // error check param
		return -1; 
	}
	a->type = APPLE; // set type to APPLE
	a->weight = 25; // randomly chosen value
	a->worms = 5;	// randomly chosen value
	return 0;
}
