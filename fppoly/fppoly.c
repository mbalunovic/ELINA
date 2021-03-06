/*
 *
 *  This source file is part of ELINA (ETH LIbrary for Numerical Analysis).
 *  ELINA is Copyright © 2019 Department of Computer Science, ETH Zurich
 *  This software is distributed under GNU Lesser General Public License Version 3.0.
 *  For more information, see the ELINA project website at:
 *  http://elina.ethz.ch
 *
 *  THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
 *  EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
 *  THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
 *  IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 *  TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL ETH ZURICH BE LIABLE FOR ANY     
 *  DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
 *  SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
 *  ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
 *  CONTRACT, TORT OR OTHERWISE).
 *
 */

#include "fppoly.h"


fppoly_t* fppoly_of_abstract0(elina_abstract0_t* a)
{
  return (fppoly_t*)a->value;
}

elina_abstract0_t* abstract0_of_fppoly(elina_manager_t* man, fppoly_t* fp)
{
  elina_abstract0_t* r = malloc(sizeof(elina_abstract0_t));
  assert(r);
  r->value = fp;
  r->man = elina_manager_copy(man);
  return r;
}


static inline void fppoly_internal_free(fppoly_internal_t* pr)
{
    if (pr) {
	pr->funid = ELINA_FUNID_UNKNOWN;
	free(pr);
	pr = NULL;
    }
}


static inline fppoly_internal_t* fppoly_internal_alloc(void)
{
    fppoly_internal_t* pr = (fppoly_internal_t*)malloc(sizeof(fppoly_internal_t));
    pr->funid = ELINA_FUNID_UNKNOWN;
    pr->man = NULL;
    pr->funopt = NULL; 
    pr->min_denormal = ldexpl(1.0,-1074);
    pr->ulp = ldexpl(1.0,-52);
    return pr;
}


/* back pointer to our internal structure from the manager */
fppoly_internal_t* fppoly_init_from_manager(elina_manager_t* man, elina_funid_t funid)
{
	
    fppoly_internal_t* pr = (fppoly_internal_t*)man->internal;
    pr->funid = funid;
	
    if (!(pr->man)) pr->man = man;
	
    return pr;
}


elina_manager_t * fppoly_manager_alloc(void){
	void** funptr;
	fesetround(FE_UPWARD);
	fppoly_internal_t *pr = fppoly_internal_alloc();
	elina_manager_t *man = elina_manager_alloc("fppoly",/* Library name */
			"1.0", /* version */
			pr, /* internal structure */
			(void (*)(void*))fppoly_internal_free /* free function for internal */
			);
	funptr = man->funptr;
	funptr[ELINA_FUNID_FREE] = &fppoly_free;
	/* 3.Printing */
	funptr[ELINA_FUNID_FPRINT] = &fppoly_fprint;
	return man;
}



void expr_fprint(FILE * stream, expr_t *expr){
	if((expr->inf_coeff==NULL) || (expr->sup_coeff==NULL)){
		fprintf(stdout,"+ [%.20lf, %.20lf]\n",-expr->inf_cst,expr->sup_cst);
		return;
	}
	size_t size = expr->size;
	size_t i;
	for(i=0; i < size; i++){
		if(i==0){
			if(expr->type==DENSE){
				fprintf(stream, "[%.20lf, %.20lf]x0 ", -expr->inf_coeff[0],expr->sup_coeff[0]);
			}
			else{
				fprintf(stream, "[%.20lf, %.20lf]x%zu ", -expr->inf_coeff[0],expr->sup_coeff[0],expr->dim[0]);
			}
		}
		
		else{
			if(expr->type==DENSE){
				fprintf(stream,"+ [%.20lf, %.20lf]x%zu ",-expr->inf_coeff[i],expr->sup_coeff[i],i);
			}
			else{
				fprintf(stream,"+ [%.20lf, %.20lf]x%zu ",-expr->inf_coeff[i],expr->sup_coeff[i],expr->dim[i]);
			}
		}
	}
	
	fprintf(stdout,"+ [%.20lf, %.20lf]\n",-expr->inf_cst,expr->sup_cst);
	
}



void elina_double_interval_mul_expr_coeff(fppoly_internal_t *pr, double * res_inf, double *res_sup, double inf, double sup, double inf_expr, double sup_expr){
	elina_double_interval_mul(res_inf,res_sup,inf,sup,inf_expr,sup_expr);
	double maxA = fmax(fabs(inf_expr),fabs(sup_expr));
	double tmp1, tmp2;
	elina_double_interval_mul(&tmp1,&tmp2, inf, sup, maxA*pr->ulp, maxA*pr->ulp);
	*res_inf += tmp1;
	*res_sup += tmp2;
}

void elina_double_interval_mul_cst_coeff(fppoly_internal_t *pr, double * res_inf, double *res_sup, double inf, double sup, double inf_expr, double sup_expr){
	elina_double_interval_mul_expr_coeff(pr, res_inf, res_sup, inf, sup, inf_expr, sup_expr);
	*res_inf += pr->min_denormal;
	*res_sup += pr->min_denormal;	
}

void expr_print(expr_t * expr){
	expr_fprint(stdout, expr);	
}

expr_t * alloc_expr(void){
	expr_t *expr = (expr_t *)malloc(sizeof(expr_t));
	expr->inf_coeff = NULL;
	expr->sup_coeff = NULL;
	expr->dim = NULL;
	return expr;
}

expr_t * create_dense_expr(double *coeff, double cst, size_t size){
	expr_t *expr = (expr_t *)malloc(sizeof(expr_t));
	expr->inf_coeff = (double *)malloc(size*sizeof(double));
	expr->sup_coeff = (double *)malloc(size*sizeof(double));
	expr->dim= NULL;
	size_t i;
	expr->size = size;
	expr->inf_cst = -cst;
	expr->sup_cst = cst;
	expr->type = DENSE;
	for(i=0; i < size; i++){
		expr->inf_coeff[i] = -coeff[i];
		expr->sup_coeff[i] = coeff[i];
	}
	return expr;
}


expr_t * create_cst_expr(double l, double u){
	expr_t *expr = (expr_t*)malloc(sizeof(expr_t));
	expr->inf_coeff = NULL;
	expr->sup_coeff = NULL;
	expr->dim = NULL;
	expr->type = SPARSE;
	expr->size = 0;
	expr->inf_cst = l;
	expr->sup_cst = u;
	return expr;
}

expr_t * create_sparse_expr(double *coeff, double cst, size_t *dim, size_t size){
	expr_t *expr = (expr_t *)malloc(sizeof(expr_t));
	if(size>0){
		expr->inf_coeff = (double *)malloc(size*sizeof(double));
		expr->sup_coeff = (double *)malloc(size*sizeof(double));
		expr->dim = (size_t *)malloc(size*sizeof(size_t));
	}
	else{
		expr->inf_coeff = NULL;
		expr->sup_coeff = NULL;
		expr->dim = NULL;
	}
	size_t i;
	expr->size = size;
	expr->inf_cst = -cst;
	expr->sup_cst = cst;
	expr->type = SPARSE;
	for(i=0; i < size; i++){
		expr->inf_coeff[i] = -coeff[i];
		expr->sup_coeff[i] = coeff[i];
		expr->dim[i] = dim[i];
	}
	return expr;
}


void free_expr(expr_t *expr){
	if(expr->inf_coeff){
		free(expr->inf_coeff);
		expr->inf_coeff = NULL;
	}
	if(expr->sup_coeff){
		free(expr->sup_coeff);
		expr->sup_coeff = NULL;
	}
	if(expr->type==SPARSE && expr->dim){
		free(expr->dim);
	}
	expr->dim = NULL;
	free(expr);
	expr = NULL;  
}

expr_t * copy_cst_expr(expr_t *src){
	expr_t *dst = (expr_t *)malloc(sizeof(expr_t));
	dst->inf_coeff = NULL;
	dst->sup_coeff = NULL;
	dst->inf_cst = src->inf_cst;
	dst->sup_cst = src->sup_cst; 
	dst->type = src->type;
	dst->dim = NULL;
	dst->size = src->size; 
	return dst;
}



expr_t * copy_expr(expr_t *src){
	expr_t *dst = (expr_t *)malloc(sizeof(expr_t));
	dst->inf_coeff = (double *)malloc(src->size*sizeof(double));
	dst->sup_coeff = (double *)malloc(src->size*sizeof(double));
	
	size_t i;
	dst->inf_cst = src->inf_cst;
	dst->sup_cst = src->sup_cst; 
	dst->type = src->type;
	for(i=0; i < src->size; i++){
		dst->inf_coeff[i] = src->inf_coeff[i];
		dst->sup_coeff[i] = src->sup_coeff[i];
	}
	if(src->type==SPARSE){
		dst->dim = (size_t *)malloc(src->size*sizeof(size_t));
		for(i=0; i < src->size; i++){
			dst->dim[i] = src->dim[i];
		}
	}
	dst->size = src->size; 
	return dst;
}

expr_t* concretize_dense_sub_expr(fppoly_internal_t *pr, expr_t * expr, double *inf, double *sup, size_t start, size_t size){
	expr_t * res = (expr_t *)malloc(sizeof(expr_t));
	res->inf_coeff = (double *)malloc(start*sizeof(double));
	res->sup_coeff = (double *)malloc(start*sizeof(double));
	size_t i;
	res->inf_cst = expr->inf_cst;
	res->sup_cst = expr->sup_cst;
	res->type = expr->type;
	for(i=0; i < start; i++){
		res->inf_coeff[i] = expr->inf_coeff[i];
		res->sup_coeff[i] = expr->sup_coeff[i];
	}
	for(i=start; i< size;i++){
		double tmp1,tmp2;
		elina_double_interval_mul_expr_coeff(pr,&tmp1,&tmp2,inf[i-start],sup[i-start],expr->inf_coeff[i],expr->sup_coeff[i]);
		res->inf_cst += tmp1;
		res->sup_cst += tmp2;
	}
	res->size = start;
	return res;
}


void merge_sparse_expr(expr_t *expr, size_t l, size_t m, size_t r) {
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;

    /* create temp arrays */
    size_t *L = (size_t *)malloc(n1*sizeof(size_t));
    size_t *R = (size_t *)malloc(n2*sizeof(size_t));
    double *L2 = (double *)malloc(n1*sizeof(double));
    double *R2 = (double *)malloc(n2*sizeof(double));
    double *L3 = (double *)malloc(n1*sizeof(double));
    double *R3 = (double *)malloc(n2*sizeof(double));
    
    /* Copy data to temp arrays L[] and R[] */
    for (i = 0; i < n1; i++) {
        L[i] = expr->dim[l + i];
        L2[i] = expr->inf_coeff[l + i];
	L3[i] = expr->sup_coeff[l + i];
    }
    for (j = 0; j < n2; j++) {
        R[j] = expr->dim[m + 1 + j];
        R2[j] = expr->inf_coeff[m + 1 + j];
	R3[j] = expr->sup_coeff[m + 1 + j];
    }

    /* Merge the temp arrays back into arr[l..r]*/
    i = 0; // Initial index of first subarray
    j = 0; // Initial index of second subarray
    k = l; // Initial index of merged subarray
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            expr->dim[k] = L[i];
            expr->inf_coeff[k] = L2[i];
	    expr->sup_coeff[k] = L3[i];
            i++;
        } else {
            expr->dim[k] = R[j];
            expr->inf_coeff[k] = R2[j];
	    expr->sup_coeff[k] = R3[j];
            j++;
        }
        k++;
    }

    /* Copy the remaining elements of L[], if there
       are any */
    while (i < n1) {
        expr->dim[k] = L[i];
        expr->inf_coeff[k] = L2[i];
	expr->sup_coeff[k] = L3[i];
        i++;
        k++;
    }

    /* Copy the remaining elements of R[], if there
       are any */
    while (j < n2) {
        expr->dim[k] = R[j];
        expr->inf_coeff[k] = R2[j];
	expr->sup_coeff[k] = R3[j];
        j++;
        k++;
    }
    free(L);
    free(R);
    free(L2);
    free(R2);
    free(L3);
    free(R3);
}


/* l is for left index and r is right index of the
   sub-array of arr to be sorted */
void merge_sort_sparse_expr(expr_t *expr, size_t l, size_t r) {
    if (l < r) {
        // Same as (l+r)/2, but avoids overflow for
        // large l and h
        size_t m = l + (r - l) / 2;

        // Sort first and second halves
        merge_sort_sparse_expr(expr, l, m);
        merge_sort_sparse_expr(expr, m + 1, r);

        merge_sparse_expr(expr, l, m, r);
    }
}

void sort_sparse_expr(expr_t *expr){
	merge_sort_sparse_expr(expr,0,expr->size-1);
}

neuron_t *neuron_alloc(void){
	neuron_t *res =  (neuron_t *)malloc(sizeof(neuron_t));
	res->expr = NULL;
	res->lb = -INFINITY;
	res->ub = INFINITY;
	res->lexpr = NULL;
	res->uexpr = NULL;
	return res;
}

layer_t * create_layer(size_t size, layertype_t type, activation_type_t activation){
	layer_t *layer = (layer_t*)malloc(sizeof(layer_t));
	layer->dims = size;
	layer->type = type;
        layer->activation = activation;
	layer->neurons = (neuron_t**)malloc(size*sizeof(neuron_t*));
	size_t i;
	for(i=0; i < size; i++){
		layer->neurons[i] = neuron_alloc();
	}
	layer->h_t_inf = NULL;
	layer->h_t_sup = NULL;
	layer->c_t_inf = NULL;
	layer->c_t_sup = NULL;
	return layer;
}


void fppoly_from_network_input_box(fppoly_t *res, size_t intdim, size_t realdim, double *inf_array, double *sup_array){
	
	res->layers = NULL;
	res->numlayers = 0;
	res->lstm_index = 0;
	size_t num_pixels = intdim + realdim;
	res->input_inf = (double *)malloc(num_pixels*sizeof(double));
	res->input_sup = (double *)malloc(num_pixels*sizeof(double));
	res->input_lexpr = NULL;
	res->input_uexpr = NULL;
	size_t i;
	for(i=0; i < num_pixels; i++){
		res->input_inf[i] = -inf_array[i];
		res->input_sup[i] = sup_array[i];
	}
	res->num_pixels = num_pixels;
	res->out = NULL;
}


elina_abstract0_t * fppoly_from_network_input(elina_manager_t *man, size_t intdim, size_t realdim, double *inf_array, double *sup_array){
	fppoly_t * res = (fppoly_t *)malloc(sizeof(fppoly_t));
	fppoly_from_network_input_box(res, intdim, realdim, inf_array, sup_array);
	return abstract0_of_fppoly(man,res);
}

void fppoly_set_network_input_box(elina_manager_t *man, elina_abstract0_t* element, size_t intdim, size_t realdim, double *inf_array, double * sup_array){
    fppoly_t * res = fppoly_of_abstract0(element);
    size_t num_pixels = intdim + realdim;
	res->numlayers = 0;
    size_t i;
    for(i=0; i < num_pixels; i++){
        res->input_inf[i] = -inf_array[i];
        res->input_sup[i] = sup_array[i];
    }
}

elina_abstract0_t* fppoly_from_network_input_poly(elina_manager_t *man, size_t intdim, size_t realdim, double *inf_array, double *sup_array, 
                                                  double * lexpr_weights, double * lexpr_cst, size_t * lexpr_dim, double * uexpr_weights,
						  double * uexpr_cst, size_t * uexpr_dim, size_t expr_size){
	fppoly_t * res = (fppoly_t *)malloc(sizeof(fppoly_t));
	
	fppoly_from_network_input_box(res, intdim, realdim, inf_array, sup_array);
	size_t num_pixels = intdim + realdim;
	res->input_lexpr = (expr_t **)malloc(num_pixels*sizeof(expr_t *));
	res->input_uexpr = (expr_t **)malloc(num_pixels*sizeof(expr_t *));
	
	size_t i;
        double * tmp_weights = (double*)malloc(expr_size*sizeof(double));
	size_t * tmp_dim = (size_t*)malloc(expr_size*sizeof(size_t));
	
	for(i = 0; i < num_pixels; i++){
		
		size_t j;
		for(j=0; j < expr_size; j++){
			tmp_weights[j] = lexpr_weights[i*expr_size+j];
			tmp_dim[j] = lexpr_dim[i*expr_size+j];
		}
		res->input_lexpr[i] = create_sparse_expr(tmp_weights, lexpr_cst[i], tmp_dim, expr_size);
		sort_sparse_expr(res->input_lexpr[i]);
	//printf("w: %p %g %g %g cst: %g dim: %p %zu %zu %zu\n",lexpr_weights[i],lexpr_weights[i][0],lexpr_weights[i][1], lexpr_weights[i][2],lexpr_cst[i],lexpr_dim[i],lexpr_dim[i][0],lexpr_dim[i][1], lexpr_dim[i][2]);
		//expr_print(res->input_lexpr[i]);
		//fflush(stdout);
		for(j=0; j < expr_size; j++){
			tmp_weights[j] = uexpr_weights[i*expr_size+j];
			tmp_dim[j] = uexpr_dim[i*expr_size+j];
		}
		res->input_uexpr[i] = create_sparse_expr(tmp_weights, uexpr_cst[i], tmp_dim, expr_size);
		sort_sparse_expr(res->input_uexpr[i]);
	//	expr_print(res->input_uexpr[i]);
	//	fflush(stdout);
	}
	free(tmp_weights);
	free(tmp_dim);
	return abstract0_of_fppoly(man,res);	

}

void fppoly_alloc_first_layer(fppoly_t *fp, size_t size, layertype_t type, activation_type_t activation){
	layer_t *layer = create_layer(size, type, activation);
        fp->layers = (layer_t **)malloc(2000*sizeof(layer_t *));
	fp->layers[0] = layer;
	fp->numlayers = 1;
	return;
}

void fppoly_add_new_layer(fppoly_t *fp, size_t size, layertype_t type, activation_type_t activation){
	size_t numlayers = fp->numlayers;
	fp->layers[numlayers] = create_layer(size,type, activation);
	fp->numlayers++;
	return;
}


void elina_double_interval_add_expr_coeff(fppoly_internal_t *pr, double * res_inf, double *res_sup, double inf, double sup, double inf_expr, double sup_expr){
	*res_inf = inf + inf_expr;
	*res_sup = sup + sup_expr;
	double maxA = fmax(fabs(inf_expr),fabs(sup_expr));
	double tmp1, tmp2;
	elina_double_interval_mul(&tmp1,&tmp2, inf, sup, maxA*pr->ulp, maxA*pr->ulp);
	*res_inf += tmp1;
	*res_sup += tmp2;
}

void elina_double_interval_add_cst_coeff(fppoly_internal_t *pr, double * res_inf, double *res_sup, double inf, double sup, double inf_expr, double sup_expr){
	elina_double_interval_add_expr_coeff(pr, res_inf, res_sup, inf, sup, inf_expr, sup_expr);
	*res_inf += pr->min_denormal;
	*res_sup += pr->min_denormal;	
}






expr_t * multiply_expr(fppoly_internal_t *pr, expr_t *expr, double mul_inf, double mul_sup){
	expr_t * res = alloc_expr();
	if(expr->size > 0){
		res->inf_coeff = malloc(expr->size*sizeof(double));
		res->sup_coeff = malloc(expr->size*sizeof(double));
	}
	else{
		res->inf_coeff = NULL;		
		res->sup_coeff = NULL;
	}
	res->type = expr->type;
	size_t i;
	for(i=0; i < expr->size; i++){
		//res->coeff[i] = mul_coeff*expr->coeff[i];
		elina_double_interval_mul_expr_coeff(pr,&res->inf_coeff[i],&res->sup_coeff[i],mul_inf,mul_sup,expr->inf_coeff[i],expr->sup_coeff[i]);
		
	}
	if(expr->type==SPARSE){
		if(expr->size>0){
			res->dim = (size_t*)malloc(expr->size*sizeof(size_t));
			for(i=0; i < expr->size; i++){
				res->dim[i] = expr->dim[i];
			}
		}
		else{
			res->dim = NULL;
		}
	}
	res->size = expr->size;
	
	elina_double_interval_mul_cst_coeff(pr,&res->inf_cst,&res->sup_cst,mul_inf,mul_sup,expr->inf_cst,expr->sup_cst);
	
	//res->cst = mul_coeff*expr->cst;
	return res;
}


expr_t * multiply_cst_expr(fppoly_internal_t *pr, expr_t *expr, double mul_inf, double mul_sup){
	expr_t * res = alloc_expr();
	res->inf_coeff = NULL;		
	res->sup_coeff = NULL;
	res->dim = NULL;
	res->type = expr->type;
	res->size = expr->size;
	elina_double_interval_mul_cst_coeff(pr,&res->inf_cst,&res->sup_cst,mul_inf,mul_sup,expr->inf_cst,expr->sup_cst);
	//res->cst = mul_coeff*expr->cst;
	return res;
}






void add_cst_expr(fppoly_internal_t *pr, expr_t * exprA, expr_t *exprB){
	double maxA = fmax(fabs(exprA->inf_cst),fabs(exprA->sup_cst));
	double maxB = fmax(fabs(exprB->inf_cst),fabs(exprB->sup_cst));
	exprA->inf_cst = exprA->inf_cst + exprB->inf_cst  + (maxA + maxB)*pr->ulp + pr->min_denormal; 
	exprA->sup_cst = exprA->sup_cst + exprB->sup_cst + (maxA + maxB)*pr->ulp + pr->min_denormal; 
	return;
}

//A = A + B
void add_expr(fppoly_internal_t *pr,expr_t * exprA, expr_t * exprB){
	//
	size_t sizeB = exprB->size;
	if(sizeB==0){
		double maxA = fmax(fabs(exprA->inf_cst),fabs(exprA->sup_cst));
		double maxB = fmax(fabs(exprB->inf_cst),fabs(exprB->sup_cst));
		exprA->inf_cst = exprA->inf_cst + exprB->inf_cst  + (maxA + maxB)*pr->ulp + pr->min_denormal; 
		exprA->sup_cst = exprA->sup_cst + exprB->sup_cst  + (maxA + maxB)*pr->ulp + pr->min_denormal; 
		return;
	}
	size_t i;
	if(exprA->size==0){
		
		exprA->size = exprB->size;
		double maxA = fmax(fabs(exprA->inf_cst),fabs(exprA->sup_cst));
		double maxB = fmax(fabs(exprB->inf_cst),fabs(exprB->sup_cst));
		exprA->inf_cst += exprB->inf_cst  + (maxA + maxB)*pr->ulp + pr->min_denormal;
		exprA->sup_cst += exprB->sup_cst  + (maxA + maxB)*pr->ulp + pr->min_denormal;
		exprA->inf_coeff = (double *)malloc(sizeB*sizeof(double));
		exprA->sup_coeff = (double *)malloc(sizeB*sizeof(double));
		for(i=0; i < sizeB; i++){
			exprA->inf_coeff[i] = exprB->inf_coeff[i];
			exprA->sup_coeff[i] = exprB->sup_coeff[i];
		} 
		exprA->type = exprB->type;
		if(exprA->type==SPARSE){
			exprA->dim = (size_t *)malloc(sizeB*sizeof(size_t));
			for(i=0; i < sizeB; i++){
				exprA->dim[i] = exprB->dim[i];
			}
		} 
		
		return;
	}
	else{
		size_t sizeA = exprA->size;
		assert(sizeA==sizeB);
		double maxA = fmax(fabs(exprA->inf_cst),fabs(exprA->sup_cst));
		double maxB = fmax(fabs(exprB->inf_cst),fabs(exprB->sup_cst));
		exprA->inf_cst += exprB->inf_cst  + (maxA + maxB)*pr->ulp + pr->min_denormal;
		exprA->sup_cst += exprB->sup_cst  + (maxA + maxB)*pr->ulp + pr->min_denormal;
		if(exprA->type==DENSE){
			if(exprB->type==DENSE){
				for(i=0; i < sizeB; i++){
					maxA = fmax(fabs(exprA->inf_coeff[i]),fabs(exprA->sup_coeff[i]));
					maxB = fmax(fabs(exprB->inf_coeff[i]),fabs(exprB->sup_coeff[i]));
					exprA->inf_coeff[i] = exprA->inf_coeff[i] + exprB->inf_coeff[i] + (maxA + maxB)*pr->ulp;
					exprA->sup_coeff[i] = exprA->sup_coeff[i] + exprB->sup_coeff[i] + (maxA + maxB)*pr->ulp;
				}
			}
			else{
				
				size_t k = 0;
				for(i=0; i < sizeA; i++){
					if(k < sizeB && exprB->dim[k]==i){
						maxA = fmax(fabs(exprA->inf_coeff[i]),fabs(exprA->sup_coeff[i]));
						maxB = fmax(fabs(exprB->inf_coeff[k]),fabs(exprB->sup_coeff[k]));
						exprA->inf_coeff[i] = exprA->inf_coeff[i] + exprB->inf_coeff[k] + (maxA + maxB)*pr->ulp ;
						exprA->sup_coeff[i] = exprA->sup_coeff[i] + exprB->sup_coeff[k] + (maxA + maxB)*pr->ulp;
						k++;
					}
				}
			}
		}
		else{
			size_t sizeB = exprB->size;
			size_t k;
			double * new_inf_coeff;
			double * new_sup_coeff;
			if(exprB->type==DENSE){
				
				i=0;
				new_inf_coeff = (double *)malloc(sizeB*sizeof(double));
				new_sup_coeff = (double *)malloc(sizeB*sizeof(double));				
				for(k=0; k < sizeB; k++){
					if(i < sizeA && exprA->dim[i] == k){
						maxA = fmax(fabs(exprA->inf_coeff[i]),fabs(exprA->sup_coeff[i]));
						maxB = fmax(fabs(exprB->inf_coeff[k]),fabs(exprB->sup_coeff[k]));
						new_inf_coeff[k] = exprA->inf_coeff[i] + exprB->inf_coeff[k] + (maxA + maxB)*pr->ulp;
						new_sup_coeff[k] = exprA->sup_coeff[i] + exprB->sup_coeff[k] + (maxA + maxB)*pr->ulp;
						i++;
					}
					else{
						new_inf_coeff[k] = exprB->inf_coeff[k];
						new_sup_coeff[k] = exprB->sup_coeff[k];
					}
				}
				exprA->type = DENSE;
				exprA->size = sizeB;
				free(exprA->dim);
				exprA->dim = NULL;
			}
			else{
				i=0;
				k=0;
				size_t l = 0;
				//printf("before sort\n");
				//expr_print(exprA);
				//fflush(stdout);
				//if(exprA->size>0){
				//	sort_sparse_expr(exprA);
				//}
				//printf("after sort\n");
				//expr_print(exprA);
				//fflush(stdout);
				//sort_sparse_expr(exprB);
				new_inf_coeff = (double *)malloc((sizeA+sizeB)*sizeof(double));
				new_sup_coeff = (double *)malloc((sizeA+sizeB)*sizeof(double));
				size_t * new_dim = (size_t *)malloc((sizeA+sizeB)*sizeof(size_t));
				while(i < sizeA && k < sizeB){
					if(exprA->dim[i] < exprB->dim[k]){
						new_inf_coeff[l] = exprA->inf_coeff[i];
						new_sup_coeff[l] = exprA->sup_coeff[i];
						new_dim[l] = exprA->dim[i];
						i++;
						
					}
					else if(exprB->dim[k] < exprA->dim[i]){
						new_inf_coeff[l] = exprB->inf_coeff[k];
						new_sup_coeff[l] = exprB->sup_coeff[k];
						new_dim[l] = exprB->dim[k];
						k++;
					}
					else{
						maxA = fmax(fabs(exprA->inf_coeff[i]),fabs(exprA->sup_coeff[i]));
						maxB = fmax(fabs(exprB->inf_coeff[k]),fabs(exprB->sup_coeff[k]));
						new_inf_coeff[l] = exprA->inf_coeff[i] + exprB->inf_coeff[k] + (maxA + maxB)*pr->ulp;
						new_sup_coeff[l] = exprA->sup_coeff[i] + exprB->sup_coeff[k] + (maxA + maxB)*pr->ulp;
						new_dim[l] = exprA->dim[i];
						i++;
						k++;
					}
					l++;
				}
				while(i < sizeA){
					new_inf_coeff[l] = exprA->inf_coeff[i];
					new_sup_coeff[l] = exprA->sup_coeff[i];
					new_dim[l] = exprA->dim[i];
					i++;
					l++;
				}
				while(k < sizeB){
					new_inf_coeff[l] = exprB->inf_coeff[k];
					new_sup_coeff[l] = exprB->sup_coeff[k];
					new_dim[l] = exprB->dim[k];
					k++;
					l++;
				}
				
				new_inf_coeff = (double*)realloc(new_inf_coeff,l*sizeof(double));
				new_sup_coeff = (double*)realloc(new_sup_coeff,l*sizeof(double));
				free(exprA->dim);
				exprA->dim = NULL;
				new_dim = (size_t *)realloc(new_dim,l*sizeof(size_t));
				exprA->dim = new_dim;
				exprA->size = l;
			}
			if(exprA->inf_coeff){
				free(exprA->inf_coeff);
				exprA->inf_coeff = NULL;
			}
			if(exprA->sup_coeff){
				free(exprA->sup_coeff);
				exprA->sup_coeff = NULL;
			}
			exprA->inf_coeff = new_inf_coeff;
			exprA->sup_coeff = new_sup_coeff;
			
		}
	}
}


