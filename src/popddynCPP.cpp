#include <RcppArmadillo.h>
//[[Rcpp::depends(RcppArmadillo)]]
using namespace Rcpp;

//' Population dynamics model for one annual time-step
//'
//' Project population forward one time-step given current numbers-at-age and total mortality
//'
//' @param nareas The number of spatial areas
//' @param maxage The maximum age 
//' @param SSBcurr A numeric vector of length nareas with the current spawning biomass in each area
//' @param Ncurr A numeric matrix (maxage, nareas) with current numbers-at-age in each area
//' @param Zcurr A numeric matrix (maxage, nareas) with total mortality-at-age in each area
//' @param PerrYr A numeric value with recruitment deviation for current year 
//' @param hs Steepness of SRR
//' @param R0a Numeric vector with unfished recruitment by area
//' @param SSBpR Numeric vector with unfished spawning stock per recruit by area 
//' @param aR Numeric vector with Ricker SRR a parameter by area
//' @param bR Numeric vector with Ricker SRR b parameter by area
//' @param mov Numeric matrix (nareas by nareas) with the movement matrix
//' @param SRrel Integer indicating the stock-recruitment relationship to use (1 for Beverton-Holt, 2 for Ricker)
//' 
//' @author A. Hordyk
//' 
//' @export
//' @keywords internal
//[[Rcpp::export]]
arma::mat popdynOneTScpp(double nareas, double maxage, Rcpp::NumericVector SSBcurr,
                         NumericMatrix Ncurr,  Rcpp::NumericMatrix Zcurr, double PerrYr,
                         double hs,  Rcpp::NumericVector R0a,  Rcpp::NumericVector SSBpR,
                         Rcpp::NumericVector aR,  Rcpp::NumericVector bR,  arma::cube mov,
                         double SRrel, int plusgroup=0) {
  
  arma::mat Nnext(maxage, nareas);
  // arma::mat tempMat2(nareas, nareas);	
  arma::mat Nstore(maxage, nareas); 

  // Recruitment assuming regional R0 and stock wide steepness
  for (int A=0; A < nareas; A++) {
    if (SRrel == 1) {
      // BH SRR
      Nnext(0, A) = PerrYr * (4*R0a(A) * hs * SSBcurr(A))/(SSBpR(A) * R0a(A) * (1-hs) + (5*hs-1) * SSBcurr(A));
    }	
    if (SRrel == 2) {
      // most transparent form of the Ricker uses alpha and beta params
      
      Nnext(0, A) = PerrYr * aR(A) * SSBcurr(A) * exp(-bR(A) * SSBcurr(A));
    }

    // Nnext(0, A) = Nnext(0, A) *exp(-Mage0);
    
    // Mortality
    for (int age=1; age<maxage; age++) {
      Nnext(age, A) = Ncurr(age-1, A) * exp(-Zcurr(age-1, A)); // Total mortality
    }
    if (plusgroup > 0) {
      Nnext(maxage-1, A) = Nnext(maxage-1, A)/ (1-exp(-Zcurr(maxage-1, A))); // Total mortality
    }
  }

  // Move stock
  for (int age=0; age<maxage; age++) {
  
    arma::mat tempMat2(nareas, nareas);
    for (int AA = 0; AA < nareas; AA++) {   // (from areas)
      for (int BB = 0; BB < nareas; BB++) { // (to areas)
        arma::vec temp = mov.subcube(age, AA, BB, age, AA, BB);
        double temp2 = temp(0);
        tempMat2(BB, AA) = Nnext(age, AA) * temp2; // movement fractions
      }
    }
    for (int BB = 0; BB < nareas; BB++) { // to areas
      Nstore(age, BB) = sum(tempMat2.row(BB));   // sums all rows (from areas) in each column (to areas)
    }
  }
  
  return Nstore;
} 



