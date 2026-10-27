/* Reed-Solomon decoder
 * Copyright 2002 Phil Karn, KA9Q
 * May be used under the terms of the GNU Lesser General Public License (LGPL)
 *
 * This file is intended to be #included inside a function body.
 * The enclosing function must provide:
 *   data_t *data        - the RS block (modified in place)
 *   int    *eras_pos    - erasure positions (or NULL); on return holds the
 *                         corrected positions, so it needs NROOTS entries
 *   int     no_eras     - number of known erasures
 *   int     pad         - number of padding bytes (NN - block_length)
 *   int     retval      - receives error count (-1 = uncorrectable)
 * Macros ALPHA_TO, INDEX_OF, NN, FCR, PRIM, NROOTS, A0, IPRIM, MODNN
 * and typedef data_t must be in scope (supplied by fixed.h).
 */

  /* --- local variable declarations (all before any goto) --- */
  int i, j, r, k, el;
  data_t u, q, tmp, num1, num2, den, discr_r;
  data_t lambda[NROOTS + 1], s[NROOTS];
  data_t b[NROOTS + 1], t[NROOTS + 1], omega[NROOTS + 1];
  data_t root[NROOTS], reg[NROOTS + 1], loc[NROOTS];
  int syn_error, count;

  /* --- Step 1: form syndromes ---
   * evaluate data(x) at roots of g(x): s[i] = data(alpha^(FCR+i)) */
  for (i = 0; i < NROOTS; i++)
    s[i] = data[0];

  for (j = 1; j < NN - pad; j++) {
    for (i = 0; i < NROOTS; i++) {
      if (s[i] == 0) {
        s[i] = data[j];
      } else {
        s[i] = data[j] ^ ALPHA_TO[MODNN(INDEX_OF[s[i]] + (FCR + i) * PRIM)];
      }
    }
  }

  /* convert syndromes to index form; check for zero */
  syn_error = 0;
  for (i = 0; i < NROOTS; i++) {
    syn_error |= s[i];
    s[i] = INDEX_OF[s[i]];
  }

  if (!syn_error) {
    /* syndrome is zero -> codeword, no errors */
    count = 0;
    goto finish;
  }

  /* --- Step 2: init error locator polynomial --- */
  memset(&lambda[1], 0, NROOTS * sizeof(lambda[0]));
  lambda[0] = 1;

  if (no_eras > 0) {
    /* eras_pos is relative to the shortened block; add pad for the full codeword */
    lambda[1] = ALPHA_TO[MODNN(PRIM * (NN - 1 - (eras_pos[0] + pad)))];
    for (i = 1; i < no_eras; i++) {
      u = (data_t)MODNN(PRIM * (NN - 1 - (eras_pos[i] + pad)));
      for (j = i + 1; j > 0; j--) {
        tmp = INDEX_OF[lambda[j - 1]];
        if (tmp != A0)
          lambda[j] ^= ALPHA_TO[MODNN(u + tmp)];
      }
    }
  }

  for (i = 0; i < NROOTS + 1; i++)
    b[i] = INDEX_OF[lambda[i]];

  /* --- Step 3: Berlekamp-Massey --- */
  r = no_eras;
  el = no_eras;
  while (++r <= NROOTS) {
    discr_r = 0;
    for (i = 0; i < r; i++) {
      if ((lambda[i] != 0) && (s[r - i - 1] != A0)) {
        discr_r ^= ALPHA_TO[MODNN(INDEX_OF[lambda[i]] + s[r - i - 1])];
      }
    }
    discr_r = INDEX_OF[discr_r];
    if (discr_r == A0) {
      memmove(&b[1], b, NROOTS * sizeof(b[0]));
      b[0] = A0;
    } else {
      t[0] = lambda[0];
      for (i = 0; i < NROOTS; i++) {
        if (b[i] != A0)
          t[i + 1] = lambda[i + 1] ^ ALPHA_TO[MODNN(discr_r + b[i])];
        else
          t[i + 1] = lambda[i + 1];
      }
      if (2 * el <= r + no_eras - 1) {
        el = r + no_eras - el;
        for (i = 0; i <= NROOTS; i++)
          b[i] = (lambda[i] == 0) ? A0 : (data_t)MODNN(INDEX_OF[lambda[i]] - discr_r + NN);
      } else {
        memmove(&b[1], b, NROOTS * sizeof(b[0]));
        b[0] = A0;
      }
      memcpy(lambda, t, (NROOTS + 1) * sizeof(t[0]));
    }
  }

  /* convert lambda to index form; find degree el */
  el = 0;
  for (i = 0; i <= NROOTS; i++) {
    lambda[i] = INDEX_OF[lambda[i]];
    if (i > 0 && lambda[i] != A0)
      el = i;
  }
  omega[NROOTS] = A0;

  /* --- Step 4: Chien search for roots --- */
  memcpy(&reg[1], &lambda[1], NROOTS * sizeof(reg[0]));
  count = 0;
  for (i = 1, k = IPRIM - 1; i <= NN; i++, k = MODNN(k + IPRIM)) {
    q = 1;
    for (j = el; j > 0; j--) {
      if (reg[j] != A0) {
        reg[j] = (data_t)MODNN(reg[j] + j);
        q ^= ALPHA_TO[reg[j]];
      }
    }
    if (q != 0)
      continue;
    root[count] = (data_t)i;
    loc[count]  = (data_t)k;
    if (++count == el)
      break;
  }
  if (el != count) {
    count = -1;
    goto finish;
  }

  /* --- Step 5: Forney algorithm --- */
  /* compute error evaluator omega(x) = s(x)*lambda(x) mod x^NROOTS */
  for (i = 0; i < NROOTS; i++) {
    tmp = 0;
    j = (el < i) ? el : i;
    for (; j >= 0; j--) {
      if ((s[i - j] != A0) && (lambda[j] != A0))
        tmp ^= ALPHA_TO[MODNN(s[i - j] + lambda[j])];
    }
    omega[i] = INDEX_OF[tmp];
  }

  /* compute error magnitudes */
  for (j = count - 1; j >= 0; j--) {
    num1 = 0;
    for (i = el; i >= 0; i--) {
      if (omega[i] != A0)
        num1 ^= ALPHA_TO[MODNN(omega[i] + i * root[j])];
    }
    num2 = ALPHA_TO[MODNN(root[j] * (FCR - 1) + NN)];
    den  = 0;
    for (i = ((el < NROOTS - 1) ? el : NROOTS - 1) & ~1; i >= 0; i -= 2) {
      if (lambda[i + 1] != A0)
        den ^= ALPHA_TO[MODNN(lambda[i + 1] + i * root[j])];
    }
    if (den == 0) {
      count = -1;
      goto finish;
    }
    if (num1 != 0) {
      /* a root inside the virtual zero padding means a miscorrection */
      if (loc[j] < pad) {
        count = -1;
        goto finish;
      }
      data[loc[j] - pad] ^=
          ALPHA_TO[MODNN(INDEX_OF[num1] + INDEX_OF[num2] + NN - INDEX_OF[den])];
    }
  }

  /* caller's eras_pos must have room for NROOTS entries */
  if (eras_pos != NULL) {
    for (i = 0; i < count; i++)
      eras_pos[i] = loc[i] - pad;
  }

finish:
  retval = count;