expr_t * replace_input_poly_cons_in_lexpr(fppoly_internal_t *pr, expr_t * expr, fppoly_t * fp){
	size_t dims = expr->size;
	size_t i,k;
	double tmp1, tmp2;
	expr_t * res;
	if(expr->type==DENSE){
		k = 0;
	}
	else{
		k = expr->dim[0];		
	}
	expr_t * mul_expr = NULL;

	if(expr->sup_coeff[0] <0){
		mul_expr = fp->input_uexpr[k];
	}
	else if(expr->inf_coeff[0] < 0){
		mul_expr = fp->input_lexpr[k];
	}
		
	if(mul_expr!=NULL){
		if(mul_expr->size==0){
			res = multiply_cst_expr(pr,mul_expr,expr->inf_coeff[0],expr->sup_coeff[0]);
		}
		else{
			res = multiply_expr(pr,mul_expr,expr->inf_coeff[0],expr->sup_coeff[0]);
		}
	}
		
	else{
		elina_double_interval_mul(&tmp1,&tmp2,expr->inf_coeff[0],expr->sup_coeff[0],fp->input_inf[k],fp->input_sup[k]);
		res = create_cst_expr(tmp1, -tmp1);
	}
	for(i=1; i < dims; i++){
		if(expr->type==DENSE){
			k = i;
		}
		else{
			k = expr->dim[i];
		}
			
		expr_t * mul_expr = NULL;
		expr_t * sum_expr = NULL;
		if(expr->sup_coeff[i] <0){
			mul_expr = fp->input_uexpr[k];
		}
		else if(expr->inf_coeff[i] <0){
			mul_expr = fp->input_lexpr[k];
		}
			
		if(mul_expr!=NULL){
			if(mul_expr->size==0){
				sum_expr = multiply_cst_expr(pr,mul_expr, expr->inf_coeff[i],expr->sup_coeff[i]);
				add_cst_expr(pr,res,sum_expr);
			}	
			else if(expr->inf_coeff[i]!=0 && expr->sup_coeff[i]!=0){
				sum_expr = multiply_expr(pr,mul_expr, expr->inf_coeff[i],expr->sup_coeff[i]);
				add_expr(pr,res,sum_expr);
			}
				//free_expr(mul_expr);
			if(sum_expr!=NULL){
				free_expr(sum_expr);
			}
		}
		else{
			elina_double_interval_mul(&tmp1,&tmp2,expr->inf_coeff[i],expr->sup_coeff[i],fp->input_inf[k],fp->input_sup[k]);
			res->inf_cst = res->inf_cst + tmp1;
			res->sup_cst = res->sup_cst - tmp1;
		}
	}
		
	res->inf_cst = res->inf_cst + expr->inf_cst; 
	res->sup_cst = res->sup_cst + expr->sup_cst; 
	return res;
}


expr_t * replace_input_poly_cons_in_uexpr(fppoly_internal_t *pr, expr_t * expr, fppoly_t * fp){
	size_t dims = expr->size;
	size_t i,k;
	double tmp1, tmp2;
	expr_t * res;
	if(expr->type==DENSE){
		k = 0;
	}
	else{
		k = expr->dim[0];		
	}
	expr_t * mul_expr = NULL;
			
	if(expr->sup_coeff[0] <0){
		mul_expr = fp->input_lexpr[k];
	}
	else if(expr->inf_coeff[0] < 0){
		mul_expr = fp->input_uexpr[k];
	}
		
	if(mul_expr!=NULL){
		if(mul_expr->size==0){
			res = multiply_cst_expr(pr,mul_expr,expr->inf_coeff[0],expr->sup_coeff[0]);
		}
		else{
			res = multiply_expr(pr,mul_expr,expr->inf_coeff[0],expr->sup_coeff[0]);
		}
	}
	else{
		elina_double_interval_mul(&tmp1,&tmp2,expr->inf_coeff[0],expr->sup_coeff[0],fp->input_inf[k],fp->input_sup[k]);
		res = create_cst_expr(-tmp2, tmp2);
	}
                //printf("finish\n");
		//fflush(stdout);
	for(i=1; i < dims; i++){
		if(expr->type==DENSE){
			k = i;
		}
		else{
			k = expr->dim[i];
		}
		expr_t * mul_expr = NULL;
		expr_t * sum_expr = NULL;
		if(expr->sup_coeff[i] <0){
			mul_expr = fp->input_lexpr[k];
		}
		else if(expr->inf_coeff[i] <0){
			mul_expr = fp->input_uexpr[k];
		}
			
		if(mul_expr!=NULL){
			if(mul_expr->size==0){
				sum_expr = multiply_cst_expr(pr,mul_expr, expr->inf_coeff[i],expr->sup_coeff[i]);
				add_cst_expr(pr,res,sum_expr);
			}	
			else if(expr->inf_coeff[i]!=0 && expr->sup_coeff[i]!=0){
				sum_expr = multiply_expr(pr,mul_expr, expr->inf_coeff[i],expr->sup_coeff[i]);
				add_expr(pr,res,sum_expr);
			}
				//free_expr(mul_expr);
			if(sum_expr!=NULL){
				free_expr(sum_expr);
			}
		}
		else{
			elina_double_interval_mul(&tmp1,&tmp2,expr->inf_coeff[i],expr->sup_coeff[i],fp->input_inf[k],fp->input_sup[k]);
			res->inf_cst = res->inf_cst - tmp2;
			res->sup_cst = res->sup_cst + tmp2;
		}
	}
	res->inf_cst = res->inf_cst + expr->inf_cst; 
	res->sup_cst = res->sup_cst + expr->sup_cst; 
	return res;
}

double compute_lb_from_expr(fppoly_internal_t *pr, expr_t * expr, fppoly_t * fp){
	size_t i,k;
	double tmp1, tmp2;
        //printf("start\n");
        //fflush(stdout);
	if((fp->input_lexpr!=NULL) && (fp->input_uexpr!=NULL)){
		expr =  replace_input_poly_cons_in_lexpr(pr, expr, fp);
	}
	/* expr_print(expr); */
	/* fflush(stdout); */
	size_t dims = expr->size;
	double res_inf = expr->inf_cst;
	if(expr->inf_coeff==NULL || expr->sup_coeff==NULL){
		return 0;
	}
	for(i=0; i < dims; i++){
		//if(expr->inf_coeff[i]<0){
		if(expr->type==DENSE){
			k = i;
		}
		else{
			k = expr->dim[i];
		}
			elina_double_interval_mul(&tmp1,&tmp2,expr->inf_coeff[i],expr->sup_coeff[i],fp->input_inf[k],fp->input_sup[k]);
			//printf("tmp1: %g\n",tmp1);
			res_inf = res_inf + tmp1;
			
	}
//	printf("inf: %g\n",-res_inf);
//	fflush(stdout);
        if(fp->input_lexpr!=NULL && fp->input_uexpr!=NULL){
		free_expr(expr);
	}
        //printf("finish\n");
        //fflush(stdout);
	return res_inf;
}

double compute_ub_from_expr(fppoly_internal_t *pr, expr_t * expr, fppoly_t * fp){
	size_t i,k;
	double tmp1, tmp2;

	if((fp->input_lexpr!=NULL) && (fp->input_uexpr!=NULL)){
		expr =  replace_input_poly_cons_in_uexpr(pr, expr, fp);
	}

	size_t dims = expr->size;
	double res_sup = expr->sup_cst;
	if(expr->inf_coeff==NULL || expr->sup_coeff==NULL){
		return 0;
	}
	for(i=0; i < dims; i++){
		//if(expr->inf_coeff[i]<0){
		if(expr->type==DENSE){
			k = i;
		}
		else{
			k = expr->dim[i];
		}		
		elina_double_interval_mul(&tmp1,&tmp2,expr->inf_coeff[i],expr->sup_coeff[i],fp->input_inf[k],fp->input_sup[k]);
		res_sup = res_sup + tmp2;
			
	}
	//printf("sup: %g\n",res_sup);
	//fflush(stdout);
	if(fp->input_lexpr!=NULL && fp->input_uexpr!=NULL){
		free_expr(expr);
	}
	return res_sup;
}


void ffn_handle_first_layer(elina_manager_t* man, elina_abstract0_t * abs, double **weights, double *bias, size_t size, size_t num_pixels, activation_type_t activation, bool alloc){
	//printf("start \n");
	//fflush(stdout);
	fppoly_t *res = fppoly_of_abstract0(abs);
	//printf("coming here\n");
	//fflush(stdout);
	
    if(alloc){
        fppoly_alloc_first_layer(res,size, FFN, activation);
    } else {
        res->numlayers = 1;
	}
	fppoly_internal_t *pr = fppoly_init_from_manager(man, ELINA_FUNID_ASSIGN_LINEXPR_ARRAY);
	size_t i, j;
	
	//for(i=0; i < num_pixels; i++){
	//	elina_interval_print(itv[i]);
	//	printf("\n");
	//}	
	//fflush(stdout);
	neuron_t ** neurons = res->layers[0]->neurons;
	for(i=0; i < size; i++){
		neuron_t *neuron = neurons[i];
		double * weight_i = weights[i];
		double bias_i = bias[i];
		neuron->expr = create_dense_expr(weight_i,bias_i,num_pixels);
		neuron->lb = compute_lb_from_expr(pr, neuron->expr,res);
		neuron->ub = compute_ub_from_expr(pr, neuron->expr,res);
	}
	
	//printf("return here\n");
	//fppoly_fprint(stdout,man,res,NULL);
	//fflush(stdout);
	return;	
}

void ffn_handle_first_relu_layer(elina_manager_t* man, elina_abstract0_t * abs, double **weights, double *bias,   size_t size, size_t num_pixels){
        ffn_handle_first_layer(man, abs, weights, bias, size, num_pixels, RELU, true);
}

void ffn_handle_first_sigmoid_layer(elina_manager_t* man, elina_abstract0_t * abs, double **weights, double *bias,  size_t size, size_t num_pixels){
        ffn_handle_first_layer(man, abs, weights, bias, size, num_pixels, SIGMOID, true);
}


void ffn_handle_first_tanh_layer(elina_manager_t* man, elina_abstract0_t * abs, double **weights, double *bias,  size_t size, size_t num_pixels){
        ffn_handle_first_layer(man, abs, weights, bias,  size, num_pixels, TANH, true);
}


void ffn_handle_first_parabola_layer(elina_manager_t* man, elina_abstract0_t * abs, double **weights, double *bias,  size_t size, size_t num_pixels){
        ffn_handle_first_layer(man, abs, weights, bias, size, num_pixels, PARABOLA, true);
}

void ffn_handle_first_log_layer(elina_manager_t* man, elina_abstract0_t * abs, double **weights, double *bias,  size_t size, size_t num_pixels){
        ffn_handle_first_layer(man, abs, weights, bias, size, num_pixels, LOG, true);
}

void ffn_handle_first_relu_layer_no_alloc(elina_manager_t* man, elina_abstract0_t * abs, double **weights, double *bias,   size_t size, size_t num_pixels){
    ffn_handle_first_layer(man, abs, weights, bias, size, num_pixels, RELU, false);
}

void ffn_handle_first_sigmoid_layer_no_alloc(elina_manager_t* man, elina_abstract0_t * abs, double **weights, double *bias,  size_t size, size_t num_pixels){
    ffn_handle_first_layer(man, abs, weights, bias, size, num_pixels, SIGMOID, false);
}


void ffn_handle_first_tanh_layer_no_alloc(elina_manager_t* man, elina_abstract0_t * abs, double **weights, double *bias,  size_t size, size_t num_pixels){
    ffn_handle_first_layer(man, abs, weights, bias,  size, num_pixels, TANH, false);
}


void ffn_handle_first_parabola_layer_no_alloc(elina_manager_t* man, elina_abstract0_t * abs, double **weights, double *bias,  size_t size, size_t num_pixels){
    ffn_handle_first_layer(man, abs, weights, bias, size, num_pixels, PARABOLA, false);
}

void ffn_handle_first_log_layer_no_alloc(elina_manager_t* man, elina_abstract0_t * abs, double **weights, double *bias,  size_t size, size_t num_pixels){
    ffn_handle_first_layer(man, abs, weights, bias, size, num_pixels, LOG, false);
}

void compute_square(double lb, double ub, double *sqr_inf, double *sqr_sup) {
  if (lb > 0 && ub > 0) {
	*sqr_inf = 0;
	*sqr_sup = (lb * lb > ub * ub) ? lb * lb : ub * ub;
  } else {
	*sqr_inf = (lb * lb < ub * ub) ? -lb * lb : -ub * ub;
	*sqr_sup = (lb * lb > ub * ub) ? lb * lb : ub * ub;
  }
}

expr_t * lexpr_replace_parabola_bounds(fppoly_internal_t * pr, expr_t * expr, neuron_t ** neurons){
	size_t num_neurons = expr->size;
	size_t i,k;
	expr_t * res = alloc_expr();  
	res->inf_coeff = (double *)malloc(num_neurons*sizeof(double));
	res->sup_coeff = (double *)malloc(num_neurons*sizeof(double));
	res->inf_cst = expr->inf_cst;
	res->sup_cst = expr->sup_cst;
	res->type = expr->type;
	res->size = num_neurons;

	/* printf("computing parabola lexpr...\n"); */
	/* expr_print(expr); */

	for(i = 0; i < num_neurons; i++){
		if(expr->type==DENSE){
			k = i;
		}
		else{
			k = expr->dim[i];
		}
		neuron_t *neuron_k = neurons[k];
		double lb = neurons[k]->lb;
		double ub = neurons[k]->ub;
		/* printf("sqr ... %lf %lf\n",lb,ub); */
		res->inf_coeff[i] = 0.0;
		res->sup_coeff[i] = 0.0;
		if((expr->sup_coeff[i]==0) && (expr->inf_coeff[i]==0)){
			continue;
		}
		double u_plus_l_sup = (ub-lb);
		double u_plus_l_inf = -(ub-lb);

		double sqr_inf, sqr_sup;
		compute_square(lb, ub, &sqr_inf, &sqr_sup);
		/* printf("sqr_inf = %lf, sqr_sup = %lf\n",sqr_inf,sqr_sup); */
		
                //= (u_plus_l)*(u_plus_l)
		if (ub + lb < 1e-4) {
		  res->inf_coeff[i] = 0.0;
		  res->sup_coeff[i] = 0.0;
		  double tmp1, tmp2;
		  elina_double_interval_mul(&tmp1,&tmp2,expr->inf_coeff[i], expr->sup_coeff[i], sqr_inf, sqr_sup);
		  res->inf_cst = res->inf_cst + tmp1;
		  res->sup_cst = res->sup_cst + tmp2;
		  continue;
		}

		if(expr->sup_coeff[i]<0){
			double lu_sup;
            double lu_inf;
            elina_double_interval_mul_cst_coeff(pr,&lu_inf,&lu_sup,lb,-lb,-ub,ub);
			//res->coeff[i] = lambda*expr->coeff[i];
			//res->cst = res->cst + expr->coeff[i]*mu;
			elina_double_interval_mul_expr_coeff(pr,&res->inf_coeff[i],&res->sup_coeff[i],u_plus_l_inf,u_plus_l_sup,expr->inf_coeff[i],expr->sup_coeff[i]);
			double tmp1, tmp2;
			/* printf("[%lf %lf] x [%lf %lf]\n",lu_sup,lu_inf,expr->inf_coeff[i],expr->sup_coeff[i]); */
			elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,lu_sup,lu_inf,expr->inf_coeff[i],expr->sup_coeff[i]);
			/* printf("tmp1 = %.10lf, tmp2 = %.10lf\n",tmp1,tmp2); */
			res->inf_cst = res->inf_cst + tmp1 + pr->min_denormal;
			res->sup_cst = res->sup_cst + tmp2 + pr->min_denormal;
		}
		else if (expr->inf_coeff[i]<0){
			res->inf_coeff[i] = 0;
			res->sup_coeff[i] = 0;
			double u_plus_l_sq_inf, u_plus_l_sq_sup;
			elina_double_interval_mul_cst_coeff(pr,&u_plus_l_sq_inf,&u_plus_l_sq_sup,u_plus_l_inf/2,u_plus_l_sup/2,u_plus_l_inf/2,u_plus_l_sup/2);
			elina_double_interval_mul_expr_coeff(pr,&res->inf_coeff[i],&res->sup_coeff[i],u_plus_l_inf,u_plus_l_sup,expr->inf_coeff[i],expr->sup_coeff[i]);
			double tmp1, tmp2;
			elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,-u_plus_l_sq_inf,-u_plus_l_sq_sup,expr->inf_coeff[i],expr->sup_coeff[i]);
			res->inf_cst = res->inf_cst + tmp1 + pr->min_denormal;
			res->sup_cst = res->sup_cst + tmp2 + pr->min_denormal;
			
			/* double tmp1, tmp2; */
			/* elina_double_interval_mul(&tmp1,&tmp2,expr->inf_coeff[i], expr->sup_coeff[i], sqr_inf, -sqr_inf); */
			/* res->inf_cst = res->inf_cst + tmp1 + pr->min_denormal; */
			/* res->sup_cst = res->sup_cst + tmp2 + pr->min_denormal; */
		}
		else{
		  /* printf("c\n"); */
			res->inf_coeff[i] = 0.0;
			res->sup_coeff[i] = 0.0;
			double tmp1, tmp2;
			elina_double_interval_mul(&tmp1,&tmp2,expr->inf_coeff[i], expr->sup_coeff[i], sqr_inf, sqr_sup);
			res->inf_cst = res->inf_cst + tmp1;
			res->sup_cst = res->sup_cst + tmp2;
		}
		/* printf("[%d] res->inf_cst = %lf, res->sup_cst = %lf\n", (int)i, res->inf_cst, res->sup_cst); */
	}
	if(expr->type==SPARSE){
		res->dim = (size_t*)malloc(num_neurons*sizeof(size_t));
		for(i=0; i < num_neurons; i++){
			res->dim[i] = expr->dim[i];
		}
	}
	return res;
}

expr_t * uexpr_replace_parabola_bounds(fppoly_internal_t *pr, expr_t * expr, neuron_t ** neurons){
	size_t num_neurons = expr->size;
	size_t i, k;
	expr_t * res = alloc_expr();
	res->inf_coeff = (double *)malloc(num_neurons*sizeof(double));
	res->sup_coeff = (double *)malloc(num_neurons*sizeof(double));
	res->inf_cst = expr->inf_cst;
	res->sup_cst = expr->sup_cst;
	res->type = expr->type;
	res->size = num_neurons;
	
	for(i = 0; i < num_neurons; i++){
		if(expr->type==DENSE){
			k = i;
		}
		else{
			k = expr->dim[i];
		}
		neuron_t *neuron_k = neurons[k];
		double lb = neurons[k]->lb;
		double ub = neurons[k]->ub;
		res->inf_coeff[i] = 0.0;
		res->sup_coeff[i] = 0.0;
		if((expr->sup_coeff[i]==0) && (expr->inf_coeff[i]==0)){
			continue;
		}
		double u_plus_l_sup = (ub-lb);
		double u_plus_l_inf = -(ub-lb);

		double sqr_inf, sqr_sup;
		compute_square(lb, ub, &sqr_inf, &sqr_sup);
		if (ub + lb < 1e-4) {
		  res->inf_coeff[i] = 0.0;
		  res->sup_coeff[i] = 0.0;
		  double tmp1, tmp2;
		  elina_double_interval_mul(&tmp1,&tmp2,expr->inf_coeff[i], expr->sup_coeff[i], sqr_inf, sqr_sup);
		  res->inf_cst = res->inf_cst + tmp1;
		  res->sup_cst = res->sup_cst + tmp2;
		  continue;
		}
		
		if(expr->inf_coeff[i]<0){
		  double lu_sup;
		  double lu_inf;
		  elina_double_interval_mul_cst_coeff(pr,&lu_inf,&lu_sup,lb,-lb,-ub,ub);
			elina_double_interval_mul_expr_coeff(pr,&res->inf_coeff[i],&res->sup_coeff[i],u_plus_l_inf,u_plus_l_sup,expr->inf_coeff[i],expr->sup_coeff[i]);
			double tmp1, tmp2;
			elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,lu_sup,lu_inf,expr->inf_coeff[i],expr->sup_coeff[i]);
			res->inf_cst = res->inf_cst + tmp1 + pr->min_denormal;
			res->sup_cst = res->sup_cst + tmp2 + pr->min_denormal;
		}
		else if (expr->sup_coeff[i]<0){
		    res->inf_coeff[i] = 0;
			res->sup_coeff[i] = 0;
			double u_plus_l_sq_inf, u_plus_l_sq_sup;
			elina_double_interval_mul_cst_coeff(pr,&u_plus_l_sq_inf,&u_plus_l_sq_sup,u_plus_l_inf/2,u_plus_l_sup/2,u_plus_l_inf/2,u_plus_l_sup/2);
			elina_double_interval_mul_expr_coeff(pr,&res->inf_coeff[i],&res->sup_coeff[i],u_plus_l_inf,u_plus_l_sup,expr->inf_coeff[i],expr->sup_coeff[i]);
			double tmp1, tmp2;
			elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,-u_plus_l_sq_inf,-u_plus_l_sq_sup,expr->inf_coeff[i],expr->sup_coeff[i]);
			res->inf_cst = res->inf_cst + tmp1 + pr->min_denormal;
			res->sup_cst = res->sup_cst + tmp2 + pr->min_denormal;
			
			/* double tmp1, tmp2; */
			/* elina_double_interval_mul(&tmp1,&tmp2,expr->inf_coeff[i], expr->sup_coeff[i], sqr_inf, -sqr_inf); */
			/* res->inf_cst = res->inf_cst + tmp1 + pr->min_denormal; */
			/* res->sup_cst = res->sup_cst + tmp2 + pr->min_denormal; */
		}
		else {
			res->inf_coeff[i] = 0.0;
			res->sup_coeff[i] = 0.0;
			double tmp1, tmp2;
			elina_double_interval_mul(&tmp1,&tmp2,expr->inf_coeff[i], expr->sup_coeff[i], sqr_inf, sqr_sup);
			res->inf_cst = res->inf_cst + tmp1;
			res->sup_cst = res->sup_cst + tmp2;
		}
	}

	
	if(expr->type==SPARSE){
		res->dim = (size_t*)malloc(num_neurons*sizeof(size_t));
		for(i=0; i < num_neurons; i++){
			res->dim[i] = expr->dim[i];
		}
	}
	return res;
}