//' Population dynamics model in CPP
//'
//' Project population forward pyears given current numbers-at-age and total mortality, etc 
//' for the future years
//'
//' @param nareas The number of spatial areas
//' @param maxage The maximum age 
//' @param SSBcurr A numeric vector of length nareas with the current spawning biomass in each area
//' @param Ncurr A numeric matrix (maxage, nareas) with current numbers-at-age in each area
//' @param pyears The number of years to project the population forward
//' @param M_age Numeric matrix (maxage, pyears) with natural mortality by age and year
//' @param Asize_c Numeric vector (length nareas) with size of each area
//' @param MatAge Numeric vector with proportion mature by age
//' @param WtAge Numeric matrix (maxage, pyears) with weight by age and year
//' @param Vuln Numeric matrix (maxage, pyears) with vulnerability by age and year
//' @param Retc Numeric matrix (maxage, pyears) with retention by age and year
//' @param Prec Numeric vector (pyears) with recruitment error
//' @param movc Numeric array (nareas by nareas) with the movement matrix
//' @param SRrelc Integer indicating the stock-recruitment relationship to use (1 for Beverton-Holt, 2 for Ricker)
//' @param Effind Numeric vector (length pyears) with the fishing effort by year
//' @param Spat_targc Integer. Spatial targetting
//' @param hc Numeric. Steepness of stock-recruit relationship
//' @param R0c Numeric vector of length nareas with unfished recruitment by area
//' @param SSBpRc Numeric vector of length nareas with unfished spawning per recruit by area
//' @param aRc Numeric. Ricker SRR a value by area
//' @param bRc Numeric. Ricker SRR b value by area
//' @param Qc Numeric. Catchability coefficient
//' @param Fapic Numeric. Apical F value
//' @param maxF A numeric value specifying the maximum fishing mortality for any single age class
//' @param MPA Spatial closure by year and area
//' @param control Integer. 1 to use q and effort to calculate F, 2 to use Fapic (apical F) and 
//' vulnerablity to calculate F.
//' @param plusgroup Integer. Include a plus-group (1) or not (0)?
//' 
//' @author A. Hordyk
//' @export
//' @keywords internal
//[[Rcpp::export]]
List popdynCPP(double nareas, double maxage, arma::mat Ncurr, double pyears,
               arma::mat M_age, arma::vec Asize_c, arma::mat MatAge, arma::mat WtAge,
               arma::mat Vuln, arma::mat Retc, arma::vec Prec,
               List movc, double SRrelc, arma::vec Effind,
               double Spat_targc, double hc, NumericVector R0c, NumericVector SSBpRc,
               NumericVector aRc, NumericVector bRc, double Qc, double Fapic, double maxF, 
               arma::mat MPA, int control, double SSB0c, int plusgroup=0) {
  
  arma::cube Narray(maxage, pyears, nareas, arma::fill::zeros);
  arma::cube Barray(maxage, pyears, nareas, arma::fill::zeros);
  arma::cube SSNarray(maxage, pyears, nareas, arma::fill::zeros);
  arma::cube SBarray(maxage, pyears, nareas, arma::fill::zeros);
  arma::cube VBarray(maxage, pyears, nareas, arma::fill::zeros);
  arma::cube Marray(maxage, pyears, nareas, arma::fill::zeros);
  arma::cube FMarray(maxage, pyears, nareas, arma::fill::zeros);
  arma::cube FMretarray(maxage, pyears, nareas, arma::fill::zeros);
  arma::cube Zarray(maxage, pyears, nareas, arma::fill::zeros);
  
  NumericVector tempVec(nareas);
  arma::vec fishdist(nareas);
  
  // make clones of spawning/recruit by area because they are updated (sometimes)
  NumericVector R0c2(clone(R0c));
  NumericVector aRc2(clone(aRc));
  NumericVector bRc2(clone(bRc));
  
  NumericVector SSB0a(nareas);
  double R0 = sum(R0c);

  // Initial year
  Narray.subcube(0, 0, 0, maxage-1, 0, nareas-1) = Ncurr;
  

  int yr = 0;
  arma::cube movcy = movc(yr);
  
  for (int A=0; A<nareas; A++) {
    Barray.subcube(0, 0, A, maxage-1, 0, A) = Ncurr.col(A) % WtAge.col(0);
    SSNarray.subcube(0, 0, A, maxage-1, 0, A) = Ncurr.col(A) % MatAge.col(0); 
    SBarray.subcube(0, 0, A, maxage-1, 0, A) = Ncurr.col(A) % WtAge.col(0) % MatAge.col(0);
    VBarray.subcube(0, 0, A, maxage-1, 0, A) = Ncurr.col(A) % WtAge.col(0) % Vuln.col(0);
    Marray.subcube(0, 0, A, maxage-1, 0, A) = M_age.col(0);
    tempVec(A) = accu(VBarray.slice(A));
  }
  
  // fishdist = (pow(tempVec, Spat_targc))/mean((pow(tempVec, Spat_targc)));
  fishdist = (pow(tempVec, Spat_targc))/sum((pow(tempVec, Spat_targc)));

  // calculate F at age for first year
  if (control == 1) {
    for (int A=0; A<nareas; A++) {
      FMarray.subcube(0,0, A, maxage-1, 0, A) =  (Effind(0) * Qc * fishdist(A) * Vuln.col(0))/Asize_c(A);
      FMretarray.subcube(0,0, A, maxage-1, 0, A) =  (Effind(0) * Qc * fishdist(A) * Retc.col(0))/Asize_c(A);
    }
  }
  if (control == 2) {
    for (int A=0; A<nareas; A++) {
      FMarray.subcube(0,0, A, maxage-1, 0, A) =  (Fapic * fishdist(A) * Vuln.col(0))/Asize_c(A);
      FMretarray.subcube(0,0, A, maxage-1, 0, A) =  (Fapic * fishdist(A) * Retc.col(0))/Asize_c(A);
    }
  }
  
  // apply Fmax condition 
  // arma::uvec tempvals = arma::find(FMarray > (1-exp(-maxF)));
  // FMarray.elem(tempvals).fill(1-exp(-maxF));
  // arma::uvec tempvals2 = arma::find(FMretarray > (1-exp(-maxF)));
  // FMretarray.elem(tempvals2).fill(1-exp(-maxF));
  
  arma::uvec tempvals = arma::find(FMarray > maxF);
  FMarray.elem(tempvals).fill(maxF);
  arma::uvec tempvals2 = arma::find(FMretarray > maxF);
  FMretarray.elem(tempvals2).fill(maxF);
  
  Zarray.subcube(0,0, 0, maxage-1, 0, nareas-1) = Marray.subcube(0,0, 0, maxage-1, 0, nareas-1) + FMarray.subcube(0,0, 0, maxage-1, 0, nareas-1);

  for (int yr=0; yr<(pyears-1); yr++) {
    // Rcpp::Rcout << "yr = " << yr << std::endl;
    arma::vec SB(nareas);
    arma::cube movcy = movc(yr);
    
    for (int A=0; A<nareas; A++) SB(A) = accu(SBarray.subcube(0, yr, A, maxage-1, yr, A));
    if ((yr >0) & (control==3)) SB = SSB0a;
          
    arma::mat Ncurr2 = Narray.subcube(0, yr, 0, maxage-1, yr, nareas-1);
    arma::mat Zcurr = Zarray.subcube(0, yr, 0, maxage-1, yr, nareas-1);
    // double age0M = M_age(0,yr+1);
    
    arma::mat NextYrN = popdynOneTScpp(nareas, maxage, wrap(SB), wrap(Ncurr2), wrap(Zcurr), 
                                       Prec(yr+maxage), hc, R0c2, SSBpRc, 
                                       aRc2, bRc2, movcy, SRrelc,
                                       plusgroup); 
  
    Narray.subcube(0, yr+1, 0, maxage-1, yr+1, nareas-1) = NextYrN;
    for (int A=0; A<nareas; A++) {
      Barray.subcube(0, yr+1, A, maxage-1, yr+1, A) = NextYrN.col(A) % WtAge.col(yr+1);
      SSNarray.subcube(0, yr+1, A, maxage-1, yr+1, A) = NextYrN.col(A) % MatAge.col(yr+1);
      SBarray.subcube(0, yr+1, A, maxage-1, yr+1, A) = NextYrN.col(A) % WtAge.col(yr+1) % MatAge.col(yr+1);
      VBarray.subcube(0, yr+1, A, maxage-1, yr+1, A) = NextYrN.col(A) % WtAge.col(yr+1) % Vuln.col(yr+1);
      Marray.subcube(0, yr+1, A, maxage-1, yr+1, A) = M_age.col(yr+1);
      tempVec(A) = accu(VBarray.subcube(0, yr+1, A, maxage-1, yr+1, A));
    }
  
    // fishdist = (pow(tempVec, Spat_targc))/mean((pow(tempVec, Spat_targc)));
    fishdist = (pow(tempVec, Spat_targc))/sum((pow(tempVec, Spat_targc)));
   
    arma::vec d1(nareas);
    for (int A=0; A<nareas; A++) {
      d1(A) = MPA(yr,A) * fishdist(A);// historical closures
    }
    double fracE = sum(d1); // fraction of current effort in open areas
    arma::vec fracE2(nareas);
    for (int A=0; A<nareas; A++) {
      fracE2(A) = d1(A) * (fracE + (1-fracE))/fracE;
    }
    fishdist = fracE2;

    // calculate F at age for next year
    if (control == 1) {
      for (int A=0; A<nareas; A++) {
        FMarray.subcube(0,yr+1, A, maxage-1, yr+1, A) =  (Effind(yr+1) * Qc * fishdist(A) * Vuln.col(yr+1))/Asize_c(A);
        FMretarray.subcube(0,yr+1, A, maxage-1, yr+1, A) =  (Effind(yr+1) * Qc * fishdist(A) * Retc.col(yr+1))/Asize_c(A);
      }
    }
 
    if (control == 2) {
      for (int A=0; A<nareas; A++) {
        FMarray.subcube(0,yr+1, A, maxage-1, yr+1, A) =  (Fapic * fishdist(A) * Vuln.col(yr+1))/Asize_c(A);
        FMretarray.subcube(0,yr+1, A, maxage-1, yr+1, A) =  (Fapic * fishdist(A) * Retc.col(yr+1))/Asize_c(A);
      }
  
    }
    
    if (control ==3) { // simulate unfished dynamics & update recruitment by area
      Zarray.subcube(0,yr+1, 0, maxage-1, yr+1, nareas-1) = Marray.subcube(0,yr+1, 0, maxage-1, yr+1, nareas-1);
      
      // get spatial distribution 
      for (int A=0; A<nareas; A++) {
        SSB0a(A) = accu(SBarray.subcube(0, yr+1, A, maxage-1, yr+1, A));
        R0c2(A) = accu(SBarray.subcube(0, yr, A, maxage-1, yr, A));
      }
      
      SSB0a =   SSB0a/ (sum(SSB0a)/SSB0c); 
      R0c2 = R0c2/ (sum(R0c2)/R0);
      
      // recalculate recruitment parameters
      for (int A=0; A<nareas; A++) {
        bRc2(A) = log(5 * hc)/(0.8 * SSB0a(A));
        aRc2(A) = exp(bRc2(A) * SSB0a(A))/ SSBpRc(A);
      }

    } else {
      // apply Fmax condition 
      // arma::uvec tempvals3 = arma::find(FMarray > (1-exp(-maxF)));
      // FMarray.elem(tempvals3).fill(1-exp(-maxF));
      // arma::uvec tempvals4 = arma::find(FMretarray > (1-exp(-maxF)));
      // FMretarray.elem(tempvals4).fill(1-exp(-maxF));
      
      arma::uvec tempvals3 = arma::find(FMarray > maxF);
      FMarray.elem(tempvals3).fill(maxF);
      arma::uvec tempvals4 = arma::find(FMretarray > maxF);
      FMretarray.elem(tempvals4).fill(maxF);
      Zarray.subcube(0,yr+1, 0, maxage-1, yr+1, nareas-1) = Marray.subcube(0,yr+1, 0, maxage-1, yr+1, nareas-1) + FMarray.subcube(0,yr+1, 0, maxage-1, yr+1, nareas-1);
      
    }
   
  }
  
  List out(8);
  out(0) = Narray;
  out(1) = Barray;
  out(2) = SSNarray;
  out(3) = SBarray;
  out(4) = VBarray;
  out(5) = FMarray;
  out(6) = FMretarray;
  out(7) = Zarray;
  
  return out;
}

  

