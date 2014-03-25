/*
 * interval_arithmetic.c
 *
 *  Created on: Mar 24, 2014
 *      Author: dejan
 */

#include "interval/arithmetic.h"

int interval_endpoint_lt(const rational_t* a, int a_open, const rational_t* b, int b_open) {
  int cmp = rational_cmp(a, b);
  if (cmp == 0) {
    return (!a_open && b_open);
  } else {
    return cmp < 0;
  }
}

int dyadic_interval_endpoint_lt(const dyadic_rational_t* a, int a_open, const dyadic_rational_t*b, int b_open) {
  int cmp = dyadic_rational_cmp(a, b);
  if (cmp == 0) {
    return (!a_open && b_open);
  } else {
    return cmp < 0;
  }
}

void interval_add(interval_t* S, const interval_t* I1, const interval_t* I2) {

  if (I1->is_point && I2->is_point) {
    if (!S->is_point) {
      rational_destruct(&S->b);
    }
    rational_add(&S->a, &I1->a, &I2->a);
    S->b_open = S->a_open = 0;
    S->is_point = 1;
    return;
  }

  if (I2->is_point) {
    // Reuse symmetry
    interval_add(S, I2, I1);
    return;
  }

  if (I1->is_point) {
    // Just shift by I1->a
    interval_assign(S, I2);
    rational_add(&S->a, &S->a, &I1->a);
    rational_add(&S->b, &S->b, &I1->a);
    return;
  }

  // [a, b] + [c, d] = [a + c, b + d]
  interval_t result;
  rational_construct(&result.a);
  rational_construct(&result.b);
  rational_add(&result.a, &I1->a, &I2->a);
  rational_add(&result.b, &I1->b, &I2->b);
  result.a_open = I1->a_open || I2->a_open;
  result.b_open = I1->b_open || I2->b_open;
  result.is_point = 0;
  interval_swap(&result, S);
  interval_destruct(&result);
}

void dyadic_interval_add(dyadic_interval_t* S, const dyadic_interval_t* I1, const dyadic_interval_t* I2) {

  if (I1->is_point && I2->is_point) {
    if (!S->is_point) {
      dyadic_rational_destruct(&S->b);
    }
    dyadic_rational_add(&S->a, &I1->a, &I2->a);
    S->b_open = S->a_open = 0;
    S->is_point = 1;
    return;
  }

  if (I2->is_point) {
    // Reuse symmetry
    dyadic_interval_add(S, I2, I1);
    return;
  }

  if (I1->is_point) {
    // Just shift by I1->a
    dyadic_interval_assign(S, I2);
    dyadic_rational_add(&S->a, &S->a, &I1->a);
    dyadic_rational_add(&S->b, &S->b, &I1->a);
    return;
  }

  // Both non-points
  // [a, b] + [c, d] = [a + c, b + d]
  dyadic_interval_t result;
  dyadic_rational_construct(&result.a);
  dyadic_rational_construct(&result.b);
  dyadic_rational_add(&result.a, &I1->a, &I2->a);
  dyadic_rational_add(&result.b, &I1->b, &I2->b);
  result.a_open = I1->a_open || I2->a_open;
  result.b_open = I1->b_open || I2->b_open;
  result.is_point = 0;
  dyadic_interval_swap(&result, S);
  dyadic_interval_destruct(&result);
}