expr_t * lexpr_replace_log_bounds(fppoly_internal_t * pr, expr_t * expr, neuron_t ** neurons){
	size_t num_neurons = expr->size;
	size_t i,k;
	expr_t * res = alloc_expr();  
	res->inf_coeff = (double *)malloc(num_neurons*sizeof(double));
	res->sup_coeff = (double *)malloc(num_neurons*sizeof(double));
	res->inf_cst = expr->inf_cst;
	res->sup_cst = expr->sup_cst;
	res->type = expr->type;
	res->size = num_neurons;
	
	for(i = 0; i < num_neurons; i++){
		if(expr->type==DENSE){
			k = i;
		}
		else{
			k = expr->dim[i];
		}
		neuron_t *neuron_k = neurons[k];
		double lb = neurons[k]->lb;
		double ub = neurons[k]->ub;

		if (-lb < 0) {
		  /* printf("negative value at log!!!\n"); */
		  res->inf_coeff[i] = 0.0;
		  res->sup_coeff[i] = 0.0;
		  res->inf_cst = INFINITY;
		  res->sup_cst = INFINITY;
		}

		res->inf_coeff[i] = 0.0;
		res->sup_coeff[i] = 0.0;
		if((expr->sup_coeff[i]==0) && (expr->inf_coeff[i]==0)){
			continue;
		}
		double u_plus_l_inf = -(lb+ub);
		double u_plus_l_sup = -u_plus_l_inf;
		double u_minus_l_sup = ub +lb;
		double u_minus_l_inf = -(ub+lb);
		
		if (ub + lb < 1e-4) {
		  double tmp1, tmp2;
		  double log_lb = -log(-lb), log_ub = log(-lb);
		  if (log_lb > 0) {
			log_lb = -log_lb;
			log_ub = -log_ub;
		  }
		  elina_double_interval_mul(&tmp1, &tmp2, expr->inf_coeff[i], expr->sup_coeff[i], -log(-lb), log(-lb));
		  res->inf_cst = res->inf_cst + tmp1 + pr->min_denormal;
		  res->sup_cst = res->sup_cst + tmp2 + pr->min_denormal;
		  continue;
		}
		
		//= (u_plus_l)*(u_plus_l)
		if(expr->sup_coeff[i]<0){
			double one_inf = 1;
			double one_sup = -1;
			double u_plus_l_sup = ub-lb;
			double u_plus_l_inf = -(ub-lb);

			double lambda_sup = -2/u_plus_l_inf;
			double lambda_inf = -2/u_plus_l_sup;
			elina_double_interval_mul_expr_coeff(pr,&res->inf_coeff[i],&res->sup_coeff[i],lambda_inf,lambda_sup,expr->inf_coeff[i],expr->sup_coeff[i]);
	
			
			fesetround(FE_DOWNWARD);
			double mu_inf = -log(-u_plus_l_inf/2);
			fesetround(FE_UPWARD);
			double mu_sup = log(u_plus_l_sup/2);			
			elina_double_interval_add_cst_coeff(pr,&mu_inf,&mu_sup,one_inf, one_sup, mu_inf, mu_sup);
			double tmp1, tmp2;
			elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,mu_inf,mu_sup,expr->inf_coeff[i],expr->sup_coeff[i]);
			res->inf_cst = res->inf_cst + tmp1 + pr->min_denormal;
			res->sup_cst = res->sup_cst + tmp2 + pr->min_denormal;
		}
		else if (expr->inf_coeff[i]<0){
		  double inv_u_by_l_sup = -1/u_minus_l_inf;
		  double inv_u_by_l_inf = -1/u_minus_l_sup;

		  double u_by_l_sup = -ub/lb;
		  double u_by_l_inf = ub/lb;

		  fesetround(FE_DOWNWARD);			
		  double log_u_by_l_inf = -log(-u_by_l_inf);
		  double log_l_inf = -log(-lb);

		  fesetround(FE_UPWARD);
		  double log_u_by_l_sup = log(u_by_l_sup);
		  double log_l_sup = log(-lb);

		  double lambda_inf, lambda_sup;
		  elina_double_interval_mul_cst_coeff(pr,&lambda_inf,&lambda_sup,log_u_by_l_inf,log_u_by_l_sup,inv_u_by_l_inf,inv_u_by_l_sup);
		  elina_double_interval_mul_expr_coeff(pr,&res->inf_coeff[i],&res->sup_coeff[i],lambda_inf,lambda_sup,expr->inf_coeff[i],expr->sup_coeff[i]);

		  double mu_inf, mu_sup;
		  elina_double_interval_mul_cst_coeff(pr,&mu_inf,&mu_sup,-lb,lb,lambda_inf,lambda_sup);
		  elina_double_interval_add_cst_coeff(pr,&mu_inf,&mu_sup,log_l_inf, log_l_sup, mu_inf, mu_sup);

		  double tmp1, tmp2;
		  elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,mu_inf,mu_sup,expr->inf_coeff[i],expr->sup_coeff[i]);
		  res->inf_cst = res->inf_cst + tmp1 + pr->min_denormal;
		  res->sup_cst = res->sup_cst + tmp2 + pr->min_denormal;
		}
		else{
			res->inf_coeff[i] = 0.0;
			res->sup_coeff[i] = 0.0;
			double tmp1, tmp2;
			elina_double_interval_mul(&tmp1,&tmp2,expr->inf_coeff[i], expr->sup_coeff[i], -log(-lb), log(ub));	
			res->inf_cst = res->inf_cst + tmp1;
			res->sup_cst = res->sup_cst + tmp2;
		}
	}
	
	if(expr->type==SPARSE){
		res->dim = (size_t*)malloc(num_neurons*sizeof(size_t));
		for(i=0; i < num_neurons; i++){
			res->dim[i] = expr->dim[i];
		}
	}

	return res;
}

expr_t * uexpr_replace_log_bounds(fppoly_internal_t *pr, expr_t * expr, neuron_t ** neurons){
	size_t num_neurons = expr->size;
	size_t i, k;
	expr_t * res = alloc_expr();
	res->inf_coeff = (double *)malloc(num_neurons*sizeof(double));
	res->sup_coeff = (double *)malloc(num_neurons*sizeof(double));
	res->inf_cst = expr->inf_cst;
	res->sup_cst = expr->sup_cst;
	res->type = expr->type;
	res->size = num_neurons;

	for(i = 0; i < num_neurons; i++){
		if(expr->type==DENSE){
			k = i;
		}
		else{
			k = expr->dim[i];
		}
		neuron_t *neuron_k = neurons[k];
		double lb = neurons[k]->lb;
		double ub = neurons[k]->ub;
		res->inf_coeff[i] = 0.0;
		res->sup_coeff[i] = 0.0;
		if((expr->sup_coeff[i]==0) && (expr->inf_coeff[i]==0)){
			continue;
		}
		double u_plus_l_inf = -(lb+ub);
		double u_plus_l_sup = -u_plus_l_inf;

		if (-lb < 0) {
		  /* printf("negative value at log!!!\n"); */
		  res->inf_coeff[i] = 0.0;
		  res->sup_coeff[i] = 0.0;
		  res->inf_cst = INFINITY;
		  res->sup_cst = INFINITY;
		}
		if (ub + lb < 1e-4) {
		  double tmp1, tmp2;
		  double log_lb = -log(-lb), log_ub = log(-lb);
		  if (log_lb > 0) {
			log_lb = -log_lb;
			log_ub = -log_ub;
		  }
		  elina_double_interval_mul(&tmp1, &tmp2, expr->inf_coeff[i], expr->sup_coeff[i], -log(-lb), log(-lb));
		  res->inf_cst = res->inf_cst + tmp1 + pr->min_denormal;
		  res->sup_cst = res->sup_cst + tmp2 + pr->min_denormal;
		  continue;
		}
		
		//= (u_plus_l)*(u_plus_l)
		if(expr->inf_coeff[i]<0){
			double one_inf = 1;
			double one_sup = -1;
			double u_plus_l_sup = ub-lb;
			double u_plus_l_inf = -(ub-lb);

			double lambda_sup = -2/u_plus_l_inf;
			double lambda_inf = -2/u_plus_l_sup;
			elina_double_interval_mul_expr_coeff(pr,&res->inf_coeff[i],&res->sup_coeff[i],lambda_inf,lambda_sup,expr->inf_coeff[i],expr->sup_coeff[i]);
	
			
			fesetround(FE_DOWNWARD);
			double mu_inf = -log(-u_plus_l_inf/2);
			fesetround(FE_UPWARD);
			double mu_sup = log(u_plus_l_sup/2);
			elina_double_interval_add_cst_coeff(pr,&mu_inf,&mu_sup,one_inf, one_sup, mu_inf, mu_sup);
			double tmp1, tmp2;
			elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,mu_inf,mu_sup,expr->inf_coeff[i],expr->sup_coeff[i]);
			res->inf_cst = res->inf_cst + tmp1 + pr->min_denormal;
			res->sup_cst = res->sup_cst + tmp2 + pr->min_denormal;
		}
		else if (expr->sup_coeff[i]<0){
			double u_minus_l_sup = ub +lb;
			double u_minus_l_inf = -(ub+lb);

			double inv_u_by_l_sup = -1/u_minus_l_inf;
			double inv_u_by_l_inf = -1/u_minus_l_sup;

			double u_by_l_sup = -ub/lb;
			double u_by_l_inf = ub/lb;

			fesetround(FE_DOWNWARD);
			double log_u_by_l_inf = -log(-u_by_l_inf);
			double log_l_inf = -log(-lb);
			
			fesetround(FE_UPWARD);
			double log_u_by_l_sup = log(u_by_l_sup);
			double log_l_sup = log(-lb);

			double lambda_inf, lambda_sup;
			elina_double_interval_mul_cst_coeff(pr,&lambda_inf,&lambda_sup,log_u_by_l_inf,log_u_by_l_sup,inv_u_by_l_inf,inv_u_by_l_sup);
			elina_double_interval_mul_expr_coeff(pr,&res->inf_coeff[i],&res->sup_coeff[i],lambda_inf,lambda_sup,expr->inf_coeff[i],expr->sup_coeff[i]);

			double mu_inf, mu_sup;
			elina_double_interval_mul_cst_coeff(pr,&mu_inf,&mu_sup,-lb,lb,lambda_inf,lambda_sup);
			elina_double_interval_add_cst_coeff(pr,&mu_inf,&mu_sup,log_l_inf, log_l_sup, mu_inf, mu_sup);

			double tmp1, tmp2;
			elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,mu_inf,mu_sup,expr->inf_coeff[i],expr->sup_coeff[i]);
			res->inf_cst = res->inf_cst + tmp1 + pr->min_denormal;
			res->sup_cst = res->sup_cst + tmp2 + pr->min_denormal;
		}
		else{
  		    res->inf_coeff[i] = 0.0;
			res->sup_coeff[i] = 0.0;
			double tmp1, tmp2;
			elina_double_interval_mul(&tmp1,&tmp2,expr->inf_coeff[i], expr->sup_coeff[i], -log(-lb), log(ub));	
			res->inf_cst = res->inf_cst + tmp1;
			res->sup_cst = res->sup_cst + tmp2;
		}
	}

	

	if(expr->type==SPARSE){
		res->dim = (size_t*)malloc(num_neurons*sizeof(size_t));
		for(i=0; i < num_neurons; i++){
			res->dim[i] = expr->dim[i];
		}
	}
	
	return res;
}


expr_t * lexpr_replace_relu_bounds(fppoly_internal_t * pr, expr_t * expr, neuron_t ** neurons){
	size_t num_neurons = expr->size;
	size_t i,k;
	expr_t * res = alloc_expr();  
	res->inf_coeff = (double *)malloc(num_neurons*sizeof(double));
	res->sup_coeff = (double *)malloc(num_neurons*sizeof(double));
	res->inf_cst = expr->inf_cst;
	res->sup_cst = expr->sup_cst;
	res->type = expr->type;
	res->size = num_neurons;  

	for(i = 0; i < num_neurons; i++){
		if(expr->type==DENSE){
			k = i;
		}
		else{
			k = expr->dim[i];
		}
		neuron_t *neuron_k = neurons[k];
		double lb = neurons[k]->lb;
		double ub = neurons[k]->ub;

		double width = ub + lb;
		double lambda_inf = -ub/width;
		double lambda_sup = ub/width;
		if((expr->sup_coeff[i]==0) && (expr->inf_coeff[i]==0)){
			res->inf_coeff[i] = 0.0;
			res->sup_coeff[i] = 0.0;
			continue;
		}
		else if(neuron_k->ub<=0){
			res->inf_coeff[i] = 0.0;
			res->sup_coeff[i] = 0.0;
			continue;
		}
		else if(neuron_k->lb<0){
			res->inf_coeff[i] = expr->inf_coeff[i];
			res->sup_coeff[i] = expr->sup_coeff[i];
		} else if(expr->sup_coeff[i]<0){
			
			double mu_inf = lambda_inf*neurons[k]->lb;
			double mu_sup = lambda_sup*neurons[k]->lb;
			//res->coeff[i] = lambda*expr->coeff[i];
			//res->cst = res->cst + expr->coeff[i]*mu;
			elina_double_interval_mul_expr_coeff(pr,&res->inf_coeff[i],&res->sup_coeff[i],lambda_inf,lambda_sup,expr->inf_coeff[i],expr->sup_coeff[i]);
			double tmp1, tmp2;
			elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,mu_inf,mu_sup,expr->inf_coeff[i],expr->sup_coeff[i]);
			res->inf_cst = res->inf_cst + tmp1 + pr->min_denormal;
			res->sup_cst = res->sup_cst + tmp2 + pr->min_denormal;
		}
		else if (expr->inf_coeff[i]<0){
			
			double area1 = lb*ub;
			double area2 = 0.5*ub*width;
			double area3 = 0.5*lb*width;
			if((area1 < area2) && (area1 < area3)){
			//if(1){
				//res->coeff[i] = lambda*expr->coeff[i];
				elina_double_interval_mul_expr_coeff(pr,&res->inf_coeff[i],&res->sup_coeff[i],lambda_inf,lambda_sup,expr->inf_coeff[i],expr->sup_coeff[i]);
				
			}
			else if((area2 < area1) && (area2 < area3)){
				res->inf_coeff[i] = 0.0;
				res->sup_coeff[i] = 0.0;
			}
			else{
				res->inf_coeff[i] = expr->inf_coeff[i];
				res->sup_coeff[i] = expr->sup_coeff[i];
			}
		}
		else{
			
			res->inf_coeff[i] = 0.0;
			res->sup_coeff[i] = 0.0;
			double tmp1, tmp2;
			elina_double_interval_mul(&tmp1,&tmp2,expr->inf_coeff[i],expr->sup_coeff[i],0,ub);
			res->inf_cst = res->inf_cst + tmp1;
			res->sup_cst = res->sup_cst + tmp2;
		}
	}
	if(expr->type==SPARSE){
		res->dim = (size_t*)malloc(num_neurons*sizeof(size_t));
		for(i=0; i < num_neurons; i++){
			res->dim[i] = expr->dim[i];
		}
	}
	return res;
}

expr_t * uexpr_replace_relu_bounds(fppoly_internal_t *pr, expr_t * expr, neuron_t ** neurons){
	size_t num_neurons = expr->size;
	size_t i, k;
	expr_t * res = alloc_expr();
	res->inf_coeff = (double *)malloc(num_neurons*sizeof(double));
	res->sup_coeff = (double *)malloc(num_neurons*sizeof(double));
	res->inf_cst = expr->inf_cst;
	res->sup_cst = expr->sup_cst;
	res->type = expr->type;
	res->size = num_neurons;  
	for(i = 0; i < num_neurons; i++){
		if(expr->type==DENSE){
			k = i;
		}
		else{
			k = expr->dim[i];
		}
		neuron_t *neuron_k = neurons[k];
		double lb = neurons[k]->lb;
		double ub = neurons[k]->ub;
		double width = ub + lb;
		double lambda_inf = -ub/width;
		double lambda_sup = ub/width;
		if((expr->sup_coeff[i]==0) && (expr->inf_coeff[i]==0)){
			res->inf_coeff[i] = 0.0;
			res->sup_coeff[i] = 0.0;
			continue;
		}
		else if(neuron_k->ub<=0){
			res->inf_coeff[i] = 0.0;
			res->sup_coeff[i] = 0.0;
			continue;
		}
		else if(neuron_k->lb<0){
			res->inf_coeff[i] = expr->inf_coeff[i];
			res->sup_coeff[i] = expr->sup_coeff[i];
		} else if(expr->inf_coeff[i]<0){
			
			double mu_inf = lambda_inf*neurons[k]->lb;
			double mu_sup = lambda_sup*neurons[k]->lb;
			//res->coeff[i] = lambda*expr->coeff[i];
			//res->cst = res->cst + expr->coeff[i]*mu;
			elina_double_interval_mul_expr_coeff(pr,&res->inf_coeff[i],&res->sup_coeff[i],lambda_inf,lambda_sup,expr->inf_coeff[i],expr->sup_coeff[i]);
			double tmp1, tmp2;
			elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,mu_inf,mu_sup,expr->inf_coeff[i],expr->sup_coeff[i]);
			res->inf_cst = res->inf_cst + tmp1 + pr->min_denormal;
			res->sup_cst = res->sup_cst + tmp2 + pr->min_denormal;
		}
		else if(expr->sup_coeff[i]<0){
			
			double area1 = lb*ub;
			double area2 = 0.5*ub*width;
			double area3 = 0.5*lb*width;
			if((area1 < area2) && (area1 < area3)){
			//if(1){
				//res->coeff[i] = lambda*expr->coeff[i];
				elina_double_interval_mul_expr_coeff(pr,&res->inf_coeff[i],&res->sup_coeff[i],lambda_inf,lambda_sup,expr->inf_coeff[i],expr->sup_coeff[i]);
			}
			else if((area2 < area1) && (area2 < area3)){
				res->inf_coeff[i] = 0.0;
				res->sup_coeff[i] = 0.0;
			}
			else{
				res->inf_coeff[i] = expr->inf_coeff[i];
				res->sup_coeff[i] = expr->sup_coeff[i];
			}
			//
		}
		else{
			
			res->inf_coeff[i] = 0.0;
			res->sup_coeff[i] = 0.0;
			double tmp1, tmp2;
			elina_double_interval_mul(&tmp1,&tmp2,expr->inf_coeff[i],expr->sup_coeff[i],0,ub);
			res->inf_cst = res->inf_cst + tmp1;
			res->sup_cst = res->sup_cst + tmp2;
		}
		
	}
	if(expr->type==SPARSE){
		res->dim = (size_t*)malloc(num_neurons*sizeof(size_t));
		for(i=0; i < num_neurons; i++){
			res->dim[i] = expr->dim[i];
		}
	}
	return res;
}

void compute_chord_slope(double *slope_inf, double *slope_sup, double f_sup_l, double f_sup_u, 
			 double f_inf_l, double f_inf_u, double inf_l, double inf_u, double sup_l, double sup_u){
	double num_l =  f_sup_l + f_inf_u;
	double num_u =  f_sup_u + f_inf_l;

	double den_l = sup_l + inf_u; 
	double den_u = sup_u + inf_l;

	elina_double_interval_div(slope_inf, slope_sup, num_l, num_u, den_l, den_u);
}

void compute_derivative(double *slope_inf, double *slope_sup, double s_curve_l, double s_curve_u, double sq_l, double sq_u, bool is_sigmoid){
    double sq_den_sup_l, sq_den_sup_u;
    elina_double_interval_mul(&sq_den_sup_l, &sq_den_sup_u, sq_l, sq_u, sq_l, sq_u);
    if(is_sigmoid){
            elina_double_interval_div(slope_inf, slope_sup, s_curve_l, s_curve_u, sq_den_sup_l, sq_den_sup_u);
    }
    else{
            *slope_inf = -1 + sq_den_sup_u;
            *slope_sup = 1 + sq_den_sup_l;
    }

}

void compute_slope_and_intercept_s_curve_lexpr(fppoly_internal_t * pr, double * slope_inf, double *slope_sup, 
						double *intercept_inf, double *intercept_sup, double inf_coeff, 
						double sup_coeff, double lb, double ub, bool is_sigmoid, bool *boxify){
	
  fesetround(FE_DOWNWARD);
  double e_sup_l = is_sigmoid ? -exp(ub) : -tanh(ub);
  double e_inf_l = is_sigmoid ? -exp(-lb) : -tanh(-lb);
  fesetround(FE_UPWARD); 
  double e_sup_u = is_sigmoid ? exp(ub) : tanh(ub);
  double e_inf_u = is_sigmoid ? exp(-lb) : tanh(-lb);
  double f_sup_l, f_sup_u;
  double f_inf_l, f_inf_u;
  double den_sup_l, den_sup_u;
  double den_inf_l, den_inf_u;
  if(is_sigmoid){
	den_sup_l = -1 + e_sup_l;
	den_sup_u = 1 + e_sup_u;				
	den_inf_l = -1 + e_inf_l;
	den_inf_u = 1 + e_inf_u;		
	elina_double_interval_div(&f_sup_l, &f_sup_u, e_sup_l, e_sup_u, den_sup_l, den_sup_u);
	elina_double_interval_div(&f_inf_l, &f_inf_u, e_inf_l, e_inf_u, den_inf_l, den_inf_u);
  }
  else{
	f_inf_l = e_inf_l;
	f_inf_u = e_inf_u;
	f_sup_l = e_sup_l;
	f_sup_u = e_sup_u;
	den_inf_l = e_inf_l;
	den_inf_u = e_inf_u;
	den_sup_l = e_sup_l;
	den_sup_u = e_sup_u;
  }
  
  if((-lb==ub)|| (-f_inf_l==f_sup_u)){
	*slope_inf = 0.0;
	*slope_sup = 0.0;	
	//
	double tmp1, tmp2;
	elina_double_interval_mul(&tmp1,&tmp2,inf_coeff,sup_coeff,f_inf_l,f_sup_u);
	*intercept_inf = tmp1;
	*intercept_sup = tmp2;
	*boxify = true;	
  }
  else if(sup_coeff < 0 || inf_coeff < 0){
	double add_inf, add_sup;
	double mul_inf, mul_sup;
	double x_l, x_u;
	double f_x_l, f_x_u;
	if(sup_coeff < 0){
	  if(ub<0){
		compute_chord_slope(slope_inf, slope_sup, f_sup_l, f_sup_u, f_inf_l, f_inf_u, lb, -lb, -ub, ub);		
		x_l = ub;
		x_u = -ub;
		f_x_l = f_sup_l;
		f_x_u = f_sup_u;
		
		elina_double_interval_mul_cst_coeff(pr,&add_inf,&add_sup,x_l, x_u, *slope_inf,*slope_sup);
		elina_double_interval_add_cst_coeff(pr,intercept_inf,intercept_sup,f_x_l, f_x_u, add_inf, add_sup);
		double tmp1, tmp2, tmp3, tmp4;
		elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,lb, -lb, *slope_inf,*slope_sup);
		elina_double_interval_add_cst_coeff(pr,&tmp3,&tmp4,*intercept_inf, *intercept_sup, tmp1, tmp2);
		if(tmp4<f_inf_u){
		  *boxify = true;
		}
	  }
	  else if(lb<=0){
		compute_derivative(slope_inf, slope_sup, e_sup_l, e_sup_u, den_sup_l, den_sup_u, is_sigmoid);
		x_l = ub;
		x_u = -ub;
		f_x_l = f_sup_l;
		f_x_u = f_sup_u;
		elina_double_interval_mul_cst_coeff(pr,&add_inf,&add_sup,x_l, x_u, *slope_inf,*slope_sup);
		elina_double_interval_add_cst_coeff(pr,intercept_inf,intercept_sup,f_x_l, f_x_u, add_inf, add_sup);
		double tmp1, tmp2, tmp3, tmp4;
		elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,lb, -lb, *slope_inf,*slope_sup);
		elina_double_interval_add_cst_coeff(pr,&tmp3,&tmp4,*intercept_inf, *intercept_sup, tmp1, tmp2);
		if(-tmp3<f_inf_u){
		  
		  *boxify = true;
		}
	  }
	  else{
		if(lb<=ub){
		  //double slope_inf1, slope_sup1;
		  //double slope_inf2, slope_sup2;
		  compute_derivative(slope_inf, slope_sup, e_sup_l, e_sup_u, den_sup_l, den_sup_u, is_sigmoid);
          
		}
		else{
		  compute_derivative(slope_inf, slope_sup, e_inf_l, e_inf_u, den_inf_l, den_inf_u, is_sigmoid);
		}
		x_l = ub;
		x_u = -ub;
		f_x_l = f_sup_l;
		f_x_u = f_sup_u;
		elina_double_interval_mul_cst_coeff(pr,&add_inf,&add_sup,x_l, x_u, *slope_inf,*slope_sup);
		elina_double_interval_add_cst_coeff(pr,intercept_inf, intercept_sup,f_x_l, f_x_u, add_inf, add_sup);
		double tmp1, tmp2, tmp3, tmp4;
		elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,lb, -lb, *slope_inf,*slope_sup);
		elina_double_interval_add_cst_coeff(pr,&tmp3,&tmp4,*intercept_inf, *intercept_sup, tmp1, tmp2);
		if(-tmp3<f_inf_u){      
		  *boxify = true;
		}
	  }
	}
	else{
	  if(ub < 0){
		compute_derivative( slope_inf, slope_sup, e_inf_l, e_inf_u, den_inf_l, den_inf_u, is_sigmoid);
		x_l = -lb;
		x_u = lb;
		f_x_l = f_inf_l;
		f_x_u = f_inf_u;
		elina_double_interval_mul_cst_coeff(pr,&add_inf,&add_sup,x_l, x_u, *slope_inf,*slope_sup);
		elina_double_interval_add_cst_coeff(pr, intercept_inf, intercept_sup,f_x_l, f_x_u, add_inf, add_sup);
		double tmp1, tmp2, tmp3, tmp4;
		elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,-ub, ub, *slope_inf,*slope_sup);
		elina_double_interval_add_cst_coeff(pr,&tmp3,&tmp4,*intercept_inf, *intercept_sup, tmp1, tmp2);
		if(tmp4>-f_sup_l){
		  *boxify = true;
		}	
	  }
	  else if(lb<=0){
		compute_chord_slope(slope_inf, slope_sup, f_sup_l, f_sup_u, f_inf_l, f_inf_u, lb, -lb, -ub, ub);
			    
		x_l = -lb;
		x_u = lb;
		f_x_l = f_inf_l;
		f_x_u = f_inf_u;
		elina_double_interval_mul(&add_inf,&add_sup,x_l, x_u, *slope_inf,*slope_sup);
		elina_double_interval_add_cst_coeff(pr,intercept_inf, intercept_sup,f_x_l, f_x_u, add_inf, add_sup);
		double tmp1, tmp2, tmp3, tmp4;
		elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,-ub, ub, *slope_inf,*slope_sup);
		elina_double_interval_add_cst_coeff(pr,&tmp3,&tmp4, *intercept_inf, *intercept_sup, tmp1, tmp2);
				
		if(-tmp3>f_sup_u){	
		  *boxify = true;
		}
	  }
	  else{
		if(lb<=ub){
		  compute_derivative( slope_inf, slope_sup, e_sup_l, e_sup_u, den_sup_l, den_sup_u, is_sigmoid);
		}
		else{
		  compute_derivative( slope_inf, slope_sup, e_inf_l, e_inf_u, den_inf_l, den_inf_u, is_sigmoid);
		}
		x_l = -lb;
		x_u = lb;
		f_x_l = f_inf_l;
		f_x_u = f_inf_u;
		elina_double_interval_mul_cst_coeff(pr,&add_inf,&add_sup,x_l, x_u, *slope_inf,*slope_sup);
		elina_double_interval_add_cst_coeff(pr,intercept_inf, intercept_sup,f_x_l, f_x_u, add_inf, add_sup);
		double tmp1, tmp2, tmp3, tmp4;
		elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,-ub, ub, *slope_inf,*slope_sup);
		elina_double_interval_add_cst_coeff(pr,&tmp3,&tmp4, *intercept_inf, *intercept_sup, tmp1, tmp2);
		if(tmp4>-f_sup_l){
		               
		  *boxify = true;
		}
	  }        
	}
	if(*boxify){
	  *slope_inf = 0.0;
	  *slope_sup = 0.0;	
	  double tmp1, tmp2;
	  elina_double_interval_mul(&tmp1,&tmp2,inf_coeff, sup_coeff,f_inf_l,f_sup_u);
	  *intercept_inf = tmp1;
	  *intercept_sup = tmp2;
			
	}			
  }
  else{
			
	*slope_inf = 0.0;
	*slope_sup = 0.0;
	double tmp1, tmp2;
	elina_double_interval_mul(&tmp1,&tmp2,inf_coeff,sup_coeff,f_inf_l,f_sup_u);
	*intercept_inf = tmp1;
	*intercept_sup = tmp2;
	*boxify = true;
  }
		
  return;
}

