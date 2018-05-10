// Implementation of Dictionary Learning based on 
// Smooth Sparse Coding via Marginal Regression for Learning Sparse Representations (2012)
// with some augmentations to better allow for very high very large dictionaris

#pragma optimize("", on)
using namespace Eigen;

struct DLState {
	MatrixXf dictionary;
	SparseMatrix<float> code;
	SparseMatrix<float> weights; // this is gonna be symmetric, can we optimize that with eigen?
	int k, n, d;
	float inv_mat_norm;
	struct DicitonaryUpdateInfo {
		MatrixXf c;
		float kpn;
	} dict_info;
};

SparseMatrix<float> calc_weights(std::vector<Receiver> *recs) {
	// @temp test: set weights to be identity.

	std::vector<Triplet<float>> trips;
	for (int i = 0; i < recs->size(); i++) {
		Triplet<float> trip(i, i, 1);
		trips.push_back(trip);
	}

	SparseMatrix<float> ret(recs->size(), recs->size());
	ret.setFromTriplets(trips.begin(), trips.end());
	return ret;
}
float get_relative_err(DLState *state, SparseMatrix<float>*mat) {
	auto approx = (state->dictionary*state->code);
	float err = (approx - *mat).norm() * state->inv_mat_norm;
	return err;
}


struct CoeffInfo {
	int index;
	float value;
};

void find_n_largest_magnitude(int n, CoeffInfo* storage, VectorXf vec) {
	memset(storage, 0, sizeof(CoeffInfo)*n);

	for (int k = 0; k < vec.size(); k++) {
		CoeffInfo ci = { k, fabs((vec)(k)) };

		for (int i = 0; i < n; i++) {
			if (ci.value > storage[i].value) {
				memmove(&storage[i + 1], &storage[i], sizeof(CoeffInfo)*(n - (i + 1)));
				storage[i] = ci;
				break;
			}
		}
	}
}

void calc_dictionary_correlation_matrix(DLState *state, int num_coeffs) {
	auto Correlation = (state->dictionary* state->dictionary.transpose()).eval();
	CoeffInfo *most_correleted = (CoeffInfo *)calloc(num_coeffs, sizeof(CoeffInfo));

	auto dict = state->dictionary*state->dictionary.transpose();

	free(most_correleted);
}



void update_code(DLState *state, SparseMatrix<float> *mat, int sparsity) {
	// NOTE 1: 
	// 
	// While the paper choses the n smallest coeffs
	// such that the sum over the l1 norm (of the correlations) is smaller than some constant
	// we choose a constant number of coefficients
	// which makes the runtime calculations simpler

	// Note 2: 
	// 
	//  Marginal Regression only needs one matrix multiplication to decide which coefficients to chose
	//  (plus selecting biggest coeffs afterwards but that's cheap)
	//	However actually calculating this *one* matrix multiplication per iteration is 
	//  The bottle neck for very high dimensional data and quickly becomes unfeasable
	//  
	//  The traditional approach is to do online dictionary learning instead
	//  This considerably speeds things up because the mini-batch sized used is much smaller than 
	//  the full number of samples
	//	
	//  With dictionary learning you only need to compute the full correlation matrix once at the end of the 
	//  process to obtain the final sparse codes
	//  
	//  We instead chose different approach, we assume that the atoms to encode each sample does not
	//  change much between the iterations. As such we do not need to compute the full correlation matrix 
	//  but instead only the correlation for a subset of potential atoms.
	//
	//  We define the potential atoms as atoms used in the pervious iteration and 
	//  atoms which are sufficiently correlated with the atoms used in the previous iteration.
	//
	//  This allows us to *never* compute the full correlation matrix

	

	// gotta do the eval or eigen deferes the eval 
	// ie for each element we try to extract we do one mm mul lol
	
	auto DNT = state->dictionary.colwise().normalized().transpose().eval();
	VectorXf unnormalization = state->dictionary.colwise().norm();
	MatrixXf dict_corr = DNT * DNT.transpose();
	MatrixXf DTD = dict_corr.cwiseProduct(unnormalization*unnormalization.transpose());


	// storage space for when we calculate the n most correlated values in each iteration
	CoeffInfo *most_correleted = (CoeffInfo *)calloc(sparsity, sizeof(CoeffInfo));

	typedef Triplet<float> Trip;
	Trip* coeffs = (Trip *)malloc(sparsity * state->n * sizeof(Trip));


	SparseMatrix<float> atom_atom_correlation
		= SparseMatrix<float>(dict_corr.cols(), dict_corr.rows());

	const int spread = 4;
	atom_atom_correlation.reserve(VectorXi::Constant(dict_corr.rows(), spread));
	for (int i = 0; i < dict_corr.cols(); i++) {
		find_n_largest_magnitude(spread, most_correleted, dict_corr.col(i));

		for (int j = 0; j < spread; j++)
			if (most_correleted[j].value != 0.0)
				atom_atom_correlation.insert(most_correleted[j].index, i) = 1.0;
	}

	for (int i = 0; i < state->n; i++) {

		// candidates are previously used atoms + 
		// atoms that are sufficiently correlated with previously used atoms

		SparseVector<float> previous_sparse_code = state->code.col(i); // mul with weight matrix as well
		SparseVector<float> expanded_sparse_code = atom_atom_correlation * previous_sparse_code;
		VectorXi candidate_atoms =
			Map<VectorXi>(expanded_sparse_code.innerIndexPtr(), expanded_sparse_code.nonZeros());

		VectorXf candidate_correlation = DNT(candidate_atoms, placeholders::all) * mat->col(i);

		find_n_largest_magnitude(sparsity, most_correleted, candidate_correlation);

		VectorXi used_atom_candidate_index = Map<VectorXi, 0, InnerStride<>>
			(&most_correleted[0].index, sparsity, InnerStride<>(2));

		VectorXi used_atoms = candidate_atoms(used_atom_candidate_index);

		// solve the least squares problem using only the selected atoms
		// DTM is equivalent to Dictionary.transpose * mat.col(i) + extraction of used atoms
		// but we reuse some previous computations
		// note that we only use a few atoms so this is quite fast ie. ldlt on a matrix of [sparsity x sparsity]
		auto DTM = candidate_correlation(used_atom_candidate_index).cwiseProduct(unnormalization(used_atoms));
		auto sparse_code = DTD(used_atoms, used_atoms).ldlt().solve(DTM).eval();

		for (int j = 0; j < sparsity; j++) {
			coeffs[i*sparsity + j] = Trip(used_atoms(j), i, sparse_code[j]);
		}
	}
	state->code.setFromTriplets(coeffs, &coeffs[sparsity*state->n]);
	free(most_correleted);
	free(coeffs);
}

