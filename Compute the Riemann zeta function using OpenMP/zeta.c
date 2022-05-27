// http://dl.dropbox.com/u/10988984/code/RiemannSiegel.java.html

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327
#endif

double twopi = 2.0 * M_PI;

int even (int n) {
    if (n%2 == 0)
	return 1;
    else
	return -1;
}

/**
 * Riemann-Siegel theta function
 * theta(t) approximation using Stirling series
 */
double theta (double t) {
    return (t/2.0 * log(t/2.0/M_PI) - t/2.0 - M_PI/8.0 + 1.0/48.0/t + 7.0/5760.0/t/t/t);
}

/**
 * C terms for Riemann-Siegel coefficients of remainder terms
 */
double C (int n, double z) {
    if (n==0)
	return(.38268343236508977173 * pow(z, 0.0) 
	       +.43724046807752044936 * pow(z, 2.0) 
	       +.13237657548034352332 * pow(z, 4.0) 
	       -.01360502604767418865 * pow(z, 6.0)
	       -.01356762197010358089 * pow(z, 8.0) 
	       -.00162372532314446528 * pow(z,10.0) 
	       +.00029705353733379691 * pow(z,12.0) 
	       +.00007943300879521470 * pow(z,14.0) 
	       +.00000046556124614505 * pow(z,16.0) 
	       -.00000143272516309551 * pow(z,18.0) 
	       -.00000010354847112313 * pow(z,20.0) 
	       +.00000001235792708386 * pow(z,22.0) 
	       +.00000000178810838580 * pow(z,24.0) 
	       -.00000000003391414390 * pow(z,26.0) 
	       -.00000000001632663390 * pow(z,28.0) 
	       -.00000000000037851093 * pow(z,30.0) 
	       +.00000000000009327423 * pow(z,32.0) 
	       +.00000000000000522184 * pow(z,34.0) 
	       -.00000000000000033507 * pow(z,36.0) 
	       -.00000000000000003412 * pow(z,38.0)
	       +.00000000000000000058 * pow(z,40.0) 
	       +.00000000000000000015 * pow(z,42.0)); 
    else if (n==1) 
	return(-.02682510262837534703 * pow(z, 1.0) 
	       +.01378477342635185305 * pow(z, 3.0) 
	       +.03849125048223508223 * pow(z, 5.0) 
	       +.00987106629906207647 * pow(z, 7.0)
	       -.00331075976085840433 * pow(z, 9.0) 
	       -.00146478085779541508 * pow(z,11.0) 
	       -.00001320794062487696 * pow(z,13.0) 
	       +.00005922748701847141 * pow(z,15.0) 
	       +.00000598024258537345 * pow(z,17.0) 
	       -.00000096413224561698 * pow(z,19.0) 
	       -.00000018334733722714 * pow(z,21.0) 
	       +.00000000446708756272 * pow(z,23.0) 
	       +.00000000270963508218 * pow(z,25.0) 
	       +.00000000007785288654 * pow(z,27.0)
	       -.00000000002343762601 * pow(z,29.0) 
	       -.00000000000158301728 * pow(z,31.0) 
	       +.00000000000012119942 * pow(z,33.0) 
	       +.00000000000001458378 * pow(z,35.0) 
	       -.00000000000000028786 * pow(z,37.0) 
	       -.00000000000000008663 * pow(z,39.0) 
	       -.00000000000000000084 * pow(z,41.0) 
	       +.00000000000000000036 * pow(z,43.0) 
	       +.00000000000000000001 * pow(z,45.0)); 
    else if (n==2) 
	return(+.00518854283029316849 * pow(z, 0.0) 
	       +.00030946583880634746 * pow(z, 2.0) 
	       -.01133594107822937338 * pow(z, 4.0) 
	       +.00223304574195814477 * pow(z, 6.0)
	       +.00519663740886233021 * pow(z, 8.0) 
	       +.00034399144076208337 * pow(z,10.0) 
	       -.00059106484274705828 * pow(z,12.0) 
	       -.00010229972547935857 * pow(z,14.0) 
	       +.00002088839221699276 * pow(z,16.0) 
	       +.00000592766549309654 * pow(z,18.0) 
	       -.00000016423838362436 * pow(z,20.0) 
	       -.00000015161199700941 * pow(z,22.0) 
	       -.00000000590780369821 * pow(z,24.0) 
	       +.00000000209115148595 * pow(z,26.0) 
	       +.00000000017815649583 * pow(z,28.0) 
	       -.00000000001616407246 * pow(z,30.0) 
	       -.00000000000238069625 * pow(z,32.0) 
	       +.00000000000005398265 * pow(z,34.0) 
	       +.00000000000001975014 * pow(z,36.0) 
	       +.00000000000000023333 * pow(z,38.0) 
	       -.00000000000000011188 * pow(z,40.0) 
	       -.00000000000000000416 * pow(z,42.0) 
	       +.00000000000000000044 * pow(z,44.0) 
	       +.00000000000000000003 * pow(z,46.0)); 
    else if (n==3) 
	return(-.00133971609071945690 * pow(z, 1.0) 
	       +.00374421513637939370 * pow(z, 3.0) 
	       -.00133031789193214681 * pow(z, 5.0) 
	       -.00226546607654717871 * pow(z, 7.0)
	       +.00095484999985067304 * pow(z, 9.0) 
	       +.00060100384589636039 * pow(z,11.0) 
	       -.00010128858286776622 * pow(z,13.0) 
	       -.00006865733449299826 * pow(z,15.0) 
	       +.00000059853667915386 * pow(z,17.0) 
	       +.00000333165985123995 * pow(z,19.0)
	       +.00000021919289102435 * pow(z,21.0) 
	       -.00000007890884245681 * pow(z,23.0) 
	       -.00000000941468508130 * pow(z,25.0) 
	       +.00000000095701162109 * pow(z,27.0) 
	       +.00000000018763137453 * pow(z,29.0) 
	       -.00000000000443783768 * pow(z,31.0) 
	       -.00000000000224267385 * pow(z,33.0) 
	       -.00000000000003627687 * pow(z,35.0) 
	       +.00000000000001763981 * pow(z,37.0) 
	       +.00000000000000079608 * pow(z,39.0) 
	       -.00000000000000009420 * pow(z,41.0) 
	       -.00000000000000000713 * pow(z,43.0) 
	       +.00000000000000000033 * pow(z,45.0) 
	       +.00000000000000000004 * pow(z,47.0)); 
    else 
	return(+.00046483389361763382 * pow(z, 0.0) 
	       -.00100566073653404708 * pow(z, 2.0) 
	       +.00024044856573725793 * pow(z, 4.0) 
	       +.00102830861497023219 * pow(z, 6.0)
	       -.00076578610717556442 * pow(z, 8.0) 
	       -.00020365286803084818 * pow(z,10.0) 
	       +.00023212290491068728 * pow(z,12.0) 
	       +.00003260214424386520 * pow(z,14.0) 
	       -.00002557906251794953 * pow(z,16.0) 
	       -.00000410746443891574 * pow(z,18.0) 
	       +.00000117811136403713 * pow(z,20.0) 
	       +.00000024456561422485 * pow(z,22.0) 
	       -.00000002391582476734 * pow(z,24.0) 
	       -.00000000750521420704 * pow(z,26.0) 
	       +.00000000013312279416 * pow(z,28.0) 
	       +.00000000013440626754 * pow(z,30.0) 
	       +.00000000000351377004 * pow(z,32.0) 
	       -.00000000000151915445 * pow(z,34.0) 
	       -.00000000000008915418 * pow(z,36.0) 
	       +.00000000000001119589 * pow(z,38.0) 
	       +.00000000000000105160 * pow(z,40.0) 
	       -.00000000000000005179 * pow(z,42.0) 
	       -.00000000000000000807 * pow(z,44.0) 
	       +.00000000000000000011 * pow(z,46.0) 
	       +.00000000000000000004 * pow(z,48.0));
}