void compute_slope_and_intercept_s_curve_uexpr(fppoly_internal_t * pr, double * slope_inf, double *slope_sup, 
						double *intercept_inf, double *intercept_sup, double inf_coeff, 
						double sup_coeff, double lb, double ub, bool is_sigmoid, bool *boxify){
	fesetround(FE_DOWNWARD);
        double e_sup_l = is_sigmoid ? -exp(ub) : -tanh(ub);
        double e_inf_l = is_sigmoid ? -exp(-lb) : -tanh(-lb);
        
        fesetround(FE_UPWARD);
        double e_sup_u = is_sigmoid ? exp(ub) : tanh(ub);
        double e_inf_u = is_sigmoid ? exp(-lb) : tanh(-lb);
        
        double f_sup_l, f_sup_u;
        double f_inf_l, f_inf_u;
        double den_sup_l, den_sup_u;
        double den_inf_l, den_inf_u;
        double connecting_slope_l, connecting_slope_u;
        
        if(is_sigmoid){
            den_sup_l = -1 + e_sup_l;
            den_sup_u = 1 + e_sup_u;
            den_inf_l = -1 + e_inf_l;
            den_inf_u = 1 + e_inf_u;
            elina_double_interval_div(&f_sup_l, &f_sup_u, e_sup_l, e_sup_u, den_sup_l, den_sup_u);
            elina_double_interval_div(&f_inf_l, &f_inf_u, e_inf_l, e_inf_u, den_inf_l, den_inf_u);
        }
        else{
            f_inf_l = e_inf_l;
            f_inf_u = e_inf_u;
            f_sup_l = e_sup_l;
            f_sup_u = e_sup_u;
            den_inf_l = e_inf_l;
            den_inf_u = e_inf_u;
            den_sup_l = e_sup_l;
            den_sup_u = e_sup_u;
        }
        
        if((ub + lb < 0.01) || (-f_inf_l==f_sup_u)){
	    *slope_inf = 0.0;
	    *slope_sup = 0.0;
	    *boxify = true;
            double tmp1, tmp2;
            elina_double_interval_mul(&tmp1,&tmp2,inf_coeff,sup_coeff,f_inf_l,f_sup_u);
	    *intercept_inf = tmp1;
	    *intercept_sup = tmp2;
            
        }
       
        else if(sup_coeff <0 || inf_coeff < 0){
            double add_inf, add_sup;
            double mul_inf, mul_sup;
            double x_l, x_u;
            double f_x_l, f_x_u;
            
            if(sup_coeff < 0){
                if(ub<0){
			
                    compute_derivative(slope_inf, slope_sup, e_inf_l, e_inf_u, den_inf_l, den_inf_u, is_sigmoid);
                    x_l = -lb;
                    x_u = lb;
                    f_x_l = f_inf_l;
                    f_x_u = f_inf_u;
                    elina_double_interval_mul_cst_coeff(pr,&add_inf,&add_sup,x_l, x_u, *slope_inf,*slope_sup);
                    elina_double_interval_add_cst_coeff(pr,intercept_inf,intercept_sup,f_x_l, f_x_u, add_inf, add_sup);
		    double tmp1, tmp2, tmp3, tmp4;
                    elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,-ub, ub, *slope_inf,*slope_sup);
                    elina_double_interval_add_cst_coeff(pr,&tmp3,&tmp4,*intercept_inf, *intercept_sup, tmp1, tmp2);
                    if(tmp4>-f_sup_l){
                        *boxify = true;
                    }
                }
                else if(lb<=0){
		   		
		            compute_chord_slope(slope_inf, slope_sup, f_sup_l, f_sup_u, f_inf_l, f_inf_u, lb, -lb, -ub, ub);
			   
			    x_l = -lb;
			    x_u = lb;
			    f_x_l = f_inf_l;
			    f_x_u = f_inf_u;
			    elina_double_interval_mul_cst_coeff(pr,&add_inf,&add_sup,x_l, x_u, *slope_inf,*slope_sup);
			    elina_double_interval_add_cst_coeff(pr,intercept_inf,intercept_sup,f_x_l, f_x_u, add_inf, add_sup);
			    double tmp1, tmp2, tmp3, tmp4;
                    	    elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,-ub, ub, *slope_inf,*slope_sup);
                            elina_double_interval_add_cst_coeff(pr,&tmp3,&tmp4, *intercept_inf, *intercept_sup, tmp1, tmp2);
                    	    if(-tmp3>f_sup_u){
                        	*boxify = true;
                    	    }
		            //}
                }
                else{
                    //double slope_inf1, slope_sup1;
                    //double slope_inf2, slope_sup2;
                    if(lb<=ub){
                         compute_derivative(slope_inf, slope_sup, e_sup_l, e_sup_u, den_sup_l, den_sup_u, is_sigmoid);
                    
                    }
                    else{
                         compute_derivative( slope_inf, slope_sup, e_inf_l, e_inf_u, den_inf_l, den_inf_u, is_sigmoid);
                    }
                    
                    x_l = -lb;
                    x_u = lb;
                    f_x_l = f_inf_l;
                    f_x_u = f_inf_u;
                    elina_double_interval_mul_cst_coeff(pr,&add_inf,&add_sup,x_l, x_u, *slope_inf,*slope_sup);
                    elina_double_interval_add_cst_coeff(pr,intercept_inf,intercept_sup,f_x_l, f_x_u, add_inf, add_sup);
                    double tmp1, tmp2, tmp3, tmp4;
                    elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,-ub, ub, *slope_inf,*slope_sup);
                    elina_double_interval_add_cst_coeff(pr,&tmp3,&tmp4,*intercept_inf, *intercept_sup, tmp1, tmp2);
                    if(tmp4>-f_sup_l){
                        *boxify = true;
                    }
                }
               
            }
            else{
                if(ub < 0){
		   		
		     compute_chord_slope(slope_inf, slope_sup, f_sup_l, f_sup_u, f_inf_l, f_inf_u, lb, -lb, -ub, ub);
		           
		     x_l = ub;
		     x_u = -ub;
		     f_x_l = f_sup_l;
		     f_x_u = f_sup_u;
		     elina_double_interval_mul_cst_coeff(pr,&add_inf,&add_sup,x_l, x_u, *slope_inf,*slope_sup);
		     elina_double_interval_add_cst_coeff(pr,intercept_inf,intercept_sup,f_x_l, f_x_u, add_inf, add_sup);
		     double tmp1, tmp2, tmp3, tmp4;
                     elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,lb, -lb, *slope_inf,*slope_sup);
                     elina_double_interval_add_cst_coeff(pr,&tmp3,&tmp4,*intercept_inf, *intercept_sup, tmp1, tmp2);
			
                    if(tmp4<f_inf_u){
                        *boxify = true;
                    }
                }
                else if(lb<=0){
			
                    compute_derivative( slope_inf, slope_sup, e_sup_l, e_sup_u, den_sup_l, den_sup_u, is_sigmoid);
			
                    x_l = ub;
                    x_u = -ub;
                    f_x_l = f_sup_l;
                    f_x_u = f_sup_u;
                    elina_double_interval_mul_cst_coeff(pr,&add_inf,&add_sup,x_l, x_u, *slope_inf,*slope_sup);
                    elina_double_interval_add_cst_coeff(pr,intercept_inf,intercept_sup,f_x_l, f_x_u, add_inf, add_sup);
                    double tmp1, tmp2, tmp3, tmp4;
                    elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,lb, -lb, *slope_inf,*slope_sup);
                    elina_double_interval_add_cst_coeff(pr,&tmp3,&tmp4,*intercept_inf, *intercept_sup, tmp1, tmp2);
			
                    if(-tmp3<f_inf_u){
                        *boxify = true;
                    }


                }
                else{
                    
                    if(lb<=ub){
                    	compute_derivative( slope_inf, slope_sup, e_sup_l, e_sup_u, den_sup_l, den_sup_u, is_sigmoid);
                    }
                    else{
                        compute_derivative( slope_inf, slope_sup, e_inf_l, e_inf_u, den_inf_l, den_inf_u, is_sigmoid);
                    }
                   
                    
                    x_l = ub;
                    x_u = -ub;
                    f_x_l = f_sup_l;
                    f_x_u = f_sup_u;
                    elina_double_interval_mul_cst_coeff(pr,&add_inf,&add_sup,x_l, x_u, *slope_inf,*slope_sup);
                    elina_double_interval_add_cst_coeff(pr,intercept_inf, intercept_sup,f_x_l, f_x_u, add_inf, add_sup);
                    double tmp1, tmp2, tmp3, tmp4;
                    elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,lb, -lb, *slope_inf,*slope_sup);
                    elina_double_interval_add_cst_coeff(pr,&tmp3,&tmp4,*intercept_inf, *intercept_sup, tmp1, tmp2);
                    if(-tmp3<f_inf_u){
                        *boxify = true;
                    }

                }
                
               
            }
	   
            if(*boxify){
                *slope_inf = 0.0;
		*slope_sup = 0.0;
                double tmp1, tmp2;
                elina_double_interval_mul(&tmp1,&tmp2,inf_coeff,sup_coeff,f_inf_l,f_sup_u);
		*intercept_inf = tmp1;
		*intercept_sup = tmp2;
		
            }
            
           
        }
        
        
        else{
           
            *slope_inf = 0.0;
	    *slope_sup = 0.0;
            double tmp1, tmp2;
            elina_double_interval_mul(&tmp1,&tmp2,inf_coeff, sup_coeff,f_inf_l,f_sup_u);
            *intercept_inf = tmp1;
	    *intercept_sup = tmp2;
	    *boxify = true;
        }
	

}


expr_t * lexpr_replace_s_curve_bounds(fppoly_internal_t * pr, expr_t * expr, neuron_t ** neurons, bool is_sigmoid){
	size_t num_neurons = expr->size;
	size_t i,k;
	expr_t * res = alloc_expr();  
	res->inf_coeff = (double *)malloc(num_neurons*sizeof(double));
	res->sup_coeff = (double *)malloc(num_neurons*sizeof(double));
	res->inf_cst = expr->inf_cst;
	res->sup_cst = expr->sup_cst;
	res->type = expr->type;
	res->size = num_neurons;  

	for(i = 0; i < num_neurons; i++){
		if(expr->type==DENSE){
			k = i;
		}
		else{
			k = expr->dim[i];
		}
		neuron_t *neuron_k = neurons[k];
		double lb = neurons[k]->lb;
		double ub = neurons[k]->ub;
		//if(expr->sup_coeff[i]<0 || expr->inf_coeff[i] < 0){
		double slope_inf, slope_sup;
		double intercept_inf, intercept_sup;
		double mul_inf, mul_sup;
		bool boxify = false;
		compute_slope_and_intercept_s_curve_lexpr(pr, &slope_inf, &slope_sup, 
						&intercept_inf, &intercept_sup, expr->inf_coeff[i], 
						expr->sup_coeff[i], lb, ub, is_sigmoid, &boxify);
		if(boxify){
			res->inf_coeff[i] = 0.0;
		    	res->sup_coeff[i] = 0.0;
			res->inf_cst = res->inf_cst + intercept_inf;
		   	res->sup_cst = res->sup_cst + intercept_sup;
		}
		else{
			elina_double_interval_mul_expr_coeff(pr,&res->inf_coeff[i],&res->sup_coeff[i],slope_inf,slope_sup,expr->inf_coeff[i],expr->sup_coeff[i]);
			elina_double_interval_mul_cst_coeff(pr, &mul_inf, &mul_sup, intercept_inf, intercept_sup, expr->inf_coeff[i], expr->sup_coeff[i] );
			elina_double_interval_add_cst_coeff(pr,&res->inf_cst,&res->sup_cst,mul_inf, mul_sup, res->inf_cst, res->sup_cst);
		}
	
		
	}
	if(expr->type==SPARSE){
		res->dim = (size_t*)malloc(num_neurons*sizeof(size_t));
		for(i=0; i < num_neurons; i++){
			res->dim[i] = expr->dim[i];
		}
	}
	return res;
}

expr_t * uexpr_replace_s_curve_bounds(fppoly_internal_t *pr, expr_t * expr, neuron_t ** neurons, bool is_sigmoid){
    size_t num_neurons = expr->size;
    size_t i,k;
    expr_t * res = alloc_expr();
    res->inf_coeff = (double *)malloc(num_neurons*sizeof(double));
    res->sup_coeff = (double *)malloc(num_neurons*sizeof(double));
    res->inf_cst = expr->inf_cst;
    res->sup_cst = expr->sup_cst;
    res->type = expr->type;
    res->size = num_neurons;
    
    for(i = 0; i < num_neurons; i++){
        if(expr->type==DENSE){
            k = i;
        }
        else{
            k = expr->dim[i];
        }
        neuron_t *neuron_k = neurons[k];
        double lb = neurons[k]->lb;
        double ub = neurons[k]->ub;
        double slope_inf, slope_sup;
	double intercept_inf, intercept_sup;
	double mul_inf, mul_sup;
	bool boxify = false;
	compute_slope_and_intercept_s_curve_uexpr(pr, &slope_inf, &slope_sup, 
						&intercept_inf, &intercept_sup, expr->inf_coeff[i], 
						expr->sup_coeff[i], lb, ub, is_sigmoid, &boxify);
	if(boxify){
		res->inf_coeff[i] = 0.0;
		res->sup_coeff[i] = 0.0;
		res->inf_cst = res->inf_cst + intercept_inf;
		res->sup_cst = res->sup_cst + intercept_sup;
	}
	else{
		elina_double_interval_mul_expr_coeff(pr,&res->inf_coeff[i],&res->sup_coeff[i],slope_inf,slope_sup,expr->inf_coeff[i],expr->sup_coeff[i]);
		elina_double_interval_mul_cst_coeff(pr, &mul_inf, &mul_sup, intercept_inf, intercept_sup, expr->inf_coeff[i], expr->sup_coeff[i] );
		elina_double_interval_add_cst_coeff(pr,&res->inf_cst,&res->sup_cst,mul_inf, mul_sup, res->inf_cst, res->sup_cst);
	}
    }
    if(expr->type==SPARSE){
        res->dim = (size_t*)malloc(num_neurons*sizeof(size_t));
        for(i=0; i < num_neurons; i++){
            res->dim[i] = expr->dim[i];
        }
    }
    return res;
}


expr_t * uexpr_replace_sigmoid_bounds(fppoly_internal_t *pr, expr_t * expr, neuron_t ** neurons){
    return uexpr_replace_s_curve_bounds(pr, expr, neurons, true);
}

expr_t * uexpr_replace_tanh_bounds(fppoly_internal_t *pr, expr_t * expr, neuron_t ** neurons){
    return uexpr_replace_s_curve_bounds(pr, expr, neurons, false);
}

expr_t * lexpr_replace_sigmoid_bounds(fppoly_internal_t *pr, expr_t * expr, neuron_t ** neurons){
    return lexpr_replace_s_curve_bounds(pr, expr, neurons, true);
}

expr_t * lexpr_replace_tanh_bounds(fppoly_internal_t *pr, expr_t * expr, neuron_t ** neurons){
    return lexpr_replace_s_curve_bounds(pr, expr, neurons, false);
}

expr_t * lexpr_replace_maxpool_or_lstm_bounds(fppoly_internal_t * pr, expr_t * expr, neuron_t ** neurons){
	//printf("begin\n");
	//fflush(stdout);
	size_t num_neurons = expr->size;
	size_t i,k;
	expr_t *res;
	if(expr->type==DENSE){
		k = 0;
	}
	else{
		k = expr->dim[0];
	}
	neuron_t *neuron_k = neurons[k];
	if(expr->sup_coeff[0]<0){
		//expr_print(neuron_k->uexpr);
		if(neuron_k->uexpr==NULL){
			res = (expr_t *)malloc(sizeof(expr_t));
			res->inf_coeff = res->sup_coeff =  NULL;
			res->dim = NULL;
			res->size = 0;
			res->type = SPARSE;
			elina_double_interval_mul_cst_coeff(pr,&res->inf_cst,&res->sup_cst,neuron_k->lb,neuron_k->ub,expr->inf_coeff[0],expr->sup_coeff[0]);
		}
		else{
			res = multiply_expr(pr,neuron_k->uexpr,expr->inf_coeff[0],expr->sup_coeff[0]);
		}
		//printf("multiply end %zu \n",k);
		//expr_print(res);
		//fflush(stdout);
	}
	else if(expr->inf_coeff[0]<0){
		//expr_print(neuron_k->lexpr);
		if(neuron_k->lexpr==NULL){
			res = (expr_t *)malloc(sizeof(expr_t));
			res->inf_coeff = res->sup_coeff = NULL;
			res->dim = NULL;
			res->size = 0;
			res->type = SPARSE;
			elina_double_interval_mul_cst_coeff(pr,&res->inf_cst,&res->sup_cst,neuron_k->lb,neuron_k->ub,expr->inf_coeff[0],expr->sup_coeff[0]);
		}
		else{
			res = multiply_expr(pr,neuron_k->lexpr,expr->inf_coeff[0],expr->sup_coeff[0]);
		}
		//printf("multiply end %zu \n",k);
		//expr_print(res);
		//fflush(stdout);
	}
	else{
		//printf("WTF1\n");
		//fflush(stdout);
		double tmp1, tmp2;
		elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,neuron_k->lb,neuron_k->ub,expr->inf_coeff[0],expr->sup_coeff[0]);
		double coeff[1];
		size_t dim[1];
		coeff[0] = 0;
		dim[0] = 0;
		res = create_sparse_expr(coeff,-tmp1,dim,1);
	}
	//printf("middle\n");
	//fflush(stdout);
	expr_t * mul_expr;
	for(i = 1; i < num_neurons; i++){
		if(expr->type==DENSE){
			k = i;
		}
		else{
			k = expr->dim[i];
		}
		neuron_t *neuron_k = neurons[k];
		if(expr->sup_coeff[i]<0){
			//expr_print(neuron_k->uexpr);
			//printf("add start %zu %zu\n",k,i);
			
				//expr_print(res);
				
			if(neuron_k->uexpr==NULL){
				mul_expr = (expr_t *)malloc(sizeof(expr_t));
				mul_expr->inf_coeff = mul_expr->sup_coeff = NULL;
				mul_expr->dim = NULL;
				mul_expr->size = 0;
				mul_expr->type = SPARSE;
				//printf("lb: %g %g\n");
				elina_double_interval_mul_cst_coeff(pr,&mul_expr->inf_cst,&mul_expr->sup_cst,neuron_k->lb,neuron_k->ub,expr->inf_coeff[i],expr->sup_coeff[i]);
				res->inf_cst += mul_expr->inf_cst;
				res->sup_cst += mul_expr->sup_cst;
			}
			else{
				mul_expr = multiply_expr(pr,neuron_k->uexpr,expr->inf_coeff[i],expr->sup_coeff[i]);
				
				add_expr(pr,res,mul_expr);
				
			}
			//expr_print(mul_expr);
				//fflush(stdout);
			//printf("add finish\n");
				//expr_print(res);
				//fflush(stdout);
			free_expr(mul_expr);
		}
		else if (expr->inf_coeff[i]<0){
			//expr_print(neuron_k->lexpr);
			//printf("add start %zu %zu\n",k,i);
			
				//expr_print(res);
				
			if(neuron_k->lexpr==NULL){
				mul_expr = (expr_t *)malloc(sizeof(expr_t));
				mul_expr->inf_coeff = mul_expr->sup_coeff = NULL;
				mul_expr->dim = NULL;
				mul_expr->size = 0;
				mul_expr->type = SPARSE;
				elina_double_interval_mul_cst_coeff(pr,&mul_expr->inf_cst,&mul_expr->sup_cst,neuron_k->lb,neuron_k->ub,expr->inf_coeff[i],expr->sup_coeff[i]);
				res->inf_cst += mul_expr->inf_cst;
				res->sup_cst += mul_expr->sup_cst;
			}
			else{
				mul_expr = multiply_expr(pr,neuron_k->lexpr,expr->inf_coeff[i],expr->sup_coeff[i]);
				//printf("add start1 %zu %zu\n",k,i);
				//expr_print(res);
				//expr_print(mul_expr);
				//fflush(stdout);
				add_expr(pr,res,mul_expr);
				
			}
			//expr_print(mul_expr);
			//	fflush(stdout);
			//printf("add finish1\n");
			//expr_print(res);
			//fflush(stdout);
			free_expr(mul_expr);
		}
		else{
			//printf("WTF2\n");
			//fflush(stdout);
			double tmp1, tmp2;
			elina_double_interval_mul_expr_coeff(pr,&tmp1,&tmp2,neuron_k->lb,neuron_k->ub,expr->inf_coeff[i],expr->sup_coeff[i]);
			res->inf_cst = res->inf_cst + tmp1;
			res->sup_cst = res->sup_cst - tmp1;
		}
	}
	//printf("finish\n");	
	//fflush(stdout);
	res->inf_cst = res->inf_cst + expr->inf_cst;
	res->sup_cst = res->sup_cst + expr->sup_cst;
	return res;
}

expr_t * uexpr_replace_maxpool_or_lstm_bounds(fppoly_internal_t * pr, expr_t * expr, neuron_t ** neurons){
	size_t num_neurons = expr->size;
	size_t i,k;
	expr_t *res;
	if(expr->type==DENSE){
		k = 0;
	}
	else{
		k = expr->dim[0];
	}
	neuron_t *neuron_k = neurons[k];
	if(expr->sup_coeff[0]<0){
		res = multiply_expr(pr,neuron_k->lexpr,expr->inf_coeff[0],expr->sup_coeff[0]);
	}
	else if(expr->inf_coeff[0]<0){
		res = multiply_expr(pr,neuron_k->uexpr,expr->inf_coeff[0],expr->sup_coeff[0]);
	}
	else{
		
		double tmp1, tmp2;
		elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,neuron_k->lb,neuron_k->ub,expr->inf_coeff[0],expr->sup_coeff[0]);
		double coeff[1];
		size_t dim[1];
		coeff[0] = 0;
		dim[0] = 0;
		res = create_sparse_expr(coeff,tmp2,dim,1);
	}

	for(i = 1; i < num_neurons; i++){
		if(expr->type==DENSE){
			k = i;
		}
		else{
			k = expr->dim[i];
		}
		neuron_t *neuron_k = neurons[k];
		if(expr->sup_coeff[i]<0){
			expr_t * mul_expr = multiply_expr(pr,neuron_k->lexpr,expr->inf_coeff[i],expr->sup_coeff[i]);
			add_expr(pr,res,mul_expr);
			free_expr(mul_expr);
		}
		else if (expr->inf_coeff[i]<0){
			expr_t * mul_expr = multiply_expr(pr,neuron_k->uexpr,expr->inf_coeff[i],expr->sup_coeff[i]);
			add_expr(pr,res,mul_expr);
			free_expr(mul_expr);
		}
		else{
			
			double tmp1, tmp2;
			elina_double_interval_mul_cst_coeff(pr,&tmp1,&tmp2,neuron_k->lb,neuron_k->ub,expr->inf_coeff[i],expr->sup_coeff[i]);
			res->inf_cst = res->inf_cst - tmp2;
			res->sup_cst = res->sup_cst + tmp2;
		}
	}
	res->inf_cst = res->inf_cst + expr->inf_cst;
	res->sup_cst = res->sup_cst + expr->sup_cst;
	return res;
}



expr_t * expr_from_previous_layer(fppoly_internal_t *pr, expr_t * expr, layer_t * prev_layer){
	if(expr->size==0){
		return copy_cst_expr(expr);
	}	
	if(expr->inf_coeff==NULL || expr->sup_coeff==NULL ){
		return alloc_expr();
	}
	//printf("coming here %zu\n",expr->size);
	//	fflush(stdout);
	neuron_t **prev_neurons = prev_layer->neurons;
	size_t out_num_neurons = prev_layer->dims;
	size_t in_num_neurons = expr->size;
	size_t i,k;
	expr_t * res;
	
		if(expr->type==DENSE){
			k = 0;
		}
		else{
			k = expr->dim[0];		
		}
		
		//printf("start2 %p %lu %p\n",expr->dim,k,prev_neurons[k]->expr);
		//fflush(stdout);
		//if(prev_layer->type==MAXPOOL){
		//printf("i: %zu k: %zu\n",i,k);
		//expr_print(prev_neurons[k]->expr);
		//fflush(stdout);
		//}
		if(prev_neurons[k]->expr->size==0){
			
			res = multiply_cst_expr(pr,prev_neurons[k]->expr,expr->inf_coeff[0],expr->sup_coeff[0]);
			
		}
		else{
			
			res = multiply_expr(pr,prev_neurons[k]->expr,expr->inf_coeff[0],expr->sup_coeff[0]);
		}
    //printf("debug\n");
    //fflush(stdout);
	for(i=1; i < in_num_neurons; i++){
		if(expr->type==DENSE){
			k = i;
		}
		else{
			k = expr->dim[i];
		}
		expr_t * mul_expr;
		//printf("i: %zu k: %zu\n",i,k);
		//expr_print(res);
		//expr_print(prev_neurons[k]->expr);
		//fflush(stdout);
		//if(prev_layer->type==MAXPOOL){
		
		//}
		if(prev_neurons[k]->expr->size==0){
			mul_expr = multiply_cst_expr(pr,prev_neurons[k]->expr, expr->inf_coeff[i],expr->sup_coeff[i]);
			add_cst_expr(pr,res,mul_expr);
			free_expr(mul_expr);
		}
		else if(expr->inf_coeff[i]!=0 || expr->sup_coeff[i]!=0){
			mul_expr = multiply_expr(pr,prev_neurons[k]->expr, expr->inf_coeff[i],expr->sup_coeff[i]);
			//printf("start\n");
			//fflush(stdout);
			add_expr(pr,res,mul_expr);
			free_expr(mul_expr);
			//printf("finish\n");
			//fflush(stdout);
		}
		//if(prev_layer->type==MAXPOOL){
		//printf("i: %zu k: %zu\n",i,k);
		//expr_print(mul_expr);
		//printf("output\n");
		//expr_print(res);
		//fflush(stdout);
		//}
		
	}
	res->inf_cst = res->inf_cst + expr->inf_cst; 
	res->sup_cst = res->sup_cst + expr->sup_cst; 
	//printf("finish\n");
	//fflush(stdout);
	return res;
}

