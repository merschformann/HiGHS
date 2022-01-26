#include "Highs.h"
#include "catch.hpp"
#include "util/HFactor.h"

const bool dev_run = true;

HVector rhs;
HVector col_aq;
HVector row_ep;
std::vector<HighsInt> basic_set;
std::vector<double> solution;
HighsLp lp;
HighsInt num_col;
HighsInt num_row;
HighsInt basis_change;
HFactor factor;
HighsInt rowOut(const HighsInt variable_out);
bool iterate(const HighsInt variable_out, const HighsInt variable_in);
bool testSolve();

TEST_CASE("Factor-get-set-invert", "[highs_test_factor]") {
  std::string filename;
  std::string model;
  const bool avgas = false;
  if (avgas) {
    model = "avgas";
  } else {
    model = "adlittle";
  }
  filename = std::string(HIGHS_DIR) + "/check/instances/" + model + ".mps";

  Highs highs;
  if (!dev_run) highs.setOptionValue("output_flag", false);
  highs.readModel(filename);
  lp = highs.getLp();
  num_col = lp.num_col_;
  num_row = lp.num_row_;
  std::vector<HighsInt> variable_out;
  std::vector<HighsInt> variable_in;
  if (avgas) {
    variable_out = {16, 9, 15, 12, 8, 14};
    variable_in = {5, 2, 0, 4, 3, 6};
  } else {
    variable_out = {97,151,124,101,138,130,102,143,146,140,142,116,48,1,126,134,144,117,69,3,110,101,31,30,56,100,139,129,128,127,53,150,114,131,113,111,108,136,63,120,106,112,123,59,45,71,141,132,135,121,4,144,26,145,137,98,52,6,125,134,105,27,55,147,90,103,64,3,99,50,58,80,117,119};
    variable_in = {1,69,76,95,75,71,48,56,3,77,80,6,50,55,30,31,64,53,72,101,3,134,90,51,0,2,61,60,59,117,52,47,63,35,38,26,41,4,144,25,44,29,45,32,24,5,68,66,56,94,67,91,27,7,58,18,69,92,31,63,12,14,6,74,30,11,49,79,53,81,42,82,58,1};
  }
  HighsRandom random;
	    solution.resize(num_row);
	    for (HighsInt iRow=0; iRow<num_row; iRow++) {
    solution[iRow] = random.fraction();
    basic_set.push_back(num_col+iRow);
	    }
  rhs.setup(num_row);
  col_aq.setup(num_row);
  row_ep.setup(num_row);
  factor.setup(lp.a_matrix_, basic_set);
  factor.build();
  HighsInt from_basis_change = 0;
  HighsInt to_basis_change = avgas ? 3 : 65;
  for (basis_change = from_basis_change; basis_change < to_basis_change; basis_change++) {
    if (basis_change == 50) factor.build();
    REQUIRE(iterate(variable_out[basis_change], variable_in[basis_change]));
  }

  std::vector<HighsInt> get_basic_set = basic_set;
  InvertibleRepresentation invert = factor.getInvert();
  std::vector<InvertibleRepresentation> invert_set;
  from_basis_change = to_basis_change;
  to_basis_change = variable_out.size();
  for (basis_change = from_basis_change; basis_change < to_basis_change; basis_change++) {
    REQUIRE(iterate(variable_out[basis_change], variable_in[basis_change]));
    invert_set.push_back(factor.getInvert());
  }
  basis_change = -1;
  basic_set = get_basic_set;
  factor.setInvert(invert);
  REQUIRE(testSolve());

  for (basis_change = from_basis_change; basis_change < to_basis_change; basis_change++)
    REQUIRE(iterate(variable_out[basis_change], variable_in[basis_change]));
 
}

HighsInt rowOut(const HighsInt variable_out) {
  for (HighsInt iRow=0; iRow<num_row; iRow++) 
    if (basic_set[iRow] == variable_out) return iRow;
  return -1;
}

bool iterate(const HighsInt variable_out, const HighsInt variable_in) {
  const HighsInt row_out = rowOut(variable_out);
  if (row_out < 0) return false;
  row_ep.clear();
  row_ep.count = 1;
  row_ep.index[0] = row_out;
  row_ep.array[row_out] = 1;
  row_ep.packFlag = true;
  factor.btranCall(row_ep, 1);
    
    
  col_aq.clear();
  col_aq.packFlag = true;
  lp.a_matrix_.collectAj(col_aq, variable_in, 1);
  factor.ftranCall(col_aq, 1);
    
  basic_set[row_out] = variable_in;
  HighsInt rebuild_reason = 0;
  HighsInt lc_row_out = row_out;
  factor.update(&col_aq, &row_ep, &lc_row_out, &rebuild_reason);
  if (rebuild_reason) return false;
    
  return testSolve();
}

bool testSolve() {
  // FTRAN
  rhs.clear();
  for (HighsInt iCol=0; iCol<num_row; iCol++)
    lp.a_matrix_.collectAj(rhs, basic_set[iCol], solution[iCol]);
  factor.ftranCall(rhs, 1);
  double error_norm = 0;
  for (HighsInt iRow=0; iRow<num_row; iRow++) 
    error_norm = std::max(std::fabs(solution[iRow] - rhs.array[iRow]), error_norm);
  printf("FTRAN %2d: %g\n", (int)basis_change, error_norm);
  if (error_norm > 1e-4) return false;
  // BTRAN
  rhs.clear();
  for (HighsInt iCol=0; iCol<num_row; iCol++) {
    rhs.array[iCol] = lp.a_matrix_.computeDot(solution, basic_set[iCol]);
    if (rhs.array[iCol]) rhs.index[rhs.count++] = iCol;
  }
  factor.btranCall(rhs, 1);
  error_norm = 0;
  for (HighsInt iRow=0; iRow<num_row; iRow++) 
    error_norm = std::max(std::fabs(solution[iRow] - rhs.array[iRow]), error_norm);
  printf("BTRAN %2d: %g\n", (int)basis_change, error_norm);
  return error_norm < 1e-4;
}