/**
 * Riemann-Siegel Z(t) function with additional parameter
 * for number of R terms to add
 */
double Z (double t, int r) {    
    // t/2pi^1/2
    double t2 = sqrt(t/twopi);
    double N = fabs(t2);
    double p = t2 - N;
    int k = 0;
    double R = 0.0;
    double tt = theta(t);
    
    double sum = 0.0;
    for (int n = 1; n <= N; n++) {
      sum += (pow(n, -0.5) * cos(tt - (t * log(n))));
    }
    sum = 2.0 * sum;
    // add remainder R here
    double piot = M_PI/t;
    while (k <= r) { 
      R = R + C(k,2.0*p-1.0) * pow(2.0 * piot, ((double) k)*0.5); 
      ++k;
    } 
    R = even((int) N-1) * pow(2.0 * M_PI / t,0.25) * R;
    
    return sum + R;
}

int main(int argc, char** argv) {
    int r = 23000;
    int N = 370;
    double d = 105.0;

    {
        int c;
        while ((c = getopt (argc, argv, "r:N:d:")) != -1) {
            switch (c) {
            case 'r':
                r = atoi(optarg);
                break;
            case 'N':
                N = atoi(optarg);
                break;
            case 'd':
                d = strtod(optarg, NULL);
                break;
            default:
                return EXIT_FAILURE;
            }
        }
    }

    double * result = (double*) malloc(N*sizeof(double));
    for (int i = 0; i < N; d += 0.5, ++i) {
	result[i] = Z(d, r);
    }

    double sum = 0.0;
    for (int i = 0; i < N; i++) {
        sum += result[i];
    }

    printf("sum is %lf\n", sum);

    free (result);
    return EXIT_SUCCESS;

#if 0
    // test values - Haselgrove Table II Z(t)
    printf("Z(t) for %lf=%lf\n", 102.5, Z(102.5,r));
    printf("Z(t) for %lf=%lf\n", 108.5, Z(108.5,r));
    printf("Z(t) for %lf=%lf\n", 114.5, Z(114.5,r));
    printf("Z(t) for %lf=%lf\n", 124.5, Z(124.5,r));
    printf("Z(t) for %lf=%lf\n", 131.0, Z(131.0,r));

    printf("Z(t) for %lf=%lf\n", 14.134725142, Z(14.134725142,r));
    printf("Z(t) for %lf=%lf\n", 21.022039639, Z(21.022039639,r));
    printf("Z(t) for %lf=%lf\n", 25.010857580, Z(25.010857580,r));
    printf("Z(t) for %lf=%lf\n", 30.424876126, Z(30.424876126,r));
    printf("Z(t) for %lf=%lf\n", 32.935061588, Z(32.935061588,r));
#endif
}