expr_t * lexpr_unroll_lstm_layer(fppoly_internal_t *pr, expr_t * expr, neuron_t ** neurons){
	return NULL;
}


void * update_state_using_previous_layers(void *args){
	nn_thread_t * data = (nn_thread_t *)args;
	elina_manager_t * man = data->man;
	fppoly_t *fp = data->fp;
	fppoly_internal_t * pr = fppoly_init_from_manager(man, ELINA_FUNID_ASSIGN_LINEXPR_ARRAY);
	size_t layerno = data->layerno;
	size_t idx_start = data->start;
	size_t idx_end = data->end;
	size_t i;
	int k;
	
	neuron_t ** out_neurons = fp->layers[layerno]->neurons;
	size_t num_out_neurons = fp->layers[layerno]->dims;
	//printf("idx: %zu %zu\n",idx_start,idx_end);
	//fflush(stdout);
	for(i=idx_start; i < idx_end; i++){
  	  /* printf("update: i %zu \n",i); */
	  /* fflush(stdout); */
		expr_t * lexpr = copy_expr(out_neurons[i]->expr);
		expr_t * uexpr = copy_expr(out_neurons[i]->expr);
		for(k=layerno - 1; k >=0; k--){
		  /* printf("k = %d, type=%d, activation=%d\n",(int)k,(int)fp->layers[k]->type,(int)fp->layers[k]->activation); */
			neuron_t ** aux_neurons = fp->layers[k]->neurons;
			size_t dims = fp->layers[k]->dims;
			expr_t * tmp_l;
			expr_t * tmp_u;
			if(fp->layers[k]->type==FFN || fp->layers[k]->type==CONV){
				 tmp_l = lexpr;
				 tmp_u = uexpr;
                //if(i==0 && layerno==1){
			//	printf("before relu %zu\n",lexpr->size);
			//	expr_print(lexpr);
			//	expr_print(uexpr);
			//	fflush(stdout);
                //}
                if(fp->layers[k]->activation==RELU){
                    lexpr = lexpr_replace_relu_bounds(pr,lexpr,aux_neurons);
                    //printf("doesnt return\n");
				
                    //fflush(stdout);
                    uexpr = uexpr_replace_relu_bounds(pr,uexpr,aux_neurons);
                }
                else if(fp->layers[k]->activation==SIGMOID){
                        //printf("start\n");
                        //fflush(stdout);
                    lexpr = lexpr_replace_sigmoid_bounds(pr,lexpr,aux_neurons);
                        //printf("finish\n");
                        //fflush(stdout);
                    uexpr = uexpr_replace_sigmoid_bounds(pr,uexpr,aux_neurons);
                }
                else if(fp->layers[k]->activation==TANH){
                    lexpr = lexpr_replace_tanh_bounds(pr,lexpr,aux_neurons);
                    uexpr = uexpr_replace_tanh_bounds(pr,uexpr,aux_neurons);
                }
		else if(fp->layers[k]->activation==PARABOLA){
		    lexpr = lexpr_replace_parabola_bounds(pr,lexpr,aux_neurons);
                    uexpr = uexpr_replace_parabola_bounds(pr,uexpr,aux_neurons);
		}
		else if(fp->layers[k]->activation==LOG){
		    lexpr = lexpr_replace_log_bounds(pr,lexpr,aux_neurons);
			uexpr = uexpr_replace_log_bounds(pr,uexpr,aux_neurons);
		}
		//else if(fp->layers[k]->activation==MULT){
		if(fp->layers[k]->activation!=NONE){
				free_expr(tmp_l);
				
				free_expr(tmp_u);
		}	
               // if(i==0 && layerno==1){
                //    printf("after relu %zu\n",lexpr->size);
                //    expr_print(lexpr);
                 //   expr_print(uexpr);
                 //   fflush(stdout);
               // }
				//if(k>=1){
					tmp_l = lexpr;
					tmp_u = uexpr;
					//if(fp->layers[k]->type==MAXPOOL){
					//printf("before replacing %zu %zu\n",lexpr->size,uexpr->size);
					//expr_print(lexpr);
					//expr_print(uexpr);
					//fflush(stdout);
					//}
					lexpr = expr_from_previous_layer(pr,lexpr, fp->layers[k]);
					//printf("doesnt return\n");
					//expr_print(uexpr);
					//fflush(stdout);
					uexpr = expr_from_previous_layer(pr,uexpr, fp->layers[k]);

					//if(fp->layers[k]->type==MAXPOOL){
					////fflush(stdout);
					//}
					free_expr(tmp_l);
					free_expr(tmp_u);
			}
			else if(fp->layers[k]->type==MAXPOOL || fp->layers[k]->type==LSTM){
				expr_t * tmp_l = lexpr;
				expr_t * tmp_u = uexpr;
				//printf("before maxpool %zu %zu\n",lexpr->size,i);
				//expr_print(lexpr);
				//expr_print(uexpr);
				//fflush(stdout);
				lexpr = lexpr_replace_maxpool_or_lstm_bounds(pr,lexpr,aux_neurons);
				
				//fflush(stdout);
				uexpr = uexpr_replace_maxpool_or_lstm_bounds(pr,uexpr,aux_neurons);
				//printf("after maxpool %zu\n",lexpr->size);
				//expr_print(lexpr);
				//fflush(stdout);
				free_expr(tmp_l);
				
				free_expr(tmp_u);
			}
			//}
			else{
				expr_t *tmp_l = lexpr;
				expr_t *tmp_u = uexpr;
				lexpr = lexpr_unroll_lstm_layer(pr,lexpr, aux_neurons);
				
				//uexpr = uexpr_unroll_lstm_layer(pr, uexpr, aux_neurons);
				
				free_expr(tmp_l);
				free_expr(tmp_u);
			}
			//printf("coming here\n");
			//fflush(stdout);
		}
		//expr_print(lexpr);
		//printf("uexpr: %zu %zu\n",uexpr->size,i);
		//expr_print(uexpr);
		//fflush(stdout);
		
		out_neurons[i]->lb = compute_lb_from_expr(pr, lexpr,fp); 
		//- bias_i;
		out_neurons[i]->ub = compute_ub_from_expr(pr, uexpr,fp); //+ bias_i;
		//printf("lb: %g ub: %g\n",out_neurons[i]->lb,out_neurons[i]->ub);
		if(fp->out!=NULL){
			
			fp->out->lexpr[i] = lexpr;
			fp->out->uexpr[i] = uexpr;
		}
		else{
			free_expr(lexpr);
			free_expr(uexpr);
		}
	}

	//printf("thread finish\n");
	//fflush(stdout);
	return NULL;
}


void update_state_using_previous_layers_parallel(elina_manager_t *man, fppoly_t *fp, size_t layerno){
  	//size_t NUM_THREADS = get_nprocs();
  size_t NUM_THREADS = sysconf(_SC_NPROCESSORS_ONLN);
	nn_thread_t args[NUM_THREADS];
	pthread_t threads[NUM_THREADS];
	size_t num_out_neurons = fp->layers[layerno]->dims;
	size_t i;
	//printf("start %zu\n",num_out_neurons);
	//fflush(stdout);
	if(num_out_neurons < NUM_THREADS){
		for (i = 0; i < num_out_neurons; i++){
	    		args[i].start = i; 
	    		args[i].end = i+1;   
			args[i].man = man;
			args[i].fp = fp;
			args[i].layerno = layerno;
	    		pthread_create(&threads[i], NULL,update_state_using_previous_layers, (void*)&args[i]);
			
	  	}
		for (i = 0; i < num_out_neurons; i = i + 1){
			pthread_join(threads[i], NULL);
		}
	}
	else{
		size_t idx_start = 0;
		size_t idx_n = num_out_neurons / NUM_THREADS;
		size_t idx_end = idx_start + idx_n;
		
		
	  	for (i = 0; i < NUM_THREADS; i++){
	    		args[i].start = idx_start; 
	    		args[i].end = idx_end;   
			args[i].man = man;
			args[i].fp = fp;
			args[i].layerno = layerno;
	    		pthread_create(&threads[i], NULL,update_state_using_previous_layers, (void*)&args[i]);
			idx_start = idx_end;
			idx_end = idx_start + idx_n;
	    		if(idx_end>num_out_neurons){
				idx_end = num_out_neurons;
			}
			if((i==NUM_THREADS-2)){
				idx_end = num_out_neurons;
				
			}
	  	}
		//idx_start = idx_end;
	    	//idx_end = num_out_neurons;
		//args[i].start = idx_start; 
	    	//args[i].end = idx_end;   
		//args[i].man = man;
		//args[i].fp = fp;
		//args[i].layerno = layerno;
	    	//pthread_create(&threads[i], NULL,update_state_using_previous_layers, (void*)&args[i]);
		for (i = 0; i < NUM_THREADS; i = i + 1){
			pthread_join(threads[i], NULL);
		}
	}
	//printf("end\n");
	//fflush(stdout);
}


void ffn_handle_intermediate_layer(elina_manager_t* man, elina_abstract0_t* element, double **weights, double * bias, size_t num_out_neurons, size_t num_in_neurons, activation_type_t activation, bool alloc){
    //printf("ReLU start here %zu %zu\n",num_in_neurons,num_out_neurons);
    //fflush(stdout);
    fppoly_t *fp = fppoly_of_abstract0(element);
    size_t numlayers = fp->numlayers;
    if(alloc){
        fppoly_add_new_layer(fp,num_out_neurons, FFN, activation);
    } else {
        fp->numlayers++;
	}
	/* printf("here, num_layers=%d\n",numlayers); */
    neuron_t **out_neurons = fp->layers[numlayers]->neurons;
	/* printf("here2\n"); */
    size_t i;
    for(i=0; i < num_out_neurons; i++){
        double * weight_i = weights[i];
        double bias_i = bias[i];
	
        out_neurons[i]->expr = create_dense_expr(weight_i,bias_i,num_in_neurons);
    }
    update_state_using_previous_layers_parallel(man,fp,numlayers);
    
    //printf("return here2\n");
    //fppoly_fprint(stdout,man,fp,NULL);
    //fflush(stdout);
    return;
}

void ffn_handle_intermediate_affine_layer(elina_manager_t* man, elina_abstract0_t* element, double **weights, double * bias,  size_t num_out_neurons, size_t num_in_neurons){
    ffn_handle_intermediate_layer(man, element, weights, bias,  num_out_neurons, num_in_neurons, NONE, true);
}

void ffn_handle_intermediate_relu_layer(elina_manager_t* man, elina_abstract0_t* element, double **weights, double * bias,  size_t num_out_neurons, size_t num_in_neurons){
    ffn_handle_intermediate_layer(man, element, weights, bias, num_out_neurons, num_in_neurons, RELU, true);
}

void ffn_handle_intermediate_sigmoid_layer(elina_manager_t* man, elina_abstract0_t* element, double **weights, double * bias,  size_t num_out_neurons, size_t num_in_neurons){
    ffn_handle_intermediate_layer(man, element, weights, bias, num_out_neurons, num_in_neurons, SIGMOID, true);
}

void ffn_handle_intermediate_tanh_layer(elina_manager_t* man, elina_abstract0_t* element, double **weights, double * bias,  size_t num_out_neurons, size_t num_in_neurons){
    ffn_handle_intermediate_layer(man, element, weights, bias,  num_out_neurons, num_in_neurons, TANH, true);
}


void ffn_handle_intermediate_parabola_layer(elina_manager_t* man, elina_abstract0_t* element, double **weights, double * bias, size_t num_out_neurons, size_t num_in_neurons){
	
    ffn_handle_intermediate_layer(man, element, weights, bias, num_out_neurons, num_in_neurons, PARABOLA, true);
	
}

void ffn_handle_intermediate_log_layer(elina_manager_t* man, elina_abstract0_t* element, double **weights, double * bias,  size_t num_out_neurons, size_t num_in_neurons){
    ffn_handle_intermediate_layer(man, element, weights, bias,  num_out_neurons, num_in_neurons, LOG, true);
}

void ffn_handle_intermediate_affine_layer_no_alloc(elina_manager_t* man, elina_abstract0_t* element, double **weights, double * bias,  size_t num_out_neurons, size_t num_in_neurons){
    ffn_handle_intermediate_layer(man, element, weights, bias,  num_out_neurons, num_in_neurons, NONE, false);
}

void ffn_handle_intermediate_relu_layer_no_alloc(elina_manager_t* man, elina_abstract0_t* element, double **weights, double * bias,  size_t num_out_neurons, size_t num_in_neurons){
    ffn_handle_intermediate_layer(man, element, weights, bias, num_out_neurons, num_in_neurons, RELU, false);
}

void ffn_handle_intermediate_sigmoid_layer_no_alloc(elina_manager_t* man, elina_abstract0_t* element, double **weights, double * bias,  size_t num_out_neurons, size_t num_in_neurons){
    ffn_handle_intermediate_layer(man, element, weights, bias, num_out_neurons, num_in_neurons, SIGMOID, false);
}

void ffn_handle_intermediate_tanh_layer_no_alloc(elina_manager_t* man, elina_abstract0_t* element, double **weights, double * bias,  size_t num_out_neurons, size_t num_in_neurons){
    ffn_handle_intermediate_layer(man, element, weights, bias,  num_out_neurons, num_in_neurons, TANH, false);
}


void ffn_handle_intermediate_parabola_layer_no_alloc(elina_manager_t* man, elina_abstract0_t* element, double **weights, double * bias, size_t num_out_neurons, size_t num_in_neurons){
    
    ffn_handle_intermediate_layer(man, element, weights, bias, num_out_neurons, num_in_neurons, PARABOLA, false);
    
}

void ffn_handle_intermediate_log_layer_no_alloc(elina_manager_t* man, elina_abstract0_t* element, double **weights, double * bias,  size_t num_out_neurons, size_t num_in_neurons){
    ffn_handle_intermediate_layer(man, element, weights, bias,  num_out_neurons, num_in_neurons, LOG, false);
}


double apply_relu_lexpr(fppoly_internal_t *pr, expr_t **lexpr_p, neuron_t * neuron){
	expr_t * lexpr = *lexpr_p;
	size_t i;
	size_t size = lexpr->size;
	double lb = neuron->lb;
	double ub = neuron->ub;
	double width = lb+ub;
	if(ub<0){
		free_expr(*lexpr_p);
		*lexpr_p = NULL;
		return 0;
	}
	if(lb<0){
		return lb;
	}
	double area1 = lb*ub;
	double area2 = 0.5*ub*width;
	if(area1<area2){
		double lambda_inf = -ub/width;
		double lambda_sup = ub/width;
		for(i=0; i < size; i++){
			//lexpr->coeff[i] = lexpr->coeff[i]*lambda;
			 elina_double_interval_mul_expr_coeff(pr,&lexpr->inf_coeff[i], &lexpr->sup_coeff[i],lambda_inf, lambda_sup, lexpr->inf_coeff[i], lexpr->sup_coeff[i]);
		}
		//lexpr->cst = lexpr->cst*lambda;
		elina_double_interval_mul_cst_coeff(pr,&lexpr->inf_cst, &lexpr->sup_cst,lambda_inf, lambda_sup, lexpr->inf_cst, lexpr->sup_cst);
		//double res, res1;
		
		return -(lambda_inf*lb); 
	}
	else{
		free_expr(*lexpr_p);
		*lexpr_p = NULL;
		return 0;
	}
}


double apply_relu_uexpr(fppoly_internal_t *pr, expr_t **uexpr_p, neuron_t * neuron){
	expr_t * uexpr = *uexpr_p;
	size_t i;
	size_t size = uexpr->size;
	double lb = neuron->lb;
	double ub = neuron->ub;
	double width = lb+ub;
	if(ub<0){
		free_expr(*uexpr_p);
		*uexpr_p = NULL;
		return 0;
	}
	if(lb<0){
		return ub;
	}
	double lambda_inf = -ub/width;
	double lambda_sup = ub/width;
	for(i=0; i < size; i++){
		//uexpr->coeff[i] = uexpr->coeff[i]*lambda; 
		 elina_double_interval_mul_expr_coeff(pr,&uexpr->inf_coeff[i], &uexpr->sup_coeff[i],lambda_inf, lambda_sup, uexpr->inf_coeff[i], uexpr->sup_coeff[i]);
	}
	elina_double_interval_mul_cst_coeff(pr,&uexpr->inf_cst, &uexpr->sup_cst,lambda_inf, lambda_sup, uexpr->inf_cst, uexpr->sup_cst);
	double mu_inf = lambda_inf*lb;
	double mu_sup = lambda_sup*lb;
	//uexpr->cst = uexpr->cst*lambda;
	//uexpr->cst = uexpr->cst + lambda*lb;
	uexpr->inf_cst += mu_inf;
	uexpr->sup_cst += mu_sup; 
	return ub; 
}

void handle_final_relu_layer(fppoly_internal_t *pr, output_abstract_t * out, neuron_t **neurons, size_t size, bool has_relu){
	size_t i;
	/* printf("handling last layer...\n"); */
	/* printf("size = %d", (int)size); */
        if(has_relu){
		for(i=0; i < size; i++){

			out->output_inf[i] = apply_relu_lexpr(pr,&out->lexpr[i],neurons[i]);
			out->output_sup[i] = apply_relu_uexpr(pr,&out->uexpr[i],neurons[i]);
		}
	}
	else{
		for(i=0; i < size; i++){
			out->output_inf[i] = neurons[i]->lb;
			out->output_sup[i] = neurons[i]->ub;
		}
	}	
}


double apply_s_curve_lexpr(fppoly_internal_t *pr, expr_t **lexpr_p, neuron_t * neuron, bool is_sigmoid, bool force_boxify){
	expr_t * lexpr = *lexpr_p;
	size_t i;
	size_t size = lexpr->size;
	double lb = neuron->lb;
	double ub = neuron->ub;
	/* printf("neuron lb = %.10lf, ub = %.10lf\n",lb,ub); */
	bool boxify = false;
	if (ub + lb < 0.01) {
	  boxify = true;
	}
	double slope_inf, slope_sup;

	double intercept_inf, intercept_sup;
	compute_slope_and_intercept_s_curve_lexpr(pr, &slope_inf, &slope_sup, 
						&intercept_inf, &intercept_sup, -1, 
						1, lb, ub, is_sigmoid, &boxify);
	/* printf("before: "); */
	/* expr_print(lexpr); */
	/* printf("\t[l] slope_inf = %.10lf, intercept_inf = %.10lf\n", slope_inf, intercept_inf); */
	/* printf("\t[l] slope_sup = %.10lf, intercept_sup = %.10lf\n", slope_sup, intercept_sup); */

	fesetround(FE_DOWNWARD);
	double e_inf_l = is_sigmoid ? -exp(-lb) : -tanh(-lb);
	fesetround(FE_UPWARD); 
	double e_inf_u = is_sigmoid ? exp(-lb) : tanh(-lb);
	double f_inf_l, f_inf_u;
	double den_inf_l, den_inf_u;
	if(is_sigmoid){
		den_inf_l = -1 + e_inf_l;
		den_inf_u = 1 + e_inf_u;
		elina_double_interval_div(&f_inf_l, &f_inf_u, e_inf_l, e_inf_u, den_inf_l, den_inf_u);
	}
	else{
		f_inf_l = e_inf_l;
		f_inf_u = e_inf_u;
	}
	if (force_boxify) {
	  boxify = true;
	}
	if(boxify){
		for(i=0; i< size; i++){
			lexpr->inf_coeff[i] = 0.0;
			lexpr->sup_coeff[i] = 0.0;
		}
		lexpr->inf_cst = f_inf_l;
		lexpr->sup_cst = f_inf_u;
	}
	else{
	  for(i=0; i < size; i++){
		  elina_double_interval_mul_expr_coeff(pr,&lexpr->inf_coeff[i],&lexpr->sup_coeff[i],slope_inf,slope_sup,lexpr->inf_coeff[i],lexpr->sup_coeff[i]);
		}
		elina_double_interval_mul_cst_coeff(pr, &lexpr->inf_cst, &lexpr->sup_cst, slope_inf, slope_sup, lexpr->inf_cst, lexpr->sup_cst );
		elina_double_interval_add_cst_coeff(pr,&lexpr->inf_cst,&lexpr->sup_cst,intercept_inf, intercept_sup, lexpr->inf_cst, lexpr->sup_cst);
	}
	/* printf("after:\n"); */
	/* expr_print(lexpr); */
	return f_inf_l;
}

double apply_s_curve_uexpr(fppoly_internal_t *pr, expr_t **uexpr_p, neuron_t * neuron, bool is_sigmoid, bool force_boxify){
	expr_t * uexpr = *uexpr_p;
	size_t i;
	size_t size = uexpr->size;
	double lb = neuron->lb;
	double ub = neuron->ub;
	bool boxify = false;
	if (ub + lb < 0.01) {
	  boxify = true;
	}
	double slope_inf, slope_sup;
	double intercept_inf, intercept_sup;
	compute_slope_and_intercept_s_curve_uexpr(pr, &slope_inf, &slope_sup, 
						&intercept_inf, &intercept_sup, -1, 
						1, lb, ub, is_sigmoid, &boxify);
	/* printf("[u] slope_inf = %.10lf, intercept_inf = %.10lf\n", slope_inf, intercept_inf); */
	/* printf("[u] slope_sup = %.10lf, intercept_sup = %.10lf\n", slope_sup, intercept_sup); */

	fesetround(FE_DOWNWARD);
	double e_sup_l = is_sigmoid ? -exp(ub) : -tanh(ub);
	fesetround(FE_UPWARD); 
	double e_sup_u = is_sigmoid ? exp(ub) : tanh(ub);
	double f_sup_l, f_sup_u;
	double den_sup_l, den_sup_u;
	if(is_sigmoid){
		den_sup_l = -1 + e_sup_l;
		den_sup_u = 1 + e_sup_u;					
		elina_double_interval_div(&f_sup_l, &f_sup_u, e_sup_l, e_sup_u, den_sup_l, den_sup_u);
	}
	else{
		f_sup_l = e_sup_l;
		f_sup_u = e_sup_u;
	}
	if (force_boxify) {
	  boxify = true;
	}
	if(boxify){
		for(i=0; i< size; i++){
			uexpr->inf_coeff[i] = 0.0;
			uexpr->sup_coeff[i] = 0.0;
		}
		uexpr->inf_cst = f_sup_l;
		uexpr->sup_cst = f_sup_u;
	}
	else{
	  for(i=0; i < size; i++){
		elina_double_interval_mul_expr_coeff(pr,&uexpr->inf_coeff[i],&uexpr->sup_coeff[i],slope_inf,slope_sup,uexpr->inf_coeff[i],uexpr->sup_coeff[i]);
	  }
	  elina_double_interval_mul_cst_coeff(pr, &uexpr->inf_cst, &uexpr->sup_cst, slope_inf, slope_sup, uexpr->inf_cst, uexpr->sup_cst );
	  elina_double_interval_add_cst_coeff(pr,&uexpr->inf_cst,&uexpr->sup_cst,intercept_inf, intercept_sup, uexpr->inf_cst, uexpr->sup_cst);
	}
	/* printf("after: "); */
	/* expr_print(uexpr); */
	/* printf("f_sup_u = %.10lf\n",f_sup_u); */
	return f_sup_u;
}


double apply_sigmoid_lexpr(fppoly_internal_t *pr, expr_t **lexpr_p, neuron_t * neuron, bool force_boxify){
  return apply_s_curve_lexpr(pr,lexpr_p,neuron,true,force_boxify);
}

double apply_tanh_lexpr(fppoly_internal_t *pr, expr_t **lexpr_p, neuron_t * neuron, bool force_boxify){
  return apply_s_curve_lexpr(pr,lexpr_p,neuron,false,force_boxify);
}

double apply_sigmoid_uexpr(fppoly_internal_t *pr, expr_t **uexpr_p, neuron_t * neuron, bool force_boxify){
  return apply_s_curve_uexpr(pr,uexpr_p,neuron,true,force_boxify);
}

double apply_tanh_uexpr(fppoly_internal_t *pr, expr_t **uexpr_p, neuron_t * neuron, bool force_boxify){
  return apply_s_curve_uexpr(pr,uexpr_p,neuron,false,force_boxify);
}

void handle_final_sigmoid_layer(fppoly_internal_t *pr, output_abstract_t * out, neuron_t **neurons, size_t size, bool has_sigmoid){
	size_t i;
        if(has_sigmoid){
		for(i=0; i < size; i++){
		  out->output_inf[i] = apply_sigmoid_lexpr(pr,&out->lexpr[i],neurons[i],false);
		  out->output_sup[i] = apply_sigmoid_uexpr(pr,&out->uexpr[i],neurons[i],false);
		}
	}
	else{
		for(i=0; i < size; i++){
			out->output_inf[i] = neurons[i]->lb;
			out->output_sup[i] = neurons[i]->ub;
		}
	}	
}

void handle_final_tanh_layer(fppoly_internal_t *pr, output_abstract_t * out, neuron_t **neurons, size_t size, bool has_tanh){
	size_t i;
        if(has_tanh){
		for(i=0; i < size; i++){
		  out->output_inf[i] = apply_tanh_lexpr(pr,&out->lexpr[i],neurons[i],false);
		  out->output_sup[i] = apply_tanh_uexpr(pr,&out->uexpr[i],neurons[i],false);
		}
	}
	else{
		for(i=0; i < size; i++){
			out->output_inf[i] = neurons[i]->lb;
			out->output_sup[i] = neurons[i]->ub;
		}
	}	
}