void set_dict_info(DLState *state, float k, float n) {
	state->dict_info.kpn = k + n;
	state->dict_info.c = MatrixXf(state->k, state->k);
	state->dict_info.c.setIdentity();
	state->dict_info.c.array() *= n;
	state->dict_info.c.array() += k;
}

void update_dictionary(DLState *state, SparseMatrix<float> *mat) {
	auto D = state->dictionary;
	auto B = state->code;
	if (state->dict_info.kpn != 0) {
		// @Note: we're not using the sparse solvers here
		//        the matrices should be pretty dense multiplication 
		MatrixXf temp_1 = ((B * B.transpose()).toDense() + (D.transpose()*D).cwiseProduct(state->dict_info.c));
		MatrixXf temp_2 = (*mat*B.transpose() + state->dict_info.kpn * D).transpose().toDense();
		state->dictionary = temp_1.ldlt().solve(temp_2).transpose();
	} else {
		// if k + n is 0 this is just normal MOD aka linear least squares fitting
		MatrixXf temp_1 = (B * B.transpose()).toDense();
		MatrixXf temp_2 = (*mat*B.transpose()).transpose().toDense();
		state->dictionary = temp_1.ldlt().solve(temp_2).transpose();
	}
	
	// if any atoms is zero (ie from not being used)
	// we just set it to the next atom + a small pertubation
	for (int i = 0; i < state->dictionary.cols(); i++) {
		if (state->dictionary.col(i).isZero()) {
			auto next_atom = state->dictionary.col((i + 1) % state->k);
			state->dictionary.col(i).setRandom();
			state->dictionary.col(i) *= next_atom.norm() / state->dictionary.col(i).norm() * 0.1;
			state->dictionary.col(i) += next_atom;
		}
	}
}
#include <random>
void init_code(DLState *state, int non_zeros) {
	state->code = SparseMatrix<float>(state->k, state->n);

	// we know how many non_zero elements there are in each col so no need to set with triplets
	state->code.reserve(VectorXi::Constant(state->n, non_zeros));
	// initially we select a few random atoms.
	// notise that we do not neccesarily need num_zeros to match with later iterations
	// in fact we probably want it to be larger since in later iterations it will not be random 

	int *col_indices = (int *)malloc(sizeof(int)*state->k);
	for (int i = 0; i < state->k; i++) col_indices[i] = i;

	for (int i = 0; i < state->n; i++) {
		std::random_shuffle(col_indices, &col_indices[state->k]);
		for (int j = 0; j < non_zeros; j++)
			state->code.insert(col_indices[j], i) = 1.0;
	}

	free(col_indices);
}

DLState smooth_dictionary_learning(SparseMatrix<float>*mat, std::vector<Receiver> *receivers, float radius, int num_atoms) {
	int k = num_atoms;
	int n = mat->cols();
	int d = mat->rows();

	DLState state;
	state.k = k;
	state.n = n;
	state.d = d;
	state.dictionary = MatrixXf(d, k);
	init_code(&state, 8);
	state.dictionary.setRandom();
	state.weights = calc_weights(receivers);
	state.inv_mat_norm = 1.0 / mat->norm();

	set_dict_info(&state, 0, 0);
	float rel = -1;
	int its = 0;
	float target_err = 0.01;
	//update_dictionary(&state, mat);
	printf("initialization done!\n");
	do {
		update_code(&state, mat, 8);
		rel = get_relative_err(&state, mat);
		printf("err: %d: %.6f\n", its, rel);
		update_dictionary(&state, mat);
		rel = get_relative_err(&state, mat);
		printf("err: %d: %.6f\n", its, rel);
		++its;
	} while ((rel > target_err && its < 100) || its < 10);

	// final update without ortohonality + normalization penalization
	// the penalty improves the code update step but we're not doing any more code updating
	set_dict_info(&state, 0, 0);
	update_dictionary(&state, mat);
	rel = get_relative_err(&state, mat);
	printf(".......................\n");
	printf("final error: %d: %.6f\n", its, rel);
	return state;
}

#pragma optimize("", on)