void interval_mul(interval_t* P, const interval_t* I1, const interval_t* I2) {
  if (I1->is_point) {
    if (I2->is_point) {
      // Just multiply the points
      rational_mul(&P->a, &I1->a, &I2->a);
      if (!P->is_point) {
        rational_destruct(&P->b);
        P->is_point = 1;
      }
      P->a_open = P->b_open = 0;
    } else {
      // Depending on the sign of a, we might have to flip
      int a_sgn = rational_sgn(&I1->a);
      if (a_sgn == 0) {
        // It's just 0
        if (!P->is_point) {
          rational_destruct(&P->b);
          P->is_point = 1;
        }
        P->a_open = P->b_open = 0;
        rational_assign_int(&P->a, 0, 1);
      } else if (a_sgn > 0) {
        // Regular multiplication
        if (P->is_point) {
          rational_construct(&P->b);
          P->is_point = 0;
        }
        P->a_open = I2->a_open;
        P->b_open = I2->b_open;
        rational_mul(&P->a, &I1->a, &I2->a);
        rational_mul(&P->b, &I1->a, &I2->b);
      } else {
        // Multiplying with a negative, flip the edges
        if (P->is_point) {
          rational_construct(&P->b);
          P->is_point = 0;
        }
        P->a_open = I2->b_open;
        P->b_open = I2->a_open;
        rational_mul(&P->a, &I1->a, &I2->b);
        rational_mul(&P->b, &I1->a, &I2->a);
      }
    }
  } else if (I2->is_point) {
    interval_mul(P, I2, I1);
  } else {
    if (P->is_point) {
      rational_construct(&P->b);
      P->is_point = 0;
    }

    //
    // I1 x I2 = { x*y | x in I1, y in I2 }
    //         = { x*y | I1.a < x < I1.b, I2.a < y < I2.b }
    //         = { x*y |

    interval_t result;
    interval_construct_zero(&result);

    rational_t tmp;
    rational_construct(&tmp);

    // I1.a x I2.a
    rational_mul(&result.a, &I1->a, &I2->a);
    rational_construct_copy(&result.b, &result.a);
    result.a_open = result.b_open = I1->a_open || I2->a_open;
    result.is_point = 0;

    // I1.a x I2.b
    int tmp_open = I1->a_open || I2->b_open;
    rational_mul(&tmp, &I1->a, &I2->b);
    if (interval_endpoint_lt(&tmp, tmp_open, &result.a, result.a_open)) {
      rational_swap(&tmp, &result.a);
      result.a_open = tmp_open;
    } else if (interval_endpoint_lt(&result.b, result.b_open, &tmp, tmp_open)) {
      rational_swap(&tmp, &result.b);
      result.b_open = tmp_open;
    }

    // I1.b x I2.a
    tmp_open = I1->b_open || I2->a_open;
    rational_mul(&tmp, &I1->b, &I2->a);
    if (interval_endpoint_lt(&tmp, tmp_open, &result.a, result.a_open)) {
      rational_swap(&tmp, &result.a);
      result.a_open = tmp_open;
    } else if (interval_endpoint_lt(&result.b, result.b_open, &tmp, tmp_open)) {
      rational_swap(&tmp, &result.b);
      result.b_open = tmp_open;
    }

    // I1.b x I2.b
    tmp_open = I1->b_open || I2->b_open;
    rational_mul(&tmp, &I1->b, &I2->b);
    if (interval_endpoint_lt(&tmp, tmp_open, &result.a, result.a_open)) {
      rational_swap(&tmp, &result.a);
      result.a_open = tmp_open;
    } else if (interval_endpoint_lt(&result.b, result.b_open, &tmp, tmp_open)) {
      rational_swap(&tmp, &result.b);
      result.b_open = tmp_open;
    }

    interval_swap(&result, P);
    interval_destruct(&result);
    rational_destruct(&tmp);
  }
}