void ffn_handle_last_layer(elina_manager_t* man, elina_abstract0_t* element, double **weights, double * bias, size_t num_out_neurons, size_t num_in_neurons, bool has_activation, activation_type_t activation, bool alloc){
  /* printf("last, num_out_neurons = %d\n",(int)num_out_neurons); */
	//fflush(stdout);
	fppoly_t *fp = fppoly_of_abstract0(element);
	size_t numlayers = fp->numlayers;
    if(alloc){
        if(has_activation){
	   fppoly_add_new_layer(fp,num_out_neurons, FFN, activation);
        }
        else{
           fppoly_add_new_layer(fp,num_out_neurons, FFN, NONE);
        }
    } 
	output_abstract_t * out = (output_abstract_t*)malloc(sizeof(output_abstract_t));
	out->output_inf = (double *)malloc(num_out_neurons*sizeof(double)); 
	out->output_sup = (double *)malloc(num_out_neurons*sizeof(double)); 
	out->lexpr = (expr_t **)malloc(num_out_neurons*sizeof(expr_t *));
	out->uexpr = (expr_t **)malloc(num_out_neurons*sizeof(expr_t *));
	fp->out = out;
	neuron_t ** out_neurons = fp->layers[numlayers]->neurons;
	fppoly_internal_t *pr = fppoly_init_from_manager(man, ELINA_FUNID_ASSIGN_LINEXPR_ARRAY);
	size_t i;
	for(i=0; i < num_out_neurons; i++){
		double * weight_i = weights[i];
		double bias_i = bias[i];
		out_neurons[i]->expr = create_dense_expr(weight_i,bias_i,num_in_neurons);
		/* printf("i = %d: ", (int)i); */
		/* expr_print(out_neurons[i]->expr); */
	}
	update_state_using_previous_layers_parallel(man,fp,numlayers);
    if(activation==RELU){
        handle_final_relu_layer(pr,fp->out,out_neurons, num_out_neurons, has_activation);
    }
    else if(activation==SIGMOID){
	handle_final_sigmoid_layer(pr,fp->out,out_neurons, num_out_neurons, has_activation);
    }
    else if(activation==TANH){
	handle_final_tanh_layer(pr,fp->out,out_neurons, num_out_neurons, has_activation);
    }
    
    else{
	for(i=0; i < num_out_neurons; i++){
		out->output_inf[i] = out_neurons[i]->lb;
		out->output_sup[i] = out_neurons[i]->ub;
	}
    }
    //printf("finish\n");
    //fppoly_fprint(stdout,man,fp,NULL);
    //fflush(stdout);
	return;

}

void ffn_handle_last_relu_layer(elina_manager_t* man, elina_abstract0_t* element, double **weights, double * bias,  size_t num_out_neurons, size_t num_in_neurons, bool has_relu){
    ffn_handle_last_layer(man, element, weights, bias, num_out_neurons, num_in_neurons, has_relu, RELU, true);
}

void ffn_handle_last_sigmoid_layer(elina_manager_t* man, elina_abstract0_t* element, double **weights, double * bias,  size_t num_out_neurons, size_t num_in_neurons, bool has_sigmoid){
    ffn_handle_last_layer(man, element, weights, bias, num_out_neurons, num_in_neurons, has_sigmoid, SIGMOID, true);
}

void ffn_handle_last_tanh_layer(elina_manager_t* man, elina_abstract0_t* element, double **weights, double * bias,  size_t num_out_neurons, size_t num_in_neurons, bool has_tanh){
    ffn_handle_last_layer(man, element, weights, bias, num_out_neurons, num_in_neurons, has_tanh, TANH, true);
}

void ffn_handle_last_parabola_layer(elina_manager_t* man, elina_abstract0_t* element, double **weights, double * bias, size_t num_out_neurons, size_t num_in_neurons, bool has_parabola){
    ffn_handle_last_layer(man, element, weights, bias, num_out_neurons, num_in_neurons, has_parabola, PARABOLA, true);
}

void ffn_handle_last_log_layer(elina_manager_t* man, elina_abstract0_t* element, double **weights, double * bias,  size_t num_out_neurons, size_t num_in_neurons, bool has_log){
    ffn_handle_last_layer(man, element, weights, bias, num_out_neurons, num_in_neurons, has_log, LOG, true);
}


void ffn_handle_last_relu_layer_no_alloc(elina_manager_t* man, elina_abstract0_t* element, double **weights, double * bias,  size_t num_out_neurons, size_t num_in_neurons, bool has_relu){
    ffn_handle_last_layer(man, element, weights, bias, num_out_neurons, num_in_neurons, has_relu, RELU, false);
}

void ffn_handle_last_sigmoid_layer_no_alloc(elina_manager_t* man, elina_abstract0_t* element, double **weights, double * bias,  size_t num_out_neurons, size_t num_in_neurons, bool has_sigmoid){
    ffn_handle_last_layer(man, element, weights, bias, num_out_neurons, num_in_neurons, has_sigmoid, SIGMOID, false);
}

void ffn_handle_last_tanh_layer_no_alloc(elina_manager_t* man, elina_abstract0_t* element, double **weights, double * bias,  size_t num_out_neurons, size_t num_in_neurons, bool has_tanh){
    ffn_handle_last_layer(man, element, weights, bias, num_out_neurons, num_in_neurons, has_tanh, TANH, false);
}

void ffn_handle_last_parabola_layer_no_alloc(elina_manager_t* man, elina_abstract0_t* element, double **weights, double * bias, size_t num_out_neurons, size_t num_in_neurons, bool has_parabola){
    ffn_handle_last_layer(man, element, weights, bias, num_out_neurons, num_in_neurons, has_parabola, PARABOLA, false);
}

void ffn_handle_last_log_layer_no_alloc(elina_manager_t* man, elina_abstract0_t* element, double **weights, double * bias,  size_t num_out_neurons, size_t num_in_neurons, bool has_log){
    ffn_handle_last_layer(man, element, weights, bias, num_out_neurons, num_in_neurons, has_log, LOG, false);
}

double get_lb_using_previous_layers(elina_manager_t *man, fppoly_t *fp, expr_t *expr, size_t layerno){
	size_t i;
	int k;
	//size_t numlayers = fp->numlayers;
	expr_t * lexpr = copy_expr(expr);
        fppoly_internal_t * pr = fppoly_init_from_manager(man,ELINA_FUNID_ASSIGN_LINEXPR_ARRAY);
		double ret;
		double best_res = -INFINITY;
	for(k=layerno - 1; k >=0; k--){
	//	fflush(stdout);	
		neuron_t ** aux_neurons = fp->layers[k]->neurons;
		expr_t * tmp_l;
				
	   if(fp->layers[k]->type==FFN || fp->layers[k]->type==CONV){
			
		    if(fp->layers[k]->activation==RELU){
		        tmp_l = lexpr;
		        lexpr = lexpr_replace_relu_bounds(pr,lexpr,aux_neurons);
		        free_expr(tmp_l);
		    }
		    else if(fp->layers[k]->activation==SIGMOID){
		        tmp_l = lexpr;
			//printf("start\n");
			//fflush(stdout);
		        lexpr = lexpr_replace_sigmoid_bounds(pr,lexpr,aux_neurons);
			//printf("finish\n");
			//fflush(stdout);
		        free_expr(tmp_l);
		    }
		    else if(fp->layers[k]->activation==TANH){
		         tmp_l = lexpr;
		        lexpr = lexpr_replace_tanh_bounds(pr,lexpr,aux_neurons);
		        free_expr(tmp_l);
		    }
		
		    else if(fp->layers[k]->activation==PARABOLA){
		         tmp_l = lexpr;
		        lexpr = lexpr_replace_parabola_bounds(pr,lexpr,aux_neurons);
		        free_expr(tmp_l);
		    }		
			else if(fp->layers[k]->activation==LOG){
				tmp_l = lexpr;
		        	lexpr = lexpr_replace_log_bounds(pr,lexpr,aux_neurons);
		        	free_expr(tmp_l);
			}
			
			/* size_t num_neurons = lexpr->size; */
			/* double lb_inf = lexpr->inf_cst, lb_sup = lexpr->sup_cst; */
			/* for (i = 0; i < num_neurons; ++i) { */
			/*   double tmp1, tmp2; */
			/*   elina_double_interval_mul(&tmp1,&tmp2,aux_neurons[i]->lb,aux_neurons[i]->ub,lexpr->inf_coeff[i],lexpr->sup_coeff[i]); */
			/*   lb_inf += tmp1; */
			/*   lb_sup += tmp2; */
			/* } */
			/* if (k == (int)(layerno - 1) || lb_inf < best_res) { */
			/*   best_res = lb_inf; */
			/* } */


			/* printf("%d, activation = %d\n",(int)k, (int)fp->layers[k]->activation); */
			/* expr_print(lexpr); */

				tmp_l = lexpr;			
				lexpr = expr_from_previous_layer(pr,lexpr, fp->layers[k]);		
				free_expr(tmp_l);
			}
		else{
			expr_t * tmp_l = lexpr;
			lexpr = lexpr_replace_maxpool_or_lstm_bounds(pr,lexpr,aux_neurons);	
			free_expr(tmp_l);
		}

	}
		
	double res = compute_lb_from_expr(pr,lexpr,fp);
	return res;
	//return best_res;
}


double get_ub_using_previous_layers(elina_manager_t *man, fppoly_t *fp, expr_t *expr, size_t layerno){
	size_t i;
	int k;
	//size_t numlayers = fp->numlayers;
	expr_t * uexpr = copy_expr(expr);
        fppoly_internal_t * pr = fppoly_init_from_manager(man,ELINA_FUNID_ASSIGN_LINEXPR_ARRAY);
		double best_res = INFINITY;
	for(k=layerno - 1; k >=0; k--){
		neuron_t ** aux_neurons = fp->layers[k]->neurons;
		expr_t * tmp_u;
		if(fp->layers[k]->type==FFN || fp->layers[k]->type==CONV){
			
		    	if(fp->layers[k]->activation==RELU){
		        	tmp_u = uexpr;
		        	uexpr = uexpr_replace_relu_bounds(pr,uexpr,aux_neurons);
		        	free_expr(tmp_u);
		    	}
		    	else if(fp->layers[k]->activation==SIGMOID){
		        	tmp_u = uexpr;
		        	uexpr = uexpr_replace_sigmoid_bounds(pr,uexpr,aux_neurons);
		        	free_expr(tmp_u);
		    	}
		    	else if(fp->layers[k]->activation==TANH){
		         	tmp_u = uexpr;
		        	uexpr = uexpr_replace_tanh_bounds(pr,uexpr,aux_neurons);
		        	free_expr(tmp_u);
		    	}
		
		    	else if(fp->layers[k]->activation==PARABOLA){
		         	tmp_u = uexpr;
		        	uexpr = uexpr_replace_parabola_bounds(pr,uexpr,aux_neurons);
		        	free_expr(tmp_u);
		    	}		
			else if(fp->layers[k]->activation==LOG){
				tmp_u = uexpr;
		        	uexpr = uexpr_replace_log_bounds(pr,uexpr,aux_neurons);
		        	free_expr(tmp_u);
			}

			/* size_t num_neurons = uexpr->size; */
			/* double ub_sup = uexpr->sup_cst; */
			/* for (i = 0; i < num_neurons; ++i) { */
			/*   double tmp1, tmp2; */
			/*   elina_double_interval_mul(&tmp1,&tmp2,aux_neurons[i]->lb,aux_neurons[i]->ub,uexpr->inf_coeff[i],uexpr->sup_coeff[i]); */
			/*   ub_sup += tmp2; */
			/* } */
			/* if (k == (int)(layerno - 1) || ub_sup < best_res) { */
			/*   best_res = ub_sup; */
			/* } */
			
			tmp_u = uexpr;			
			uexpr = expr_from_previous_layer(pr,uexpr, fp->layers[k]);		
			free_expr(tmp_u);
		}
		else{
			expr_t * tmp_u = uexpr;
			uexpr = uexpr_replace_maxpool_or_lstm_bounds(pr,uexpr,aux_neurons);	
			free_expr(tmp_u);
		}
			
	}
		
	double res = compute_ub_from_expr(pr,uexpr,fp);
	return res;
	//return best_res;
}

void coeff_to_interval(elina_coeff_t *coeff, double *inf, double *sup){
	double d;
	if(coeff->discr==ELINA_COEFF_SCALAR){
		elina_scalar_t * scalar = coeff->val.scalar;
		d = scalar->val.dbl;
		*inf = -d;
		*sup = d;
	}
	else{
		elina_interval_t *interval = coeff->val.interval;
		d = interval->inf->val.dbl;
		*inf = -d;
		d = interval->sup->val.dbl;
		*sup = d;	
	}
		
}

expr_t * elina_linexpr0_to_expr(elina_linexpr0_t *linexpr0){
	size_t size = linexpr0->size;
	size_t i;
	expr_t *res = (expr_t*)malloc(sizeof(expr_t));
	res->inf_coeff = (double*)malloc(size*sizeof(double));
	res->sup_coeff = (double*)malloc(size*sizeof(double));
	res->size = size;
	if(linexpr0->discr==ELINA_LINEXPR_SPARSE){
		res->type = SPARSE;
		res->dim = (size_t *)malloc(size*sizeof(size_t));
	}
	else{
		res->type = DENSE;
		res->dim = NULL;
	}
	size_t k;
	for(i=0; i< size; i++){
		elina_coeff_t *coeff;
		if(res->type==SPARSE){
			k = linexpr0->p.linterm[i].dim;
			res->dim[i] = k;
			coeff = &linexpr0->p.linterm[i].coeff;
			coeff_to_interval(coeff,&res->inf_coeff[i],&res->sup_coeff[i]);
		}
		else{
		 	k = i;
			coeff = &linexpr0->p.coeff[k];	
			coeff_to_interval(coeff,&res->inf_coeff[k],&res->sup_coeff[k]);
		}
		
	}
	elina_coeff_t *cst = &linexpr0->cst;
	coeff_to_interval(cst,&res->inf_cst,&res->sup_cst);
	return res;
}


elina_interval_t * get_bounds_for_linexpr0(elina_manager_t *man, elina_abstract0_t *element, elina_linexpr0_t *linexpr0, size_t layerno){
	
	elina_interval_t * res = elina_interval_alloc();
	fppoly_t *fp = fppoly_of_abstract0(element);
    fppoly_internal_t *pr = fppoly_init_from_manager(man, ELINA_FUNID_ASSIGN_LINEXPR_ARRAY);
    //printf("start %p %lu\n",fp->layers[layerno],layerno);
    //fflush(stdout);
	expr_t * tmp = elina_linexpr0_to_expr(linexpr0);
    //printf("coming here\n");
    //fflush(stdout);
    expr_t *expr = expr_from_previous_layer(pr,tmp, fp->layers[layerno]);
    //printf("end\n");
    //fflush(stdout);
	expr_t * expr2 = copy_expr(expr);
	
	double lb = get_lb_using_previous_layers(man,fp,expr,layerno);
	
	double ub = get_ub_using_previous_layers(man,fp,expr2,layerno);
	
	elina_interval_set_double(res,-lb,ub);
	free_expr(expr);
	free_expr(expr2);
    free_expr(tmp);
	return res;
}
     

void create_lstm_layer(elina_manager_t *man, elina_abstract0_t *abs, size_t h, bool alloc){
  fppoly_t *fp = fppoly_of_abstract0(abs);
  if (alloc) {
	size_t numlayers = fp->numlayers;
	fppoly_add_new_layer(fp,h, LSTM, NONE);
	fp->lstm_index = numlayers;
  } else {
	fp->numlayers++;
  }
}

void dp_multiply(fppoly_internal_t *pr, size_t h, expr_t* res,
				   double a_lb, double a_ub, expr_t *a,
				   double b_lb, double b_ub, expr_t *b) {
  size_t i;
  double a_0 = 0.5 * (a_ub - a_lb), b_0 = 0.5 * (b_ub - b_lb);
  double a_r = 0.5 * (a_ub + a_lb), b_r = 0.5 * (b_ub + b_lb);
  
  /* printf("a_0 = %.10lf, b_0 = %.10lf\n", a_0, b_0); */
  /* printf("a_r = %.10lf, b_r = %.10lf\n", a_r, b_r); */
  /* printf("erorr: %.10lf\n", (a_r * b_r)); */
  /* printf("\n"); */
  
  res->inf_cst = (a_r * b_r) + (a_0 * b_0);
  res->sup_cst = (a_r * b_r) - (a_0 * b_0);
  double tmp_a_inf, tmp_a_sup, tmp_b_inf, tmp_b_sup;
  elina_double_interval_mul(&tmp_a_inf, &tmp_a_sup, a->inf_cst, a->sup_cst, -b_0, b_0);
  elina_double_interval_mul(&tmp_b_inf, &tmp_b_sup, b->inf_cst, b->sup_cst, -a_0, a_0);
  res->inf_cst += tmp_a_inf + tmp_b_inf;
  res->sup_cst += tmp_a_sup + tmp_b_sup;
  
  for (i = 0; i < h; ++i) {
  	res->inf_coeff[i] = 0;
  	res->sup_coeff[i] = 0;
  	double tmp1, tmp2;
  	elina_double_interval_mul_expr_coeff(pr, &tmp1, &tmp2, -b_0, b_0, a->inf_coeff[i], a->sup_coeff[i]);
	elina_double_interval_add_cst_coeff(pr, &res->inf_coeff[i], &res->sup_coeff[i], tmp1, tmp2, res->inf_coeff[i], res->sup_coeff[i]);
  	elina_double_interval_mul_expr_coeff(pr, &tmp1, &tmp2, -a_0, a_0, b->inf_coeff[i], b->sup_coeff[i]);
	elina_double_interval_add_cst_coeff(pr, &res->inf_coeff[i], &res->sup_coeff[i], tmp1, tmp2, res->inf_coeff[i], res->sup_coeff[i]);
  }
}

void mul_exprs(fppoly_internal_t *pr, size_t h, expr_t* res_lexpr, expr_t* res_uexpr,
			   double a_lb, double a_ub, expr_t* a_lexpr, expr_t* a_uexpr,
			   double b_lb, double b_ub, expr_t* b_lexpr, expr_t* b_uexpr) {
  bool boxify = true;
  if (!boxify && a_lb < 0 && b_lb < 0) {
  	dp_multiply(pr, h, res_lexpr, a_lb, a_ub, a_lexpr, b_lb, b_ub, b_lexpr);
  	dp_multiply(pr, h, res_uexpr, a_lb, a_ub, a_uexpr, b_lb, b_ub, b_uexpr);
  } else if (!boxify && a_ub < 0 && b_lb < 0) {
  	dp_multiply(pr, h, res_lexpr, a_lb, a_ub, a_lexpr, b_lb, b_ub, b_uexpr);
  	dp_multiply(pr, h, res_uexpr, a_lb, a_ub, a_uexpr, b_lb, b_ub, b_lexpr);
  } else if (!boxify && b_ub < 0 && a_lb < 0) {
  	dp_multiply(pr, h, res_lexpr, a_lb, a_ub, a_uexpr, b_lb, b_ub, b_lexpr);
  	dp_multiply(pr, h, res_uexpr, a_lb, a_ub, a_lexpr, b_lb, b_ub, a_uexpr);
  } else if (!boxify && a_ub < 0 && b_ub < 0) {
  	dp_multiply(pr, h, res_lexpr, a_lb, a_ub, a_uexpr, b_lb, b_ub, b_uexpr);
  	dp_multiply(pr, h, res_uexpr, a_lb, a_ub, a_lexpr, b_lb, b_ub, a_lexpr);
  } else {
	double res_lb, res_ub;
	elina_double_interval_mul(&res_lb, &res_ub, a_lb, a_ub, b_lb, b_ub);
	size_t i;
	for (i = 0; i < h; ++i) {
	  res_lexpr->inf_coeff[i] = res_uexpr->inf_coeff[i] = 0;
	  res_lexpr->sup_coeff[i] = res_uexpr->sup_coeff[i] = 0;
	}
	res_lexpr->inf_cst = res_uexpr->inf_cst = res_lb;
	res_lexpr->sup_cst = res_uexpr->sup_cst = res_ub;
  }
}

double dmin(double a, double b) { return a < b ? a : b; }
double dmax(double a, double b) { return a > b ? a : b; }
double s(double x) { return 1.0 / (1.0 + exp(-x)); }
double s_(double x) { return exp(-x) / ((1 + exp(-x)) * (1 + exp(-x))); }
double t(bool th, double x) { return th ? tanh(x) : x; }
double t_(bool th, double x) { return th ? 1 - tanh(x) * tanh(x) : 1; }
double f(bool th, double x, double y) { return s(x) * t(th, y); }
double dxf(bool th, double x, double y) { return s_(x) * t(th, y); }
double dyf(bool th, double x, double y) { return s(x) * t_(th, y); }
void set_plane(double *A, double *B, double *C, double tA, double tB, double tC) { *A = tA; *B = tB; *C = tC; }

void compute_sigmoid_mul_tanh_planes(bool th, double lx, double ux, double ly, double uy, 
									 double* Al, double* Bl, double* Cl,
									 double* Au, double* Bu, double* Cu) {
  if (ly >= 0) {
	double x_slope = dmin(dxf(th, ux, uy), (f(th, ux, uy) - f(th, lx, uy)) / (ux - lx));
	double Au1 = x_slope;
	double Bu1 = 0;
	double Cu1 = x_slope * (-ux) + f(th, ux, uy);

	double y_slope = dmin(dyf(th, ux, uy), (f(th, ux, uy) - f(th, ux, ly)) / (uy - ly));
	double Au2 = 0;
	double Bu2 = y_slope;
	double Cu2 = y_slope * (-uy) + f(th, ux, uy);
	
	if ((ux + lx) * Au1 / 2 + Cu1 < (uy + ly) * Bu2 / 2 + Cu2) {
	  set_plane(Au, Bu, Cu, Au1, Bu1, Cu1);
	} else {
	  set_plane(Au, Bu, Cu, Au2, Bu2, Cu2);
	}
	
	x_slope = dmin(dxf(th, lx, ly), (f(th, lx, ly) - f(th, ux, ly)) / (lx - ux));
	double Al1 = x_slope;
	double Bl1 = 0;
	double Cl1 = x_slope * (-lx) + f(th, lx, ly);
	y_slope = dmin(dyf(th, lx, ly), (f(th, lx, ly) - f(th, lx, uy)) / (ly - uy));
	double Al2 = 0;
	double Bl2 = y_slope;
	double Cl2 = y_slope * (-ly) + f(th, lx, ly);

	if ((ux + lx) * Al1 / 2 + Cl1 > (uy + ly) * Bl2 / 2 + Cl2) {
	  set_plane(Al, Bl, Cl, Al1, Bl1, Cl1);
	} else {
	  set_plane(Al, Bl, Cl, Al2, Bl2, Cl2);
	}
  } else if (ly <= 0 && 0 <= uy) {
	double x_slope = dmin(dxf(th, ux, uy), (f(th, ux, uy) - f(th, lx, uy)) / (ux - lx));
	double Au1 = x_slope;
	double Bu1 = 0;
	double Cu1 = x_slope * (-ux) + f(th, ux, uy);

	double y_slope = dmin(dyf(th, ux, uy), (f(th, ux, uy) - f(th, lx, ly)) / (uy - ly));
	double Au2 = 0;
	double Bu2 = y_slope;
	double Cu2 = y_slope * (-uy) + f(th, ux, uy);

	if ((ux + lx) * Au1 / 2 + Cu1 < (uy + ly) * Bu2 / 2 + Cu2) {
	  set_plane(Au, Bu, Cu, Au1, Bu1, Cu1);
	} else {
	  set_plane(Au, Bu, Cu, Au2, Bu2, Cu2);
	}

	x_slope = dmax(dxf(th, ux, ly), (f(th, ux, ly) - f(th, lx, ly)) / (ux - lx));
	double Al1 = x_slope;
	double Bl1 = 0;
	double Cl1 = x_slope * (-ux) + f(th, ux, ly);
	
	y_slope = dmin(dyf(th, ux, ly), (f(th, ux, ly) - f(th, lx, uy)) / (ly - uy));
	double Al2 = 0;
	double Bl2 = y_slope;
	double Cl2 = y_slope * (-ly) + f(th, ux, ly);

	if ((ux + lx) * Al1 / 2 + Cl1 > (uy + ly) * Bl2 / 2 + Cl2) {
	  set_plane(Al, Bl, Cl, Al1, Bl1, Cl1);
	} else {
	  set_plane(Al, Bl, Cl, Al2, Bl2, Cl2);
	}
  } else {
	double x_slope = dmax(dxf(th, lx, uy), (f(th, lx, uy) - f(th, ux, uy)) / (lx - ux));
	double Au1 = x_slope;
	double Bu1 = 0;
	double Cu1 = x_slope * (-lx) + f(th, lx, uy);

	double y_slope = dmin(dyf(th, lx, uy), (f(th, lx, uy) - f(th, lx, ly)) / (uy - ly));
	double Au2 = 0;
	double Bu2 = y_slope;
	double Cu2 = y_slope * (-uy) + f(th, lx, uy);
	
	if ((ux + lx) * Au1 / 2 + Cu1 < (uy + ly) * Bu2 / 2 + Cu2) {
	  set_plane(Au, Bu, Cu, Au1, Bu1, Cu1);
	} else {
	  set_plane(Au, Bu, Cu, Au2, Bu2, Cu2);
	}
	
	x_slope = dmax(dxf(th, ux, ly), (f(th, ux, ly) - f(th, lx, ly)) / (ux - lx));
	double Al1 = x_slope;
	double Bl1 = 0;
	double Cl1 = x_slope * (-ux) + f(th, ux, ly);
	
	y_slope = dmin(dyf(th, ux, ly), (f(th, ux, ly) - f(th, ux, uy)) / (ly - uy));
	double Al2 = 0;
	double Bl2 = y_slope;
	double Cl2 = y_slope * (-ly) + f(th, ux, ly);

	if ((ux + lx) * Al1 / 2 + Cl1 > (uy + ly) * Bl2 / 2 + Cl2) {
	  set_plane(Al, Bl, Cl, Al1, Bl1, Cl1);
	} else {
	  set_plane(Al, Bl, Cl, Al2, Bl2, Cl2);
	}
  }
}

// computes sigmoid(a) * tanh(b)
void mul_sigmoid_tanh_exprs(bool th, fppoly_internal_t *pr, size_t h, expr_t* res_lexpr, expr_t* res_uexpr,
							double a_lb, double a_ub, expr_t* a_lexpr, expr_t* a_uexpr,
							double b_lb, double b_ub, expr_t* b_lexpr, expr_t* b_uexpr) {
  if (a_ub + a_lb < 1e-2 || b_ub + b_lb < 1e-2) {
	/* printf("a_lb = %.20lf, a_ub = %.20lf\n",a_lb,a_ub); */
	/* printf("b_lb = %.20lf, b_ub = %.20lf\n",b_lb,b_ub); */
	
	double res_lb, res_ub;
	double fa_lb = -s(-a_lb);
	double fa_ub = s(a_ub);
	double fb_lb = -t(th, -b_lb);
	double fb_ub = t(th, b_ub);
	  
	elina_double_interval_mul(&res_lb, &res_ub, fa_lb, fa_ub, fb_lb, fb_ub);
	size_t i;
	for (i = 0; i < h; ++i) {
	  res_lexpr->inf_coeff[i] = res_uexpr->inf_coeff[i] = 0;
	  res_lexpr->sup_coeff[i] = res_uexpr->sup_coeff[i] = 0;
	}
	res_lexpr->inf_cst = res_uexpr->inf_cst = res_lb;
	res_lexpr->sup_cst = res_uexpr->sup_cst = res_ub;
	return;
  }
  double Al, Bl, Cl, Au, Bu, Cu;
  double delta = 1e-5;
  size_t i;
  expr_t* tmp_lexpr;
  expr_t* tmp_uexpr;
  
  compute_sigmoid_mul_tanh_planes(th, -a_lb - delta, a_ub + delta, -b_lb - delta, b_ub + delta, &Al, &Bl, &Cl, &Au, &Bu, &Cu);

  if (Al > 0) {
	*res_lexpr = *multiply_expr(pr, a_lexpr, -Al, Al);
  } else {
	*res_lexpr = *multiply_expr(pr, a_uexpr, -Al, Al);
  }
  if (Bl > 0) {
	tmp_lexpr = multiply_expr(pr, b_lexpr, -Bl, Bl);
  } else {
	tmp_lexpr = multiply_expr(pr, b_uexpr, -Bl, Bl);
  }
  add_expr(pr, res_lexpr, tmp_lexpr);
  elina_double_interval_add_cst_coeff(pr, &res_lexpr->inf_cst, &res_lexpr->sup_cst, -Cl, Cl, res_lexpr->inf_cst, res_lexpr->sup_cst);

  if (Au > 0) {
	*res_uexpr = *multiply_expr(pr, a_uexpr, -Au, Au);
  } else {
	*res_uexpr = *multiply_expr(pr, a_lexpr, -Au, Au);
  }
  if (Bu > 0) {
	tmp_uexpr = multiply_expr(pr, b_uexpr, -Bu, Bu);
  } else {
	tmp_uexpr = multiply_expr(pr, b_lexpr, -Bu, Bu);
  }
  add_expr(pr, res_uexpr, tmp_uexpr);
  elina_double_interval_add_cst_coeff(pr, &res_uexpr->inf_cst, &res_uexpr->sup_cst, -Cu, Cu, res_uexpr->inf_cst, res_uexpr->sup_cst);
  /* printf("-> res_lexpr: "); expr_print(res_lexpr); */
  /* printf("-> res_uexpr: "); expr_print(res_uexpr); */
}


void handle_lstm_layer(elina_manager_t *man, elina_abstract0_t *abs, double **weights,  double *bias, size_t d, size_t h){
	fppoly_t *fp = fppoly_of_abstract0(abs);
	fppoly_internal_t *pr = fppoly_init_from_manager(man, ELINA_FUNID_ASSIGN_LINEXPR_ARRAY);
	size_t lstm_index = fp->lstm_index;
	layer_t *layer = fp->layers[lstm_index];
	neuron_t **out_neurons = fp->layers[lstm_index]->neurons;
	size_t i;
	neuron_t * neuron = neuron_alloc();
	bool first_time_step = (layer->h_t_inf==NULL && layer->h_t_sup==NULL);
	bool force_boxify = false;

	double* prev_lb = (double*)malloc(h*sizeof(double));
	double* prev_ub = (double*)malloc(h*sizeof(double));
	layer_t *prev_layer = fp->layers[lstm_index-1];
	for (i = 0; i < prev_layer->dims; ++i) {
	  // assumes there is relu before LSTM
	  prev_lb[i] = prev_layer->neurons[i]->lb;
	  prev_ub[i] = prev_layer->neurons[i]->ub;
	  if (prev_lb[i] > 0) {
		prev_lb[i] = 0;
	  }
	  if (prev_ub[i] < 0) {
		prev_ub[i] = 0;
	  }
	  /* printf("prev_neuron: %lf %lf\n",prev_lb[i],prev_ub[i]); */
	}

	double *new_h_inf = (double*)malloc(h*sizeof(double));
	double *new_h_sup = (double*)malloc(h*sizeof(double));
	double *new_c_inf = (double*)malloc(h*sizeof(double));
	double *new_c_sup = (double*)malloc(h*sizeof(double));
	
	size_t k = h + d;
	if(first_time_step){
		layer->h_t_inf = (double*)malloc(h*sizeof(double));
		layer->h_t_sup = (double*)malloc(h*sizeof(double));
		layer->c_t_inf = (double*)malloc(h*sizeof(double));
		layer->c_t_sup = (double*)malloc(h*sizeof(double));
	} else {
	  /* printf("previous cell state:\n"); */
	  /* for (i = 0; i < h; ++i) { */
	  /* 	printf("inf = %.10lf, sup = %.10lf\n", layer->c_t_inf[i], layer->c_t_sup[i]); */
	  /* } */
	  /* printf("previous hidden state:\n"); */
	  /* for (i = 0; i < h; ++i) { */
	  /* 	printf("inf = %.10lf, sup = %.10lf\n", layer->h_t_inf[i], layer->h_t_sup[i]); */
	  /* } */
	}
	
	for(i=0; i< h; i++){
	    expr_t *f_t_lexpr, *i_t_lexpr, *o_t_lexpr, *c_t_lexpr;
		if(first_time_step){
			i_t_lexpr =  create_dense_expr(weights[i],bias[i],d);
			c_t_lexpr =  create_dense_expr(weights[h+i],bias[h+i],d);	
			f_t_lexpr =  create_dense_expr(weights[2*h+i],bias[2*h+i],d);
			o_t_lexpr =  create_dense_expr(weights[3*h+i],bias[3*h+i],d);
		}		
		else{
			expr_t * tmp1 = create_dense_expr(weights[i],bias[i],d+h);
			expr_t * tmp2 = create_dense_expr(weights[h+i],bias[h+i],d+h);
			expr_t * tmp3 = create_dense_expr(weights[2*h+i],bias[2*h+i],d+h);
			expr_t * tmp4 = create_dense_expr(weights[3*h+i],bias[3*h+i],d+h);
			i_t_lexpr = concretize_dense_sub_expr(pr, tmp1, layer->h_t_inf, layer->h_t_sup, d, d+h);
			c_t_lexpr = concretize_dense_sub_expr(pr, tmp2, layer->h_t_inf, layer->h_t_sup, d, d+h);
			f_t_lexpr = concretize_dense_sub_expr(pr, tmp3, layer->h_t_inf, layer->h_t_sup, d, d+h);
			o_t_lexpr = concretize_dense_sub_expr(pr, tmp4, layer->h_t_inf, layer->h_t_sup, d, d+h);
			free_expr(tmp1);	
			free_expr(tmp2);
			free_expr(tmp3);
			free_expr(tmp4);
		}


		// compute forget gate
		expr_t *f_t_uexpr = copy_expr(f_t_lexpr);
		expr_t *pre_f_t_lexpr = copy_expr(f_t_lexpr);
		expr_t *pre_f_t_uexpr = copy_expr(f_t_lexpr);
		
		expr_t *tmp_f_t_lexpr = copy_expr(f_t_lexpr);
		expr_t *tmp_f_t_uexpr = copy_expr(f_t_uexpr);
		double pre_lb_f_t = get_lb_using_previous_layers(man, fp, tmp_f_t_lexpr, lstm_index);
		double pre_ub_f_t = get_ub_using_previous_layers(man, fp, tmp_f_t_uexpr, lstm_index);
		
		/* expr_print(f_t_lexpr); */
		/* printf("pre_lb_f_t = %.30lf\n",pre_lb_f_t); */
		/* printf("pre_ub_f_t = %.30lf\n",pre_ub_f_t); */
				
		free_expr(tmp_f_t_lexpr);
		free_expr(tmp_f_t_uexpr);

		neuron->lb = pre_lb_f_t;
		neuron->ub = pre_ub_f_t;
		double lb_f_t = apply_sigmoid_lexpr(pr, &f_t_lexpr, neuron, force_boxify);
		double ub_f_t = apply_sigmoid_uexpr(pr, &f_t_uexpr, neuron, force_boxify);

		// compute input gate
		expr_t *i_t_uexpr = copy_expr(i_t_lexpr);
		expr_t *pre_i_t_lexpr = copy_expr(i_t_lexpr);
		expr_t *pre_i_t_uexpr = copy_expr(i_t_uexpr);
		
		expr_t *tmp_i_t_lexpr = copy_expr(i_t_lexpr);
		expr_t *tmp_i_t_uexpr = copy_expr(i_t_uexpr);
		double pre_lb_i_t = get_lb_using_previous_layers(man, fp, tmp_i_t_lexpr,lstm_index);
		double pre_ub_i_t = get_ub_using_previous_layers(man, fp, tmp_i_t_uexpr, lstm_index);	
		free_expr(tmp_i_t_lexpr);
		free_expr(tmp_i_t_uexpr);
		
		neuron->lb = pre_lb_i_t;
		neuron->ub = pre_ub_i_t;
		double lb_i_t = apply_sigmoid_lexpr(pr, &i_t_lexpr, neuron, force_boxify);
		double ub_i_t = apply_sigmoid_uexpr(pr, &i_t_uexpr, neuron, force_boxify);

		// compute output gate
		expr_t *o_t_uexpr = copy_expr(o_t_lexpr);
		expr_t *pre_o_t_lexpr = copy_expr(o_t_lexpr);
		expr_t *pre_o_t_uexpr = copy_expr(o_t_uexpr);
		
		expr_t *tmp_o_t_lexpr = copy_expr(o_t_lexpr);
		expr_t *tmp_o_t_uexpr = copy_expr(o_t_uexpr);
		double pre_lb_o_t = get_lb_using_previous_layers(man, fp, tmp_o_t_lexpr, lstm_index);
		double pre_ub_o_t = get_ub_using_previous_layers(man, fp, tmp_o_t_uexpr, lstm_index);
		free_expr(tmp_o_t_lexpr);
		free_expr(tmp_o_t_uexpr);

		neuron->lb = pre_lb_o_t;
		neuron->ub = pre_ub_o_t;		
		double lb_o_t = apply_sigmoid_lexpr(pr, &o_t_lexpr, neuron, force_boxify);
		double ub_o_t = apply_sigmoid_uexpr(pr, &o_t_uexpr, neuron, force_boxify);
		
		// compute cell state
		expr_t *c_t_uexpr = copy_expr(c_t_lexpr);
		expr_t *pre_c_t_lexpr = copy_expr(c_t_lexpr);
		expr_t *pre_c_t_uexpr = copy_expr(c_t_uexpr);
		
		expr_t *tmp_c_t_lexpr = copy_expr(c_t_lexpr);
		expr_t *tmp_c_t_uexpr = copy_expr(c_t_uexpr);
		double pre_lb_c_t = get_lb_using_previous_layers(man, fp, tmp_c_t_lexpr, lstm_index);
		double pre_ub_c_t = get_ub_using_previous_layers(man, fp, tmp_c_t_uexpr, lstm_index);
		free_expr(tmp_c_t_lexpr);
		free_expr(tmp_c_t_uexpr);
		
		neuron->lb = pre_lb_c_t;
		neuron->ub = pre_ub_c_t;
		double lb_c_t = apply_tanh_lexpr(pr,&c_t_lexpr, neuron, force_boxify);
		double ub_c_t = apply_tanh_uexpr(pr,&c_t_uexpr, neuron, force_boxify);

		// multiply cell and input
		expr_t* ci_lexpr = copy_expr(c_t_lexpr);
		expr_t* ci_uexpr = copy_expr(c_t_uexpr);
		if (force_boxify) {
		  mul_exprs(pr, h, ci_lexpr, ci_uexpr,
		  		  lb_c_t, ub_c_t, c_t_lexpr, c_t_uexpr,
		  		  lb_i_t, ub_i_t, i_t_lexpr, i_t_uexpr);
		} else {
		  mul_sigmoid_tanh_exprs(true, pr, h, ci_lexpr, ci_uexpr,
								 pre_lb_i_t, pre_ub_i_t, pre_i_t_lexpr, pre_i_t_uexpr,
								 pre_lb_c_t, pre_ub_c_t, pre_c_t_lexpr, pre_c_t_uexpr);
		}
		c_t_lexpr = ci_lexpr;
		c_t_uexpr = ci_uexpr;

		/* expr_print(c_t_lexpr); */
		/* expr_print(c_t_uexpr); */

		if (!first_time_step) {
		  expr_t* tmp_l = copy_expr(f_t_lexpr);
		  expr_t* tmp_u = copy_expr(f_t_uexpr);
		  expr_t* box_l;
		  expr_t* box_u;
		  double tmp1, tmp2;
		  
		  // pure intervals
		  /* tmp_l = multiply_expr(pr,f_t_lexpr,0,0); */
		  /* tmp_u = multiply_expr(pr,f_t_uexpr,0,0); */
		  /* elina_double_interval_mul_expr_coeff(pr,&tmp1,&tmp2,lb_f_t,ub_f_t,layer->c_t_inf[i],layer->c_t_sup[i]); */
		  /* tmp_l->inf_cst += tmp1; */
		  /* tmp_l->sup_cst += tmp2; */
		  /* tmp_u->inf_cst += tmp1; */
		  /* tmp_u->sup_cst += tmp2; */

		  /* add_expr(pr,c_t_lexpr,tmp_l); */
		  /* add_expr(pr,c_t_uexpr,tmp_u); */
		  /* free_expr(tmp_l); */
		  /* free_expr(tmp_u); */

		  // sigmoid bounds
		  if (!force_boxify) {
			expr_t* prev_c_lexpr = multiply_expr(pr, f_t_lexpr, 0, 0);
			expr_t* prev_c_uexpr = multiply_expr(pr, f_t_uexpr, 0, 0);
			prev_c_lexpr->inf_cst += layer->c_t_inf[i];
			prev_c_lexpr->sup_cst += layer->c_t_sup[i];
			prev_c_uexpr->inf_cst += layer->c_t_inf[i];
			prev_c_uexpr->sup_cst += layer->c_t_sup[i];
			mul_sigmoid_tanh_exprs(false, pr, h, tmp_l, tmp_u,
								   pre_lb_f_t, pre_ub_f_t, pre_f_t_lexpr, pre_f_t_uexpr,
								   layer->c_t_inf[i], layer->c_t_sup[i], prev_c_lexpr, prev_c_uexpr);
			
			/* double tmp_lb = get_lb_using_previous_layers(man, fp, tmp_l, lstm_index); */
			/* double tmp_ub = get_lb_using_previous_layers(man, fp, tmp_u, lstm_index); */
		  } else {
			box_l = multiply_expr(pr, f_t_lexpr, 0, 0);
			box_u = multiply_expr(pr, f_t_uexpr, 0, 0);
			elina_double_interval_mul_expr_coeff(pr, &tmp1, &tmp2, lb_f_t, ub_f_t, layer->c_t_inf[i], layer->c_t_sup[i]);
			box_l->inf_cst += tmp1;
			box_l->sup_cst += tmp2;
			box_u->inf_cst += tmp1;
			box_u->sup_cst += tmp2;
		  }
		  
		  /* printf("tmp_lb = %.10lf, tmp_ub = %.10lf\n",tmp_lb,tmp_ub); */
		  /* printf("box_lb = %.10lf, box_ub = %.10lf\n",-box_l->inf_cst,box_u->sup_cst); */

		  if (!force_boxify) {
			add_expr(pr, c_t_lexpr, tmp_l);
			add_expr(pr, c_t_uexpr, tmp_u);
		  } else {
			add_expr(pr, c_t_lexpr, box_l);
			add_expr(pr, c_t_uexpr, box_u);
			free_expr(box_l);
			free_expr(box_u);
		  }
		  free_expr(tmp_l);
		  free_expr(tmp_u);
		}

		pre_c_t_lexpr = copy_expr(c_t_lexpr);
		pre_c_t_uexpr = copy_expr(c_t_uexpr);
		tmp_c_t_lexpr = copy_expr(c_t_lexpr);
		tmp_c_t_uexpr = copy_expr(c_t_uexpr);
		lb_c_t = get_lb_using_previous_layers(man, fp, tmp_c_t_lexpr, lstm_index);
		ub_c_t = get_ub_using_previous_layers(man, fp, tmp_c_t_uexpr, lstm_index);
		free_expr(tmp_c_t_lexpr);
		free_expr(tmp_c_t_uexpr);
		
		new_c_inf[i] = lb_c_t;
		new_c_sup[i] = ub_c_t;
		neuron->lb = lb_c_t;
		neuron->ub = ub_c_t;
		lb_c_t = apply_tanh_lexpr(pr,&c_t_lexpr, neuron, force_boxify);
		ub_c_t = apply_tanh_uexpr(pr,&c_t_uexpr, neuron, force_boxify);
		
		expr_t *h_t_lexpr = copy_expr(c_t_lexpr);
		expr_t *h_t_uexpr = copy_expr(c_t_uexpr);
		if (force_boxify) {
		  mul_exprs(pr,h,h_t_lexpr,h_t_uexpr,lb_c_t,ub_c_t,c_t_lexpr,c_t_uexpr,lb_o_t,ub_o_t,o_t_lexpr,o_t_uexpr);
		} else {
		  mul_sigmoid_tanh_exprs(true, pr, h, h_t_lexpr, h_t_uexpr,
								 pre_lb_o_t, pre_ub_o_t, pre_o_t_lexpr, pre_o_t_uexpr,
								 new_c_inf[i], new_c_sup[i], pre_c_t_lexpr, pre_c_t_uexpr);
		}
		
		new_h_inf[i] = get_lb_using_previous_layers(man, fp, h_t_lexpr, lstm_index);
		new_h_sup[i] = get_ub_using_previous_layers(man, fp, h_t_uexpr, lstm_index);

		out_neurons[i]->lb = new_h_inf[i];
		out_neurons[i]->ub = new_h_sup[i];
		out_neurons[i]->lexpr = h_t_lexpr;
		out_neurons[i]->uexpr = h_t_uexpr;
		
		/* expr_print(h_t_lexpr); */

		free_expr(f_t_lexpr);
		free_expr(f_t_uexpr);
		free_expr(i_t_lexpr);
		free_expr(i_t_uexpr);
		free_expr(c_t_lexpr);
		free_expr(c_t_uexpr);
	}
	free_neuron(neuron);


	for (i = 0; i < h; i++) { 
	  layer->c_t_inf[i] = new_c_inf[i];
	  layer->c_t_sup[i] = new_c_sup[i];
	  layer->h_t_inf[i] = new_h_inf[i];
	  layer->h_t_sup[i] = new_h_sup[i];
	  /* printf("[%d] c_t: [%.20lf, %.20lf]\n", (int)i, new_c_inf[i], new_c_sup[i]); */
	  /* printf("[%d] h_t: [%.20lf, %.20lf]\n", (int)i, new_h_inf[i], new_h_sup[i]); */
	}
	free(new_c_inf);
	free(new_c_sup);
	free(new_h_inf);
	free(new_h_sup);
	//update_state_using_previous_layers_parallel(man,fp,numlayers);
	return;
}




bool is_greater(elina_manager_t* man, elina_abstract0_t* element, elina_dim_t y, elina_dim_t x){
	fppoly_t *fp = fppoly_of_abstract0(element);
	fppoly_internal_t * pr = fppoly_init_from_manager(man,ELINA_FUNID_ASSIGN_LINEXPR_ARRAY);
	if(1){
	  size_t out_size = fp->layers[fp->numlayers - 1]->dims;
	  
	  expr_t * sub = (expr_t *)malloc(sizeof(expr_t));
	  sub->size = out_size;
	  sub->type = DENSE;
	  
	  sub->inf_coeff = (double*)malloc(sub->size*sizeof(double));
	  sub->sup_coeff = (double*)malloc(sub->size*sizeof(double));

	  size_t i;
	  for (i = 0; i < sub->size; ++i) {
		sub->inf_coeff[i] = 0;
		sub->sup_coeff[i] = 0;
	  }
	  sub->inf_cst = 0;
	  sub->sup_cst = 0;
	  sub->inf_coeff[y] = -1;
	  sub->sup_coeff[y] = 1;
	  sub->inf_coeff[x] = 1;
	  sub->sup_coeff[x] = -1;

	  /* expr_print(sub); */
	  
	  double lb = get_lb_using_previous_layers(man, fp, sub, fp->numlayers);
	  
	  free_expr(sub);
	  
	  if(lb<0){
		return true;
	  }
	  else{
		return false;
	  }
	}
	output_abstract_t * out = fp->out;
	expr_t * exprA = out->lexpr[y];
	expr_t * exprB = out->uexpr[x];
	if(exprA==NULL){
		return false;
	}
	else{
		if(exprB==NULL){
			if(out->output_inf[y]<0){
				return true;
			}
			else{
				return false;
			}
			
		}
		else{
			//printf("before access %zu %zu\n", exprA->size,exprB->size);
			//fflush(stdout);
			size_t sizeA = exprA->size;
			size_t sizeB = exprB->size;
			//printf("after access\n");
			//fflush(stdout);
			size_t i,k;
			expr_t * sub = (expr_t *)malloc(sizeof(expr_t));
			//
			//sub->size = size;
			sub->inf_cst = exprA->inf_cst + exprB->sup_cst;
			sub->sup_cst = exprA->sup_cst + exprB->inf_cst;
			//printf("getting here\n");
			//expr_print(exprA);
			//expr_print(exprB);
			//fflush(stdout);
			if(exprA->type==DENSE){
				sub->inf_coeff = (double*)malloc(sizeA*sizeof(double));
				sub->sup_coeff = (double*)malloc(sizeA*sizeof(double));
				sub->dim=NULL;
				sub->size = sizeA;
				sub->type = DENSE;
				if(exprB->type==DENSE){
						for(i=0; i < sizeA; i++){
							sub->inf_coeff[i] = exprA->inf_coeff[i] + exprB->sup_coeff[i];
							sub->sup_coeff[i] = exprA->sup_coeff[i] + exprB->inf_coeff[i];
						} 
				}
				else{
					k = 0;
					for(i=0; i < sizeA; i++){
						if(k < sizeB && exprB->dim[k]==i){
							sub->inf_coeff[i] = exprA->inf_coeff[i] + exprB->sup_coeff[k];
							sub->sup_coeff[i] = exprA->sup_coeff[i] + exprB->inf_coeff[k];
							k++;
						}
						else{
							sub->inf_coeff[i] = exprA->inf_coeff[i];
							sub->sup_coeff[i] = exprA->sup_coeff[i];
						}
					}
				}
				
			}
			else{
				if(exprB->type==DENSE){
					sub->inf_coeff = (double*)malloc(sizeB*sizeof(double));
					sub->sup_coeff = (double*)malloc(sizeB*sizeof(double));
					sub->dim=NULL;
					sub->size = sizeB;
					sub->type = DENSE;
					i = 0;
					for(k=0; k < sizeB; k++){
						if(i < sizeA && exprA->dim[i]==k){
							sub->inf_coeff[k] = exprA->inf_coeff[i] + exprB->sup_coeff[k];
							sub->sup_coeff[k] = exprA->sup_coeff[i] + exprB->inf_coeff[k];
							i++;
						}
						else{
							sub->inf_coeff[i] = exprB->sup_coeff[k];
							sub->sup_coeff[i] = exprB->inf_coeff[k];
						}
					}
				}
				else{
					sub->inf_coeff = (double*)malloc((sizeA+sizeB)*sizeof(double));
					sub->sup_coeff = (double*)malloc((sizeA+sizeB)*sizeof(double));
					sub->dim=NULL;
					
					sub->type = SPARSE;
					size_t l = 0;
					i=0;
					k=0;
					sub->dim = (size_t *)malloc((sizeA+sizeB)*sizeof(size_t));
					while(i < sizeA && k < sizeB){
						if(exprA->dim[i] < exprB->dim[k]){
							sub->inf_coeff[l] = exprA->inf_coeff[i];
							sub->sup_coeff[l] = exprA->sup_coeff[i];
							sub->dim[l] = exprA->dim[i];
							i++;
						
						}
						else if(exprB->dim[k] < exprA->dim[i]){
							sub->inf_coeff[l] = exprB->sup_coeff[k];
							sub->sup_coeff[l] = exprB->inf_coeff[k];
							sub->dim[l] = exprB->dim[k];
							k++;
						}
						else{
							sub->inf_coeff[l] = exprA->inf_coeff[i] + exprB->sup_coeff[k];
							sub->sup_coeff[l] = exprA->sup_coeff[i] + exprB->inf_coeff[k];
							sub->dim[l] = exprA->dim[i];
							i++;
							k++;
						}
						l++;
					}
					while(i < sizeA){
						sub->inf_coeff[l] = exprA->inf_coeff[i];
						sub->sup_coeff[l] = exprA->sup_coeff[i];
						sub->dim[l] = exprA->dim[i];
						i++;
						l++;
					}
					while(k < sizeB){
						sub->inf_coeff[l] = exprB->inf_coeff[k];
						sub->sup_coeff[l] = exprB->sup_coeff[k];
						sub->dim[l] = exprB->dim[k];
						k++;
						l++;
					}
					sub->size = l;
					sub->inf_coeff = (double*)realloc(sub->inf_coeff,l*sizeof(double));
					sub->sup_coeff = (double*)realloc(sub->sup_coeff,l*sizeof(double));
					sub->dim = (size_t *)realloc(sub->dim,l*sizeof(size_t));
				}
			}
			
			//expr_print(sub);
			//fflush(stdout);
			double lb = compute_lb_from_expr(pr,sub,fp);
			//printf("y: %zu x: %zu lb: %g\n",y,x,lb);
			//fflush(stdout);
			free_expr(sub);
			//double lb = -out->output_inf[y] - out->output_sup[x];
			if(lb<0){
				return true;
			}
			else{
				return false;
			}
		}
	}
	
}


long int max(long int a, long int b){
	return a> b? a : b;

}