void dyadic_interval_mul(dyadic_interval_t* P, const dyadic_interval_t* I1, const dyadic_interval_t* I2) {
  if (I1->is_point) {
    if (I2->is_point) {
      // Just multiply the points
      dyadic_rational_mul(&P->a, &I1->a, &I2->a);
      if (!P->is_point) {
        dyadic_rational_destruct(&P->b);
        P->is_point = 1;
      }
      P->a_open = P->b_open = 0;
    } else {
      // Depending on the sign of a, we might have to flip
      int a_sgn = dyadic_rational_sgn(&I1->a);
      if (a_sgn == 0) {
        // It's just 0
        if (!P->is_point) {
          dyadic_rational_destruct(&P->b);
          P->is_point = 1;
        }
        P->a_open = P->b_open = 0;
        dyadic_rational_assign_int(&P->a, 0, 1);
      } else if (a_sgn > 0) {
        // Regular multiplication
        if (P->is_point) {
          dyadic_rational_construct(&P->b);
          P->is_point = 0;
        }
        P->a_open = I2->a_open;
        P->b_open = I2->b_open;
        dyadic_rational_mul(&P->a, &I1->a, &I2->a);
        dyadic_rational_mul(&P->b, &I1->a, &I2->b);
      } else {
        // Multiplying with a negative, flip the edges
        if (P->is_point) {
          dyadic_rational_construct(&P->b);
          P->is_point = 0;
        }
        P->a_open = I2->b_open;
        P->b_open = I2->a_open;
        dyadic_rational_mul(&P->a, &I1->a, &I2->b);
        dyadic_rational_mul(&P->b, &I1->a, &I2->a);
      }
    }
  } else if (I2->is_point) {
    dyadic_interval_mul(P, I2, I1);
  } else {
    if (P->is_point) {
      dyadic_rational_construct(&P->b);
      P->is_point = 0;
    }

    //
    // I1 x I2 = { x*y | x in I1, y in I2 }
    //         = { x*y | I1.a < x < I1.b, I2.a < y < I2.b }
    //         = { x*y |

    dyadic_interval_t result;
    dyadic_interval_construct_zero(&result);

    dyadic_rational_t tmp;
    dyadic_rational_construct(&tmp);

    // I1.a x I2.a
    dyadic_rational_mul(&result.a, &I1->a, &I2->a);
    dyadic_rational_construct_copy(&result.b, &result.a); // was not constructed (is 0)
    result.a_open = result.b_open = I1->a_open || I2->a_open;
    result.is_point = 0;

    // I1.a x I2.b
    int tmp_open = I1->a_open || I2->b_open;
    dyadic_rational_mul(&tmp, &I1->a, &I2->b);
    if (dyadic_interval_endpoint_lt(&tmp, tmp_open, &result.a, result.a_open)) {
      dyadic_rational_swap(&tmp, &result.a);
      result.a_open = tmp_open;
    } else if (dyadic_interval_endpoint_lt(&result.b, result.b_open, &tmp, tmp_open)) {
      dyadic_rational_swap(&tmp, &result.b);
      result.b_open = tmp_open;
    }

    // I1.b x I2.a
    tmp_open = I1->b_open || I2->a_open;
    dyadic_rational_mul(&tmp, &I1->b, &I2->a);
    if (dyadic_interval_endpoint_lt(&tmp, tmp_open, &result.a, result.a_open)) {
      dyadic_rational_swap(&tmp, &result.a);
      result.a_open = tmp_open;
    } else if (dyadic_interval_endpoint_lt(&result.b, result.b_open, &tmp, tmp_open)) {
      dyadic_rational_swap(&tmp, &result.b);
      result.b_open = tmp_open;
    }

    // I1.b x I2.b
    tmp_open = I1->b_open || I2->b_open;
    dyadic_rational_mul(&tmp, &I1->b, &I2->b);
    if (dyadic_interval_endpoint_lt(&tmp, tmp_open, &result.a, result.a_open)) {
      dyadic_rational_swap(&tmp, &result.a);
      result.a_open = tmp_open;
    } else if (dyadic_interval_endpoint_lt(&result.b, result.b_open, &tmp, tmp_open)) {
      dyadic_rational_swap(&tmp, &result.b);
      result.b_open = tmp_open;
    }

    dyadic_interval_swap(&result, P);
    dyadic_interval_destruct(&result);
    dyadic_rational_destruct(&tmp);
  }
}

void interval_pow(interval_t* P, const interval_t* I, unsigned n) {
  if (n == 0) {
    // I^0 = [1]
    if (!P->is_point) {
      P->is_point = 1;
      rational_destruct(&P->b);
    }
    rational_assign_int(&P->a, 1, 1);
    P->a_open = 0;
    P->b_open = 0;
  } else if (I->is_point) {
    // Plain power
    if (!P->is_point) {
      rational_destruct(&P->b);
      P->is_point = 1;
      P->a_open = P->b_open = 0;
    }
    rational_pow(&P->a, &I->a, n);
  } else {
    if (P->is_point) {
      P->is_point = 0;
      rational_construct(&P->b);
    }
    if (n % 2) {
      // For odd powers we are monotonic, i.e. [a, b]^n = [a^n, b^n]
      P->a_open = I->a_open;
      P->b_open = I->b_open;
      rational_pow(&P->a, &I->a, n);
      rational_pow(&P->b, &I->b, n);
    } else {
      // Even powers depend on whether 0 is in the interval
      int sgn = interval_sgn(I);
      rational_pow(&P->a, &I->a, n);
      rational_pow(&P->b, &I->b, n);
      if (sgn == 0) {
        // P = [0, max(a, b)^n]
        if (interval_endpoint_lt(&P->b, I->b_open, &P->a, I->a_open)) {
          rational_swap(&P->b, &P->a);
          P->b_open = I->a_open;
        }
        rational_assign_int(&P->a, 0, 1);
        P->a_open = 0;
      } else if (sgn > 0) {
        // P = I^n
        P->a_open = I->a_open;
        P->b_open = I->b_open;
      } else {
        // negative turns positive, so we flip
        rational_swap(&P->a, &P->b);
        P->a_open = I->b_open;
        P->b_open = I->a_open;
      }
    }
  }
}