void conv_handle_first_layer(elina_manager_t *man, elina_abstract0_t *abs, double *filter_weights, double *filter_bias, 
					  size_t *input_size, size_t *filter_size, size_t num_filters, size_t *strides, bool is_valid_padding, bool has_bias){
	
	size_t i, j;
	size_t num_pixels = input_size[0]*input_size[1]*input_size[2];
	
	size_t output_size[3];
	if(is_valid_padding){
		output_size[0] = ceil((double)(input_size[0] - filter_size[0]+1) / (double)strides[0]);
		output_size[1] = ceil((double)(input_size[1] - filter_size[1]+1) / (double)strides[1]);
	}
	else{
		output_size[0] = ceil((double)input_size[0] / (double)strides[0]);
		output_size[1] = ceil((double)input_size[1] / (double)strides[1]);
	}
	
	output_size[2] = num_filters;
	size_t size = output_size[0]*output_size[1]*output_size[2];
	
	fppoly_internal_t * pr = fppoly_init_from_manager(man,ELINA_FUNID_ASSIGN_LINEXPR_ARRAY);
	fppoly_t *res = fppoly_of_abstract0(abs);
	fppoly_alloc_first_layer(res,size,  CONV, RELU);

	neuron_t ** neurons = res->layers[0]->neurons;
	size_t out_x, out_y, out_z;
        size_t inp_x, inp_y, inp_z;
	size_t x_shift, y_shift;

	long int pad_along_height=0, pad_along_width=0;
	long int pad_top=0,  pad_left=0,  tmp=0;
	if(!is_valid_padding){
		if (input_size[0] % strides[0] == 0){
			long int tmp = filter_size[0] - strides[0];
	  		pad_along_height = max(tmp, 0);
		}
		else{
			tmp = filter_size[0] - (input_size[0] % strides[0]);
	  		pad_along_height = max(tmp, 0);
		}
		if (input_size[1] % strides[1] == 0){
			tmp = filter_size[1] - strides[1];
	  		pad_along_width = max(tmp, 0);
		}
		else{
			tmp = filter_size[1] - (input_size[1] % strides[1]);
	  		pad_along_width = max(tmp, 0);
		}
		pad_top = pad_along_height / 2;
		
		pad_left = pad_along_width / 2;
		
	}

	for(out_x=0; out_x < output_size[0]; out_x++) {
	    for(out_y = 0; out_y < output_size[1]; out_y++) {
		 for(out_z=0; out_z < output_size[2]; out_z++) {
		     size_t mat_x = out_x*output_size[1]*output_size[2] + out_y*output_size[2] + out_z;
		     size_t num_coeff = input_size[2]*filter_size[0]*filter_size[1];
		     size_t actual_coeff = 0;
		     double *coeff = (double *)malloc(num_coeff*sizeof(double));
		     size_t *dim = (size_t *)malloc(num_coeff*sizeof(double));
		     i=0;
		     for(inp_z=0; inp_z <input_size[2]; inp_z++) {
			 for(x_shift = 0; x_shift < filter_size[0]; x_shift++) {
			     for(y_shift =0; y_shift < filter_size[1]; y_shift++) {
				     long int x_val = out_x*strides[0]+x_shift-pad_top;	
			  	     long int y_val = out_y*strides[1]+y_shift-pad_left;
			  	     if(y_val<0 || y_val >= (long int)input_size[1]){
			     			continue;
			  	     }
				     
			  	     if(x_val<0 || x_val >= (long int)input_size[0]){
			     			continue;
			  	     }
				     size_t mat_y = x_val*input_size[1]*input_size[2] + y_val*input_size[2] + inp_z;
				     if(mat_y>=num_pixels){		 
			     			continue;
		          	     } 
				     size_t filter_index = x_shift*filter_size[1]*input_size[2]*output_size[2] + y_shift*input_size[2]*output_size[2] + inp_z*output_size[2] + out_z;
				     coeff[i] = filter_weights[filter_index];
				     dim[i] = mat_y;
				     i++;
				     actual_coeff++;
			     }
			}
		    }
		    double cst = has_bias? filter_bias[out_z] : 0;
	            neurons[mat_x]->expr = create_sparse_expr(coeff,cst,dim,actual_coeff);
		    sort_sparse_expr(neurons[mat_x]->expr);
		  
		
		   neurons[mat_x]->lb = compute_lb_from_expr(pr, neurons[mat_x]->expr,res);
		   neurons[mat_x]->ub = compute_ub_from_expr(pr, neurons[mat_x]->expr,res);
		   free(coeff);
		   free(dim);
	        }
	     }
	}

	
	//printf("return here\n");
	//fppoly_fprint(stdout,man,res,NULL);
	//fflush(stdout);
	return;
}



void conv_handle_intermediate_relu_layer(elina_manager_t* man, elina_abstract0_t* element, double *filter_weights, double * filter_bias,  
				         size_t * input_size, size_t *filter_size, size_t num_filters, size_t *strides, bool is_valid_padding, bool has_bias){
	//printf("conv intermediate starts here\n");
	//fflush(stdout);
	fppoly_t *fp = fppoly_of_abstract0(element);
	size_t numlayers = fp->numlayers;
	size_t i, j;
	size_t num_pixels = input_size[0]*input_size[1]*input_size[2];
	size_t output_size[3];
	if(is_valid_padding){
		output_size[0] = ceil((double)(input_size[0] - filter_size[0]+1) / (double)strides[0]);
		output_size[1] = ceil((double)(input_size[1] - filter_size[1]+1) / (double)strides[1]);
	}
	else{
		output_size[0] = ceil((double)input_size[0] / (double)strides[0]);
		output_size[1] = ceil((double)input_size[1] / (double)strides[1]);
	}
	
	output_size[2] = num_filters;
	size_t num_out_neurons = output_size[0]*output_size[1]*output_size[2];
	//printf("num_out_neurons: %zu %zu\n",num_out_neurons,num_pixels);
	//fflush(stdout);
	fppoly_add_new_layer(fp,num_out_neurons, CONV, RELU);
	neuron_t ** out_neurons = fp->layers[numlayers]->neurons;
	size_t out_x, out_y, out_z;
        size_t inp_x, inp_y, inp_z;
	size_t x_shift, y_shift;

	long int pad_along_height=0, pad_along_width=0;
	long int pad_top=0,  pad_left=0,  tmp=0;
	if(!is_valid_padding){
		if (input_size[0] % strides[0] == 0){
			long int tmp = filter_size[0] - strides[0];
	  		pad_along_height = max(tmp, 0);
		}
		else{
			tmp = filter_size[0] - (input_size[0] % strides[0]);
	  		pad_along_height = max(tmp, 0);
		}
		if (input_size[1] % strides[1] == 0){
			tmp = filter_size[1] - strides[1];
	  		pad_along_width = max(tmp, 0);
		}
		else{
			tmp = filter_size[1] - (input_size[1] % strides[1]);
	  		pad_along_width = max(tmp, 0);
		}
		pad_top = pad_along_height / 2;
		
		pad_left = pad_along_width / 2;
		
	}

	for(out_x=0; out_x < output_size[0]; out_x++) {
	    for(out_y = 0; out_y < output_size[1]; out_y++) {
		 for(out_z=0; out_z < output_size[2]; out_z++) {
		     size_t mat_x = out_x*output_size[1]*output_size[2] + out_y*output_size[2] + out_z;
		     size_t num_coeff = input_size[2]*filter_size[0]*filter_size[1];
		     size_t actual_coeff = 0;
		     double *coeff = (double *)malloc(num_coeff*sizeof(double));
		     size_t *dim = (size_t *)malloc(num_coeff*sizeof(double));
		     i=0;
		     for(inp_z=0; inp_z <input_size[2]; inp_z++) {
			 for(x_shift = 0; x_shift < filter_size[0]; x_shift++) {
			     for(y_shift =0; y_shift < filter_size[1]; y_shift++) {
				     long int x_val = out_x*strides[0]+x_shift-pad_top;	
			  	     long int y_val = out_y*strides[1]+y_shift-pad_left;
			  	     if(y_val<0 || y_val >= (long int)input_size[1]){
			     			continue;
			  	     }
				     
			  	     if(x_val<0 || x_val >= (long int)input_size[0]){
			     			continue;
			  	     }
				     size_t mat_y = x_val*input_size[1]*input_size[2] + y_val*input_size[2] + inp_z;
				     if(mat_y>=num_pixels){		 
			     			continue;
		          	     }
				     size_t filter_index = x_shift*filter_size[1]*input_size[2]*output_size[2] + y_shift*input_size[2]*output_size[2] + inp_z*output_size[2] + out_z;
				     coeff[i] = filter_weights[filter_index];
				     dim[i] = mat_y;
				     actual_coeff++;
				     i++;
			     }
			}
		    }
		   double cst = has_bias? filter_bias[out_z] : 0;
	           out_neurons[mat_x]->expr = create_sparse_expr(coeff,cst,dim,actual_coeff);
		   sort_sparse_expr(out_neurons[mat_x]->expr); 
		   free(coeff);
		   free(dim);
	        }
	     }
	}
	
	update_state_using_previous_layers_parallel(man,fp,numlayers);
	
	//printf("return here2\n");
	//fppoly_fprint(stdout,man,fp,NULL);
	//fflush(stdout);
	return;
}


size_t handle_maxpool_layer(elina_manager_t *man, elina_abstract0_t *element, 
			   size_t *pool_size, size_t *input_size){
	//assert(dimensionality==3);
	//printf("maxpool start\n");
	//fflush(stdout);
	//printf("maxpool start\n");
	//fflush(stdout);
	assert(pool_size[0]==2 && pool_size[1]==2 && pool_size[2]==1);
	//assert(stride[0]==2 && stride[1]==2 && stride[2]==1);
	
	size_t i,j,k;
	size_t * output_size = (size_t *)malloc(3*sizeof(size_t));
	for(i=0; i < 3; i++){
		output_size[i] = input_size[i]/pool_size[i];
	}


	size_t num_input_neurons = input_size[0]*input_size[1]*input_size[2];
	size_t num_out_neurons = output_size[0]*output_size[1]*output_size[2];

    	size_t o12 = output_size[1]*output_size[2];
   	size_t i12 = input_size[1]*input_size[2];
    	size_t p01 = pool_size[0]*pool_size[1];
	
	fppoly_t * fp = fppoly_of_abstract0(element);
	size_t numlayers = fp->numlayers;
	fppoly_add_new_layer(fp,num_out_neurons, MAXPOOL, NONE);
	size_t out_pos;
	double * inf = (double *) calloc(p01,sizeof(double));
	double * sup = (double *) calloc(p01,sizeof(double));
	size_t * pool_map = (size_t *)calloc(p01,sizeof(size_t));
	neuron_t ** out_neurons = fp->layers[numlayers]->neurons;
	size_t count = 0;
	for(out_pos=0; out_pos<num_out_neurons; out_pos++){
		size_t out_x = out_pos / o12;
		size_t out_y = (out_pos-out_x*o12) / output_size[2];
		size_t out_z = out_pos-out_x*o12 - out_y*output_size[2];
		size_t inp_x = out_x*pool_size[0];
		size_t inp_y = out_y*pool_size[1];
		size_t inp_z = out_z;
		size_t inp_pos = inp_x*i12 + inp_y*input_size[2] + inp_z;
		size_t pool_start_dim = out_pos*pool_size[0]*pool_size[1];
		//printf("inpXYZ: %zu, %zu, %zu %zu %zu\n", inp_x, inp_y, inp_z, out_pos, num_out_neurons);
        	//printf("outXYZ: %zu, %zu, %zu\n", out_x, out_y, out_z);
		//fflush(stdout);
		size_t x_shift, y_shift, l = 0;
		double sum_u = 0.0;
		double sum_l = 0.0;
		double max_u = -INFINITY;
		double max_l = -INFINITY;
		
		size_t max_l_var = 0.0; 
		size_t max_u_var = 0.0;
		size_t min_width_var = 0.0;
		double min_width = INFINITY;
		for(x_shift = 0; x_shift < pool_size[0]; x_shift++){
			for(y_shift = 0; y_shift < pool_size[1]; y_shift++){
				size_t pool_cur_dim = inp_pos + x_shift*i12 + y_shift*input_size[2];
				//printf("pool_cur_dim %zu %zu %zu\n",pool_cur_dim,fp->layers[numlayers-1]->dims,numlayers);
				//fflush(stdout);
				pool_map[l] = pool_cur_dim;
				// use the ReLU bounds from the previous layer
				double lb = -fp->layers[numlayers-1]->neurons[pool_cur_dim]->lb;
				double ub = fp->layers[numlayers-1]->neurons[pool_cur_dim]->ub;
				if(ub<=0){
					inf[l] = 0.0;
					sup[l] = 0.0;
				}
				else if(lb>0){
					inf[l] = lb;
					sup[l] = ub;
				}
				else{
					inf[l] = 0;
					sup[l] = ub;
				}
				//printf("inf: %g %g\n",inf[l],sup[l]);
				//fflush(stdout);
				sum_u = sum_u + sup[l];
				sum_l = sum_l + inf[l];
				if(sup[l]>max_u){
					max_u = sup[l];
					max_u_var = pool_map[l];
				}
				if(inf[l] > max_l){
					max_l = inf[l];
					max_l_var = pool_map[l];
				}
				if((ub-lb) < min_width){
					min_width = ub - lb;
					min_width_var = pool_map[l];
				}
				l++;
			}
		}
		
		bool flag = false;
		size_t var = 0;
		for(j=0; j < p01; j++){
			bool is_greater = true;
			for(k = 0;  k < p01; k++){
				if(k==j)continue;
				if((inf[k]==sup[k]) && (inf[j]>=sup[k])){
					continue;
				}
				else if((inf[j]==inf[k]) && (sup[j]==sup[k]) && (inf[j]==sup[j])){
					continue;
				}
				else if(inf[j]<=sup[k]){
					is_greater = false;
					break;
				}
			}
			if(is_greater){
				flag = true;
				var =pool_map[j];
				break;
			}
		}
		//printf("max_l: %gmax_u: %g\n",max_l,max_u);
		//fflush(stdout);
		if(flag){
		//if(0){
			//x_new = x_var
			count++;
			//printf("out_pos: %zu\n",out_pos);
			//fflush(stdout);
			double coeff[1];
			size_t dim[1];
			coeff[0] = 1;
			dim[0] = var;
			out_neurons[out_pos]->lexpr = create_sparse_expr(coeff,0,dim,1);
			out_neurons[out_pos]->uexpr = create_sparse_expr(coeff,0,dim,1);
			//out_neurons[out_pos]->expr = create_sparse_expr(coeff,0,dim,1);
		}
		else{
			//max_l	<= x_new <= max_u
			double lcoeff[1];
			size_t ldim[1];
			lcoeff[0] = 1;
			ldim[0] = max_l_var;
			//lcoeff[0] = 0;
			//ldim[0] = 0;
			//printf("max_l: %gmax_u: %g\n",max_l,max_u);
			//fflush(stdout);
			//expr_t * expr = (expr_t *)malloc(sizeof(expr_t));
			//expr->inf_coeff= expr->sup_coeff = expr->dim = NULL;
			//expr->size = 0;
			//expr->inf_cst = -max_l;
			
			//out_neurons[out_pos]->lexpr = NULL;//create_sparse_expr(NULL,max_l,NULL,0);
			out_neurons[out_pos]->lexpr = create_sparse_expr(lcoeff,0,ldim,1);
			//double *ucoeff = (double *)malloc(p01*sizeof(double));
			//size_t *udim = (size_t *)malloc(p01*sizeof(size_t));
			//for(j=0; j < p01; j++){
			//	ucoeff[j] = 1.0;
			//	udim[j] = pool_map[j];
			//}
			double ucoeff[1];
			size_t udim[1];
			ucoeff[0] = 0;
			udim[0] = 0;
			out_neurons[out_pos]->uexpr = create_sparse_expr(ucoeff,max_u,udim,1);
			//out_neurons[out_pos]->uexpr = create_sparse_expr(ucoeff,max_l-sum_l,udim,p01);
			//sort_sparse_expr(out_neurons[out_pos]->uexpr);
			//free(ucoeff);
			//free(udim);
			//out_neurons[out_pos]->lexpr = create_cst_expr(-max_l,max_l);
			//out_neurons[out_pos]->uexpr = create_cst_expr(-max_u,max_u);
		}
		out_neurons[out_pos]->lb = -max_l;
		out_neurons[out_pos]->ub = max_u;		
	}
	//update_state_using_previous_layers_parallel(man,fp,numlayers);
        free(inf);
	free(sup);
	free(pool_map);
	free(output_size);
        //printf("count: %zu\n",count);
	//fflush(stdout);
	//printf("return here2\n");
	//fppoly_fprint(stdout,man,fp,NULL);
	//fflush(stdout);
	return num_out_neurons;
}

void free_neuron(neuron_t *neuron){
	if(neuron->expr){
		free_expr(neuron->expr);
	}
	if(neuron->lexpr){
		free_expr(neuron->lexpr);
	}
	if(neuron->uexpr){
		free_expr(neuron->uexpr);
	}
	free(neuron);
}

void free_non_lstm_layer_expr(elina_manager_t *man, elina_abstract0_t *abs, size_t layerno){
    fppoly_t *fp = fppoly_of_abstract0(abs);
    if(layerno >= fp->numlayers){
        fprintf(stdout,"the layer does not exist\n");
        return;
    }
    layer_t * layer = fp->layers[layerno];
    size_t dims = layer->dims;
    size_t i;
    for(i=0; i < dims; i++){
        neuron_t *neuron = layer->neurons[i];
        if(neuron->expr){
            free_expr(neuron->expr);
        }
        if(neuron->lexpr){
            free_expr(neuron->lexpr);
        }
        if(neuron->uexpr){
            free_expr(neuron->uexpr);
        }
    }
}

void layer_free(layer_t * layer){
	size_t dims = layer->dims;
	size_t i;
	for(i=0; i < dims; i++){
		free_neuron(layer->neurons[i]);
	}
	free(layer->neurons);
	layer->neurons = NULL;
	if(layer->h_t_inf!=NULL){
		free(layer->h_t_inf);
		layer->h_t_inf = NULL;
	}

	if(layer->h_t_sup!=NULL){
		free(layer->h_t_sup);
		layer->h_t_sup = NULL;
	}
	
	if(layer->c_t_inf!=NULL){
		free(layer->c_t_inf);
		layer->c_t_inf = NULL;
	}

	if(layer->c_t_sup!=NULL){
		free(layer->c_t_sup);
		layer->c_t_sup = NULL;
	}

	free(layer);
	layer = NULL;
}



void fppoly_free(elina_manager_t *man, fppoly_t *fp){
	size_t i;
	size_t output_size = fp->layers[fp->numlayers-1]->dims;
	for(i=0; i < fp->numlayers; i++){
		layer_free(fp->layers[i]);
	}

	if (fp->out != NULL) {
		for(i=0; i < output_size; i++){
			if(fp->out->lexpr[i]){
				free_expr(fp->out->lexpr[i]);
			}
			if(fp->out->uexpr[i]){
				free_expr(fp->out->uexpr[i]);
			}
		}
	}
	
	free(fp->layers);
	fp->layers = NULL;
	free(fp->input_inf);
	fp->input_inf = NULL;
        if(fp->input_lexpr!=NULL && fp->input_uexpr!=NULL){
		for(i=0; i < fp->num_pixels; i++){
			free(fp->input_lexpr[i]);
			free(fp->input_uexpr[i]);
		}
	
		free(fp->input_lexpr);
		fp->input_lexpr = NULL;
		free(fp->input_uexpr);
		fp->input_uexpr = NULL;
        }
	free(fp->input_sup);
	fp->input_sup = NULL;
	if (fp->out != NULL) {
	  free(fp->out->output_inf);
	  fp->out->output_inf = NULL;
	  free(fp->out->output_sup);
	  fp->out->output_sup = NULL;
	  free(fp->out->lexpr);
	  fp->out->lexpr = NULL;
	  free(fp->out->uexpr);
	  fp->out->uexpr = NULL;
	  free(fp->out);
	  fp->out=NULL;
	}
	free(fp);
	fp = NULL;
}


void neuron_fprint(FILE * stream, neuron_t *neuron, char ** name_of_dim){
	//expr_fprint(stream,neuron->expr);
	fprintf(stream,"[%g, %g]\n",-neuron->lb,neuron->ub);
}

void layer_fprint(FILE * stream, layer_t * layer, char** name_of_dim){
	size_t dims = layer->dims;
	size_t i;
	for(i = 0; i < dims; i++){
		fprintf(stream,"neuron: %zu ", i);
		neuron_fprint(stream, layer->neurons[i], name_of_dim);
	}
}

void fppoly_fprint(FILE* stream, elina_manager_t* man, fppoly_t* fp, char** name_of_dim){
	size_t i;
	for(i = 0; i < fp->numlayers; i++){
		fprintf(stream,"layer: %zu\n", i);
		layer_fprint(stream, fp->layers[i], name_of_dim);
	}
	size_t output_size = fp->layers[fp->numlayers-1]->dims;
	if(fp->out!=NULL){
		fprintf(stream,"OUTPUT bounds: \n");
		for(i=0; i < output_size;i++){
			fprintf(stream,"%zu: [%g,%g] \n",i,-fp->out->output_inf[i],fp->out->output_sup[i]);
		}
	}
}

elina_interval_t* box_for_lstm_neuron(elina_manager_t* man, elina_abstract0_t * abs, size_t layerno, size_t neuron_no) {
    fppoly_t *fp = fppoly_of_abstract0(abs);
	if(layerno >= fp->numlayers){
		fprintf(stdout,"the layer does not exist\n");
		return NULL;
	}
	layer_t * layer = fp->layers[layerno];
	size_t dims = layer->dims;
	if(neuron_no >= dims){
		fprintf(stdout,"the neuron does not exist\n");
		return NULL;
	}

	elina_interval_t* ret = elina_interval_alloc();
	elina_interval_set_double(ret, -layer->h_t_inf[neuron_no], layer->h_t_sup[neuron_no]);
	return ret;
}


elina_interval_t * box_for_neuron(elina_manager_t* man, elina_abstract0_t * abs, size_t layerno, size_t neuron_no){
    fppoly_t *fp = fppoly_of_abstract0(abs);
	if(layerno >= fp->numlayers){
		fprintf(stdout,"the layer does not exist\n");
		return NULL;
	}
	layer_t * layer = fp->layers[layerno];
	size_t dims = layer->dims;
	if(neuron_no >= dims){
		fprintf(stdout,"the neuron does not exist\n");
		return NULL;
	}
	neuron_t * neuron = layer->neurons[neuron_no];
	elina_interval_t * res = elina_interval_alloc();
	elina_interval_set_double(res,-neuron->lb,neuron->ub);
	return res;
}

elina_interval_t ** box_for_layer(elina_manager_t* man, elina_abstract0_t * abs, size_t layerno){
	fppoly_t *fp = fppoly_of_abstract0(abs);
	if(layerno >= fp->numlayers){
		fprintf(stdout,"the layer does not exist\n");
		return NULL;
	}
	layer_t * layer = fp->layers[layerno];
	size_t dims = layer->dims;
	elina_interval_t ** itv_arr = (elina_interval_t **)malloc(dims*sizeof(elina_interval_t *));
	size_t i;
	for(i=0; i< dims; i++){
		itv_arr[i] = box_for_neuron(man, abs, layerno, i);
		
	}
	return itv_arr;
}


size_t get_num_neurons_in_layer(elina_manager_t* man, elina_abstract0_t * abs, size_t layerno){
	fppoly_t *fp = fppoly_of_abstract0(abs);
	if(layerno >= fp->numlayers){
		fprintf(stdout,"the layer does not exist\n");
		return 0;
	}
	layer_t * layer = fp->layers[layerno];
	size_t dims = layer->dims;
	
	return dims;
}

elina_linexpr0_t * get_expr_for_output_neuron(elina_manager_t *man, elina_abstract0_t *abs, size_t i, bool is_lower){
	fppoly_internal_t *pr = fppoly_init_from_manager(man, ELINA_FUNID_ASSIGN_LINEXPR_ARRAY);
	fppoly_t *fp = fppoly_of_abstract0(abs);
	
	size_t output_size = fp->layers[fp->numlayers-1]->dims;
	if(i >= output_size){
		return NULL;
	}
	size_t num_pixels = fp->num_pixels;
	expr_t * expr = NULL;
	if(is_lower){
		expr = fp->out->lexpr[i];
	}
	else{
		expr = fp->out->uexpr[i];
	}
	elina_linexpr0_t * res = NULL;
	size_t j,k;
	if((fp->input_lexpr!=NULL) && (fp->input_uexpr!=NULL)){
		if(is_lower){
			expr =  replace_input_poly_cons_in_lexpr(pr, expr, fp);
		}
		else{
			expr =  replace_input_poly_cons_in_uexpr(pr, expr, fp);
		}
	}
	size_t expr_size = expr->size;
	if(expr->type==SPARSE){
		sort_sparse_expr(expr);
		res = elina_linexpr0_alloc(ELINA_LINEXPR_SPARSE,expr_size);
	}
	else{
		res = elina_linexpr0_alloc(ELINA_LINEXPR_DENSE,expr_size);
	}
	elina_linexpr0_set_cst_interval_double(res,-expr->inf_cst,expr->sup_cst);
	
	for(j=0;j < expr_size; j++){
		if(expr->type==DENSE){
			k = j;
		}
		else{
			k = expr->dim[j];
		}
		elina_linexpr0_set_coeff_interval_double(res,k,-expr->inf_coeff[j],expr->sup_coeff[j]);
	}
	if((fp->input_lexpr!=NULL) && (fp->input_uexpr!=NULL)){
		free_expr(expr);
	}
	return res;
}

elina_linexpr0_t * get_lexpr_for_output_neuron(elina_manager_t *man,elina_abstract0_t *abs, size_t i){
	return get_expr_for_output_neuron(man,abs,i, true);
}

elina_linexpr0_t * get_uexpr_for_output_neuron(elina_manager_t *man,elina_abstract0_t *abs, size_t i){
	return get_expr_for_output_neuron(man,abs,i, false);
}

void update_bounds_for_neuron(elina_manager_t *man, elina_abstract0_t *abs, size_t layerno, size_t neuron_no, double lb, double ub){
	fppoly_t *fp = fppoly_of_abstract0(abs);
	if(layerno >= fp->numlayers){
		fprintf(stdout,"the layer does not exist\n");
		return;
	}
	layer_t * layer = fp->layers[layerno];
	neuron_t * neuron = layer->neurons[neuron_no];
	neuron->lb = -lb;
	neuron->ub = ub;
}
